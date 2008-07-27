#include "utils.h"

namespace TestCompiler
{

#define TESTNAME "TestCompiler"

// Unregistered types and functions
const char *script1 =
"void testFunction ()                          \n"
"{                                             \n"
" Assert@ assertReached = tryToAvoidMeLeak();  \n"
"}                                             \n";

const char *script2 =
"void CompilerAssert()\n"
"{\n"
"   bool x = 0x0000000000000000;\n"
"   bool y = 1;\n"
"   x+y;\n"
"}";

const char *script3 = "void CompilerAssert(uint8[]@ &in b) { b[0] == 1; }";

const char *script4 = "class C : I {}";

const char *script5 = 
"void t() {} \n"
"void crash() { bool b = t(); } \n";

const char *script6 = "class t { bool Test(bool, float) {return false;} }";

const char *script7 =
"class Ship                           \n\
{                                     \n\
	Sprite		_sprite;              \n\
									  \n\
	string GetName() {                \n\
		return _sprite.GetName();     \n\
	}								  \n\
}";

const char *script8 = 
"float calc(float x, float y) { Print(\"GOT THESE NUMBERS: \" + x + \", \" + y + \"\n\"); return x*y; }";


const char *script9 = 
"void noop() {}\n"
"int fuzzy() {\n"
"  return noop();\n"
"}\n";

const char *script10 =
"void func() {}\n"
"void test() { int v; v = func(); }\n";

const char *script11 =
"class c                                       \n"
"{                                             \n"
"  object @obj;                                \n"
"  void func()                                 \n"
"  {type str = obj.GetTypeHandle();}           \n"
"}                                             \n";

const char *script12 =
"void f()       \n"
"{}             \n"
"               \n"
"void assert()  \n"
"{              \n"
"   2<3?f():1;  \n"
"}              \n";

bool Test2();
bool Test3();
bool Test4();

bool Test()
{
	bool fail = false;
	int r;

	fail = Test2() || fail;
	fail = Test3() || fail;
	fail = Test4() || fail;

	asIScriptEngine *engine;
	CBufferedOutStream bout;
	COutStream out;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void testFunction()\n"
                       "TestCompiler (3, 8) : Error   : Expected ';'\n" )
		fail = true;

	engine->Release();

	// test 2
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void CompilerAssert()\n"
					   "TestCompiler (3, 13) : Error   : Can't implicitly convert from 'uint' to 'bool'.\n"
					   "TestCompiler (4, 13) : Error   : Can't implicitly convert from 'uint' to 'bool'.\n"
					   "TestCompiler (5, 5) : Error   : No conversion from 'bool' to math type available.\n" )
	   fail = true;

	// test 3
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	// test 4
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "TestCompiler (1, 11) : Error   : Identifier 'I' is not a data type\n" )
		fail = true;

	// test 5
	RegisterScriptString(engine);
	bout.buffer = "";
	r = engine->ExecuteString(0, "string &ref");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 8) : Error   : Expected '('\n" )
		fail = true;

	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script5, strlen(script5), 0);
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestCompiler (2, 1) : Info    : Compiling void crash()\n"
	                   "TestCompiler (2, 25) : Error   : Can't implicitly convert from 'void' to 'bool'.\n" )
		fail = true;

	// test 6
	// Verify that script class methods can have the same signature as 
	// globally registered functions since they are in different scope
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("bool Test(bool, float)", asFUNCTION(0), asCALL_GENERIC);
	engine->AddScriptSection(0, TESTNAME, script6, strlen(script6), 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		printf("failed on 6\n");
		fail = true;
	}

	// test 7
	// Verify that declaring a void variable in script causes a compiler error, not an assert failure
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	engine->ExecuteString(0, "void m;");
	if( bout.buffer != "ExecuteString (1, 6) : Error   : Data type can't be 'void'\n" )
	{
		printf("failed on 7\n");
		fail = true;
	}
	
	// test 8
	// Don't assert on implicit conversion to object when a compile error has occurred
	bout.buffer = "";
	engine->AddScriptSection(0, "script", script7, strlen(script7));
	r = engine->Build(0);
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (3, 2) : Error   : Identifier 'Sprite' is not a data type\n"
					   "script (5, 2) : Info    : Compiling string Ship::GetName()\n"
					   "script (6, 17) : Error   : Illegal operation on 'int&'\n" )
	{
		fail = true;
	}

	// test 9
	// Don't hang on script with non-terminated string
	bout.buffer = "";
	engine->AddScriptSection(0, "script", script8, strlen(script8));
	r = engine->Build(0);
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (1, 1) : Info    : Compiling float calc(float, float)\n"
	                   "script (1, 77) : Error   : Multiline strings are not allowed in this application\n"
	                   "script (1, 32) : Error   : No matching signatures to 'Print(string@&)'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// test 10
	// Properly handle error with returning a void expression
	bout.buffer = "";
	engine->AddScriptSection(0, "script", script9, strlen(script9));
	r = engine->Build(0);
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling int fuzzy()\n"
		               "script (3, 3) : Error   : No conversion from 'void' to 'int' available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	
	// test 11
	// Properly handle error when assigning a void expression to a variable
	bout.buffer = "";
	engine->AddScriptSection(0, "script", script10, strlen(script10));
	r = engine->Build(0);
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling void test()\n"
		               "script (2, 26) : Error   : Can't implicitly convert from 'void' to 'int'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 12
	// Handle errors after use of undefined objects
	bout.buffer = "";
	engine->RegisterObjectType("type", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->AddScriptSection(0, "script", script11, strlen(script11));
	r = engine->Build(0);
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (3, 3) : Error   : Identifier 'object' is not a data type\n"
					   "script (3, 10) : Error   : Object handle is not supported for this type\n"
                       "script (4, 3) : Info    : Compiling void c::func()\n"
                       "script (5, 18) : Error   : Illegal operation on 'int&'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 13
	// Don't permit implicit conversion of integer to obj even though obj(int) is a possible constructor
	bout.buffer = "";
	r = engine->ExecuteString(0, "uint32[] a = 0;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : Can't implicitly convert from 'const uint' to 'uint[]&'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 14
	// Calling void function in ternary operator ?:
	bout.buffer = "";
	r = engine->AddScriptSection(0, "script", script12, strlen(script12));
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (4, 1) : Info    : Compiling void assert()\n"
                       "script (6, 4) : Error   : Both expressions must have the same type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 15
	// Declaring a class inside a function
	bout.buffer = "";
	r = engine->ExecuteString(0, "class XXX { int a; }; XXX b;");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : Expected expression value\n"
	                   "ExecuteString (1, 27) : Error   : Expected ';'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 16
	// Compiler should warn if uninitialized variable is used to index an array
	bout.buffer = "";
	const char *script_16 = "void func() { int[] a(1); int b; a[b] = 0; }";
	engine->AddScriptSection(0, "script", script_16, strlen(script_16));
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 36) : Warning : 'b' is not initialized.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 17
	// Compiler should warn if uninitialized variable is used with post increment operator
	bout.buffer = "";
	const char *script_17 = "void func() { int a; a++; }";
	engine->AddScriptSection(0, "script", script_17, strlen(script_17));
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 23) : Warning : 'a' is not initialized.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 18
	// Properly notify the error of comparing boolean operands
	bout.buffer = "";
	r = engine->ExecuteString(0, "bool b1,b2; if( b1 <= b2 ) {}");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 20) : Warning : 'b1' is not initialized.\n"
                       "ExecuteString (1, 20) : Warning : 'b2' is not initialized.\n"
                       "ExecuteString (1, 20) : Error   : Illegal operation on this datatype\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test 19
	// TODO: Give proper error upon returning a reference from script function
	bout.buffer = "";
	const char *script19 =
		"class Object {}\n"
		"Object &Test(Object &in object)\n"
		"{\n"
		"  return object;\n"
		"}\n";
	engine->AddScriptSection(0, "script19", script19, strlen(script19));
	r = engine->Build(0);
	if( r < 0 ) fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Success
 	return fail;
}


