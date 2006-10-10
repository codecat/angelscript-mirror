#include "utils.h"

namespace TestCompiler
{

#define TESTNAME "TestCompiler"

// Unregistered types and functions
const char *script1 =
"void testFunction ()                          \n"
"{                                             \n"
" Assert@ assertReached = tryToAvoidMeLeak();  \n"
"}                                             \n";



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
	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void testFunction()\n"
                       "TestCompiler (3, 2) : Error   : Identifier 'Assert' is not a data type\n"
                       "TestCompiler (3, 8) : Error   : Object handle is not supported for this type\n"
                       "TestCompiler (3, 26) : Error   : No matching signatures to 'tryToAvoidMeLeak()'\n" )
		fail = true;

	engine->Release();

	// Success
 	return fail;
}

} // namespace

