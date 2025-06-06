#include "utils.h"

using namespace std;

namespace TestTemplate
{

class MyTmpl
{
public:
	MyTmpl(asITypeInfo *t) 
	{
		refCount = 1;
		type = t;

		type->AddRef();
	}

	MyTmpl(asITypeInfo *t, int len)
	{
		refCount = 1;
		type = t;
		length = len;

		type->AddRef();
	}

	~MyTmpl()
	{
		if( type ) 
			type->Release();
	}

	void AddRef()
	{
		refCount++;
	}

	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}

	string GetNameOfType()
	{
		if( !type ) return "";

		string name = type->GetName();
		name += "<";
		name += type->GetEngine()->GetTypeDeclaration(type->GetSubTypeId());
		name += ">";

		return name;
	}

	MyTmpl &Assign(const MyTmpl &)
	{
		return *this;
	}

	void SetVal(void *)
	{
	}

	void *GetVal()
	{
		return 0;
	}

	asITypeInfo *type;
	int refCount;

	int length;
};

MyTmpl *MyTmpl_factory(asITypeInfo *type)
{
	return new MyTmpl(type);
}

MyTmpl *MyTmpl_factory(asITypeInfo *type, int len)
{
	return new MyTmpl(type, len);
}

// A template specialization
class MyTmpl_float
{
public:
	MyTmpl_float()
	{
		refCount = 1;
	}
	~MyTmpl_float()
	{
	}
	void AddRef()
	{
		refCount++;
	}
	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}
	string GetNameOfType()
	{
		return "MyTmpl<float>";
	}

	MyTmpl_float &Assign(const MyTmpl_float &)
	{
		return *this;
	}

	void SetVal(float &)
	{
	}

	float &GetVal()
	{
		return val;
	}

	int refCount;
	float val;
};

MyTmpl_float *MyTmpl_float_factory()
{
	return new MyTmpl_float();
}

class MyDualTmpl
{
public:
	MyDualTmpl(asITypeInfo *t)
	{
		refCount = 1;
		type = t;

		type->AddRef();
	}
	~MyDualTmpl()
	{
		if( type ) 
			type->Release();
	}
	void AddRef()
	{
		refCount++;
	}
	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}
	asITypeInfo *type;
	int refCount;
};
MyDualTmpl *MyDualTmpl_factory(asITypeInfo *type)
{
	return new MyDualTmpl(type);
}

// A value type as template
class MyValueTmpl
{
public:
	MyValueTmpl(asITypeInfo *type) : ot(type) 
	{ 
		ot->AddRef(); 
		isSet = false;
	}
	MyValueTmpl(asITypeInfo *type, const MyValueTmpl &other) : ot(type)
	{
		ot->AddRef();
		isSet = false;
		assert( ot == other.ot );
	}
	~MyValueTmpl()
	{ 
		ot->Release(); 
	}

	std::string GetSubType() { isSet = true; return std::string(ot->GetEngine()->GetTypeDeclaration(ot->GetSubTypeId())); }

	static void Construct(asITypeInfo *type, void *mem) { new(mem) MyValueTmpl(type); }
	static void CopyConstruct(asITypeInfo *type, const MyValueTmpl &other, void *mem) { new(mem) MyValueTmpl(type, other); }
	static void Destruct(MyValueTmpl *obj) { obj->~MyValueTmpl(); }

	asITypeInfo *ot;
	bool isSet;
};

bool TestScriptType();

class C
{
public:
	static C* factory(asITypeInfo* /*objType*/)
	{
		return new C;
	}
	void addRef()
	{
		asAtomicInc(refCount_);
	}
	void release()
	{
		if (asAtomicDec(refCount_) == 0) delete this;
	}

	CScriptArray* f1() const
	{
		return nullptr;
	}

	int refCount_;
};

void f2(const C*)
{
}

// Global template function, registered as "T get<class T, class K>(K lmao)"
bool get_called_correctly = false;
void get(asIScriptGeneric* gen)
{
	float arg = gen->GetArgFloat(0);
	get_called_correctly = gen->GetReturnTypeId() == asTYPEID_INT32 && gen->GetArgTypeId(0) == asTYPEID_FLOAT && arg == 1.25f;

	// It is also possible to determine the correct types from the function itself, useful for functions that do not have return type or parameters
	asIScriptFunction* func = gen->GetFunction();
	if (func->GetSubTypeId(0) != asTYPEID_INT32 || func->GetSubTypeId(1) != asTYPEID_FLOAT)
		get_called_correctly = false;
}
std::string* make()
{
	static std::string singleton("Success");
	return &singleton;
}
// Template method, registered as "void do_smth<class T>(T param)"
bool do_smth_called_correctly = false;
void do_smth(asIScriptGeneric* gen)
{
	std::string* obj = (std::string*)gen->GetObject();
	int arg = gen->GetArgDWord(0);
	do_smth_called_correctly = gen->GetArgTypeId(0) == asTYPEID_INT32 && arg == 100 && (*obj) == "Success";
}

void q2as_test_bug(asIScriptGeneric* /* gen */)
{

}

static void ScriptTestGen(asIScriptGeneric* gen)
{
	gen->SetReturnDWord(gen->GetArgDWord(0));
}

bool error = false;
void print(void* p, int typeId)
{
	error = error || (typeId != asTYPEID_INT32);
	error = error || (*(int*)p != 10);
}

