#include "utils.h"

namespace TestArrayHandle
{

#define TESTNAME "TestArrayHandle"

static const char *script1 =
"void TestArrayHandle()                          \n"
"{                                               \n"
"   string[]@[]@ a;                              \n"
"   string[]@[] b(2);                            \n"
"   Assert(@a == null);                          \n"
"   @a = @string[]@[](2);                        \n"
"   Assert(@a != null);                          \n"
"   Assert(@a[0] == null);                       \n"
"   string@[] c(10);                             \n"
"   Assert(c.length() == 10);                    \n"
"   Assert(g.length() == 2);                     \n"
"}                                               \n"
"string@[] g(2);                                 \n";
 
static const char *script2 =
"void TestArrayHandle2()                         \n"
"{                                               \n"
"   string[] s(10);                              \n"
"   Append(s);                                   \n"
"                                                \n"
"   string[]@ sh = createArray();                \n"
"   double d1 = atof(sh[0]);                     \n"
"   double d2 = atof(s[0]);                      \n"
"}                                               \n"
"void Append(string[]@ s)                        \n"
"{                                               \n"
"   for( uint n = 0; n < s.length(); n++ )       \n"
"      s[n] += \".\";                            \n"
"}                                               \n"
"string[]@ createArray()                         \n"
"{                                               \n"
"   return string[](2);                          \n"
"}                                               \n";

void StringToDouble(asIScriptGeneric *gen)
{
	std::string s = ((asCScriptString*)gen->GetArgAddress(0))->buffer;
	gen->SetReturnDouble(atof(s.c_str()));
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->RegisterGlobalFunction("double atof(const string &in)",asFUNCTION(StringToDouble),asCALL_GENERIC);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx;
	r = engine->ExecuteString(0, "TestArrayHandle()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "TestArrayHandle2()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

