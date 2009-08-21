#include "utils.h"

namespace TestGetSet
{


bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	asIScriptModule *mod;
 	asIScriptEngine *engine;
	

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

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
		
	mod->AddScriptSection("script", script1);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
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

	// Test compound assignment with accessors (not allowed)
	const char *script2 = 
		"class Test                            \n"
		"{                                     \n"
		"  void set_prop(int v) { _prop = v; } \n"
        "  int _prop;                          \n"
		"}                                     \n"
		"void main1()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.prop += 42;                       \n"
		"}                                     \n";

	mod->AddScriptSection("script", script2);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (6, 1) : Info    : Compiling void main1()\n"
	                   "script (9, 10) : Error   : Compound assignments with property accessors are not allowed\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with boolean operators
	const char *script3 = 
		"class Test                              \n"
		"{                                       \n"
		"  bool get_boolProp() { return true; }  \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  if( t.boolProp ) {}                   \n"
		"  if( t.boolProp && true ) {}           \n"
		"  if( false || t.boolProp ) {}          \n"
		"  if( t.boolProp ^^ t.boolProp ) {}     \n"
		"  if( !t.boolProp ) {}                  \n"
		"  t.boolProp ? t.boolProp : t.boolProp; \n"
		"}                                       \n";

	mod->AddScriptSection("script", script3);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with math operators
	const char *script4 = 
		"class Test                              \n"
		"{                                       \n"
		"  float get_prop() { return 1.0f; }     \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  float f = t.prop * 1;                 \n"
		"  10 / t.prop;                          \n"
		"  -t.prop;                              \n"
		"}                                       \n";

	mod->AddScriptSection("script", script4);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with bitwise operators
	const char *script5 = 
		"class Test                              \n"
		"{                                       \n"
		"  uint get_prop() { return 1; }         \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  t.prop << t.prop;                     \n"
		"  t.prop & t.prop;                      \n"
		"  ~t.prop;                              \n"
		"}                                       \n";

	mod->AddScriptSection("script", script5);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// TODO: Test multiple get accessors for same property. Should give error
	// TODO: Test multiple set accessors for same property. Should give error
	// TODO: Test mismatching type between get accessor and set accessor. Should give error
	// TODO: Test only set accessor for read expression
	// TODO: Test only get accessor for write expression
	// TODO: Test pre and post ++ (should probably fail, since the expression is not a variable)
	// TODO: Test accessor where the object is a handle
	// TODO: Test using property accessors from within class methods without 'this'
	// TODO: Test accessors with function arguments (by value, in ref, out ref, inout ref)
	// TODO: Test const/non-const get accessor
	// TODO: Test get accessor that returns a reference (only from application func to start with)
	// TODO: Test set accessor with parameter declared as out ref (shouldn't be found)
	// TODO: Test everything for properties as value type, reference type, handle, scoped type, etc.

	engine->Release();

	// Success
	return fail;
}

} // namespace

