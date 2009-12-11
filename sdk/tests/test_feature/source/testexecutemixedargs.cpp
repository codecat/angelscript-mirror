//
// Tests calling of a c-function from a script with four parameters
// of different types
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

static const char * const TESTNAME = "TestMixedArgs";

static bool testVal = false;
static bool called = false;

static int    t1 = 0;
static float  t2 = 0;
static double t3 = 0;
static char   t4 = 0;

static void cfunction(int f1, float f2, double f3, int f4) 
{
	called = true;

	t1 = f1;
	t2 = f2;
	t3 = f3;
	t4 = (char)f4;
	
	testVal = (f1 == 10) && (f2 == 1.92f) && (f3 == 3.88) && (f4 == 97);
}

static void cfunction_gen(asIScriptGeneric *gen) 
{
	called = true;

	t1 = gen->GetArgDWord(0);
	t2 = gen->GetArgFloat(1);
	t3 = gen->GetArgDouble(2);
	t4 = (char)gen->GetArgDWord(3);
	
	testVal = (t1 == 10) && (t2 == 1.92f) && (t3 == 3.88) && (t4 == 97);
}

bool TestExecuteMixedArgs()
{
	bool ret = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		engine->RegisterGlobalFunction("void cfunction(int, float, double, int)", asFUNCTION(cfunction_gen), asCALL_GENERIC);
	else
		engine->RegisterGlobalFunction("void cfunction(int, float, double, int)", asFUNCTION(cfunction), asCALL_CDECL);

	ExecuteString(engine, "cfunction(10, 1.92f, 3.88, 97)");
	
	if (!called) {
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} else if (!testVal) {
		printf("\n%s: testVal is not of expected value. Got (%d, %f, %f, %c), expected (%d, %f, %f, %c)\n\n", TESTNAME, t1, t2, t3, t4, 10, 1.92f, 3.88, 97);
		ret = true;
	}

	engine->Release();
	engine = NULL;

	return ret;
}
