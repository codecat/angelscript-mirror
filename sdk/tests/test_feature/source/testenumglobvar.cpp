//
// This test verifies enumeration of global script variables
//
// Author: Andreas Jonsson
//

#include "utils.h"

#define TESTNAME "TestEnumGlobVar"

static const char script[] = "int a; float b; double c; uint d = 0xC0DE; string e = \"test\"; obj @f = @o;";

void AddRef_Release_dummy(asIScriptGeneric *)
{
}


bool TestEnumGlobVar()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString_Generic(engine);

	int r;
	r = engine->RegisterObjectType("obj", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRef_Release_dummy), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_RELEASE, "void f()", asFUNCTION(AddRef_Release_dummy), asCALL_GENERIC); assert( r >= 0 );
	int o = 0xBAADF00D;
	r = engine->RegisterGlobalProperty("obj o", &o);

	engine->AddScriptSection(0, "test", script, sizeof(script)-1, 0);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->Build(0);

	int count = engine->GetGlobalVarCount(0);
	if( count != 6 )
	{
		printf("%s: GetGlobalVarCount() returned %d, expected 6.\n", TESTNAME, count);
		ret = true;
	}

	const char *buffer = 0;
	if( (buffer = engine->GetGlobalVarDeclaration(0,0)) == 0 )
	{
		printf("%s: GetGlobalVarDeclaration() failed\n", TESTNAME);
		ret = true;
	}
	else if( strcmp(buffer, "int a") != 0 )
	{
		printf("%s: GetGlobalVarDeclaration() returned %s\n", TESTNAME, buffer);
		ret = true;
	}

	int idx = engine->GetGlobalVarIndexByName(0, "b");
	if( idx < 0 )
	{
		printf("%s: GetGlobalVarIndexByName() returned %d\n", TESTNAME, idx);
		ret = true;
	}

	idx = engine->GetGlobalVarIndexByDecl(0, "double c");
	if( idx < 0 )
	{
		printf("%s: GetGlobalVarIndexByDecl() returned %d\n", TESTNAME, idx);
		ret = true;
	}

	if( (buffer = engine->GetGlobalVarName(0, 3)) == 0 )
	{
		printf("%s: GetGlobalVarName() failed\n", TESTNAME);
		ret = true;
	}
	else if( strcmp(buffer, "d") != 0 )
	{
		printf("%s: GetGlobalVarName() returned %s\n", TESTNAME, buffer);
		ret = true;
	}

	unsigned long *d;
	d = (unsigned long *)engine->GetAddressOfGlobalVar(0, 3);
	if( d == 0 )
	{
		printf("%s: GetGlobalVarPointer() returned %d\n", TESTNAME, r);
		ret = true;
	}
	if( *d != 0xC0DE )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	std::string *e;
	e = (std::string*)engine->GetAddressOfGlobalVar(0, 4);
	if( e == 0 )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	if( *e != "test" )
	{
		printf("%s: Failed\n", TESTNAME);
		ret = true;
	}

	int *f;
	f = *(int**)engine->GetAddressOfGlobalVar(0, 5); // We're getting a pointer to the handle
	if( f == 0 )
	{
		printf("%s: failed\n", TESTNAME);
		ret = true;
	}

	if( *f != 0xBAADF00D )
	{
		printf("%s: failed\n", TESTNAME);
		ret = true;
	}

	engine->Release();

	return ret;
}

