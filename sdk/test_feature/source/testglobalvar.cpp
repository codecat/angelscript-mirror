#include "utils.h"

#define TESTNAME "TestGlobalVar"
static const char *script1 = "float global = func() * g_f * 2.0f;";
static const char *script2 = "float global = 1.0f;";

static float func()
{
	return 3.0f;
}

static float cnst = 2.0f;
static std::string g_str = "test";

static const char *script3 =
"float f = 2;                 \n"
"string str = \"test\";       \n"
"void TestGlobalVar()         \n"
"{                            \n"
"  float a = f + g_f;         \n"
"  string s = str + g_str;    \n"
"  g_f = a;                   \n"
"  f = a;                     \n"
"  g_str = s;                 \n"
"  str = s;                   \n"
"}                            \n";

bool TestGlobalVar()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterStdString(engine);

	engine->RegisterGlobalFunction("float func()", asFUNCTION(func), asCALL_CDECL);
	engine->RegisterGlobalProperty("float g_f", &cnst);
	engine->RegisterGlobalProperty("string g_str", &g_str);

	COutStream out;
	engine->AddScriptSection("a", TESTNAME, script1, strlen(script1), 0);
	// This should fail, since we are trying to call a function in the initialization
	if( engine->Build("a", 0/*&out*/) >= 0 )
	{
		printf("%s: build erronously returned success\n", TESTNAME);
		ret = true;
	}

	engine->AddScriptSection("a", "script", script2, strlen(script2), 0);
	if( engine->Build("a", &out) < 0 )
	{
		printf("%s: build failed\n", TESTNAME);
		ret = true;
	}

	engine->AddScriptSection("a", "script", script3, strlen(script3), 0);
	if( engine->Build("a", &out) < 0 )
	{
		printf("%s: build failed\n", TESTNAME);
		ret = true;
	}

	engine->ExecuteString("a", "TestGlobalVar()");

	engine->Release();

	return ret;
}
