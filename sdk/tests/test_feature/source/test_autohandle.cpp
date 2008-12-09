#include "utils.h"

namespace TestAutoHandle
{
using std::string;

#define TESTNAME "TestAutoHandle"

void TestConstructor(string &arg1, CScriptString *arg2, double d, string &arg3, void *obj)
{
	assert(arg1 == "1");
	assert(arg2->buffer == "2");
	assert(arg3 == "3");

	arg2->Release();
}

void TestFunc(string &arg1, CScriptString *arg2, double d, string &arg3)
{
	assert(arg1 == "1");
	assert(arg2->buffer == "2");
	assert(arg3 == "3");

	arg2->Release();
}

CScriptString *str = 0;
CScriptString *TestFunc2()
{
	if( str == 0 )
		str = new CScriptString();

	str->buffer = "Test";

	return str;
}


bool Test()
{
	if( strstr(asGetLibraryOptions(),"AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}


	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void TestFunc(string@+, string@, double, string@+)", asFUNCTION(TestFunc), asCALL_CDECL);
	engine->RegisterGlobalFunction("string@+ TestFunc2()", asFUNCTION(TestFunc2), asCALL_CDECL);

	engine->RegisterObjectType("object", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectBehaviour("object", asBEHAVE_CONSTRUCT, "void f(string@+, string@, double, string@+)", asFUNCTION(TestConstructor), asCALL_CDECL_OBJLAST);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(0, "TestFunc(\"1\", \"2\", 1.0f, \"3\")");
	if( r != 0 ) fail = true;

	r = engine->ExecuteString(0, "Assert(TestFunc2() == \"Test\")");
	if( r != 0 ) fail = true;

	r = engine->ExecuteString(0, "object obj(\"1\", \"2\", 1.0f, \"3\")");
	if( r != 0 ) fail = true;

	engine->Release();

	if( str ) 
		str->Release();

	// Success
	return fail;
}

} // namespace

