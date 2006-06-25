#include "utils.h"
using namespace std;

#define TESTNAME "TestNotInitialized"

static void cfunction(asIScriptGeneric *gen)
{
	
}

bool TestNotInitialized()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalFunction("void cfunction(int)", asFUNCTION(cfunction), asCALL_GENERIC);
	
	CBufferedOutStream out;	
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &out, asCALL_THISCALL);
	int r = engine->ExecuteString(0, "int a; cfunction(a);"); 

	if( out.buffer != "ExecuteString (1, 18) : Warning : 'a' is not initialized.\n" )
	{
		printf("%s: Failed to catch use of uninitialized variable\n", TESTNAME);
		fail = true;
	}
		
	engine->Release();

	// Success
	return fail;
}
