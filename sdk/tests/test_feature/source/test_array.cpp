#include "utils.h"

namespace TestArray
{

#define TESTNAME "TestArray"

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

static const char *script5 = 
"int[] g = {1,2,3};                              \n"
"void TestArrayInitList()                        \n"
"{                                               \n"
"   Assert(g.length() == 3);                     \n"
"   Assert(g[2] == 3);                           \n"
"   int[] a = {,2,};                             \n"
"   Assert(a.length() == 3);                     \n"
"   Assert(a[1] == 2);                           \n"
"   string[] b = {\"test\", \"3\"};              \n"
"   Assert(b.length() == 2);                     \n"
"   Assert(b[0] == \"test\");                    \n"
"   Assert(b[1] == \"3\");                       \n"
"   int[][] c = {,{23},{23,4},};                 \n"
"   Assert(c.length() == 4);                     \n"
"   Assert(c[2].length() == 2);                  \n"
"   Assert(c[2][1] == 4);                        \n"
"   const int[] d = {0,1,2};                     \n"
"   Assert(d.length() == 3);                     \n"
"   Assert(d[2] == 2);                           \n"
"}                                               \n";

static const char *script6 =
"void Test()                                     \n"
"{                                               \n"
"   int[]@ e = {2,5};                            \n"
"   int[] f = {,{23}};                           \n"
"}                                               \n";

static const char *script7 =
"class TestC                                     \n"
"{                                               \n"
"  TestC() {count++; s = \"test\";}              \n"
"  string s;                                     \n"
"}                                               \n"
"int count = 0;                                  \n"
"void Test()                                     \n"
"{                                               \n"
"  TestC t;                                      \n"
"  Assert(count == 1);                           \n"
"  TestC[] array(5);                             \n"
"  Assert(count == 6);                           \n"
"}                                               \n";

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);


	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "TestArray()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	r = engine->Build(0);
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

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "TestArrayMulti()", &ctx);
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

	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	r = engine->ExecuteString(0, "TestArrayChar()", &ctx);
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

	engine->AddScriptSection(0, TESTNAME, script5, strlen(script5), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestArrayInitList()", &ctx);
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( r == asEXECUTION_EXCEPTION )
		PrintException(ctx);

	if( ctx ) ctx->Release();

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script6, strlen(script6), 0);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestArray (1, 1) : Info    : Compiling void Test()\n"
	                   "TestArray (3, 15) : Error   : Initialization lists cannot be used with 'int[]@'\n"
	                   "TestArray (4, 16) : Error   : Initialization lists cannot be used with 'int'\n" )
		fail = true;

	// Array object must call default constructor of the script classes
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script7, strlen(script7), 0);
	r = engine->Build(0);
	if( r < 0 ) 
		fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
		fail = true;
		
	// Test bool[] on Mac OS X with PPC CPU
	// Submitted by Edward Rudd
	const char *script8 =
	"bool[] f(10);              \n"
	"for (int i=0; i<10; i++) { \n"
	"	f[i] = false;           \n"
	"}                          \n"
	"Assert(f[0] == false);     \n"
	"Assert(f[1] == false);     \n"
	"f[0] = true;               \n"
	"Assert(f[0] == true);      \n"
	"Assert(f[1] == false);     \n";
	
	r = engine->ExecuteString(0, script8);
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace

