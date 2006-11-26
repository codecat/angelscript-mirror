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

const char *script2 =
"void CompilerAssert()\n"
"{\n"
"   int64 x = 0x0000000000000000;\n"
"   int64 y = 1;\n"
"   x+y;\n"
"}";

const char *script3 = "void CompilerAssert(uint8[]@ &in b) { b[0] == 1; }";

const char *script4 = "class C : I {}";

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;
	CBufferedOutStream bout;
	COutStream out;

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

	// test 2
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void CompilerAssert()\n"
					   "TestCompiler (3, 14) : Error   : Can't implicitly convert from 'uint' to 'int64'.\n"
					   "TestCompiler (4, 14) : Error   : Can't implicitly convert from 'uint' to 'int64'.\n"
					   "TestCompiler (5, 5) : Error   : No conversion from 'int64' to math type available.\n" )
	   fail = true;

	// test 3
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0, false);
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	// test 4
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0, false);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 11) : Error   : Identifier 'I' is not a data type\n" )
		fail = true;

	engine->Release();
		
	// Success
 	return fail;
}

} // namespace

