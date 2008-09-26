//
// Test compiling scripts in two named modules
//
// Author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "Test2Modules"
static const char *script = "int global; void Test() {global = 0;} float Test2() {Test(); return 0;}";

bool Test2Modules()
{
	bool ret = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->AddScriptSection("a", "script", script, strlen(script), 0);
	if( engine->Build("a") < 0 )
	{
		printf("%s: failed to build module a\n", TESTNAME);
		ret = true;
	}

	engine->AddScriptSection("b", "script", script, strlen(script), 0);
	if( engine->Build("b") < 0 )
	{
		printf("%s: failed to build module b\n", TESTNAME);
		ret = true;
	}

	if( !ret )
	{
		int aFuncID = engine->GetFunctionIDByName("a", "Test");
		if( aFuncID < 0 )
		{
			printf("%s: failed to retrieve func ID for module a\n", TESTNAME);
			ret = true;
		}

		int bFuncID = engine->GetFunctionIDByName("b", "Test");
		if( bFuncID < 0 )
		{
			printf("%s: failed to retrieve func ID for module b\n", TESTNAME);
			ret = true;
		}
	}

	engine->Release();

	// Test using an object created in another module
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterInterface("ITest");
	engine->RegisterInterfaceMethod("ITest", "void test()");
	const char *scriptA = "ITest @obj;";
	const char *scriptB = 
	"class CTest : ITest        \n"
	"{                          \n"
	"  void test() {glob = 42;} \n"
	"}                          \n"
	"int glob = 0;              \n";

	engine->AddScriptSection("a", "scriptA", scriptA, strlen(scriptA));
	r = engine->Build("a");
	if( r < 0 ) ret = true;

	engine->AddScriptSection("b", "scriptB", scriptB, strlen(scriptB));
	engine->Build("b");
	if( r < 0 ) ret = true;

	asIScriptStruct *obj = (asIScriptStruct*)engine->CreateScriptObject(engine->GetTypeIdByDecl("b", "CTest"));
	*((asIScriptStruct**)engine->GetAddressOfGlobalVar("a", 0)) = obj;
	r = engine->ExecuteString("a", "obj.test()");
	if( r != asEXECUTION_FINISHED ) ret = true;
	int val = *(int*)engine->GetAddressOfGlobalVar("b", engine->GetGlobalVarIndexByName("b", "glob"));
	if( val != 42 ) ret = true;

	engine->Release();

	return ret;
}
