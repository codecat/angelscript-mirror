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
#include <stdio.h>
#ifdef _MSC_VER
#include <direct.h>
#endif

#define TESTNAME "TestExecuteScript"

static int LoadScript(const char *filename);
static int CompileScript();
static bool ExecuteScript();

static asIScriptEngine *engine;

bool TestExecuteScript()
{
	bool ret = false;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( LoadScript("scripts/TestExecuteScript.as") < 0 )
	{
		engine->Release();
	    return false;
	}
	CompileScript();
	ret = ExecuteScript();

	engine->Release();
	engine = NULL;

	return ret;
}

const char *GetCurrentDir(char *buf, size_t size)
{
#ifdef _MSC_VER
	return _getcwd(buf, (int)size);
#elif defined(__APPLE__)
	return getcwd(buf, size);
#else
	return "";
#endif
}

static int LoadScript(const char *filename)
{
	// Read the entire script file
	FILE *f = fopen(filename, "rb");
	if( f == 0 )
	{
		char buf[256];
		printf("%s/%s\n", GetCurrentDir(buf, 256), filename);
		printf("%s: Failed to open the script file.\n", TESTNAME);
		return -1;
	}

	
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// On Win32 it is possible to do the following instead
	// int len = _filelength(_fileno(f));

	std::string code;
	code.resize(len);

	size_t c = fread(&code[0], len, 1, f);
	fclose(f);

	if( c == 0 ) 
	{
		printf("%s: Failed to load script file.\n", TESTNAME);
		return -1;
	}

	// Give the code to the script engine
	int r = engine->AddScriptSection(0, filename, code.c_str(), len, 0);
	if( r < 0 ) 
	{
		printf("%s: An error occured while adding the script section.\n", TESTNAME);
		return r;
	}

	// At this point the engine has copied the code to an 
	// internal buffer so we are free to release the memory we 
	// allocated. 

	// We can also add other script sections if we would like.
	// All script sections will be compiled together as if they
	// where one large script. 

	return 0;
}





static int CompileScript()
{
	// Create an output stream that will receive information about the build
	COutStream out;

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	int r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to compile script\n", TESTNAME);
		return -1;
	}

	// If we wish to build again, the script sections has to be added again.

	// Now we will verify the interface of the script functions we wish to call
	const char *buffer = 0;
	buffer = engine->GetFunctionDescriptorById(engine->GetFunctionIDByName(0, "main"))->GetDeclaration();
	if( buffer == 0 )
	{
		printf("%s: Failed to retrieve declaration of function 'main'\n", TESTNAME);
		return -1;
	}

	engine->ClearMessageCallback();

	return 0;
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
