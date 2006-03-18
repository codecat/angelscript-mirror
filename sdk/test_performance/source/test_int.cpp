//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestInt
{

#define TESTNAME "TestInt"

static const char *script =
"int N;                                    \n"
"                                          \n"
"void func5()                              \n"
"{                                         \n"
"    N += Average( N, N );                 \n"
"}                                         \n"
"                                          \n"
"void func4()                              \n"
"{                                         \n"
"    N += 2 * Average( N + 1, N + 2 );     \n"
"}                                         \n"
"                                          \n"
"void func3()                              \n"
"{                                         \n"
"    N *= 2 * N;                           \n"
"}                                         \n"
"                                          \n"
"void func2()                              \n"
"{                                         \n"
"    N /= 3;                               \n"
"}                                         \n"
"                                          \n"
"void Recursion( int nRec )                \n"
"{                                         \n"
"    if ( nRec >= 1 )                      \n"
"        Recursion( nRec - 1 );            \n"
"                                          \n"
"    if ( nRec == 5 )                      \n"
"        func5();                          \n"
"    else if ( nRec == 4 )                 \n"
"        func4();                          \n"
"    else if ( nRec == 3 )                 \n"
"        func3();                          \n"
"    else if ( nRec == 2 )                 \n"
"        func2();                          \n"
"    else                                  \n"
"        N *= 2;                           \n"
"}                                         \n"
"                                          \n"
"int TestInt()                             \n"
"{                                         \n"
"    N = 0;                                \n"
"    int i = 0;                            \n"
"                                          \n"
"    for ( i = 0; i < 250000; i++ )        \n"
"    {                                     \n"
"        Average( i, i );                  \n"
"        Recursion( 5 );                   \n"
"                                          \n"
"        if ( N > 100 ) N = 0;             \n"
"    }                                     \n"
"                                          \n"
"    return 0;                             \n"
"}                                         \n";

int Average(int a, int b)
{
	return (a+b)/2;
}

                                         
void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("Machine 1\n");
	printf("AngelScript 1.9.0             : 5.229 secs\n");
	printf("AngelScript 1.9.1             : 3.358 secs\n");
	printf("AngelScript 1.9.2             : 2.787 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: 1.539 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: 1.716 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: 1.054 secs\n");
	printf("\n");
	printf("Machine 2\n");
	printf("AngelScript 1.9.0             : 2.138 secs\n");
	printf("AngelScript 1.9.1             : 1.371 secs\n");
	printf("AngelScript 1.9.2             : 1.116 secs\n");
	printf("AngelScript 1.10.0 with C++ VM: .5908 secs\n");
	printf("AngelScript 1.10.0 with ASM VM: .7899 secs\n");
	printf("AngelScript 1.10.1 with ASM VM: .5400 secs\n");
	printf("AngelScript 2.0.0 with C++ VM : .6316 secs\n");
	printf("AngelScript 2.0.0 with ASM VM : .5757 secs\n");

	printf("\nBuilding...\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("int Average(int, int)", asFUNCTION(Average), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0, &out);

	asIScriptContext *ctx;
	engine->CreateContext(&ctx);
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "int TestInt()"));

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







