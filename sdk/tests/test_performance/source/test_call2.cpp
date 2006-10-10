//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "../../add_on/scriptstring/scriptstring.h"

namespace TestCall2
{

#define TESTNAME "TestCall2"

static const char *script =
"void TestCall2_A()                                               \n"
"{                                                                \n"
"}                                                                \n"
"void TestCall2_B()                                               \n"
"{                                                                \n"
"}                                                                \n";


                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.7.0 rev 37      : 1.469 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetCommonMessageStream(&out);

	RegisterScriptString(engine);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	asIScriptContext *ctx = engine->CreateContext();
	int funcId_A = engine->GetFunctionIDByDecl(0, "void TestCall2_A()");
	int funcId_B = engine->GetFunctionIDByDecl(0, "void TestCall2_B()");

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();
	int r;

	for( int n = 0; n < 5000000; n++ )
	{
		ctx->Prepare(funcId_A);
		r = ctx->Execute();
		if( r != 0 ) break;
		ctx->Prepare(funcId_B);
		r = ctx->Execute();
		if( r != 0 ) break;
	}

	time = GetSystemTimer() - time;

	if( r != 0 )
	{
		printf("Execution didn't terminate with asEXECUTION_FINISHED\n", TESTNAME);
		if( r == asEXECUTION_EXCEPTION )
		{
			printf("Script exception\n");
			printf("Func: %s\n", engine->GetFunctionName(ctx->GetExceptionFunction()));
			printf("Line: %d\n", ctx->GetExceptionLineNumber());
			printf("Desc: %s\n", ctx->GetExceptionString());
		}
	}
	else
		printf("Time = %f secs\n", time);

	ctx->Release();
	engine->Release();
}

} // namespace







