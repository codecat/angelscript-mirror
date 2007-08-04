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

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();

	return fail;
}

} // namespace

