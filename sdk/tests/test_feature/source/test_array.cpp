#include "utils.h"

namespace TestArray
{

#define TESTNAME "TestArray"


static void Assert(bool expr)
{
	if( !expr )
	{
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			printf("func: %s\n", engine->GetFunctionDeclaration(ctx->GetCurrentFunction()));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
		}
	}
}

static const char *script1 =
"string[] b;                                     \n"
"int[] g_a(3);                                   \n"
"void TestArray()                                \n"
"{                                               \n"
"   string[] a(5);                               \n"
"   Assert(a.length() == 5);                     \n"
"   a.resize(10);                                \n"
"   a.resize(5);                                 \n"
"   a[0] = \"Hello\";                            \n"
"   Assert(a[0] == \"Hello\");                   \n"
"   uint n = 0;                                  \n"
"   Assert(a[n] == \"Hello\");                   \n"
"   n++;                                         \n"
"   Assert(a[n] == \"\");                        \n"
"   b = a;                                       \n"
"   Assert(b.length() == 5);                     \n"
"   Assert(b[0] == \"Hello\");                   \n"
"   b[0] = \"Goodbye\";                          \n"
"   Assert(a[0] != \"Goodbye\");                 \n"
"   int[] ia = TestArray4();                     \n"
"   TestArray2(ia);                              \n"
"   TestArray3(ia);                              \n"
"   ia = int[](3);                               \n"
"   Assert(ia.length() == 3);                    \n"
"   ia[0] = 1;                                   \n"
"   int[] ib = ia;                               \n"
"   Assert(ib.length() == ia.length());          \n"
"   Assert(ib[0] == ia[0]);                      \n"
"}                                               \n"
"void TestArray2(int[] &inout a)                 \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"void TestArray3(int[] a)                        \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"int[] TestArray4()                              \n"
"{                                               \n"
"   int[] ia(3);                                 \n"
"   ia[0] = 1;                                   \n"
"   ia[1] = 2;                                   \n"
"   ia[2] = 3;                                   \n"
"   return ia;                                   \n"
"}                                               \n";

static const char *script2 = 
"void TestArrayException()                       \n"
"{                                               \n"
"   string[] a;                                  \n"
"   a[0] == \"Hello\";                           \n"
"}                                               \n";

static const char *script3 = 
"void TestArrayMulti()                           \n"
"{                                               \n"
"   int[][] a(2);                                \n"
"   int[] b(2);                                  \n"
"   a[0] = b;                                    \n"
"   a[1] = b;                                    \n"
"                                                \n"
"   a[0][0] = 0;                                 \n"
"   a[0][1] = 1;                                 \n"
"   a[1][0] = 2;                                 \n"
"   a[1][1] = 3;                                 \n"
"                                                \n"
"   Assert(a[0][0] == 0);                        \n"
"   Assert(a[0][1] == 1);                        \n"
"   Assert(a[1][0] == 2);                        \n"
"   Assert(a[1][1] == 3);                        \n"
"}                                               \n";

static const char *script4 = 
"void TestArrayChar()                            \n"
"{                                               \n"
"   int8[] a(2);                                 \n"
"   a[0] = 13;                                   \n"
"   a[1] = 19;                                   \n"
"                                                \n"
"   int8 a0 = a[0];                              \n"
"   int8 a1 = a[1];                              \n"
"   Assert(a[0] == 13);                          \n"
"   Assert(a[1] == 19);                          \n"
"}                                               \n";

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "TestArray()", 0, &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "TestArrayException()");
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: No exception\n", TESTNAME);
		fail = true;
	}

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0, false);
	r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "TestArrayMulti()", 0, &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failure\n", TESTNAME);
		fail = true;
	}
	if( r == asEXECUTION_EXCEPTION )
	{
		PrintException(ctx);
	}
	if( ctx ) ctx->Release();
	ctx = 0;

	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0, false);
	r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	r = engine->ExecuteString(0, "TestArrayChar()", 0, &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failure\n", TESTNAME);
		fail = true;
	}
	if( r == asEXECUTION_EXCEPTION )
	{
		PrintException(ctx);
	}

	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

