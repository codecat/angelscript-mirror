//
// Tests calling of a c-function from a script
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

static const char * const TESTNAME = "TestExecute";

static bool called = false;

static void cfunction() {
	called = true;
}

static void cfunction_generic(asIScriptGeneric *) {
	cfunction();
}

bool TestExecute()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( strstr(asGetLibraryOptions(),"AS_MAX_PORTABILITY") )
	{
		int r = engine->RegisterGlobalFunction("void cfunction()", asFUNCTION(cfunction_generic), asCALL_GENERIC); assert( r >= 0 );
	}
	else
	{
		int r = engine->RegisterGlobalFunction("void cfunction()", asFUNCTION(cfunction), asCALL_CDECL); assert( r >= 0 );
	}
	ExecuteString(engine, "cfunction()");

	if (!called) {
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	}

	
	asIScriptContext *ctx = engine->CreateContext();
	assert(ctx->SetUserData((void*)(size_t)0xDEADF00D) == 0);
	assert(ctx->GetUserData() == (void*)(size_t)0xDEADF00D);
	assert(ctx->SetUserData(0) == (void*)(size_t)0xDEADF00D);
	ctx->Release();

	engine->Release();
	engine = NULL;

	return ret;
}
