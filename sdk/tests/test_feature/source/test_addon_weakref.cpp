#include "utils.h"
#include "../../../add_on/weakref/weakref.h"

namespace Test_Addon_WeakRef
{

static const char *TESTNAME = "Test_Addon_WeakRef";

class MyClass
{
public:
	MyClass() { refCount = 1; weakRefFlag = 0; }
	~MyClass()
	{
		if( weakRefFlag )
		{
			weakRefFlag->Set(true);
			weakRefFlag->Release();
		}
	}
	void AddRef() { refCount++; }
	void Release() { if( --refCount == 0 ) delete this; }
	asISharedBool *GetWeakRefFlag()
	{
		if( !weakRefFlag )
			weakRefFlag = asCreateSharedBool();

		return weakRefFlag;
	}

	static MyClass *Factory() { return new MyClass(); }

protected:
	int refCount;
	asISharedBool *weakRefFlag;
};

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
	asIScriptEngine *engine;

	// Ordinary tests
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptWeakRef(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"class Test {} \n"
			"void main() { \n"
			"  Test @t = Test(); \n"
			"  weakref<Test> r(t); \n"
			"  assert( r.get() !is null ); \n"
			"  @t = null; \n"
			"  assert( r.get() is null ); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
			printf("%s: Failed to compile the script\n", TESTNAME);
		}

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// It shouldn't be possible to instanciate the weakref for types that do not support it
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		RegisterScriptWeakRef(engine);
		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"class Test {} \n"
			"void main() { \n"
			"  weakref<int> a; \n"         // fail
			"  weakref<string> b; \n"      // fail
			"  weakref<Test@> c; \n"       // fail
			"  weakref<array<Test>> d; \n" // fail
			"} \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "Test_Addon_WeakRef (2, 1) : Info    : Compiling void main()\n"
						   "Test_Addon_WeakRef (3, 11) : Error   : Can't instanciate template 'weakref' with subtype 'int'\n"
						   "Test_Addon_WeakRef (4, 11) : Error   : Can't instanciate template 'weakref' with subtype 'string'\n"
						   "Test_Addon_WeakRef (5, 11) : Error   : Can't instanciate template 'weakref' with subtype 'Test@'\n"
						   "Test_Addon_WeakRef (6, 11) : Error   : Can't instanciate template 'weakref' with subtype 'array<Test>'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test registering app type with weak ref
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->RegisterObjectType("MyClass", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("MyClass", asBEHAVE_FACTORY, "MyClass @f()", asFUNCTION(MyClass::Factory), asCALL_CDECL);
		engine->RegisterObjectBehaviour("MyClass", asBEHAVE_ADDREF, "void f()", asMETHOD(MyClass, AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("MyClass", asBEHAVE_RELEASE, "void f()", asMETHOD(MyClass, Release), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("MyClass", asBEHAVE_GET_WEAKREF_FLAG, "int &f()", asMETHOD(MyClass, GetWeakRefFlag), asCALL_THISCALL);

		RegisterScriptWeakRef(engine);

		const char *script =
			"void main() { \n"
			"  MyClass @t = MyClass(); \n"
			"  weakref<MyClass> r(t); \n"
			"  assert( r.get() !is null ); \n"
			"  @t = null; \n"
			"  assert( r.get() is null ); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
			printf("%s: Failed to compile the script\n", TESTNAME);
		}

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// TODO: weak: It should be possible to declare a script class to not allow weak references, and as such save the memory for the internal pointer
	// TODO: weak: add engine property to turn off automatic support for weak references to all script classes

	// Success
	return fail;
}


} // namespace

