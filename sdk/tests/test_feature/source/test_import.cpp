//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport
{

#define TESTNAME "TestImport"




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

	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->Build(0);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	// Bind all functions that the module imports
	r = engine->BindAllImportedFunctions(0); assert( r >= 0 );

	engine->ExecuteString(0, "main()");

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

	engine->AddScriptSection(0, TESTNAME ":3", script3, strlen(script3), 0);
	r = engine->Build(0); assert( r >= 0 );

	engine->AddScriptSection("DynamicModule", TESTNAME ":4", script4, strlen(script4), 0);
	r = engine->Build("DynamicModule"); assert( r >= 0 );

	// Bind all functions that the module imports
	r = engine->BindAllImportedFunctions(0); assert( r < 0 );

	engine->Release();

	// Success
	return fail;
}

} // namespace

