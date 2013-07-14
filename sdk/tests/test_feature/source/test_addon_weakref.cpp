#include "utils.h"
#include "../../../add_on/weakref/weakref.h"

namespace Test_Addon_WeakRef
{

static const char *TESTNAME = "Test_Addon_WeakRef";

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
			"  weakref<Test@> c; \n"       // ok. The weak ref will hold a pointer to the object
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
						//   "Test_Addon_WeakRef (5, 11) : Error   : Can't instanciate template 'weakref' with subtype 'Test@'\n"
						   "Test_Addon_WeakRef (6, 11) : Error   : Can't instanciate template 'weakref' with subtype 'array<Test>'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// TODO: weak: Test registering app type with weak ref
	// TODO: weak: It shouldn't be possible to register the behaviour to a type that don't support handles
	// TODO: weak: The engine should have a global function for creating a shared boolean

	// Success
	return fail;
}


} // namespace

