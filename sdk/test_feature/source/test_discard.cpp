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
"void Test() { } \n";

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);

	engine->Build(0, &out);

	engine->Discard(0);

	engine->Release();

	// Success
	return fail;
}

} // namespace

