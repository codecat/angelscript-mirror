#include "utils.h"
using std::string;
#include "../../../add_on/scriptstring/scriptstring.h"

namespace TestScriptString
{

#define TESTNAME "TestScriptString"

static string printOutput;

// This function receives the string by reference
// (in fact it is a reference to copy of the string)
static void PrintString(asCScriptString &str)
{
	printOutput = str.buffer;
}

// This function shows how to receive an 
// object handle from the script engine
static void SetString(asCScriptString *str)
{
	if( str )
	{
		str->buffer = "Handle to a string";

		// Release the string before returning
		str->Release();
	}
}

// This script tests that variables are created and destroyed in the correct order
static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n";

static const char *script3 = 
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void set(string@)", asFUNCTION(SetString), asCALL_CDECL);

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0, &out);

	engine->ExecuteString(0, "testString()", &out);

	if( printOutput != "hello Ida" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	engine->ExecuteString(0, "string s = \"test\\\\test\\\\\"", &out);

	// Verify that it is possible to use the string in constructor parameters
	printOutput = "";
	engine->ExecuteString(0, "string a; a = 1; print(a);", &out);
	if( printOutput != "1" ) fail = true;
	
	printOutput = "";
	engine->ExecuteString(0, "string a; a += 1; print(a);", &out);
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = \"a\" + 1; print(a);", &out);
	if( printOutput != "a1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1 + \"a\"; print(a);", &out);
	if( printOutput != "1a" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1; print(a);", &out);
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(\"a\" + 1.2)", &out);
	if( printOutput != "a1.2") fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(1.2 + \"a\")", &out);
	if( printOutput != "1.2a") fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a; set(@a); print(a);", &out);
	if( printOutput != "Handle to a string" ) fail = true;

    printOutput = "";
    engine->ExecuteString(0, "string a = \" \"; a[0] = 65; print(a);", &out);
    if( printOutput != "A" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	if( engine->Build(0, &out) < 0 )
		fail = true;

	asCScriptString *a = new asCScriptString("a");
	engine->RegisterGlobalProperty("string a", a);
	int r = engine->ExecuteString(0, "print(a == \"a\" ? \"t\" : \"f\")", &out);
	if( r != asEXECUTION_FINISHED ) 
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
	a->Release();

	engine->Release();

	return fail;
}

} // namespace

