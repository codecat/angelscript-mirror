//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "../../add_on/scriptstring/scriptstring.h"

namespace TestMthd
{

#define TESTNAME "TestMthd"

static const char *script =
"class cls                                                       \n"
"{                                                               \n"
"    void func0()                                    {}          \n"
"    void func1(int)                                 {}          \n"
"    void func2(int,int)                             {}          \n"
"    void func3(int,int,int)                         {}          \n"
"    void func4(int,int,int,int)                     {}          \n"
"    void func5(int,int,int,int,int)                 {}          \n"
"    void func6(int,int,int,int,int,int)             {}          \n"
"    void func7(int,int,int,int,int,int,int)         {}          \n"
"    void func8(int,int,int,int,int,int,int,int)     {}          \n"
"    void func9(int,int,int,int,int,int,int,int,int) {}          \n"
"}                                                               \n"
"                                                                \n"
"void TestMthd()                                                 \n"
"{                                                               \n"
"    cls obj;                                                    \n"
"                                                                \n"
"    for( int n = 0; n < 1000000; n++ )                          \n"
"    {                                                           \n"
"        obj.func0();                                            \n"
"        obj.func1(1);                                           \n"
"        obj.func2(1,2);                                         \n"
"        obj.func3(1,2,3);                                       \n"
"        obj.func4(1,2,3,4);                                     \n"
"        obj.func5(1,2,3,4,5);                                   \n"
"        obj.func6(1,2,3,4,5,6);                                 \n"
"        obj.func7(1,2,3,4,5,6,7);                               \n"
"        obj.func8(1,2,3,4,5,6,7,8);                             \n"
"        obj.func9(1,2,3,4,5,6,7,8,9);                           \n"
"    }                                                           \n"
"}                                                               \n";

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.7.0 rev 36      : 3.365 secs\n");
	printf("AngelScript 2.7.0 rev 37      : 2.947 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetCommonMessageStream(&out);

	RegisterScriptString(engine);

	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestMthd()"));

	printf("Executing AngelScript version...\n");

	double time = GetSystemTimer();

	int r = ctx->Execute();

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







