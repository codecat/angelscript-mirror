#include "utils.h"

namespace TestCastOp
{

#define TESTNAME "TestCastOp"

const char *script = "\
interface intf1            \n\
{                          \n\
  void Test1();            \n\
}                          \n\
interface intf2            \n\
{                          \n\
  void Test2();            \n\
}                          \n\
interface intf3            \n\
{                          \n\
  void Test3();            \n\
}                          \n\
class clss : intf1, intf2  \n\
{                          \n\
  void Test1() {}          \n\
  void Test2() {}          \n\
}                          \n";


// In this test must be possible to call Func both
// with an uint and a double. The path via TestObj2
// must not be considered by the compiler, as that 
// would make: Func(TestObj(TestObj2(2)));
/*
const char *script2 = "\
class TestObj                                     \n\
{                                                 \n\
    TestObj(int a) {this.a = a;}                  \n\
	TestObj(TestObj2 a) {this.a = a.a;}           \n\
	int a;                                        \n\
}                                                 \n\
// This object must not be used to get to TestObj \n\
class TestObj2                                    \n\
{                                                 \n\
    TestObj2(int a) {assert(false);}              \n\
	int a;                                        \n\
}                                                 \n\
void Func(TestObj obj)                            \n\
{                                                 \n\
    assert(obj.a == 2);                           \n\
}                                                 \n\
void Test()                                       \n\
{                                                 \n\
	Func(2);                                      \n\
	Func(2.1);                                    \n\
}                                                 \n";
*/

// In this test it must not be possible to implicitly convert using 
// a path that requires multiple object constructions, e.g.
// Func(TestObj1(TestObj2(2)));
const char *script3 = 
"class TestObj1                 \n"
"{                              \n"
"  TestObj1(TestObj2 a) {}      \n"
"}                              \n"
"class TestObj2                 \n"
"{                              \n"
"  TestObj2(int a) {}           \n"
"}                              \n"
"void Func(TestObj1 obj) {}     \n"
"void Test()                    \n"
"{                              \n"
"  Func(2);                     \n"
"}                              \n";

void TypeToString(asIScriptGeneric *gen)
{
//	int *i = (int*)gen->GetArgPointer(0);
	*(asCScriptString**)gen->GetReturnPointer() = new asCScriptString("type");
}

class A
{
public:
	operator const char * ( ) const { return 0; }
};

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

  	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	int res = 0;
	engine->RegisterGlobalProperty("int res", &res);

	engine->ExecuteString(0, "res = int(2342.4)");
	if( res != 2342 ) 
		fail = true;

	engine->ExecuteString(0, "double tmp = 3452.4; res = int(tmp)");
	if( res != 3452 ) 
		fail = true;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	mod->Build();

	r = engine->ExecuteString(0, "clss c; cast<intf1>(c); cast<intf2>(c);");
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "intf1 @a = clss(); cast<clss>(a).Test2(); cast<intf2>(a).Test2();");
	if( r < 0 )
		fail = true;

	// Test use of handle after invalid cast (should throw a script exception)
	r = engine->ExecuteString(0, "intf1 @a = clss(); cast<intf3>(a).Test3();");
	if( r != asEXECUTION_EXCEPTION )
		fail = true;

	// Don't permit cast operator to remove constness
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "const intf1 @a = clss(); cast<intf2>(a).Test2();");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 26) : Error   : No conversion from 'const intf1@' to 'intf2@' available.\n"
					   "ExecuteString (1, 40) : Error   : Illegal operation on 'const int'\n" )
		fail = true;

	//--------------
	// Using constructor as implicit cast operator
	// TODO: Script classes should perhaps allow implicit casts to be implemented as well
/*	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, "Test2", script2, strlen(script2));
	r = mod->Build(0);
	if( r < 0 )
		fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
		fail = true;
*/
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("Test3", script3, strlen(script3));
	r = mod->Build();
	if( r >= 0 )
		fail = true;

	//-------------
	// "test" + string(type) + "\n"
	// "test" + type + "\n" 
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	r = engine->RegisterObjectType("type", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	RegisterScriptString(engine);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY, "string@ f(const type &in)", asFUNCTION(TypeToString), asCALL_GENERIC); assert( r >= 0 );
	r = engine->ExecuteString(0, "type t; string a = \"a\" + string(t) + \"b\";"); 
	if( r < 0 )
		fail = true;
		
	// Use of constructor is not permitted to implicitly cast to a reference type 
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = engine->ExecuteString(0, "type t; string a = \"a\" + t + \"b\";"); 
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 24) : Error   : No matching operator that takes the types 'string@&' and 'type&' found\n" )
		fail = true;

	// Try using the asMETHOD macro with a cast operator
	// The first option fail to compile on MSVC2005 (Thanks Jeff Slutter)
//	engine->RegisterObjectMethod("obj", "void f()", asMETHOD(A, operator const char *), asCALL_THISCALL);
	engine->RegisterObjectMethod("obj", "void f()", asMETHODPR(A, operator const char *, () const, const char *), asCALL_THISCALL);

	engine->Release();

	//-------------------
	// Illegal cast statement
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";

	r = engine->ExecuteString(0, "uint8 a=0x80; int j=int();");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 24) : Error   : A cast operator has one argument\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Success
 	return fail;
}

} // namespace

