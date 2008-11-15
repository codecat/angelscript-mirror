#include "utils.h"

namespace TestMetaData
{

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// Test the parse token method
	asETokenClass t = engine->ParseToken("!is");
	if( t != asTC_KEYWORD ) 
		fail = true; 

	engine->Release();

	return fail;
}

} // namespace

