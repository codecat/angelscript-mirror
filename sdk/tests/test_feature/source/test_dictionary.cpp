#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"


namespace TestDictionary
{

#define TESTNAME "TestDictionary"


const char *script = 
"void Test()                       \n"
"{                                 \n"
"  dictionary dict;                \n"
"  dict.set(\"a\", 42);            \n"
"  assert(dict.exists(\"a\"));     \n"
"  uint u = 0;                     \n"
"  dict.get(\"a\", u);             \n"
"  assert(u == 42);                \n"
"  dict.delete(\"a\");             \n"
"  assert(!dict.exists(\"a\"));    \n"
"  string a = \"t\";               \n"
"  dict.set(\"a\", @a);            \n"
"  string @b;                      \n"
"  dict.get(\"a\", @b);            \n"
"  assert(b == \"t\");             \n"
"  dict.set(\"a\", a);             \n"
"  string c;                       \n"
"  dict.get(\"a\", c);             \n"
"  assert(c == \"t\");             \n"
"  int8 q = 41;                    \n"
"  dict.set(\"b\", q);             \n"
"  dict.get(\"b\", q);             \n"
"  assert(q == 41);                \n"
"  float f = 300;                  \n"
"  dict.set(\"c\", f);             \n"
"  dict.get(\"c\", f);             \n"
"  assert(f == 300);               \n"
"  int i;                          \n"
"  dict.get(\"c\", i);             \n"
"  assert(i == 300);               \n"
"  dict.get(\"b\", f);             \n"
"  assert(f == 41);                \n"
"}                                 \n";

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
 	asIScriptEngine *engine = 0;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptDictionary_Native(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		fail = true;
	}
	ctx->Release();

	engine->Release();

	//-------------------------
	// Test the generic interface as well 
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptDictionary_Generic(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		fail = true;
	}
	ctx->Release();

	engine->Release();


	return fail;
}

} // namespace

