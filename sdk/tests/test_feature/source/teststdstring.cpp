//
// This test shows how to register the std::string to be used in the scripts.
// It also used to verify that objects are always constructed before destructed.
//
// Author: Andreas Jönsson
//

#include "utils.h"
#include "stdstring.h"
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
"string getString(string &in str)          \n"
"{                                         \n"
"  return \"hello \" + str;                \n"
"}                                         \n"
"void testString2()                        \n"
"{                                         \n"
"  string str = \"Hello World!\";          \n"
"  printVal(str);                          \n"
"}                                         \n";

static const char *script3 = 
"string str = 1;                \n"
"const string str2 = \"test\";  \n"
"obj a(\"test\");               \n"
"void test()                    \n"
"{                              \n"
"   string s = str2;            \n"
"}                              \n";

static void Construct1(void *o)
{
	UNUSED_VAR(o);
}

static void Construct2(string &str, void *o)
{
	UNUSED_VAR(str);
	UNUSED_VAR(o);
}

static void Destruct(void *o)
{
	UNUSED_VAR(o);
}

static void StringByVal(string &str1, string str2)
{
	assert( str1 == str2 );
}

//--> new: object method string argument test
class StringConsumer 
{
public:
	void Consume(string str)
	{
		printOutput = str;
	} 
};
static StringConsumer consumerObject;
//<-- new: object method string argument test


class Http
{
public:
bool Get(const string & /*szURL*/, string &szHTML)
{
	assert(&szHTML != 0);
	return false;
}
};

bool TestTwoStringTypes();

