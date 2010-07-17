//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport
{

static const char * const TESTNAME = "TestImport";




static const char *script1 =
"import string Test(string s) from \"DynamicModule\";   \n"
"void main()                                            \n"
"{                                                      \n"
"  Test(\"test\");                                      \n"
"}                                                      \n";

static const char *script2 =
"string Test(string s)    \n"
"{                        \n"
"  number = 1234567890;   \n"
"  return \"blah\";       \n"
"}                        \n";


static const char *script3 =
"class A                                         \n"
"{                                               \n"
"  int a;                                        \n"
"}                                               \n"
"import void Func(A&out) from \"DynamicModule\"; \n"
"import A@ Func2() from \"DynamicModule\";       \n";


static const char *script4 = 
"class A                   \n"
"{                         \n"
"  int a;                  \n"
"}                         \n"
"void Func(A&out) {}       \n"
"A@ Func2() {return null;} \n";



bool Test()
{
	bool fail = false;

	int number = 0;
	int r;
	asIScriptEngine *engine = 0;
	COutStream out;

	// Test 1
	// Importing a function from another module
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalProperty("int number", &number);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":1", script1, strlen(script1), 0);
	mod->Build();

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":2", script2, strlen(script2), 0);
	mod->Build();

	// Bind all functions that the module imports
	r = engine->GetModule(0)->BindAllImportedFunctions(); assert( r >= 0 );

	ExecuteString(engine, "main()", engine->GetModule(0));

	engine->Release();

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Test 2
	// Two identical structures declared in different modules are not the same
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString_Generic(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":3", script3, strlen(script3), 0);
	r = mod->Build(); assert( r >= 0 );

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":4", script4, strlen(script4), 0);
	r = mod->Build(); assert( r >= 0 );

	// Bind all functions that the module imports
	r = engine->GetModule(0)->BindAllImportedFunctions(); assert( r < 0 );

	{
		const char *script = 
		    "import int test(void) from 'mod1'; \n"
		    "void main() \n"
	   	    "{ \n"
		    "  int str; \n"
		    "  str = test(); \n"
		    "}\n";

		mod->AddScriptSection("4", script);
		r = mod->Build();
		if( r < 0 )
			fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

 