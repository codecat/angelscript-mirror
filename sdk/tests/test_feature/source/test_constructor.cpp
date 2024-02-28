#include "utils.h"
#include "scriptmath3d.h"

using namespace std;

namespace TestConstructor
{

static const char * const TESTNAME = "TestConstructor";

static const char *script1 =
"obj g_obj1 = g_obj2;                      \n"
"obj g_obj2();                             \n"
"obj g_obj3(12, 3);                        \n";

static const char *script2 = 
"void TestConstructor()                    \n"
"{                                         \n"
"  obj l_obj1;                             \n"
"  l_obj1.a = 5; l_obj1.b = 7;             \n"
"  obj l_obj2();                           \n"
"  obj l_obj3(3, 4);                       \n"
"  a = l_obj1.a + l_obj2.a + l_obj3.a;     \n"
"  b = l_obj1.b + l_obj2.b + l_obj3.b;     \n"
"}                                         \n";
/*
// Illegal situations
static const char *script3 = 
"obj *g_obj4();                            \n";
*/
// Using constructor to create temporary object
static const char *script4 = 
"void TestConstructor2()                   \n"
"{                                         \n"
"  a = obj(11, 2).a;                       \n"
"  b = obj(23, 13).b;                      \n"
"}                                         \n";

class CTestConstructor
{
public:
	CTestConstructor() {a = 0; b = 0;}
	CTestConstructor(int a, int b) {this->a = a; this->b = b;}

	int a;
	int b;
};

void ConstrObj(CTestConstructor *obj)
{
	new(obj) CTestConstructor();
}

void ConstrObj(int a, int b, CTestConstructor *obj)
{
	new(obj) CTestConstructor(a,b);
}

void ConstrObj_gen1(asIScriptGeneric *gen)
{
	CTestConstructor *obj = (CTestConstructor*)gen->GetObject();
	new(obj) CTestConstructor();
}

void ConstrObj_gen2(asIScriptGeneric *gen)
{
	CTestConstructor *obj = (CTestConstructor*)gen->GetObject();
	int a = gen->GetArgDWord(0);
	int b = gen->GetArgDWord(1);
	new(obj) CTestConstructor(a,b);
}


bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	asIScriptEngine* engine;
	asIScriptModule* mod;
	int r;

