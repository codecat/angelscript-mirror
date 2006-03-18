//
// Tests releasing a suspended context
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestSuspend
{

#define TESTNAME "TestSuspend"


static int loopCount = 0;

static const char *script1 =
"string g_str = \"test\";    \n" // variable that must be released when module is released
"void TestSuspend()          \n"
"{                           \n"
"  string str = \"hello\";   \n" // variable that must be released before exiting the function
"  while( true )             \n" // never ending loop
"  {                         \n"
"    string a = str + g_str; \n" // variable inside the loop
"    Suspend();              \n"
"    loopCount++;            \n"
"  }                         \n"
"}                           \n";

static const char *script2 = 
"void TestSuspend2()         \n"
"{                           \n"
"  loopCount++;              \n"
"  loopCount++;              \n"
"  loopCount++;              \n"
"}                           \n";

bool doSuspend = false;
void Suspend()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx ) ctx->Suspend();
	doSuspend = true;
}

bool Test()
{
	bool fail = false;

	//---
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	// Verify that the function doesn't crash when the stack is empty
	asIScriptContext *ctx;
	ctx = asGetActiveContext();
	assert( ctx == 0 );

	RegisterStdString(engine);
	
	engine->RegisterGlobalFunction("void Suspend()", asFUNCTION(Suspend), asCALL_CDECL);
	engine->RegisterGlobalProperty("int loopCount", &loopCount);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);

	engine->Build(0, &out);

	engine->CreateContext(&ctx);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestSuspend()"));

	while( loopCount < 5 && !doSuspend )
		ctx->ExecuteStep(0);

	// Release the engine first
	engine->Release();

	// Now release the context
	ctx->Release();
	//---
	// If the library was built with the flag BUILD_WITH_LINE_CUES the script
	// will return after each increment of the loopCount variable.
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalProperty("int loopCount", &loopCount);
	engine->AddScriptSection(0, TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build(0, &out);

	engine->CreateContext(&ctx);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestSuspend2()"));
	loopCount = 0;
	while( ctx->GetState() != asEXECUTION_FINISHED )
		ctx->ExecuteStep(0);
	if( loopCount != 3 )
	{
		printf("%s: failed\n", TESTNAME);
		fail = true;
	}

	ctx->Prepare(asPREPARE_PREVIOUS);
	loopCount = 0;
	while( ctx->GetState() != asEXECUTION_FINISHED )
		ctx->ExecuteStep(0);
	if( loopCount != 3 )
	{
		printf("%s: failed\n", TESTNAME);
		fail = true;
	}

	ctx->Release();
	engine->Release();

	// Success
	return fail;
}

} // namespace

