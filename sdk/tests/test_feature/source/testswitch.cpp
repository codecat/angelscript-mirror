// 
// Test designed to verify functionality of the switch case
//
// Author: Andreas Jönsson
//

#include "utils.h"
using namespace std;

#define TESTNAME "TestSwitch"


static const char *script =
"void _switch()                  \n"  // 1
"{                               \n"  // 2
"  for( int n = -1; n < 2; ++n ) \n"  // 3
"    switch( n )                 \n"  // 4
"    {                           \n"  // 5
"    case 0:                     \n"  // 6
"      add(0);                   \n"  // 7
"      break;                    \n"  // 8
"    case -1:                    \n"  // 9
"      add(-1);                  \n"  // 10
"      break;                    \n"  // 11
"    case 0x5:                   \n"  // 12
"      add(5);                   \n"  // 13
"      break;                    \n"  // 14
"    case 0xF:                   \n"  // 15
"      add(15);                  \n"  // 16
"      break;                    \n"  // 17
"    default:                    \n"  // 18
"      add(255);                 \n"  // 19
"      break;                    \n"  // 20
"    }                           \n"  // 21
"}                               \n"; // 22

static const char *script2 =
"const int a = 1;                   \n"
"const int8 b = 2;                  \n"
"void _switch2()                    \n"
"{                                  \n"
"  const uint c = 3;                \n"
"  for( uint8 n = 0; n <= 5; ++n )  \n"
"  {                                \n"
"    switch( n )                    \n"
"    {                              \n"
"    case 5: Log(\"5\"); break;     \n"
"    case 4: Log(\"4\"); break;     \n"
"    case c: Log(\"3\"); break;     \n"
"    case b: Log(\"2\"); break;     \n"
"    case a: Log(\"1\"); break;     \n"
"    default: Log(\"d\"); break;    \n"
"    }                              \n"
"  }                                \n"
"  Log(\"\\n\");                    \n"
"  myFunc127(127);                  \n"
"  myFunc128(128);                  \n"
"}                                  \n"
"const uint8 c127 = 127;               \n"
"void myFunc127(uint8 value)           \n"
"{                                     \n"
"  if(value == c127)                   \n"
"    Log(\"It is the value we expect\\n\"); \n"
"                                      \n"
"  switch(value)                       \n"
"  {                                   \n"
"    case c127:                        \n"
"      Log(\"The switch works\\n\");        \n"
"      break;                          \n"
"    default:                          \n"
"      Log(\"I didnt work\\n\");            \n"
"      break;                          \n"
"  }                                   \n"
"}                                     \n"
"const uint8 c128 = 128;               \n"
"void myFunc128(uint8 value)           \n"
"{                                     \n"
"  if(value == c128)                   \n"
"    Log(\"It is the value we expect\\n\"); \n"
"                                      \n"
"  switch(value)                       \n"
"  {                                   \n"
"    case c128:                        \n"
"      Log(\"The switch works\\n\");        \n"
"      break;                          \n"
"    default:                          \n"
"      Log(\"I didnt work\\n\");            \n"
"      break;                          \n"
"  }                                   \n"
"}                                     \n";

static int sum = 0;

static void add(asIScriptGeneric *gen)
{
	sum += (int)gen->GetArgDWord(0);
}

static string _log;
static void Log(asIScriptGeneric *gen)
{
	asCScriptString *str = (asCScriptString *)gen->GetArgObject(0);
	_log += str->buffer;
}

bool TestSwitch()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Log(const string &in)", asFUNCTION(Log), asCALL_GENERIC);

	engine->RegisterGlobalFunction("void add(int)", asFUNCTION(add), asCALL_GENERIC);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, "switch", script, strlen(script), 0);
	int r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build script\n", TESTNAME);
		fail = true;
	}

	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByDecl(0, "void _switch()"));
	ctx->Execute();

	if( sum != 254 )
	{
		printf("%s: Expected %d, got %d\n", TESTNAME, 254, sum);
		fail = true;
	}

	ctx->Release();

	engine->AddScriptSection(0, "switch", script2, strlen(script2), 0);
	engine->Build(0);

	engine->ExecuteString(0, "_switch2()");

	if( _log != "d12345\n"
		        "It is the value we expect\n"
                "The switch works\n"
                "It is the value we expect\n"
                "The switch works\n" )
	{
		fail = true;
		printf("%s: Switch failed. Got: %s\n", TESTNAME, _log.c_str());
	}
 
	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->ExecuteString(0, "switch(1) {}"); 
	if( bout.buffer != "ExecuteString (1, 1) : Error   : Empty switch statement\n" )
		fail = true;

	engine->Release();

	return fail;
}
