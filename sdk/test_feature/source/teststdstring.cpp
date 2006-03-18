//
// This test shows how to register the std::string to be used in the scripts.
// It also used to verify that objects are always constructed before destructed.
//
// Author: Andreas Jönsson
//

#include "utils.h"
using namespace std;

#define TESTNAME "TestStdString"

static string printOutput;

static void PrintString(string &str)
{
	printOutput = str;
}

static void PrintStringVal(string str)
{
	printOutput = str;
}

// This script tests that variables are created and destroyed in the correct order
static const char *script =
"void blah1()\n"
"{\n"
"	if(true)\n"
"		return;\n"
"\n"
"	string blah = \"Bleh1!\";\n"
"}\n"
"\n"
"void blah2()\n"
"{\n"
"	string blah = \"Bleh2!\";\n"
"\n"
"	if(true)\n"
"		return;\n"
"}\n";

static const char *script2 =
"void testString()                         \n"
"{                                         \n"
"  print(getString(\"I\" \"d\" \"a\"));    \n"
"}                                         \n"
"                                          \n"
"string getString(string & str)            \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n"
"void testString2()                        \n"
"{                                         \n"
"  string str = \"Hello World!\";          \n"
"  printVal(str);                          \n"
"}                                         \n";

static const char *script3 = 
"void testFailure()                        \n"
"{                                         \n"
"  string a;                               \n"
"  undef *p;                               \n"
"  if( p->Get() == a )                     \n"
"  {                                       \n"
"  }                                       \n"
"}                                         \n";


static void Construct1(void *o)
{

}

static void Construct2(string &str, void *o)
{

}

static void Destruct(void *o)
{

}

bool TestStdString()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void printVal(string)", asFUNCTION(PrintStringVal), asCALL_CDECL);

	engine->RegisterObjectType("obj", 4, asOBJ_PRIMITIVE);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct1), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(string &in)", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);


	CBufferedOutStream bout;
	engine->AddScriptSection(0, "string", script3, strlen(script3), 0);
	int r = engine->Build(0, &bout);
	if( r >= 0 )
	{
		fail = true;
		printf("%s: Didn't fail as expected\n", TESTNAME);
	}

	COutStream out;
	engine->AddScriptSection(0, "string", script, strlen(script), 0);
	engine->Build(0, &out);

	r = engine->ExecuteString(0, "blah1(); blah2();", &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

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
	engine->ExecuteString(0, "obj a; a = obj(\"test\")", &out);

	// Verify that it is possible to pass strings by value
#ifdef __GNUC__
    printf("GNUC: The library cannot pass objects with destructor by value to application functions\n");
#else
	printOutput = "";
	engine->ExecuteString(0, "testString2()", &out);
	if( printOutput != "Hello World!" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}
#endif

	engine->Release();

	return fail;
}