bool TestStdString()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = TestTwoStringTypes();

	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString), asCALL_CDECL);
	engine->RegisterGlobalFunction("void printVal(string)", asFUNCTION(PrintStringVal), asCALL_CDECL);

	engine->RegisterObjectType("obj", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct1), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_CONSTRUCT, "void f(string &in)", asFUNCTION(Construct2), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("obj", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destruct), asCALL_CDECL_OBJLAST);

	engine->RegisterGlobalFunction("void StringByVal(string &in, string)", asFUNCTION(StringByVal), asCALL_CDECL);

	//--> new: object method string argument test
	engine->RegisterObjectType("StringConsumer", sizeof(StringConsumer), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectMethod("StringConsumer", "void Consume(string str)", asMETHOD(StringConsumer,Consume), asCALL_THISCALL);
	engine->RegisterGlobalProperty("StringConsumer consumerObject", &consumerObject);
	//<-- new: object method string argument test


	engine->AddScriptSection(0, "string", script, strlen(script), 0);
	engine->Build(0);

	int r = engine->ExecuteString(0, "blah1(); blah2();");
	if( r < 0 )
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

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
	engine->ExecuteString(0, "obj a; a = obj(\"test\")");
	engine->ExecuteString(0, "obj a(\"test\")");

	// Verify that it is possible to pass strings by value
	printOutput = "";
	engine->ExecuteString(0, "testString2()");
	if( printOutput != "Hello World!" )
	{
		fail = true;
		printf("%s: Failed to print the correct string\n", TESTNAME);
	}

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

	engine->ExecuteString(0, "StringByVal(\"test\", \"test\")");

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	if( engine->Build(0) < 0 )
	{
		fail = true;
	}

	//--> new: object method string argument test
	printOutput = "";
	engine->ExecuteString(0, "consumerObject.Consume(\"This is my string\")");
	if( printOutput != "This is my string") fail = true;
	//<-- new: object method string argument test

	engine->RegisterObjectType("Http", sizeof(Http), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectMethod("Http","bool get(const string &in,string &out)", asMETHOD(Http,Get),asCALL_THISCALL);
	engine->ExecuteString(0, "Http h; string str; h.get(\"string\", str);");
	engine->ExecuteString(0, "Http h; string str; string a = \"a\"; h.get(\"string\"+a, str);");

	engine->Release();

	return fail;
}

//////////////////////////////
// This test was reported by dxj19831029 on Sep 9th, 2008

class _String 
{
public:
	_String() {}
	_String(_String &o) {buffer = o.buffer;}
	~_String() {}
	_String &operator=(const _String&o) {buffer = o.buffer; return *this;}
	_String &operator=(const std::string&o) {buffer = o; return *this;}
	_String &operator+=(const _String&o) {buffer += o.buffer; return *this;}
	_String &operator+=(const std::string&o) {buffer += o; return *this;}
	std::string buffer;
};

void stringDefaultCoonstructor(void *mem)
{
	new(mem) _String();
}

void stringCopyConstructor(_String &o, void *mem)
{
	new(mem) _String(o);
}

void stringStringConstructor(std::string &o, void *mem)
{
	new(mem) _String();
	((_String*)mem)->buffer = o;
}

void stringDecontructor(_String &s)
{
	s.~_String();
}

_String operation_StringAdd(_String &a, _String &b)
{
	_String r;
	r.buffer = a.buffer + b.buffer;
	return r;
}

_String operation_StringStringAdd(_String &a, std::string &b)
{
	_String r;
	r.buffer = a.buffer + b;
	return r;
}

_String operationString_StringAdd(std::string &a, _String &b)
{
	_String r;
	r.buffer = a + b.buffer;
	return r;
}

bool TestTwoStringTypes()
{
	bool fail = false;
	int r;
	COutStream out;	

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);

	// Register the second string type
	// TODO: The second line works
//	engine->RegisterObjectType("_String", sizeof(_String),   asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_DESTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT);
	engine->RegisterObjectType("_String", sizeof(_String),   asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f()",                 asFUNCTION(stringDefaultCoonstructor), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f(const _String &in )",		asFUNCTION(stringCopyConstructor), asCALL_CDECL_OBJLAST); 
	engine->RegisterObjectBehaviour("_String", asBEHAVE_CONSTRUCT, "void f(const string &in)", asFUNCTION(stringStringConstructor), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("_String", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(stringDecontructor), asCALL_CDECL_OBJLAST);
	// = 
	engine->RegisterObjectBehaviour("_String", asBEHAVE_ASSIGNMENT,	"_String &f(const string &in )", asMETHODPR(_String, operator=, (const string &), _String& ), asCALL_THISCALL); 
	engine->RegisterObjectBehaviour("_String", asBEHAVE_ASSIGNMENT,	"_String &f(const _String &in )", asMETHODPR(_String, operator=, (const _String &), _String& ), asCALL_THISCALL); 
	// +=
	engine->RegisterObjectBehaviour("_String", asBEHAVE_ADD_ASSIGN , "_String &f(const string &in )", asMETHODPR(_String, operator+=, (const string &), _String& ), asCALL_THISCALL); 
	engine->RegisterObjectBehaviour("_String", asBEHAVE_ADD_ASSIGN , "_String &f(const _String &in )", asMETHODPR(_String, operator+=, (const _String &), _String& ), asCALL_THISCALL); 
	// comparison
/*	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const _String &in, const _String &in)", asFUNCTION(compare_StringEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const _String &in, const string &in)", asFUNCTION(compare_StringStringEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL, "bool f(const string &in, const _String &in)", asFUNCTION(compareString_StringEqual), asCALL_CDECL);
	// not equal
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const _String &in, const _String &in)", asFUNCTION(compare_StringNotEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const _String &in, const string &in)", asFUNCTION(compare_StringStringNotEqual), asCALL_CDECL);
	engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL , "bool f(const string &in, const _String &in)", asFUNCTION(compareString_StringNotEqual), asCALL_CDECL);
*/	// +
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD , "_String f(const _String &in, const _String &in)", asFUNCTION(operation_StringAdd), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD , "_String f(const _String &in, const string &in)", asFUNCTION(operation_StringStringAdd), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD , "_String f(const string &in, const _String &in)", asFUNCTION(operationString_StringAdd), asCALL_CDECL); assert( r >= 0 );

	r = engine->ExecuteString(0, "_String('ab') + 'a'");
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "_String a; a+= 'a' + 'b' ; _String b = 'c'; a += b + 'c';");
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "string a; a+= 'a' + 'b' ; string b = 'c'; a += b + 'c';");
	if( r < 0 )
		fail = true;

	engine->Release();

	return fail;
}