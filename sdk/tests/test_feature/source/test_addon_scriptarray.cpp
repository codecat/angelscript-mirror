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
"   array<array<int>> a(2);                      \n"
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
	CBufferedOutStream bout;
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

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArray()", mod, ctx);
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

	r = ExecuteString(engine, "TestArrayException()", mod);
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

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayMulti()", mod, ctx);
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
	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayChar()", mod, ctx);
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

	// Initialization lists must work for array template
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r < 0 ) fail = true;
	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayInitList()", mod, ctx);
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( r == asEXECUTION_EXCEPTION )
		PrintException(ctx);

	if( ctx ) ctx->Release();

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	r = mod->Build();
	if( r >= 0 ) fail = true;
	if( bout.buffer != "Test_Addon_ScriptArray (1, 1) : Info    : Compiling void Test()\n"
	                   "Test_Addon_ScriptArray (3, 20) : Error   : Initialization lists cannot be used with 'array<int>@'\n"
	                   "Test_Addon_ScriptArray (4, 21) : Error   : Initialization lists cannot be used with 'int'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Array object must call default constructor of the script classes
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
	r = mod->Build();
	if( r < 0 ) 
		fail = true;
	r = ExecuteString(engine, "Test()", mod);
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
	
	r = ExecuteString(engine, script8, mod);
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// Make sure it is possible to do multiple assignments with the array type
	r = ExecuteString(engine, "array<int> a, b, c; a = b = c;");
	if( r < 0 )
		fail = true;

	// Must support syntax as: array<array<int>>, i.e. without white space between the closing angled brackets.
	r = ExecuteString(engine, "array<array<int>> a(2); Assert( a.length() == 2 );");
	if( r < 0 )
		fail = true;

	// Must support arrays of handles
	r = ExecuteString(engine, "array<array<int>@> a(1); @a[0] = @array<int>(4);");
	if( r < 0 )
		fail = true;

	// Do not allow the instantiation of a template with a subtype that cannot be created
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->RegisterObjectType("single", 0, asOBJ_REF | asOBJ_NOHANDLE);
	r = ExecuteString(engine, "array<single> a;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 7) : Error   : Can't instanciate template 'array' with subtype 'single'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Test too large arrays
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine);

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "array<int> a; a.resize(0xFFFFFFFF);", 0, ctx);
		if( r != asEXECUTION_EXCEPTION )
		{
			fail = true;
		}
		else if( strcmp(ctx->GetExceptionString(), "Too large array size") != 0 )
		{
			fail = true;
		}

		r = ExecuteString(engine, "array<int> a(0xFFFFFFFF);", 0, ctx);
		if( r != asEXECUTION_EXCEPTION )
		{
			fail = true;
		}
		else if( strcmp(ctx->GetExceptionString(), "Too large array size") != 0 )
		{
			fail = true;
		}

		ctx->Release();
		engine->Release();
	}


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

	r = ExecuteString(engine, exec, module);
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}

} // namespace

