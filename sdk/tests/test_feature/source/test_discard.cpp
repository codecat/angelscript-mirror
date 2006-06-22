//
// Tests compiling a module and then discarding it
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestDiscard
{

#define TESTNAME "TestDiscard"



static const char *script1 =
"void Test()                  \n"
"{                            \n"
"    uint8[] kk(10);          \n"
"    uint8[] kk2(10);         \n"
"    func(kk, kk2, 10, 100);  \n" 
"}                            \n";


static void func(asIScriptGeneric *)
{
}



bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->RegisterGlobalFunction("uint8 func(uint8[] &in, uint8[] &inout, uint8, uint32)", asFUNCTION(func), asCALL_GENERIC); assert( r >= 0 ); 

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	engine->Discard(0);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace


