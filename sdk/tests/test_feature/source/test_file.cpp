#include "utils.h"
#include "../../../add_on/scriptfile/scriptfile.h"

namespace TestFile
{

#define TESTNAME "TestFile"

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	RegisterScriptString(engine);
	RegisterScriptFile(engine);

	const char *script =
		"file f;                                          \n"
		"f.open(\"scripts/TestExecuteScript.as\", \"r\"); \n"
		"assert( f.getSize() > 0 );                       \n"
		"string @s = f.readString(10000);                 \n"
		"assert( s.length() == uint(f.getSize()) );       \n"
		"f.close();                                       \n";

	r = engine->ExecuteString(0, script);
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