	// Test auto generated copy constructor with registered pod value type
	// https://www.gamedev.net/forums/topic/715625-copy-constructors-appear-to-be-broken/5461794/
	{
		engine = asCreateScriptEngine();

		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		RegisterScriptMath3D(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME,
			"class Foo {\n"
			"	Foo() {}\n"
			"	//Foo(int x) {}\n" // Uncomment this to fix the problem
			"	vector3 v = vector3(0,0,0);\n"
			"}\n"
			"Foo LerpTest() {\n"
			"	Foo p;\n"
			"	p.v = vector3(1, 2, 3);\n"
			"	return p;\n"
			"}\n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "assert(LerpTest().v.x == 1 && LerpTest().v.y == 2 && LerpTest().v.z == 3);", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test auto generated copy constructor
	// Reported by Patrick Jeeves
	{
		engine = asCreateScriptEngine();

		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME,
			"class bar\n"
			"{ \n"
			"  int inherited = 104; \n"
			"} \n"
			"class foo : bar\n"
			"{\n"
			"	foo@ thisWorks() { foo f = this; return f; }\n"
			"   foo@ soDoesThis() { foo f(); f = this; return f; }\n"
			"	foo@ asDoesThis() { return foo(this); }\n"
			"   int value = 42; \n"
			"}\n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "foo f; f.value = 13; f.inherited = 31; foo @g = f.thisWorks(); assert( g.value == 13 ); assert( g.inherited == 31 );", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;
		r = ExecuteString(engine, "foo f; f.value = 13; f.inherited = 31; foo @g = f.soDoesThis(); assert( g.value == 13 ); assert( g.inherited == 31 );", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;
		r = ExecuteString(engine, "foo f; f.value = 13; f.inherited = 31; foo @g = f.asDoesThis(); assert( g.value == 13 ); assert( g.inherited == 31 );", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}
/*
	// Test initialization of member of type that has copy constructur but not assignment operator (possible in C++, but not in AngelScript)
	// TODO: To support this, I would have to implement the syntax like C++ where the initialization of members are given outside the constructor body
	// TODO: Alternatively, make the constructors use CompileInitAsCopy on the first assignment of a member (I like this better)
	// Reported by Patrick Jeeves
	{
		engine = asCreateScriptEngine();

		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME,
			"class Obj1\n" // Copy constructible, but not assignable
			"{\n"
			"	Obj1(const Obj1 &in) { }\n"
			"}\n"
			"class Obj2\n" // Constructible, Copy constructible, but not assignable
			"{\n"
			"   Obj2() {} \n"
			"	Obj2(const Obj2 &in) { }\n"
			"}\n"
			"class Obj3\n" // Constructible, not Copy constructible, Assignable
			"{\n"
			"}\n"
			"class Obj4 \n"
			"{ \n"
			"  Obj4(const Obj4 &in i) \n"
			"  { \n"
			"    o1 = i.o1; \n"  // Not allowed, so it is not possible to create constructor for this type
			"    o2 = i.o2; \n"
			"    o3 = i.o3; \n"
			"  } \n"
			"  Obj1 o1; \n" // Not allowed, there is no default constructur
			"  Obj2 o2; \n"
			"  Obj3 o3; \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}
*/
	// Test object with registered constructors
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		RegisterScriptString_Generic(engine);

		r = engine->RegisterObjectType("obj", sizeof(CTestConstructor), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C); assert(r >= 0);
		if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
		{
			r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstrObj_gen1), asCALL_GENERIC); assert(r >= 0);
			r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(int,int)", asFUNCTION(ConstrObj_gen2), asCALL_GENERIC); assert(r >= 0);
		}
		else
		{
			r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstrObj, (CTestConstructor*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
			r = engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(int,int)", asFUNCTIONPR(ConstrObj, (int, int, CTestConstructor*), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		}

		r = engine->RegisterObjectProperty("obj", "int a", asOFFSET(CTestConstructor, a)); assert(r >= 0);
		r = engine->RegisterObjectProperty("obj", "int b", asOFFSET(CTestConstructor, b)); assert(r >= 0);

		int a, b;
		r = engine->RegisterGlobalProperty("int a", &a); assert(r >= 0);
		r = engine->RegisterGlobalProperty("int b", &b); assert(r >= 0);

		bout.buffer = "";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		mod->Build();

		if (bout.buffer != "")
			TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script2, strlen(script2));
		mod->Build();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		ExecuteString(engine, "TestConstructor()", mod);

		if (a != 8 || b != 11)
			TEST_FAILED;

		
		//	mod->AddScriptSection(0, TESTNAME, script3, strlen(script3));
		//	mod->Build(0);

		//	if( out.buffer != "TestConstructor (1, 12) : Info    : Compiling obj* g_obj4\n"
		//					  "TestConstructor (1, 12) : Error   : Only objects have constructors\n" )
		//		TEST_FAILED;
		
		bout.buffer = "";
		mod->AddScriptSection(TESTNAME, script4, strlen(script4));
		mod->Build();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		ExecuteString(engine, "TestConstructor2()", mod);

		if (a != 11 || b != 13)
			TEST_FAILED;

		engine->Release();
	}

	// Test that constructor allocates memory for member objects and can properly initialize it
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		
		RegisterScriptMath3D(engine);

		const char *script = 
			"class Obj \n"
			"{  \n"
			"   Obj(const vector3 &in v) \n"
			"   { \n"
			"     pos = v; \n"
			"   } \n"
			"   vector3 pos; \n"
			"} \n";

		mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		int typeId = mod->GetTypeIdByDecl("Obj");
		asITypeInfo *type = engine->GetTypeInfoById(typeId);
		asIScriptFunction *func = type->GetFactoryByDecl("Obj @Obj(const vector3 &in)");
		if( func == 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(func);
		Vector3 pos(1,2,3);
		*(Vector3**)ctx->GetAddressOfArg(0) = &pos;
		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		asIScriptObject *obj = *(asIScriptObject**)ctx->GetAddressOfReturnValue();
		Vector3 pos2 = *(Vector3*)obj->GetAddressOfProperty(0);
		if( pos2 != pos )
			TEST_FAILED;

		ctx->Release();

		engine->Release();
	}

	// Success
	return fail;
}

}