//
// Unit test author: Fredrik Ehnbom
//
// Description:
//
// Tests calling a script-function from c.
// Based on the sample found on angelscripts
// homepage.
// 

#include "utils.h"

#pragma warning (disable:4786)
#include "../../../add_on/scriptbuilder/scriptbuilder.h"

#define TESTNAME "TestExecuteScript"

static bool ExecuteScript();

static asIScriptEngine *engine;

bool TestExecuteScript()
{
	bool ret = false;
	COutStream out;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	CScriptBuilder builder;

	int r = builder.BuildScriptFromFile(engine, 0, "scripts/TestExecuteScript.as");
	if( r >= 0 )
	{
		ret = ExecuteScript();
	}

	engine->Release();
	engine = NULL;

	return ret;
}


static bool ExecuteScript()
{
	// Create a context in which the script will be executed. 

	// Several contexts may exist in parallel, holding the execution 
	// of various scripts in the same engine. At the moment contexts are not
	// thread safe though so you should make sure that only one executes 
	// at a time. An execution can be suspended to allow another 
	// context to execute.

	asIScriptContext *ctx = engine->CreateContext();
	if( ctx == 0 )
	{
		printf("%s: Failed to create a context\n", TESTNAME);
		return true;
	}

	// Prepare the context for execution

	// When a context has finished executing the context can be reused by calling
	// PrepareContext on it again. If the same stack size is used as the last time
	// there will not be any new allocation thus saving some time.

	int r = ctx->Prepare(engine->GetFunctionIDByName(0, "main"));
	if( r < 0 )
	{
		printf("%s: Failed to prepare context\n", TESTNAME);
		return true;
	}

	// If the script function takes any parameters we need to  
	// copy them to the context's stack by using SetArguments()

	// Execute script

	r = ctx->Execute();
	if( r < 0 )
	{
		printf("%s: Unexpected error during script execution\n", TESTNAME);
		return true;
	}

	if( r == asEXECUTION_FINISHED )
	{
		// If the script function is returning any  
		// data we can get it with GetReturnValue()
		float retVal = ctx->GetReturnFloat();

		if (retVal == 7.826446f)
			r = 0;
		else
			printf("%s: Script didn't return the correct value. Returned: %f, expected: %f\n", TESTNAME, retVal, 7.826446f);
	}
	else if( r == asEXECUTION_SUSPENDED )
	{
		printf("%s: Execution was suspended.\n", TESTNAME);
		
		// In this case we can call Execute again to continue 
		// execution where it last stopped.

		int funcID = ctx->GetCurrentFunction();
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		printf("func : %s\n", func->GetName());
		printf("line : %d\n", ctx->GetCurrentLineNumber());
	}
	else if( r == asEXECUTION_ABORTED )
	{
		printf("%s: Execution was aborted.\n", TESTNAME);
	}
	else if( r == asEXECUTION_EXCEPTION )
	{
		printf("%s: An exception occured during execution\n", TESTNAME);

		// Print exception description
		int funcID = ctx->GetExceptionFunction();
		const asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
		printf("func : %s\n", func->GetName());
		printf("line : %d\n", ctx->GetExceptionLineNumber());
		printf("desc : %s\n", ctx->GetExceptionString());
	}

	// Don't forget to release the context when you are finished with it
	ctx->Release();

	return false;
}
