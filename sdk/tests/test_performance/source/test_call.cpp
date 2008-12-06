//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "../../add_on/scriptstring/scriptstring.h"

namespace TestCall
{

#define TESTNAME "TestCall"

static const char *script =
"void TestCall()                                                 \n"
"{                                                               \n"
"}                                                               \n";

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.7.0 rev 37      : .9823 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script, strlen(script), 0);
	mod->Build();

	asIScriptContext *ctx = engine->CreateContext();
	int funcId = mod->GetFunctionIdByDecl("void TestCall()");

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();
	int r;

	for( int n = 0; n < 10000000; n++ )
	{
		ctx->Prepare(funcId);
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
			asIScriptFunction *func = engine->GetFunctionDescriptorById(ctx->GetExceptionFunction());
			printf("Func: %s\n", func->GetName());
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







