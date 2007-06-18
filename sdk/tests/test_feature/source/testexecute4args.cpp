//
// Tests calling of a c-function from a script with four parameters
//
// Test author: Fredrik Ehnbom
//

#include "utils.h"

#define TESTNAME "TestExecute4Args"

static bool testVal = false;
static bool called  = false;

static int	 t1 = 0;
static short t2 = 0;
static char	 t3 = 0;
static int	 t4 = 0;

static void cfunction(int f1, short f2, char f3, int f4)
{
	called = true;
	t1 = f1;
	t2 = f2;
	t3 = f3;
	t4 = f4;
	testVal = (f1 == 5) && (f2 == 9) && (f3 == 1) && (f4 == 3);
}

static asINT64 g1 = 0;
static float   g2 = 0;
static char    g3 = 0;
static int     g4 = 0;

static void cfunction2(asINT64 i1, float f2, char i3, int i4)
{
	called = true;
	
	g1 = i1;
	g2 = f2;
	g3 = i3;
	g4 = i4;
	
	testVal = ((i1 == I64(0x102030405)) && (f2 == 3) && (i3 == 24) && (i4 == 128));
}

static void cfunction_gen(asIScriptGeneric *gen)
{
	called = true;
	t1 = gen->GetArgDWord(0);
	t2 = *(short*)gen->GetArgPointer(1);
	t3 = *(char*)gen->GetArgPointer(2);
	t4 = gen->GetArgDWord(3);
	testVal = (t1 == 5) && (t2 == 9) && (t3 == 1) && (t4 == 3);
}

bool TestExecute4Args()
{
	bool ret = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		engine->RegisterGlobalFunction("void cfunction(int, int16, int8, int)", asFUNCTION(cfunction_gen), asCALL_GENERIC);
	else
		engine->RegisterGlobalFunction("void cfunction(int, int16, int8, int)", asFUNCTION(cfunction), asCALL_CDECL);

	engine->ExecuteString(0, "cfunction(5, 9, 1, 3)");

	if( !called ) 
	{
		// failure
		printf("\n%s: cfunction not called from script\n\n", TESTNAME);
		ret = true;
	} 
	else if( !testVal ) 
	{
		// failure
		printf("\n%s: testVal is not of expected value. Got (%d, %d, %d, %d), expected (%d, %d, %d, %d)\n\n", TESTNAME, t1, t2, t3, t4, 5, 9, 1, 3);
		ret = true;
	}
	
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		called = false;
		testVal = false;
		
		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void cfunction2(int64, float, int8, int)", asFUNCTION(cfunction2), asCALL_CDECL);
		
		engine->ExecuteString(0, "cfunction2(0x102030405, 3, 24, 128)");
		
		if( !called )
		{
			printf("%s: cfunction2 not called\n", TESTNAME);
			ret = true;
		}
		else if( !testVal )
		{
			printf("%s: testVal not of expected value. Got(%lld, %g, %d, %d)\n", TESTNAME, g1, g2, g3, g4);
			ret = true;
		}
	}

	engine->Release();
	
	// Success
	return ret;
}
