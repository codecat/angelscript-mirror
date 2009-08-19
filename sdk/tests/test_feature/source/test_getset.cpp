#include "utils.h"

namespace TestGetSet
{


bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptModule *mod;
 	asIScriptEngine *engine;
	

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);


	// main1 and main2 should produce the same bytecode
	const char *script1 = 
		"class Test                            \n"
		"{                                     \n"
		"  int get_prop() { return _prop; }    \n"
		"  void set_prop(int v) { _prop = v; } \n"
        "  int _prop;                          \n"
		"}                                     \n"
		"void main1()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.set_prop(42);                     \n"
		"  assert( t.get_prop() == 42 );       \n"
		"}                                     \n"
		"void main2()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.prop = 42;                        \n"
		"  assert( t.prop == 42 );             \n"
		"}                                     \n";
		
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}

	r = engine->ExecuteString(0, "main1()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = engine->ExecuteString(0, "main2()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

