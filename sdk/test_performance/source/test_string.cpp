//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "../../add_on/std_string/stdstring.h"

namespace TestString
{

#define TESTNAME "TestString"

static const char *script =
"string BuildString(string & a, string & b, string & c)          \n"
"{                                                               \n"
"    return a + b + c;                                           \n"
"}                                                               \n"
"                                                                \n"
"void TestString()                                               \n"
"{                                                               \n"
"    string a = \"Test\";                                        \n"
"    string b = \"string\";                                      \n"
"    int i = 0;                                                  \n"
"                                                                \n"
"    for ( i = 0; i < 1000000; i++ )                             \n"
"    {                                                           \n"
"        string res = BuildString(a, \" \", b);                  \n"
"    }                                                           \n"
"}                                                               \n";

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("Machine 1\n");
	printf("AngelScript 1.9.0             : 11.00 secs\n");
	printf("AngelScript 1.9.1             : 9.618 secs\n");
	printf("AngelScript 1.9.2             : 8.748 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: 7.073 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: 7.613 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: 6.432 secs\n");
	printf("\n");
	printf("Machine 2\n");
	printf("AngelScript 1.9.0             : 4.806 secs\n");
	printf("AngelScript 1.9.1             : 4.300 secs\n");
	printf("AngelScript 1.9.2             : 3.686 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: 2.973 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: 3.329 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: 2.936 secs\n");
	printf("AngelScript 2.0.0  with C++ VM: 6.182 secs\n");
	printf("AngelScript 2.0.0  with ASM VM: 5.958 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterStdString(engine);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0, &out);

	asIScriptContext *ctx;
	engine->CreateContext(&ctx);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void TestString()"));

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







