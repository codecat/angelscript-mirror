//
// Tests compiling with 2 equal functions
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace Test2Func
{

#define TESTNAME "Test2Func"


static const char *script1 =
"void Test() { } \n"
"void Test() { } \n";

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	CBufferedOutStream out;
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->Build(0, &out);

	if( out.buffer != "Test2Func (2, 1) : Error   : A function with the same name and parameters already exist\n" )
	{
		fail = true;
		printf("%s: Failed to identify the error with two equal functions\n", TESTNAME);
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

