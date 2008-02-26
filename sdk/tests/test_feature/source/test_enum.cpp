#include "utils.h"

namespace TestEnum
{

#define TESTNAME		"TestEnum"
#define TESTMODULE		"TestEnum"


//	Script for testing attributes
static const char *const script =
"enum TEST2_ENUM  									\n"
"{													\n"
"	TEST_1 = -1,									\n"
"	TEST1 = 1,										\n"
"	TEST2,											\n"
"	TEST1200 = 300 * 4,								\n"
"	TEST1201 = TEST1200 + 1,						\n"
"	TEST1202,										\n"
"	TEST1203										\n"
"}													\n"
"													\n"
"TEST2_ENUM Test1()									\n"
"{													\n"
"   output(TEST_1);                                 \n"
"   output(TEST1);                                  \n"
"   output(TEST2);                                  \n"
"   output(TEST1200);                               \n"
"   output(TEST1201);                               \n"
"   output(TEST1202);                               \n"
"   output(TEST1203);                               \n"
"	return TEST2;									\n"
"}													\n";


enum TEST_ENUM
{
	ENUM1 = 1,
	ENUM2 = ENUM1*10,
	ENUM3
};

std::string buffer;
static void scriptOutput(int val1)
{
	char buf[256];
	sprintf(buf, "%d\n", val1);
	buffer += buf;
}

static bool TestEnum()
{
	asIScriptEngine *engine;
	COutStream		 out;
	int				 r;
	bool             fail = false;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	r = engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// Register the enum value
	r = engine->RegisterEnum("TEST_ENUM"); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM1", ENUM1); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM2", ENUM2); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM3", ENUM3); assert(r >= 0);

	r = engine->RegisterGlobalFunction("void output(int val1)", asFUNCTION(scriptOutput), asCALL_CDECL); assert(r >= 0);

	// Test using the registered enum values
	r = engine->ExecuteString(0, "output(ENUM1); output(ENUM2)");
	if( r != asEXECUTION_FINISHED ) 
		fail = true;
	if( buffer != "1\n10\n" )
		fail = true;

	// Test script that declare an enum
	buffer = "";
	r = engine->AddScriptSection(NULL, NULL, script, strlen(script), 0);
	r = engine->Build(NULL);
	if( r < 0 ) 
		fail = true;

	r = engine->ExecuteString(NULL, "Test1()");
	if( r != asEXECUTION_FINISHED ) 
		fail = true;
	if( buffer != "-1\n1\n2\n1200\n1201\n1202\n1203\n" )
		fail = true;

	engine->Release();

	// TEST: enums are literal constants
	// TEST: enum values can't be declared with expressions including subsequent values
	// TEST: enum type name can be overloaded with variable name in another scope
	// TEST: enum value name can be overloaded with variable name in another scope
	// TEST: enum value can be given as expression of constants
	// TEST: number cannot be implicitly cast to enum type
	// TEST: enum can be implicitly cast to number
	// TEST: number can be explicitly cast to enum type 
	// TEST: functions can be overloaded for parameters with enum type
	// TEST: math operator with enums
	// TEST: comparison operator with enums
	// TEST: bitwise operators with enums
	// TEST: circular reference between enum value and global constant variable

	// TEST: enum throws exception if number is cast to non-declared value?

	// Success
	return fail;
}


bool Test()
{
	return TestEnum();
}



} // namespace
