#include "utils.h"

namespace TestEnum
{

#define TESTNAME		"TestEnum"
#define TESTMODULE		"TestEnum"


// Script for testing attributes
static const char *const script =
"enum TEST2_ENUM  									\n"
"{													\n"
"	TEST_1 = -1,									\n"
"	TEST1 = 1,										\n"
"	TEST2,											\n"
"	TEST1200 = 300 * 4,								\n"
"	TEST1201 = TEST1200 + 1,						\n"
"	TEST1202,										\n"
"	TEST1203,										\n"
"	TEST1205 = TEST1201 + 4,						\n"
"	TEST1_0 = TEST_1 + 1, 							\n"
"	TEST1_1 = TEST_1 + 2 							\n"
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
"   output(TEST1205);                               \n"
"   output(TEST1_0);                                \n"
"   output(TEST1_1);                                \n"
"	return TEST2;									\n"
"}													\n";

enum TEST_ENUM
{
	ENUM1 = 1,
	ENUM2 = ENUM1*10,
	ENUM3
};

void func(asIScriptGeneric *g)
{
	TEST_ENUM e = (TEST_ENUM)g->GetArgDWord(0);
	UNUSED_VAR(e);
}

std::string buffer;
static void scriptOutput(int val1)
{
	char buf[256];
#if _MSC_VER >= 1500
	sprintf_s(buf, 255, "%d\n", val1);
#else
	sprintf(buf, "%d\n", val1);
#endif
	buffer += buf;
}

static bool TestEnum()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Skipping this due to not supporting native calling conventions
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	asIScriptEngine   *engine;
	COutStream		   out;
	CBufferedOutStream bout;
	int		 		   r;
	bool               fail = false;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	r = engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// Register the enum value
	r = engine->RegisterEnum("TEST_ENUM"); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM1", ENUM1); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM2", ENUM2); assert(r >= 0);
	r = engine->RegisterEnumValue("TEST_ENUM", "ENUM3", ENUM3); assert(r >= 0);

	r = engine->RegisterGlobalFunction("void funce(TEST_ENUM)", asFUNCTION(func), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void output(int val1)", asFUNCTION(scriptOutput), asCALL_CDECL); assert(r >= 0);

	// Test calling generic function with enum value
	r = engine->ExecuteString(0, "funce(ENUM1);");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// Test using the registered enum values
	r = engine->ExecuteString(0, "output(ENUM1); output(ENUM2)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "1\n10\n" )
		fail = true;

	// Test script that declare an enum
	// enum value can be given as expression of constants
	// enum can be implicitly cast to number
	buffer = "";
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection(NULL, script, strlen(script), 0);
	r = mod->Build();
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(NULL, "Test1()");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "-1\n1\n2\n1200\n1201\n1202\n1203\n1205\n0\n1\n" )
	{
		fail = true;
		printf(buffer.c_str());
	}

	// Registered enums are literal constants
	// variable of enum type can be implictly cast to primitive
	buffer = "";
	r = engine->ExecuteString(0, "TEST_ENUM e = ENUM1; switch( e ) { case ENUM1: output(e); }");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "1\n" )
		fail = true;

	// Script declared enums behave the same
	buffer = "";
	r = engine->ExecuteString(0, "TEST2_ENUM e = TEST_1; switch( e ) {case TEST_1: output(e); }");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "-1\n" )
		fail = true;

	// enum values can't be declared with expressions including subsequent values
	bout.buffer = "";
	r = engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script2 = "enum TEST_ERR { ERR1 = ERR2, ERR2 }";
	mod = engine->GetModule("error", asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("error", script2, strlen(script2));
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "error (1, 22) : Info    : Compiling TEST_ERR ERR1\n"
					   "error (1, 24) : Error   : Use of uninitialized global variable 'ERR2'.\n"
					   "error (1, 1) : Info    : Compiling TEST_ERR ERR2\n"
                       "error (1, 1) : Error   : Use of uninitialized global variable 'ERR1'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// enum type name can't be overloaded with variable name in another scope
	bout.buffer = "";
	r = engine->ExecuteString(0, "int TEST_ENUM = 999");
	if( r >= 0  )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 5) : Error   : Illegal variable name 'TEST_ENUM'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// enum value name can be overloaded with variable name in another scope
	buffer = "";
	r = engine->ExecuteString(0, "int ENUM1 = 999; output(ENUM1)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "999\n" )
		fail = true;

	// number cannot be implicitly cast to enum type
	bout.buffer = "";
	r = engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "TEST_ENUM val = 1");
	if( r >= 0 )
		fail = true;
	r = engine->ExecuteString(0, "float f = 1.2f; TEST_ENUM val = f");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 17) : Error   : Can't implicitly convert from 'uint' to 'TEST_ENUM'.\n"
                       "ExecuteString (1, 33) : Error   : Can't implicitly convert from 'float' to 'TEST_ENUM'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// constant number can be explicitly cast to enum type
	r = engine->ExecuteString(0, "TEST_ENUM val = TEST_ENUM(1)");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// primitive value can be explicitly cast to enum type
	r = engine->ExecuteString(0, "float f = 1.2f; TEST_ENUM val = TEST_ENUM(f)");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// math operator with enums
	buffer = "";
	r = engine->ExecuteString(0, "int a = ENUM2 * 10; output(a); output(ENUM2 + ENUM1)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "100\n11\n" )
		fail = true;

	// comparison operator with enums
	buffer = "";
	r = engine->ExecuteString(0, "if( ENUM2 > ENUM1 ) output(1);");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "1\n" )
		fail = true;

	// bitwise operators with enums
	buffer = "";
	r = engine->ExecuteString(0, "output( ENUM2 << ENUM1 )");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "20\n" )
		fail = true;

	// circular reference between enum and global variable are
	// allowed if they can be resolved
	const char *script3 =
	"enum TEST_EN                \n"
	"{                           \n"
	"  EN1,                      \n"
	"  EN2 = gvar,               \n"
	"  EN3,                      \n"
	"}                           \n"
	"const int gvar = EN1 + 10;  \n";
	mod = engine->GetModule("en", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("en", script3, strlen(script3));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	buffer = "";
	r = engine->ExecuteString("en", "output(EN2); output(EN3)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "10\n11\n" )
		fail = true;

	// functions can be overloaded for parameters with enum type
	const char *script4 =
	"void func(TEST_ENUM) { output(1); } \n"
	"void func(int) { output(2); } \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r < 0 )
		fail = true;
	buffer = "";
	r = engine->ExecuteString(0, "func(1); func(1.0f); TEST_ENUM e = ENUM1; func(e)");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( buffer != "2\n2\n1\n" )
		fail = true;

	// Using registered enum type in a script
	engine->RegisterEnum("game_type_t");
	const char *script5 =
	"game_type_t random_game_type;\n"
	"void foo(game_type_t game_type)\n"
	"{\n"
	"   random_game_type = game_type;\n"
	"};\n";
	r = mod->AddScriptSection("script", script5, strlen(script5));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	// enum with assignment without comma
	const char *script6 = "enum test_wo_comma { value = 0 }";
	r = mod->AddScriptSection("script", script6, strlen(script6));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	// Enums are not object types
	int eid;
	const char *ename = mod->GetEnumByIndex(0, &eid);
	if( eid < 0 || ename == 0 )
		fail = true;
	asIObjectType *eot = engine->GetObjectTypeById(eid);
	if( eot )
		fail = true;

	// enum must allow negate and binary complement operators
	bout.buffer = "";
	r = engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "int a = -ENUM1; int b = ~ENUM1;");
	if( r < 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 25) : Warning : Implicit conversion changed sign of value\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}


bool Test()
{
	return TestEnum();
}



} // namespace
