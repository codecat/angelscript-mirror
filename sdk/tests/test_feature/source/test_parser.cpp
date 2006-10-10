#include "utils.h"

namespace TestParser
{

#define TESTNAME "TestParser"

// Unfinished class
const char *script1 =
"class myclass \n"
"{             \n";

// Const with capital C
const char *script2 =
"class myclass                  \n"
"{                              \n"
"   void f(Const int&in) {}     \n"
"};                             \n";

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;
	CBufferedOutStream bout;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestParser (3, 1) : Error   : Expected '}'\n" )
		fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestParser (3, 17) : Error   : Expected ')' or ','\n"
					   "TestParser (3, 17) : Error   : Expected method or property\n"
					   "TestParser (4, 1) : Error   : Unexpected token '}'\n" )
		fail = true;

	engine->Release();

	// Success
 	return fail;
}

} // namespace

