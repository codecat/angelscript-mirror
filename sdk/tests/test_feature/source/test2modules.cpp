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
	
	asIScriptModule *mod = engine->GetModule("a", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script), 0);
	if( mod->Build() < 0 )
	{
		printf("%s: failed to build module a\n", TESTNAME);
		ret = true;
	}

	mod = engine->GetModule("b", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script), 0);
	if( mod->Build() < 0 )
	{
		printf("%s: failed to build module b\n", TESTNAME);
		ret = true;
	}

	if( !ret )
	{
		int aFuncID = engine->GetModule("a")->GetFunctionIdByName("Test");
		if( aFuncID < 0 )
		{
			printf("%s: failed to retrieve func ID for module a\n", TESTNAME);
			ret = true;
		}

		int bFuncID = engine->GetModule("b")->GetFunctionIdByName("Test");
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

	mod = engine->GetModule("a", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("scriptA", scriptA, strlen(scriptA));
	r = mod->Build();
	if( r < 0 ) ret = true;

	mod = engine->GetModule("b", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("scriptB", scriptB, strlen(scriptB));
	mod->Build();
	if( r < 0 ) ret = true;

	asIScriptStruct *obj = (asIScriptStruct*)engine->CreateScriptObject(engine->GetModule("b")->GetTypeIdByDecl("CTest"));
	*((asIScriptStruct**)engine->GetModule("a")->GetAddressOfGlobalVar(0)) = obj;
	r = engine->ExecuteString("a", "obj.test()");
	if( r != asEXECUTION_FINISHED ) ret = true;
	int val = *(int*)engine->GetModule("b")->GetAddressOfGlobalVar(engine->GetModule("b")->GetGlobalVarIndexByName("glob"));
	if( val != 42 ) ret = true;

	engine->Release();

	return ret;
}