bool Test()
{
	RET_ON_MAX_PORT

	bool fail = TestScriptType();
	int r;
	COutStream out;
	CBufferedOutStream bout;

	// Test saving bytecode with template functions
	// https://www.gamedev.net/forums/topic/718083-template-functions-pre-compiled-byte-code/
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("T Test<T>(T v)", asFUNCTION(ScriptTestGen), asCALL_GENERIC);
		engine->SetDefaultNamespace("ns");
		engine->RegisterGlobalFunction("T Test2<T>(T v)", asFUNCTION(ScriptTestGen), asCALL_GENERIC);
		engine->SetDefaultNamespace("");
		engine->RegisterObjectType("lmao", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectBehaviour("lmao", asBEHAVE_FACTORY, "lmao@ f()", asFUNCTION(make), asCALL_CDECL);
		engine->RegisterObjectMethod("lmao", "void do_smth<class T>(T param)", asFUNCTION(do_smth), asCALL_GENERIC);

		do_smth_called_correctly = false;
		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void testfunc() \n"
			"{ \n"
			"	auto n = Test<int>(10); \n"
			"   assert( n == 10 ); \n"
			"   auto m = ns::Test2<int>(15); \n"
			"   assert( m == 15 ); \n"
			"   lmao l; \n"
			"   l.do_smth<int>(100); \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		CBytecodeStream st("test");
		r = mod->SaveByteCode(&st);
		if (r < 0)
			TEST_FAILED;

		engine->ShutDownAndRelease();

		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("T Test<T>(T v)", asFUNCTION(ScriptTestGen), asCALL_GENERIC);
		engine->SetDefaultNamespace("ns");
		engine->RegisterGlobalFunction("T Test2<T>(T v)", asFUNCTION(ScriptTestGen), asCALL_GENERIC);
		engine->SetDefaultNamespace("");
		engine->RegisterObjectType("lmao", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectBehaviour("lmao", asBEHAVE_FACTORY, "lmao@ f()", asFUNCTION(make), asCALL_CDECL);
		engine->RegisterObjectMethod("lmao", "void do_smth<class T>(T param)", asFUNCTION(do_smth), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&st);
		if (r < 0)
			TEST_FAILED;

		r = ExecuteString(engine, "testfunc()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (!do_smth_called_correctly)
			TEST_FAILED;

		engine->ShutDownAndRelease();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	// https://www.gamedev.net/forums/topic/717373-support-for-template-functions-is-now-implemented-in-2380-wip/5466542/
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("T Test<T>(T v)", asFUNCTION(ScriptTestGen), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void print(?&in)", asFUNCTION(print), asCALL_CDECL);

		r = ExecuteString(engine, 
			"auto n = Test<int>(10);\n"
			"print(n); \n"// This works and will print 10.
			"print(Test<int>(10));\n" // This forces typeId to be 10, and the print function fails
		);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (error)
			TEST_FAILED;

		engine->ShutDownAndRelease();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	// Test passing a null argument to a template function
	// Reported by Paril
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		RegisterStdString(engine);
		engine->RegisterGlobalFunction("T @+find_by_str<T>(T @+from, const string &in member, const string &in value)", asFUNCTION(0), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void testfunc() \n"
			"{ \n"
			"	find_by_str(null, '', ''); \n"  // Compiler cannot deduce the type from a null pointer
			"} \n");
		r = mod->Build();
		if (r >= 0)
			TEST_FAILED;

		engine->ShutDownAndRelease();

		if (bout.buffer != "test (1, 1) : Info    : Compiling void testfunc()\n"
						   "test (3, 13) : Error   : Template 'find_by_str' expects 1 sub type(s)\n")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}


	// Make sure object types are correctly released
	// Reported by Paril
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("T @+ test_bug<T>(T @+ from)", asFUNCTION(q2as_test_bug), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"funcdef void blocked2_f(ASEntity2 &, ASEntity2 &); \n"
			"class moveinfo2_t \n"
			"{ \n"
			"	blocked2_f    @blocked; \n"
			"} \n"
			"class ASEntity2 \n"
			"{ \n"
			"	moveinfo2_t  moveinfo; \n"
			"} \n"
			"void testfunc() \n"
			"{ \n"
			"	ASEntity2 temp; \n"
			"	test_bug<ASEntity2>(temp); \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		asITypeInfo *type = mod->GetTypeInfoByName("ASEntity2");
		assert(type);

		r = ExecuteString(engine, "testfunc()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		engine->ShutDownAndRelease();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	// Test parsing for template with multiple subtypes and auto declaration
	// Reported by Patrick Jeeves
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 0);

		// register template with 2 subtypes
		engine->RegisterObjectType("MyTmpl<class S, class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_FACTORY, "MyTmpl<S,T> @f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class U {} class V {} \n"
			"MyTmpl<U, V>@ g_obj1; \n"
			"auto@ g_obj2 = MyTmpl<U, V>(); \n"
			"void main() { \n"
			"  MyTmpl<U, V>@ obj1; \n"
			"  auto@ obj2 = MyTmpl<U, V>(); \n"
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

	// Test template methods and functions
	// Initial implementation provided by MindOfTony
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("lmao", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectBehaviour("lmao", asBEHAVE_FACTORY, "lmao@ f()", asFUNCTION(make), asCALL_CDECL);

		// Register a template method on the object type
		do_smth_called_correctly = false;
		r = engine->RegisterObjectMethod("lmao", "void do_smth<class T>(T param)", asFUNCTION(do_smth), asCALL_GENERIC);
		if (r < 0)
			TEST_FAILED;
		// Retrieve the script function and check that it is a template function and that it has template sub types
		asIScriptFunction* func = engine->GetFunctionById(r);
		if (func == 0 || func->GetFuncType() != asFUNC_TEMPLATE)
			TEST_FAILED;
		if (string(func->GetDeclaration(true, true, true)) != "void lmao::do_smth<T>(T param)")
			TEST_FAILED;
		if (func->GetSubTypeCount() != 1 || std::string(func->GetSubType(0)->GetName()) != "T")
			TEST_FAILED;

		// Register a global template function
		engine->SetDefaultNamespace("nm");
		r = engine->RegisterGlobalFunction("T get<class T, class K>(K lmao)", asFUNCTION(get), asCALL_GENERIC, 0);
		if (r < 0)
			TEST_FAILED;
		engine->SetDefaultNamespace("");
		// Retrieve the script function and check that it is a template function and that it has template sub types
		func = engine->GetFunctionById(r);
		if (func == 0 || func->GetFuncType() != asFUNC_TEMPLATE)
			TEST_FAILED;
		if (string(func->GetDeclaration(true, true, true)) != "T nm::get<T,K>(K lmao)")
			TEST_FAILED;
		if (func->GetSubTypeCount() != 2 || std::string(func->GetSubType(1)->GetName()) != "K" )
			TEST_FAILED;

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", R"(
				void main()
				{
					lmao lol;
					lol.do_smth<int>(100);
					nm::get<int, float>(1.25);
				}
			)");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "main()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (!get_called_correctly)
			TEST_FAILED;
		if (!do_smth_called_correctly)
			TEST_FAILED;

		// It is not possible to call a template function
		asIScriptContext *ctx = engine->CreateContext();
		r = ctx->Prepare(func);
		if (r < 0) TEST_FAILED; // The prepare function doesn't check the type
		r = ctx->Execute();
		if (r != asEXECUTION_EXCEPTION)
			TEST_FAILED;
		ctx->Release();

		engine->ShutDownAndRelease();
	}

	// Template specialization with multiple subtypes
	// Reported by Stefan Blumer
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// register template with child funcdef
		engine->RegisterObjectType("MyTmpl<class S, class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_FACTORY, "MyTmpl<S,T> @f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<S,T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);

		// register template specialization
		r = engine->RegisterObjectType("MyTmpl<int,float>", 0, asOBJ_REF);
		if (r < 0)
			TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyTmpl<int,float>", asBEHAVE_FACTORY, "MyTmpl<int,float> @f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		if (r < 0)
			TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyTmpl<int,float>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		if (r < 0)
			TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyTmpl<int,float>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asITypeInfo *type = engine->GetTypeInfoByDecl("MyTmpl<int,float>");
		if (type->GetSubTypeId(0) != asTYPEID_INT32 || type->GetSubTypeId(1) != asTYPEID_FLOAT)
			TEST_FAILED;

		engine->ShutDownAndRelease();
	}

	// Test template returning another template
	// Reported by Polyak Istvan
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

		r = engine->RegisterObjectType("C<class T1>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		r = engine->RegisterObjectBehaviour("C<T1>",asBEHAVE_FACTORY,"C<T1> @ f (int &in)",asFUNCTIONPR(C::factory, (asITypeInfo*), C*),asCALL_CDECL);
		r = engine->RegisterObjectBehaviour("C<T1>",asBEHAVE_ADDREF,"void f ()",asMETHODPR(C, addRef, (), void),asCALL_THISCALL);
		r = engine->RegisterObjectBehaviour("C<T1>",asBEHAVE_RELEASE,"void f ()",asMETHODPR(C, release, (), void),asCALL_THISCALL);
		r = engine->RegisterObjectMethod("C<T1>","array<T1> @ f1 () const",asMETHODPR(C, f1, () const, CScriptArray*),asCALL_THISCALL); // TODO: array<T1> shouldn't be in templateInstanceType as it is a nonsense instance 
		r = engine->RegisterGlobalFunction("const C<int> @ f2 ()",asFUNCTIONPR(f2, (const C*), void),asCALL_CDECL);

		for (int i = 0; i < 2; i++)
		{
			asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
			mod->AddScriptSection("test",
				"void f11(void)\n"
				"{ \n"
				"  C<int> c1; \n"
				"  const array<int> @ t = c1.f1(); \n"
				"}\n");
			r = mod->Build();
							  
			if (r < 0)
				TEST_FAILED;

			mod->Discard();
			engine->GarbageCollect();
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test array of array when subtype doesn't have default constructor (must give appropriate error)
	// Reported by Patrick Jeeves
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("Test", 4, asOBJ_VALUE);
		engine->RegisterObjectBehaviour("Test", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Test", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(0), asCALL_GENERIC);
		RegisterScriptArray(engine, true);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "void func() { Test[][] t; t = Test[][]();}");
		r = mod->Build();
		if (r >= 0)
			TEST_FAILED;

		if (bout.buffer != 
			"test (1, 1) : Info    : Compiling void func()\n"
			"array (0, 0) : Error   : The subtype has no default constructor\n"
			"test (1, 19) : Error   : Can't form arrays of subtype 'Test'\n"
			"array (0, 0) : Error   : The subtype has no default constructor\n"
			"test (1, 35) : Error   : Can't form arrays of subtype 'Test'\n"
			"test (1, 39) : Error   : A cast operator has one argument\n"
			"array (0, 0) : Error   : The subtype has no default constructor\n"
			"test (1, 35) : Error   : Can't form arrays of subtype 'Test'\n")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Destructor for value template type must be unique for each template instance even though there are no differences in parameters
	// https://www.gamedev.net/forums/topic/711330-unable-to-get-subtype-id-from-asiscriptgeneric-in-destructor/
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("MyValue<class T>", 1, asOBJ_VALUE | asOBJ_TEMPLATE);
		r = engine->RegisterObjectBehaviour("MyValue<T>", asBEHAVE_CONSTRUCT, "void f(int&in type)", asFUNCTION(0), asCALL_GENERIC);
		r = engine->RegisterObjectBehaviour("MyValue<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(0), asCALL_GENERIC);

		asITypeInfo* tmpl1 = engine->GetTypeInfoByDecl("MyValue<int>");
		asITypeInfo* tmpl2 = engine->GetTypeInfoByDecl("MyValue<float>");
		asEBehaviours beh;
		asIScriptFunction *destr1 = tmpl1->GetBehaviourByIndex(0, &beh);
		if (beh != asBEHAVE_DESTRUCT)
			TEST_FAILED;
		asIScriptFunction* destr2 = tmpl2->GetBehaviourByIndex(0, &beh);
		if (beh != asBEHAVE_DESTRUCT)
			TEST_FAILED;
		if (destr1 == destr2)
			TEST_FAILED;
		if (destr1->GetObjectType() != tmpl1)
			TEST_FAILED;
		if (destr2->GetObjectType() != tmpl2)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test asBEHAVE_LIST_CONSTRUCTOR with repeat T pattern on template value type
	// Reported by Suedwest
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("MyValue<class T>", 1, asOBJ_VALUE | asOBJ_TEMPLATE);
		r = engine->RegisterObjectBehaviour("MyValue<T>", asBEHAVE_LIST_CONSTRUCT, "void f(int&in type, int&in list) {repeat T}", asFUNCTION(0), asCALL_GENERIC);
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Template specialization with child funcdef
	// https://www.gamedev.net/forums/topic/701578-problem-with-child-funcdef-registration/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// register template with child funcdef
		engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*), MyTmpl*), asCALL_CDECL);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL);
		engine->RegisterFuncdef("void MyTmpl<T>::Func(const T&in if_handle_then_const param)");

		// register template specialization
		engine->RegisterObjectType("MyTmpl<void>", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("MyTmpl<void>", asBEHAVE_FACTORY, "MyTmpl<void> @f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<void>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<void>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterFuncdef("void MyTmpl<void>::Func(void)");

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		
		engine->Release();
	}
	
	// Test template bug when using multiple modules
	// https://www.gamedev.net/forums/topic/699909-template-factory-return-type-bug/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*), MyTmpl*), asCALL_CDECL);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL);

		asIScriptModule *mod1 = engine->GetModule("m1", asGM_ALWAYS_CREATE);
		asIScriptModule *mod2 = engine->GetModule("m2", asGM_ALWAYS_CREATE);	

		const char* script_text = "void main() { MyTmpl<int> s; }";

		mod1->AddScriptSection("test1", script_text);
		mod1->Build();

		mod2->AddScriptSection("test2", script_text);
		mod2->Build();

		mod1->Discard(); // this must not release the MyTmpl<int> type as it is still used by the other module

		asIScriptContext *ctx = engine->CreateContext();
		asIScriptFunction *func = mod2->GetFunctionByDecl("void main()");
		ctx->Prepare(func);
		ctx->Execute();
		ctx->Release();

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		
		engine->Release();		
	}
	
	// When unsafe reference is turned on it should still be allowed to instantiate template
	// for value subtypes even though a method takes the subtype by ref
	// https://www.gamedev.net/forums/topic/695957-can-not-create-a-value-type-template/
	{
		// First without asEP_ALLOW_UNSAFE_REFERENCES
		asIScriptEngine *engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("Template<class T>", 0, asOBJ_REF | asOBJ_NOCOUNT | asOBJ_TEMPLATE);
		engine->RegisterObjectMethod("Template<T>", "void func(T&)", asFUNCTION(0), asCALL_GENERIC);

		r = engine->RegisterGlobalProperty("Template<int> t", (void*)1);
		if (r >= 0)
			TEST_FAILED;

		if (bout.buffer != "Property (1, 10) : Error   : Attempting to instantiate invalid template type 'Template<int>'\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterGlobalProperty' with 'Template<int> t' (Code: asINVALID_DECLARATION, -10)\n")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
		
		// Now with asEP_ALLOW_UNSAFE_REFERNCES
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, true);

		engine->RegisterObjectType("Template<class T>", 0, asOBJ_REF | asOBJ_NOCOUNT | asOBJ_TEMPLATE);
		engine->RegisterObjectMethod("Template<T>", "void func(T&)", asFUNCTION(0), asCALL_GENERIC);

		r = engine->RegisterGlobalProperty("Template<int> t", (void*)1);
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Template callbacks must only be invoked after the types are fully constructed
	// http://www.gamedev.net/topic/658982-funcdef-error/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class foo {} \n"
			"funcdef void bar(array<foo>); \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Use of second template in first template's arguments
	// http://www.gamedev.net/topic/660932-variable-parameter-type-to-accept-only-handles-during-compile-also-more-template-woes/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		r = engine->RegisterObjectType("List<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE | asOBJ_NOCOUNT); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("List<T>", asBEHAVE_FACTORY, "List<T> @f(int&in)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("List_iterator<class T2>", 0, asOBJ_REF | asOBJ_TEMPLATE | asOBJ_NOCOUNT); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("List_iterator<T2>", asBEHAVE_FACTORY, "List_iterator<T2> @f(int&in, List<T2>&)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("List_iterator<T2>", "void test(List<T2>&)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

		// Add a circular reference between the two template types
		r = engine->RegisterObjectMethod("List<T>", "List_iterator<T> @begin()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("Two_lists<class T1, class T2>", 0, asOBJ_REF | asOBJ_TEMPLATE | asOBJ_NOCOUNT); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("Two_lists<T1,T2>", asBEHAVE_FACTORY, "Two_lists<T1,T2> @f(int&in)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("Two_lists<T1,T2>", "void test(List<T1>&,List<T2>&)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		asIScriptModule *mod = engine->GetModule("Test",asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"void func() { \n"
			"  List<int> list; \n"
			"  List_iterator<int> @it = List_iterator<int>(list); \n"
			"  it.test(list); \n"
			"  @it = list.begin(); \n"
			"  Two_lists<int,float> t; \n"
			"  t.test(list, List<float>()); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Properties in templates
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterStdString(engine);

		r = engine->RegisterObjectType("MyValueTmpl<class T>", sizeof(MyValueTmpl), asOBJ_VALUE | asOBJ_TEMPLATE);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(MyValueTmpl::Construct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const MyValueTmpl<T> &in)", asFUNCTION(MyValueTmpl::CopyConstruct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(MyValueTmpl::Destruct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectProperty("MyValueTmpl<T>", "bool isSet", asOFFSET(MyValueTmpl, isSet));
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectMethod("MyValueTmpl<T>", "string GetSubType()", asMETHOD(MyValueTmpl, GetSubType), asCALL_THISCALL);
		if( r < 0 ) TEST_FAILED;

		r = ExecuteString(engine, "MyValueTmpl<int> t; \n"
			"assert( t.isSet == false ); \n"
			"t.GetSubType(); \n"
			"assert( t.isSet == true ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Value types can also be registered as templates
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterStdString(engine);

		r = engine->RegisterObjectType("MyValueTmpl<class T>", sizeof(MyValueTmpl), asOBJ_VALUE | asOBJ_TEMPLATE);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(MyValueTmpl::Construct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const MyValueTmpl<T> &in)", asFUNCTION(MyValueTmpl::CopyConstruct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyValueTmpl<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(MyValueTmpl::Destruct), asCALL_CDECL_OBJLAST);
		if( r < 0 ) TEST_FAILED;
		r = engine->RegisterObjectMethod("MyValueTmpl<T>", "string GetSubType()", asMETHOD(MyValueTmpl, GetSubType), asCALL_THISCALL);
		if( r < 0 ) TEST_FAILED;

		r = ExecuteString(engine, 
			"MyValueTmpl<int> t; \n"
			"assert( t.GetSubType() == 'int' ); \n"
			"MyValueTmpl<int> c(t); \n"
			"assert( c.GetSubType() == 'int' ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test error messages when registering the template type incorrectly
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("Tmpl1<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("Tmpl1<T>", asBEHAVE_FACTORY, "Tmpl1<T> @f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Tmpl1<T>", asBEHAVE_FACTORY, "Tmpl1<T> @f(int &in, Tmpl1<T> @+)", asFUNCTION(0), asCALL_GENERIC);

		if( bout.buffer != " (0, 0) : Error   : First parameter to template factory must be a reference. This will be used to pass the object type of the template\n"
						   " (0, 0) : Error   : Failed in call to function 'RegisterObjectBehaviour' with 'Tmpl1' and 'Tmpl1<T> @f()' (Code: asINVALID_DECLARATION, -10)\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// It is allowed to declare sub types as const
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		
		engine->RegisterObjectType("MyTmpl<class T1>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyTmpl<T1>", asBEHAVE_FACTORY, "MyTmpl<T1>@ f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<T1>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("MyTmpl<T1>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
		
		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class A {} \n"
			"void func() { \n"
			"  MyTmpl<const A> a; \n" // allowed
			"  MyTmpl<const A@> b; \n" // allowed
			"  MyTmpl<const A@const> c; \n" // allowed
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test instanciating a template with same subtype for both args
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);

		engine->RegisterObjectType("MyDualTmpl<class T1, class T2>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("MyDualTmpl<T1,T2>", asBEHAVE_FACTORY, "MyDualTmpl<T1,T2>@ f(int&in)", asFUNCTION(MyDualTmpl_factory), asCALL_CDECL);
		engine->RegisterObjectBehaviour("MyDualTmpl<T1,T2>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyDualTmpl,AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("MyDualTmpl<T1,T2>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyDualTmpl,Release), asCALL_THISCALL);
		engine->RegisterObjectMethod("MyDualTmpl<T1,T2>", "T1 &func(const T2 &in)", asFUNCTION(0), asCALL_THISCALL);

		engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);
		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class A {} \n"
			"void func() { \n"
			"  MyDualTmpl<A,A> a; \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		asUINT size, destr, detect;
		engine->GetGCStatistics(&size, &destr, &detect);

		engine->DiscardModule("test");

		engine->GetGCStatistics(&size, &destr, &detect);

		// It must be possible to find method by declaration on the template instances
		asITypeInfo *ot = engine->GetTypeInfoById(engine->GetTypeIdByDecl("MyDualTmpl<int, string>"));
		if( ot == 0 )
			TEST_FAILED;
		asIScriptFunction *mthd = ot->GetMethodByDecl("int &func(const string &in)");
		if( mthd == 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test passing templates as arguments
	// http://www.gamedev.net/topic/639597-how-to-pass-arraystring-to-a-function/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"bool stringInList( array<string>& arg, string s ) \n"
			"{ \n"
			"	for( uint n = 0; n < arg.length(); n++ ) \n"
			"	{ \n"
			"		if( arg[n] == s ) \n"
			"			return true; \n"
			"	} \n"
			"	return false; \n"
			"}  \n"
			"array<string> gEngineRPMUnits = { 'stall-rpm','start-rpm','idle-rpm','redline-rpm','max-rpm' }; \n"
			"string VerifyEngineUnitSpec( string attribute, string unitSpec ) \n"
			"{ \n"
			"	// local \n"
			"	// string result; \n"
			"	// check for rpm \n"
			"	if( stringInList(gEngineRPMUnits,attribute) ) \n"
			"	{ \n"
			"		// unit has to be rpm, rps, rad/s \n"
			"		if( unitSpec!='revolution' ) \n"
			"			return 'error'; \n"
			"		else \n"
			"			return unitSpec; \n"
			"	} \n"
			"   return '';\n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( VerifyEngineUnitSpec('idle-rpm', 'revolution') == 'revolution' )", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// It should be possible to register a template type with multiple subtypes
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
		r = engine->RegisterObjectType("tmpl<class A, class B, class C>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Register a method for the template type
		bout.buffer = "";
		r = engine->RegisterObjectMethod("tmpl<A,B,C>", "C &func(const B &in, const A &in)", 0, asCALL_GENERIC);
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Get the template instance
		asITypeInfo *type = engine->GetTypeInfoById(engine->GetTypeIdByDecl("tmpl<int, bool, float>"));
		if( type->GetSubTypeCount() != 3 )
			TEST_FAILED;
		if( type->GetSubTypeId(0) != asTYPEID_INT32 ||
			type->GetSubTypeId(1) != asTYPEID_BOOL ||
			type->GetSubTypeId(2) != asTYPEID_FLOAT )
			TEST_FAILED;

		asIScriptFunction *func = type->GetMethodByName("func");
		if( func->GetReturnTypeId() != asTYPEID_FLOAT )
			TEST_FAILED;
		int t0, t1;
		func->GetParam(0, &t0);
		func->GetParam(1, &t1);
		if( t0 != asTYPEID_BOOL ||
			t1 != asTYPEID_INT32 )
			TEST_FAILED;

		engine->Release();
	}

	// Don't allow registering the same template name with different subtypes
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
		r = engine->RegisterObjectType("tmpl<class A, class B, class C>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		r = engine->RegisterObjectType("tmpl<class A>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Attempt compiling a script function that returns a multi subtype template
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = engine->RegisterObjectType("tmpl<class A>", 0, asOBJ_REF | asOBJ_TEMPLATE | asOBJ_NOCOUNT);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"tmpl<int,int> @func() { return a; } \n"
			"tmpl<int,int> @a; \n");
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "test (1, 1) : Error   : Template 'tmpl' expects 1 sub type(s)\n"
		                   "test (2, 1) : Error   : Template 'tmpl' expects 1 sub type(s)\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Reported by zeta945@gmail.com
	// http://www.gamedev.net/topic/633458-cant-create-a-template-class-with-a-default-constructor/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		
		r = engine->RegisterObjectType("myObj", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
		r = engine->RegisterObjectType("x<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_FACTORY, "x<T>@ f(int &in, int)", asFUNCTION(0), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_FACTORY, "x<T>@ f(int&in)", asFUNCTION(0), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_THISCALL); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"void test() \n"
			"{ \n"
			"     x<myObj> xxx; \n"
			"} \n");
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}
	

	// Reported by ThyReaper
	// array<const int> and array<const Obj@> doesn't give expected result
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"class Test {} \n"
			"void func() \n"
			"{ \n"
			"  array<const int> i(1); \n"
			"  array<const Test@> t(1); \n"
			"  i[0] = 1; \n" // should fail
			"  @t[0] = Test(); \n"
			"  t[0] = Test(); \n" // should fail
			"} \n");
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (2, 1) : Info    : Compiling void func()\n"
						   "test (6, 8) : Error   : Reference is read-only\n"
						   "test (8, 8) : Error   : Reference is read-only\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}


	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
	if( r == asNOT_SUPPORTED )
	{
		PRINTF("Skipping template test because it is not yet supported\n");
		engine->Release();
		return false;	
	}

	if( r < 0 )
		TEST_FAILED;

	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

	// Must be possible to register properties for templates, but not of the template subtype
	r = engine->RegisterObjectProperty("MyTmpl<T>", "int length", asOFFSET(MyTmpl,length)); assert( r >= 0 );

	// Add method to return the type of the template instance as a string
	r = engine->RegisterObjectMethod("MyTmpl<T>", "string GetNameOfType()", asMETHOD(MyTmpl, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );

	// Add method that take and return the template type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "MyTmpl<T> &Assign(const MyTmpl<T> &in)", asMETHOD(MyTmpl, Assign), asCALL_THISCALL); assert( r >= 0 );

	// Add methods that take and return the template sub type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "const T &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(const T& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	// Test that it is possible to instantiate the template type for different sub types
	r = ExecuteString(engine, "MyTmpl<int> i;    \n"
								 "MyTmpl<string> s; \n"
								 "assert( i.GetNameOfType() == 'MyTmpl<int>' ); \n"
								 "assert( s.GetNameOfType() == 'MyTmpl<string>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test that the assignment works
	r = ExecuteString(engine, "MyTmpl<int> i1, i2; \n"
		                         "i1.Assign(i2);      \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test that the method of the template that has no template subtype is also getting a unique function with the correct object type
	// https://www.gamedev.net/forums/topic/695691-subtypes-not-always-resolved-in-generic-call/
	asITypeInfo *type = engine->GetTypeInfoByDecl("MyTmpl<int>");
	asIScriptFunction *func = type->GetMethodByDecl("string GetNameOfType()");
	if (func->GetObjectType() != type)
		TEST_FAILED;

	// Test that the template sub type works
	r = ExecuteString(engine, "MyTmpl<int> i; \n"
		                         "i.SetVal(0); \n"
								 "i.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// It should be possible to register specializations of the template type
	r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF); assert( r >= 0 );
	// The specialization's factory doesn't take the hidden asITypeInfo parameter
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_FACTORY, "MyTmpl<float> @f()", asFUNCTION(MyTmpl_float_factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl_float, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl_float, Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "string GetNameOfType()", asMETHOD(MyTmpl_float, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "MyTmpl<float> &Assign(const MyTmpl<float> &in)", asMETHOD(MyTmpl_float, Assign), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "const float &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "void SetVal(const float& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "assert( f.GetNameOfType() == 'MyTmpl<float>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "MyTmpl<float> f1, f2; \n"
		                         "f1.Assign(f2);        \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "f.SetVal(0); \n"
								 "f.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// The declaration for the template specialization must work
	int typeId = engine->GetTypeIdByDecl("MyTmpl<float>");
	string decl = engine->GetTypeDeclaration(typeId);
	if( decl != "MyTmpl<float>" )
		TEST_FAILED;

	// Attempting to registering the same template specialization twice should give proper error
	// Attempting to register a template specialization when the template itself hasn't been registered should also give proper error
	// http://www.gamedev.net/topic/662414-issue-in-registering-template-specialization/
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	typeId = engine->GetTypeIdByDecl("MyTmpl<int>");
	if( typeId < 0 )
		TEST_FAILED;
	r = engine->RegisterObjectType("MyTmpl<int>", 0, asOBJ_REF);
	if( r != asNOT_SUPPORTED )
		TEST_FAILED;
	r = engine->RegisterObjectType("NoTmpl<int>", 0, asOBJ_REF);
	if( r != asINVALID_NAME )
		TEST_FAILED;
	r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF);
	if( r != asALREADY_REGISTERED )
		TEST_FAILED;
	if( bout.buffer != " (0, 0) : Error   : Cannot register. The template type instance 'MyTmpl<int>' has already been generated.\n"
					   " (0, 0) : Error   : Failed in call to function 'RegisterObjectType' with 'MyTmpl<int>' (Code: asNOT_SUPPORTED, -7)\n"
					   " (0, 0) : Error   : Failed in call to function 'RegisterObjectType' with 'NoTmpl<int>' (Code: asINVALID_NAME, -8)\n"
					   " (0, 0) : Error   : Failed in call to function 'RegisterObjectType' with 'MyTmpl<float>' (Code: asALREADY_REGISTERED, -13)\n" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Should give proper error if attempting to register anything for a type that has been generated from a template
	bout.buffer = "";
	r = engine->RegisterObjectBehaviour("MyTmpl<int>", asBEHAVE_CONSTRUCT, "MyTmpl<int> @f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asINVALID_TYPE )
		TEST_FAILED;
	r = engine->RegisterObjectMethod("MyTmpl<int>", "void f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asINVALID_TYPE )
		TEST_FAILED;
	r = engine->RegisterObjectProperty("MyTmpl<int>", "int p", 0);
	if( r != asINVALID_TYPE )
		TEST_FAILED;
	if( bout.buffer != " (0, 0) : Error   : Failed in call to function 'RegisterObjectBehaviour' with 'MyTmpl<int>' and 'MyTmpl<int> @f()' (Code: asINVALID_TYPE, -12)\n"
					   " (0, 0) : Error   : Failed in call to function 'RegisterObjectMethod' with 'MyTmpl<int>' and 'void f()' (Code: asINVALID_TYPE, -12)\n"
					   " (0, 0) : Error   : Failed in call to function 'RegisterObjectProperty' with 'MyTmpl<int>' and 'int p' (Code: asINVALID_TYPE, -12)\n" )
	{
		PRINTF("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	// Registering template specialization with related templates
	// http://www.gamedev.net/topic/662414-issue-in-registering-template-specialization/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterObjectType("vector<class T>", 0, asOBJ_REF | asOBJ_NOCOUNT | asOBJ_TEMPLATE);
		engine->RegisterObjectType("vector_iterator<class T>", 4, asOBJ_VALUE | asOBJ_TEMPLATE);
		engine->RegisterObjectBehaviour("vector_iterator<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("vector_iterator<T>", asBEHAVE_CONSTRUCT, "void f(int&in,vector<T> &)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("vector<T>", "vector_iterator<T> begin()", asFUNCTION(0), asCALL_GENERIC);

		r = engine->RegisterObjectType("vector<int8>", 0, asOBJ_REF);
		if( r < 0 )
			TEST_FAILED;
		r = engine->RegisterObjectType("vector_iterator<int8>", 0, asOBJ_REF);
		if( r < 0 )
			TEST_FAILED;
		r = engine->RegisterObjectMethod("vector<int8>", "vector_iterator<int8> @begin()", asFUNCTION(0), asCALL_GENERIC);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}
	
	// TODO: Test behaviours that take and return the template sub type
	// TODO: Test behaviours that take and return the proper template instance type

	// TODO: Even though the template doesn't accept a value subtype, it must still be possible to register a template specialization for the subtype

	// TODO: Must be possible to allow use of initialization lists

	// TODO: Must allow the subtype to be another template type, e.g. array<array<int>>

	// TODO: Must allow multiple subtypes, e.g. map<string,int>




	// Test that a proper error occurs if the instance of a template causes invalid data types, e.g. int@
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method makes it impossible to instanciate the template for primitive types
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T@)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		r = ExecuteString(engine, "MyTmpl<int> t;");
		if( r >= 0 )
		{
			TEST_FAILED;
		}

		if( bout.buffer != "ExecuteString (1, 8) : Error   : Attempting to instantiate invalid template type 'MyTmpl<int>'\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test that a template registered to take subtype by value isn't supported
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method is not supported. The subtype must not be passed by value since it would impact the ABI
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T)", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		
		r = engine->RegisterObjectMethod("MyTmpl<T>", "T GetVal()", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != " (0, 0) : Error   : Failed in call to function 'RegisterObjectMethod' with 'MyTmpl' and 'void SetVal(T)' (Code: asNOT_SUPPORTED, -7)\n"
		                   " (0, 0) : Error   : Failed in call to function 'RegisterObjectMethod' with 'MyTmpl' and 'T GetVal()' (Code: asNOT_SUPPORTED, -7)\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}


	// The factory behaviour for a template class must have a hidden reference as first parameter (which receives the asITypeInfo)
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f()", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int)", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		engine->Release();
	}

	// Must not allow registering properties with the template subtype 
	// TODO: Must be possible to use getters/setters to register properties of the template subtype
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectProperty("MyTmpl<T>", "T a", 0); 
		if( r != asINVALID_DECLARATION )
		{
			TEST_FAILED;
		}
		engine->Release();
	}

	// Must not be possible to register specialization before the template type
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF);
		if( r != asINVALID_NAME )
		{
			TEST_FAILED;
		}
		engine->Release();
	}

	// Must properly handle templates without default constructor
	// http://www.gamedev.net/topic/617408-crash-when-aggregating-a-template-class-that-has-no-default-factory/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF|asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in, int)", asFUNCTIONPR(MyTmpl_factory, (asITypeInfo*, int), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		// Class T should give error because there is no default constructor for t
		// Class S should work because it is calling the appropriate non-default constructor for s
		mod->AddScriptSection("mod", "class T { MyTmpl<int> t; } T t;\n"
			                         "class S { MyTmpl<int> s; S() { s = MyTmpl<int>(1); } } S s;");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "mod (1, 7) : Info    : Compiling auto generated T::T()\n"
						   "mod (1, 23) : Error   : No default constructor for object of type 'MyTmpl'.\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		bout.buffer = "";
		r = ExecuteString(engine, "MyTmpl<int> t;");
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 13) : Error   : No default constructor for object of type 'MyTmpl'.\n" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Reported by slicer4ever
	// http://www.gamedev.net/topic/632288-registering-specialized-template-class/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		r = engine->RegisterObjectType("ConvexHull", 0, asOBJ_REF|asOBJ_NOCOUNT);
		r = engine->RegisterObjectType("LLObject<class T>", 0, asOBJ_REF|asOBJ_NOCOUNT|asOBJ_TEMPLATE);
		r = engine->RegisterObjectType("LLObject<ConvexHull@>", 0, asOBJ_REF|asOBJ_NOCOUNT);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

 	return fail;
}


// ScriptType implemented by Michael Rubin, a.k.a iraxef
class CScriptType
{
public:

    static void Construct(asITypeInfo *type, void *mem);
    static void CopyConstruct(asITypeInfo *type, const CScriptType &other, void *mem);
    static void Destruct(CScriptType *obj);

    CScriptType(asITypeInfo *type);
    CScriptType(asITypeInfo *type, const CScriptType &other);
    virtual ~CScriptType();

    int GetSubTypeId() const;

    // CScriptType &operator=(const CScriptType&);

    // Compare two types
    bool operator==(const CScriptType &) const;
    bool Equals(int typeId) const;

    // std::string GetSubType() { return std::string(ot->GetEngine()->GetTypeDeclaration(ot->GetSubTypeId())); }

protected:
    asITypeInfo *ot;
};

static bool ScriptTypeTypeidEquals(int other, int *self)
{
    return *self == other;
}

static int ScriptTypeGetTypeof(void *, int typeId)
{
    return typeId;
}

static std::string ScriptTypeGetClassname(int typeId)
{
    asIScriptContext* ctx = asGetActiveContext();
    if ( ctx )
    {
        return ctx->GetEngine()->GetTypeDeclaration(typeId, true);
        // asITypeInfo* objType = ctx->GetEngine()->GetTypeInfoById(typeId);
        // if ( objType )
        // {

        //     const std::string ns( objType->GetNamespace() );
        //     const std::string name( objType->GetName() );

        //     return ns.empty() ? name : (ns + "::" + name);
        // }
    }

    return "";
}

static std::string ScriptTypeGetClassname(void *, int typeId)
{
    return ScriptTypeGetClassname(typeId);
}

static std::string ScriptTypeGetClassname(void *obj)
{
    CScriptType *self = reinterpret_cast<CScriptType*>(obj);
    return ScriptTypeGetClassname(self->GetSubTypeId());
}

static std::string ScriptTypeTypeidToString(int *self)
{
    return ScriptTypeGetClassname(*self);
}

void CScriptType::Construct(asITypeInfo *type, void *mem)
{
    new(mem) CScriptType(type);
}

void CScriptType::CopyConstruct(asITypeInfo *type, const CScriptType &other, void *mem)
{
    new(mem) CScriptType(type, other);
}

void CScriptType::Destruct(CScriptType *obj)
{
    obj->~CScriptType();
}

CScriptType::CScriptType(asITypeInfo *type)
    : ot(type)
{
    ot->AddRef();
}

CScriptType::CScriptType(asITypeInfo *type, const CScriptType &other)
    : ot(type)
{
    ot->AddRef();
    assert( ot == other.ot );
}

CScriptType::~CScriptType()
{
    ot->Release();
}

int CScriptType::GetSubTypeId() const
{
    return ot->GetSubTypeId();
}

bool CScriptType::operator==(const CScriptType &other) const
{
    return GetSubTypeId() == other.GetSubTypeId();
}

bool CScriptType::Equals(int typeId) const
{
    return GetSubTypeId() == typeId;
}


static void RegisterScriptType_Native(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterObjectType("typeid", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
    r = engine->RegisterObjectMethod("typeid", "bool opEquals(typeid) const", asFUNCTIONPR(ScriptTypeTypeidEquals, (int, int*), bool), asCALL_CDECL_OBJLAST); assert( r >= 0 );
    r = engine->RegisterObjectMethod("typeid", "string opImplConv() const",   asFUNCTIONPR(ScriptTypeTypeidToString, (int*), std::string), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    r = engine->RegisterGlobalFunction("typeid typeof(const ?&in)", asFUNCTIONPR(ScriptTypeGetTypeof, (void*, int), int), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterGlobalFunction("string classname(const ?&in)", asFUNCTIONPR(ScriptTypeGetClassname, (void*, int), std::string), asCALL_CDECL); assert( r >= 0 );

    // Register the 'type' type as a template
    r = engine->RegisterObjectType("type<class T>", sizeof(CScriptType), asOBJ_VALUE | asOBJ_APP_CLASS_D | asOBJ_TEMPLATE); assert( r >= 0 );

    r = engine->RegisterObjectBehaviour("type<T>", asBEHAVE_CONSTRUCT, "void f(int&in)", asFUNCTION(CScriptType::Construct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("type<T>", asBEHAVE_CONSTRUCT, "void f(int&in, const type<T> &in)", asFUNCTION(CScriptType::CopyConstruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("type<T>", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(CScriptType::Destruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );

    // The assignment operator
    // r = engine->RegisterObjectMethod("type<T>", "type<T> &opAssign(const type<T>&in)", asMETHOD(CScriptType, operator=), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("type<T>", "bool opEquals(const type<T>&in) const", asMETHOD(CScriptType, operator==), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("type<T>", "bool opEquals(typeid) const",     asMETHODPR(CScriptType, Equals, (int) const, bool), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("type<T>", "typeid opImplConv() const",       asMETHOD(CScriptType, GetSubTypeId), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("type<T>", "string opImplConv() const",       asFUNCTIONPR(ScriptTypeGetClassname, (void*), std::string), asCALL_CDECL_OBJLAST); assert( r >= 0 );
}


string g_str;
void pr(const string &s, unsigned short l)
{
	g_str += s;
	if( l != 10 )
		g_str += " l != 10 ";
	else
		g_str += " l == 10 ";
}

bool TestScriptType()
{
	bool fail = false;
	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);
	RegisterScriptType_Native(engine);

	unsigned short DLVL_INFO = 10;
	engine->RegisterGlobalProperty("const uint16 DLVL_INFO",   (void*)&DLVL_INFO);
	engine->RegisterGlobalFunction("void print(const string& in,uint16 level = DLVL_INFO)", asFUNCTION(pr), asCALL_CDECL);

	int r = ExecuteString(engine, 
		"type<int> typeInt; \n"
		"print('typeInt: ' + typeInt); \n"
		"print('typeInt: ' + type<int>());\n");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	if( g_str != "typeInt: int l == 10 typeInt: int l == 10 " )
	{
		PRINTF("%s", g_str.c_str());
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

} // namespace

