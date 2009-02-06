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
	CScriptString *str = (CScriptString*)gen->GetArgAddress(0);
	printOutput = str->buffer;
}

// This function shows how to receive an 
// object handle from the script engine
static void SetString(asIScriptGeneric *gen)
{
	CScriptString *str = (CScriptString*)gen->GetArgAddress(0);
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
	CScriptString *str = *(CScriptString**)gen->GetArgAddress(0);
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

static const char *script8 =
"void test()                    \n"
"{                              \n"
"   Func(\"test\");             \n"
"}                              \n"
"string Func(string & str)      \n"
"{                              \n"
"  return str;                  \n"
"}                              \n";


//bool Get(int * /*obj*/, const CScriptString &szURL, CScriptString &szHTML)
void Get(asIScriptGeneric *gen)
{
	const CScriptString *szURL = (CScriptString*)gen->GetArgObject(0);
	CScriptString *szHTML = (CScriptString*)gen->GetArgObject(1);

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

// void TestFunc(int, string&)
void TestFunc(asIScriptGeneric *gen)
{
	int arg0              = *(int*)gen->GetAddressOfArg(0);
	CScriptString *arg1 = *(CScriptString**)gen->GetAddressOfArg(1);
	
	assert( arg0 == 0 );
	assert( arg1->buffer == "test" );
}

void PrintRef(asIScriptGeneric *gen)
{
	std::string *ref = *(std::string**)gen->GetAddressOfArg(0);
	assert( ref != 0 );
	assert( *ref == "Some String" );
}

bool Test2();

bool Test()
{
	bool fail = false;
	COutStream out;
	asIScriptEngine *engine = 0;
	asIScriptModule *mod = 0;
	int r;

	fail = Test2() || fail;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString(engine);
	RegisterScriptStringUtils(engine);

	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set(string@)", asFUNCTION(SetString), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void set2(string@&in)", asFUNCTION(SetString2), asCALL_GENERIC);
	engine->RegisterGlobalFunction("const string &getconststringref()", asFUNCTION(GetConstStringRef), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);


	// Test index operator for temp strings
	r = engine->ExecuteString(0, "assert('abc'[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	r = engine->ExecuteString(0, "assert(string('abc')[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	r = engine->ExecuteString(0, "string a = 'abc'; assert(a[0] == 97)");
	if( r != asEXECUTION_FINISHED )
		fail = true;


	// Test string copy constructor
	r = engine->ExecuteString(0, "string tst(getconststringref()); print(tst);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( printOutput != "test" ) fail = true;


	printOutput = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	mod->Build();

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

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	if( mod->Build() < 0 )
		fail = true;

	printOutput = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	if( mod->Build() < 0 )
		fail = true;
	engine->ExecuteString(0, "test()");
	if( printOutput != "Heredoc\\x20test!" ) fail = true;

	CScriptString *a = new CScriptString("a");
	engine->RegisterGlobalProperty("string a", a);
	r = engine->ExecuteString(0, "print(a == 'a' ? 't' : 'f')");
	if( r != asEXECUTION_FINISHED ) 
	{
		fail = true;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
	a->Release();

	// test new
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	if( mod->Build() < 0 ) fail = true;
	r = engine->ExecuteString(0, "Main()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	if( mod->Build() < 0 ) fail = true;
	r = engine->ExecuteString(0, "Main()");
	if( r != asEXECUTION_FINISHED ) fail = true;


	// Test character literals
	r = engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true); assert( r >= 0 );
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
	r = engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, false); assert( r >= 0 );

	//-------------------------------------
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script7, strlen(script7), 0);
	mod->Build();
	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->RegisterObjectType("Http", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectMethod("Http","bool get(const string &in,string &out)", asFUNCTION(Get),asCALL_GENERIC);
	
	r = engine->ExecuteString(0, "Http h; string str; h.get('stringtest', str); assert(str == 'output');");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "Http h; string a = 'test', b; h.get('string'+a, b); assert(b == 'output');");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Test the string utils
	engine->ExecuteString(0, "string str = 'abcdef'; assert(findFirst(str, 'def') == 3);");
	engine->ExecuteString(0, "string str = 'abcdef'; assert(findFirstOf(str, 'feb') == 1);");
	engine->ExecuteString(0, "string str = 'a|b||d'; string@[]@ array = split(str, '|'); assert(array.length() == 4); assert(array[1] == 'b');");
	engine->ExecuteString(0, "string@[] array = {'a', 'b', '', 'd'}; assert(join(array, '|') == 'a|b||d');");

	engine->Release();

	//---------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	engine->RegisterGlobalFunction("void TestFunc(int, string&)", asFUNCTION(TestFunc), asCALL_GENERIC);
	
	// CHKREF was placed incorrectly
	r = engine->ExecuteString(0, "TestFunc(0, 'test');");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	r = engine->ExecuteString(0, "string @s; TestFunc(0, s);");
	if( r != asEXECUTION_EXCEPTION )
		fail = true;

	engine->Release();

	//----------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script7, strlen(script7), 0);
	mod->Build();
	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	//------------------------------------------
	// Test the comparison method
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptString(engine);

		std::string a = "a";
		std::string b = "b";

		int type = engine->GetTypeIdByDecl("string");
		bool c;
		r = engine->CompareScriptObjects(c, asBEHAVE_EQUAL, &a, &b, type); assert( r >= 0 );
		if( c ) fail = true;
		r = engine->CompareScriptObjects(c, asBEHAVE_NOTEQUAL, &a, &b, type); assert( r >= 0 );
		if( !c ) fail = true;
		r = engine->CompareScriptObjects(c, asBEHAVE_LESSTHAN, &a, &b, type); assert( r >= 0 );
		if( !c ) fail = true;
		r = engine->CompareScriptObjects(c, asBEHAVE_GREATERTHAN, &a, &b, type); assert( r >= 0 );
		if( c ) fail = true;
		r = engine->CompareScriptObjects(c, asBEHAVE_LEQUAL, &a, &b, type); assert( r >= 0 );
		if( !c ) fail = true;
		r = engine->CompareScriptObjects(c, asBEHAVE_GEQUAL, &a, &b, type); assert( r >= 0 );
		if( c ) fail = true;

		engine->Release();
	}

	//-----
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->RegisterGlobalFunction("void Print(string &str)",asFUNCTION(PrintRef), asCALL_GENERIC);

		const char *script =
			"string str = 'Some String'; \n"
			"void Update() \n"
			"{ \n"
			" Print(str); \n"
			"} \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script, strlen(script));
		mod->Build();

		CScriptString *str = (CScriptString*)mod->GetAddressOfGlobalVar(0);
		

		r = engine->ExecuteString(0, "Update()");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->Release();
	}

	//-------
	// Multiline strings
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->SetEngineProperty(asEP_ALLOW_MULTILINE_STRINGS, true);
		engine->RegisterGlobalFunction("void assert(bool)",asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"string str1 = '1\\n' '2'; \n"
			"string str2 = '1\n2';     \n"
			"assert(str1 == str2);     \n";

		r = engine->ExecuteString(0, script);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->Release();
	}

	return fail;
}

bool Test2()
{
	bool fail = false;

	int r;
	COutStream out;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	const char *string =   
		"class Jerome  \n"
		"{  \n"
		"  string a;  \n"
		"  string b;  \n"
		"  double c;  \n"
		"  Jerome(string A,string B,double C)  \n"
		"  {  \n"
		"    a = A;  \n"
		"    b = B;  \n"
		"    c = C;  \n"
		"    assert( a == 'Hello' ); \n"
		"    assert( b == 'Hi' ); \n"
		"    assert( c == 1.23456 ); \n"
		"  }  \n"
		"} \n"
		"Jerome cc('Hello','Hi',1.23456);  \n";
	asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", string);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}

} // namespace