//------------------------------------
// Test 2 was reported by Dentoid
float add(float &a, float &b)
{
	return a+b;
}

void doStuff(float a, float b)
{
}

bool Test2()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType( "Test", sizeof(float), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C );
	engine->RegisterGlobalBehaviour( asBEHAVE_ADD, "Test f(Test &in, Test &in)", asFUNCTION(add), asCALL_CDECL);
	engine->RegisterGlobalFunction("void doStuff(Test, Test)", asFUNCTION(doStuff), asCALL_CDECL);

	const char *script =
	"Test test1, test2;                \n"
	"doStuff( test1, test1 + test2 );  \n"  // This one will work
	"doStuff( test1 + test2, test1 );  \n"; // This one will blow

	r = engine->ExecuteString(0, script);
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();
	return fail;
}

//-----------------------------------------
// Test 3 was reported by loboWu
bool Test3()
{
	bool fail = false;
	COutStream out;
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	const char *script = 
	"uint8 search_no(uint8[]@ cmd, uint16 len, uint8[] @rcv)    \n" //mbno is nx5 array, static
	"{														    \n"
	"	if (@rcv == null) assert(false);						\n"
	"		return(255);										\n" 
	"}															\n"
	"void main()												\n"
	"{															\n"
	"	uint8[] cmd = { 0x02, 0x95, 0x45, 0x42, 0x32 };			\n"
	"	uint8[] rcv;											\n"
	"	uint16 len = 8;											\n"
	"	search_no(cmd, cmd.length(), rcv);						\n" //This is OK! @rcv won't be null
	"	search_no(cmd, GET_LEN2(cmd), rcv);						\n" //This is OK!
	"	len = GET_LEN(cmd);										\n"
	"	search_no(cmd, len, rcv);								\n" //This is OK!
	"															\n"//but 
	"	search_no(cmd, GET_LEN(cmd), rcv);						\n" //@rcv is null
	"}															\n"
	"uint16 GET_LEN(uint8[]@ cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n"
	"uint16 GET_LEN2(uint8[] cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n";

	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "main()");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();
	return fail;
}

//----------------------------------------
// Test 4 reported by dxj19831029
bool Test4()
{
	bool fail = false;
	COutStream out;
	int r = 0;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->RegisterObjectType("Chars", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_FACTORY, "Chars@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_ASSIGNMENT, "Chars &f(const Chars &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterObjectType("_Save", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectProperty("_Save", "Chars FieldName", 0);

	engine->RegisterObjectType("Struct", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_FACTORY, "Struct@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectProperty("Struct", "_Save Save", 0); 

	engine->RegisterObjectType("ScriptObject", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectMethod("ScriptObject", "Struct @f()", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterGlobalProperty("ScriptObject current", 0);

	engine->RegisterGlobalFunction("void print(Chars&)", asFUNCTION(0), asCALL_GENERIC);

	const char *script1 = "void main() { print(current.f().Save.FieldName); }"; 
	engine->AddScriptSection(0, "test", script1, strlen(script1));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	const char *script2 = "void main() { Chars a = current.f().Save.FieldName; print(a); }";
	engine->AddScriptSection(0, "test", script2, strlen(script2));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;

	engine->Release();
	return fail;
}

} // namespace

