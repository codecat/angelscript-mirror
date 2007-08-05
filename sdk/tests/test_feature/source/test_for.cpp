#include "utils.h"

namespace TestFor
{

#define TESTNAME "TestFor"

static const char *script1 = "\
float[] myArray(70);                       \n\
                                           \n\
float aSize = 8;                           \n\
                                           \n\
float MyFunction(float a)                  \n\
{                                          \n\
  return a;                                \n\
}                                          \n\
                                           \n\
void Test()                                \n\
{                                          \n\
  for (float k = 0; k< aSize; k++)         \n\
  {                                        \n\
    myArray[MyFunction(k*aSize)] = k;      \n\
  }                                        \n\
                                           \n\
  for (int i = 0; i< aSize*aSize; i++)     \n\
  {                                        \n\
//  Print(\"i = \" + i + \"\\n\");         \n\
    myArray[i] = i;                        \n\
  }                                        \n\
}                                          \n";


void Print_Generic(asIScriptGeneric *gen)
{
	asCScriptString *str = (asCScriptString*)gen->GetArgAddress(0);
	printf(str->buffer.c_str());
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

