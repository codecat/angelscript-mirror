#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"


namespace TestDictionary
{

#define TESTNAME "TestDictionary"


const char *script = 
"void Test()                       \n"
"{                                 \n"
"  dictionary dict;                \n"
"  dict.set(\"a\", 0);             \n"
"  assert(dict.exists(\"a\"));     \n"
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
	RegisterScriptDictionary(engine);

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

	return fail;
}

} // namespace

