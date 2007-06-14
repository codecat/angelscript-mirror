#include "utils.h"

namespace TestInt8
{

#define TESTNAME "TestInt8"

char RetInt8(char in)
{
	if( in != 1 )
	{
		printf("failed to pass parameter correctly\n");
	}
	return 1;
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// We'll test two things with this function
	// 1. The native interface is able to pass byte parameters correctly
	// 2. The native interface is able to return byte values correctly
	engine->RegisterGlobalFunction("int8 RetInt8(int8)", asFUNCTION(RetInt8), asCALL_CDECL);

	char var = 0;
	engine->RegisterGlobalProperty("int8 gvar", &var);

	engine->ExecuteString(0, "gvar = RetInt8(1)");
	if( var != 1 )
	{
		printf("failed to return value correctly\n");
		fail = true;
	}
	
	engine->ExecuteString(0, "Assert(RetInt8(1) == 1)");
	
	engine->Release();

	return fail;
}

} // namespace

