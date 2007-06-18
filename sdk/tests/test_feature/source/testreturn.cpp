//
// Tests if a c-function can return values to
// the script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

static bool returned = false;

static bool cfunction()
{
	return true;
}

static void cfunction_gen(asIScriptGeneric *gen) 
{
	gen->SetReturnDWord(true);
}

static bool retfalse()
{
	return false;
}

static int retfalse_fake()
{
	if( sizeof(bool) == 1 )
		// This function is designed to test AS ability to handle bools that may not be returned in full 32 bit values
		return 0x00FFFF00;
	else
		return 0;
}

static void retfalse_fake_gen(asIScriptGeneric *gen)
{
	gen->SetReturnDWord(retfalse_fake());
}

static void retfalse_gen(asIScriptGeneric *gen) 
{
	gen->SetReturnDWord(false);
}

static asINT64 reti64()
{
	return I64(0x102030405);
}

bool TestReturn()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->RegisterGlobalProperty("bool returned", &returned);
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine->RegisterGlobalFunction("bool cfunction()", asFUNCTION(cfunction_gen), asCALL_GENERIC);
		engine->RegisterGlobalFunction("bool retfalse()", asFUNCTION(retfalse_gen), asCALL_GENERIC);
		engine->RegisterGlobalFunction("bool retfalse2()", asFUNCTION(retfalse_fake_gen), asCALL_GENERIC);
	}
	else
	{
		engine->RegisterGlobalFunction("bool cfunction()", asFUNCTION(cfunction), asCALL_CDECL);
		engine->RegisterGlobalFunction("bool retfalse()", asFUNCTION(retfalse), asCALL_CDECL);
		engine->RegisterGlobalFunction("bool retfalse2()", asFUNCTION(retfalse_fake), asCALL_CDECL);
		engine->RegisterGlobalFunction("int64 reti64()", asFUNCTION(reti64), asCALL_CDECL);
		
		engine->ExecuteString(0, "assert(reti64() == 0x102030405)");
	}

	engine->ExecuteString(0, "returned = cfunction()");
	if (!returned) 
	{
		printf("\nTestReturn: cfunction didn't return properly\n\n");
		ret = true;
	}

	engine->ExecuteString(0, "Assert(!retfalse() == cfunction())");
	engine->ExecuteString(0, "Assert(retfalse() == false)");
	engine->ExecuteString(0, "returned = retfalse()");
	if( returned )
	{
		printf("\nTestReturn: retfalse didn't return properly\n\n");
		ret = true;
	}

	engine->ExecuteString(0, "Assert(!retfalse2() == cfunction())");
	engine->ExecuteString(0, "Assert(retfalse2() == false)");
	engine->ExecuteString(0, "returned = retfalse2()");
	if( returned )
	{
		printf("\nTestReturn: retfalse2 didn't return properly\n\n");
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
