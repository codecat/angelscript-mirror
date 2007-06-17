#include "utils.h"
using std::string;
#include "../../../add_on/scriptstring/scriptstring.h"

namespace TestScriptString
{

#define TESTNAME "TestScriptString"

static string printOutput;

// This function receives the string by reference
// (in fact it is a reference to copy of the string)
static void PrintString(asIScriptGeneric *gen)
{
	asCScriptString *str = (asCScriptString*)gen->GetArgAddress(0);
	printOutput = str->buffer;
}

// This function shows how to receive an 
// object handle from the script engine
static void SetString(asIScriptGeneric *gen)
{
	asCScriptString *str = (asCScriptString*)gen->GetArgAddress(0);
	if( str )
	{
		str->buffer = "Handle to a string";

		// The generic interface will release the handle in the parameter for us
		// str->Release();
	}
}

// This function shows how to receive a reference 
// to an object handle from the script engine
static void SetString2(asIScriptGeneric *gen)
{
	asCScriptString *str = *(asCScriptString**)gen->GetArgAddress(0);
	if( str )
	{
		str->buffer = "Handle to a string";

		// The generic interface will release the handle in the parameter for us
		// str->Release();
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

static const char *script4 = 
"void test()                    \n"
"{                              \n"
"   string s = \"\"\"           \n"
"Heredoc\\x20test\n"
"            \"\"\" \"\\x21\";  \n"
"   print(s);                   \n"
"}                              \n";

static const char *script5 =
"void test( string @ s )         \n"
"{                               \n"
"   string t = s;                \n"
"}                               \n"
"void Main()                     \n"
"{                               \n"
"   test(\"this is a test\");    \n"
"}                               \n";

static const char *script6 = 
"void Main()                     \n"
"{                               \n"
"   test(\"this is a test\");    \n"
"}                               \n"
"void test( string @ s )         \n"
"{                               \n"
"   string t = s;                \n"
"}                               \n";

static const char *script7 =
"void test()                    \n"
"{                              \n"
"   Func(\"test\");             \n"
"}                              \n"
"void Func(const string &in str)\n"
"{                              \n"
"}                              \n";

//bool Get(int * /*obj*/, const asCScriptString &szURL, asCScriptString &szHTML)
void Get(asIScriptGeneric *gen)
{
	const asCScriptString *szURL = (asCScriptString*)gen->GetArgObject(0);
	asCScriptString *szHTML = (asCScriptString*)gen->GetArgObject(1);

	assert(szHTML != 0);
	assert(szURL->buffer == "stringtest");
	szHTML->buffer = "output";

	gen->SetReturnDWord(false);
}

void GetConstStringRef(asIScriptGeneric *gen)
{
	static string test("test");
	gen->SetReturnAddress(&test);
}

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);
	RegisterScriptStringUtils(engine);

	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set(string@)", asFUNCTION(SetString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set2(string@&in)", asFUNCTION(SetString2), asCALL_GENERIC);
	engine->RegisterGlobalFunction("const string &getconststringref()", asFUNCTION(GetConstStringRef), asCALL_GENERIC);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// Test string copy constructor
	int r = engine->ExecuteString(0, "string tst(getconststringref()); print(tst);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "test" ) fail = true;


	printOutput = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0);

	engine->ExecuteString(0, "testString()");

	if( printOutput != "hello Ida" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

	engine->ExecuteString(0, "string s = \"test\\\\test\\\\\"");

	// Verify that it is possible to use the string in constructor parameters
	printOutput = "";
	engine->ExecuteString(0, "string a; a = 1; print(a);");
	if( printOutput != "1" ) fail = true;
	
	printOutput = "";
	engine->ExecuteString(0, "string a; a += 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = \"a\" + 1; print(a);");
	if( printOutput != "a1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1 + \"a\"; print(a);");
	if( printOutput != "1a" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "string a = 1; print(a);");
	if( printOutput != "1" ) fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(\"a\" + 1.2)");
	if( printOutput != "a1.2") fail = true;

	printOutput = "";
	engine->ExecuteString(0, "print(1.2 + \"a\")");
	if( printOutput != "1.2a") fail = true;

	// Passing a handle to a function
	printOutput = "";
	engine->ExecuteString(0, "string a; set(@a); print(a);");
	if( printOutput != "Handle to a string" ) fail = true;

	// Implicit conversion to handle
	printOutput = ""; 
	engine->ExecuteString(0, "string a; set(a); print(a);");
	if( printOutput != "Handle to a string" ) fail = true;

	// Passing a reference to a handle to the function
	printOutput = ""; 
	engine->ExecuteString(0, "string a; set2(@a); print(a);");
	if( printOutput != "Handle to a string" ) fail = true;

	// Implicit conversion to reference to a handle
	printOutput = "";
	engine->ExecuteString(0, "string a; set2(a); print(a);");
	if( printOutput != "Handle to a string" ) fail = true;

    printOutput = "";
    engine->ExecuteString(0, "string a = \" \"; a[0] = 65; print(a);");
    if( printOutput != "A" ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	if( engine->Build(0) < 0 )
		fail = true;

	printOutput = "";
	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0);
	if( engine->Build(0) < 0 )
		fail = true;
	engine->ExecuteString(0, "test()");
	if( printOutput != "Heredoc\\x20test!" ) fail = true;

	asCScriptString *a = new asCScriptString("a");
	engine->RegisterGlobalProperty("string a", a);
	r = engine->ExecuteString(0, "print(a == \"a\" ? \"t\" : \"f\")");
	if( r != asEXECUTION_FINISHED ) 
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
	a->Release();

	// test new
	engine->AddScriptSection(0, TESTNAME, script5, strlen(script5), 0);
	if( engine->Build(0) < 0 ) fail = true;
	r = engine->ExecuteString(0, "Main()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->AddScriptSection(0, TESTNAME, script6, strlen(script6), 0);
	if( engine->Build(0) < 0 ) fail = true;
	r = engine->ExecuteString(0, "Main()");
	if( r != asEXECUTION_FINISHED ) fail = true;


	// Test character literals
	printOutput = "";
	r = engine->ExecuteString(0, "print(\"\" + 'a')");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "97" ) fail = true;

	printOutput = "";
	r = engine->ExecuteString(0, "print(\"\" + '\\'')");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "39" ) fail = true;

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "print(\"\" + '')");
	if( r != -1 ) fail = true;

	//-------------------------------------
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	engine->AddScriptSection(0, "test", script7, strlen(script7), 0, false);
	engine->Build(0);
	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->RegisterObjectType("Http", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectMethod("Http","bool get(const string &in,string &out)", asFUNCTION(Get),asCALL_GENERIC);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	
	r = engine->ExecuteString(0, "Http h; string str; h.get(\"stringtest\", str); assert(str == \"output\");");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "Http h; string a = \"test\", b; h.get(\"string\"+a, b); assert(b == \"output\");");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Test the string utils
	engine->ExecuteString(0, "string str = \"abcdef\"; assert(findFirst(str, \"def\") == 3);");
	engine->ExecuteString(0, "string str = \"abcdef\"; assert(findFirstOf(str, \"feb\") == 1);");
	engine->ExecuteString(0, "string str = \"a|b||d\"; string@[]@ array = split(str, \"|\"); assert(array.length() == 4); assert(array[1] == \"b\");");
	engine->ExecuteString(0, "string@[] array = {\"a\", \"b\", \"\", \"d\"}; assert(join(array, \"|\") == \"a|b||d\");");

	engine->Release();

	return fail;
}

} // namespace

