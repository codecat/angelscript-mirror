//
// Tests the negate operator behaviour
//
// Test author: Andreas Jönsson
//

#include "utils.h"

#define TESTNAME "TestNegateOperator"

static int testVal = 0;
static bool called = false;

static int negate(int *f1) 
{
	called = true;
	return -*f1;
}

static int minus(int *f1, int *f2)
{
	called = true;
	return *f1 - *f2;
}

static void negate_gen(asIScriptGeneric *gen) 
{
	int *f1 = (int*)gen->GetObject();
	called = true;
	int ret = -*f1;
	gen->SetReturnObject(&ret);
}

static void minus_gen(asIScriptGeneric *gen)
{
	int *f1 = (int*)gen->GetArgAddress(0);
	int *f2 = (int*)gen->GetArgAddress(1);
	called = true;
	int ret = *f1 - *f2;
	gen->SetReturnObject(&ret);
}

bool TestNegateOperator()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterObjectType("obj", sizeof(int), asOBJ_VALUE | asOBJ_APP_PRIMITIVE);
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine->RegisterObjectBehaviour("obj", asBEHAVE_NEGATE, "obj f()", asFUNCTION(negate_gen), asCALL_GENERIC);
		engine->RegisterGlobalBehaviour(asBEHAVE_SUBTRACT, "obj f(obj &in, obj &in)", asFUNCTION(minus_gen), asCALL_GENERIC);
	}
	else
	{
		engine->RegisterObjectBehaviour("obj", asBEHAVE_NEGATE, "obj f()", asFUNCTION(negate), asCALL_CDECL_OBJLAST);
		engine->RegisterGlobalBehaviour(asBEHAVE_SUBTRACT, "obj f(obj &in, obj &in)", asFUNCTION(minus), asCALL_CDECL);
	}
	engine->RegisterGlobalProperty("obj testVal", &testVal);

	testVal = 1000;

	COutStream obj;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &obj, asCALL_THISCALL);
	engine->ExecuteString(0, "testVal = -testVal");

	if( !called ) 
	{
		// failure
		printf("\n%s: behaviour function was not called from script\n\n", TESTNAME);
		fail = true;
	} 
	else if( testVal != -1000 ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, -1000);
		fail = true;
	}

	called = false;
	engine->ExecuteString(0, "testVal = testVal - testVal");

	if( !called ) 
	{
		// failure
		printf("\n%s: behaviour function was not called from script\n\n", TESTNAME);
		fail = true;
	} 
	else if( testVal != 0 ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got %d, expected %d\n\n", TESTNAME, testVal, 0);
		fail = true;
	}
	

	engine->Release();
	
	// Success
	return fail;
}
