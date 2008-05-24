#include "utils.h"
using namespace std;

#define TESTNAME "TestException"

// This script will cause an exception inside a class method
const char *script1 =
"class A               \n"
"{                     \n"
"  void Test(string c) \n"
"  {                   \n"
"    int a = 0, b = 0; \n"
"    a = a/b;          \n"
"  }                   \n"
"}                     \n";

bool TestException()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);


	COutStream out;	
	asIScriptContext *ctx;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	int r = engine->ExecuteString(0, "int a = 0;\na = 10/a;", &ctx); // Throws an exception
	if( r == asEXECUTION_EXCEPTION )
	{
		int func = ctx->GetExceptionFunction();
		int line = ctx->GetExceptionLineNumber();
		const char *desc = ctx->GetExceptionString();

		if( func != 0xFFFE )
		{
			printf("%s: Exception function ID is wrong\n", TESTNAME);
			fail = true;
		}
/*
		// TODO: This is temporarily not working. Only when AddRef and Release is added
		// to asIScriptFunction can we add this support again.
		const asIScriptFunction *function = engine->GetFunctionDescriptorById(func);
		if( strcmp(function->GetName(), "@ExecuteString") != 0 )
		{
			printf("%s: Exception function name is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(function->GetDeclaration(), "void @ExecuteString()") != 0 )
		{
			printf("%s: Exception function declaration is wrong\n", TESTNAME);
			fail = true;
		}
*/
		if( line != 2 )
		{
			printf("%s: Exception line number is wrong\n", TESTNAME);
			fail = true;
		}
		if( strcmp(desc, "Divide by zero") != 0 )
		{
			printf("%s: Exception string is wrong\n", TESTNAME);
			fail = true;
		}
	}
	else
	{
		printf("%s: Failed to raise exception\n", TESTNAME);
		fail = true;
	}

	ctx->Release();

	engine->AddScriptSection(0, "script", script1, strlen(script1));
	engine->Build(0);
	r = engine->ExecuteString(0, "A a; a.Test(\"test\");");
	if( r != asEXECUTION_EXCEPTION )
	{
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}
