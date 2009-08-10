#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"

namespace Test_Addon_ScriptArray
{

static const char *TESTNAME = "Test_Addon_ScriptArray";

static const char *script1 =
"array<string> b;                                \n"
"array<int> g_a(3);                              \n"
"void TestArray()                                \n"
"{                                               \n"
"   array<string> a(5);                          \n"
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
"   array<int> ia = TestArray4();                \n"
"   TestArray2(ia);                              \n"
"   TestArray3(ia);                              \n"
"   ia = array<int>(3);                          \n"
"   Assert(ia.length() == 3);                    \n"
"   ia[0] = 1;                                   \n"
"   array<int> ib = ia;                          \n"
"   Assert(ib.length() == ia.length());          \n"
"   Assert(ib[0] == ia[0]);                      \n"
"}                                               \n"
"void TestArray2(array<int> &inout a)            \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"void TestArray3(array<int> a)                   \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"array<int> TestArray4()                         \n"
"{                                               \n"
"   array<int> ia(3);                            \n"
"   ia[0] = 1;                                   \n"
"   ia[1] = 2;                                   \n"
"   ia[2] = 3;                                   \n"
"   return ia;                                   \n"
"}                                               \n";

static const char *script2 = 
"void TestArrayException()                       \n"
"{                                               \n"
"   array<string> a;                             \n"
"   a[0] == \"Hello\";                           \n"
"}                                               \n";

// Must be possible to declare array of arrays
static const char *script3 = 
"void TestArrayMulti()                           \n"
"{                                               \n"
"   array< array<int> > a(2);                    \n"
"   array<int> b(2);                             \n"
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
"   array<int8> a(2);                            \n"
"   a[0] = 13;                                   \n"
"   a[1] = 19;                                   \n"
"                                                \n"
"   int8 a0 = a[0];                              \n"
"   int8 a1 = a[1];                              \n"
"   Assert(a[0] == 13);                          \n"
"   Assert(a[1] == 19);                          \n"
"}                                               \n";

// TODO: Initialization lists must work for template array
static const char *script5 = 
"array<int> g = {1,2,3};                         \n"
"void TestArrayInitList()                        \n"
"{                                               \n"
"   Assert(g.length() == 3);                     \n"
"   Assert(g[2] == 3);                           \n"
"   array<int> a = {,2,};                        \n"
"   Assert(a.length() == 3);                     \n"
"   Assert(a[1] == 2);                           \n"
"   array<string> b = {\"test\", \"3\"};         \n"
"   Assert(b.length() == 2);                     \n"
"   Assert(b[0] == \"test\");                    \n"
"   Assert(b[1] == \"3\");                       \n"
"   array<array<int>> c = {,{23},{23,4},};       \n"
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
"   array<int>@ e = {2,5};                       \n"
"   array<int> f = {,{23}};                      \n"
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
"  array<TestC> arr(5);                          \n"
"  Assert(count == 6);                           \n"
"}                                               \n";

bool Test2();

bool Test()
{
	bool fail = Test2();
	int r;
	COutStream out;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptArray(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);


	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
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

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	r = mod->Build();
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

	// Must be possible to declare array of arrays
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	r = mod->Build();
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


	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	r = mod->Build();
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

	// TODO: Initialization lists must work for array template
/*
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r < 0 ) fail = true;
	r = engine->ExecuteString(0, "TestArrayInitList()", &ctx);
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( r == asEXECUTION_EXCEPTION )
		PrintException(ctx);

	if( ctx ) ctx->Release();

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestArray (1, 1) : Info    : Compiling void Test()\n"
	                   "TestArray (3, 15) : Error   : Initialization lists cannot be used with 'array<int>@'\n"
	                   "TestArray (4, 16) : Error   : Initialization lists cannot be used with 'int'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
*/
	// Array object must call default constructor of the script classes
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
	r = mod->Build();
	if( r < 0 ) 
		fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
		fail = true;
		
	// Test bool[] on Mac OS X with PPC CPU
	// Submitted by Edward Rudd
	const char *script8 =
	"array<bool> f(10);         \n"
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

	// Make sure it is possible to do multiple assignments with the array type
	r = engine->ExecuteString(0, "array<int> a, b, c; a = b = c;");
	if( r < 0 )
		fail = true;

	engine->Release();

	// TODO: Must support syntax as: array<array<int>>, i.e. without white space between the closing angled brackets.


	// Success
	return fail;
}

bool Test2()
{
	bool fail = false;

	const char *script =
	"class A                               \n"
	"{                                     \n"
	"	int x;                             \n"
	"}                                     \n"
	"int sum(const array<A>& a)            \n"
	"{                                     \n"
	"	int s = 0;                         \n"
	"	for (uint i=0; i<a.length(); i++)  \n"
	"		s+=a[i].x;                     \n"
	"	return s;                          \n"
	"}                                     \n";

	const char *exec =
	"array<A> As;  \n"
    "As.resize(2); \n"
	"As[0].x = 1;  \n"
	"As[1].x = 2;  \n"
	"sum(As);      \n";

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptArray(engine);
	asIScriptModule *module = engine->GetModule("module", asGM_ALWAYS_CREATE);

	module->AddScriptSection("script", script);
	int r = module->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->ExecuteString("module", exec);
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}

} // namespace

