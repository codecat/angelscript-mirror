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

bool Test()
{
	bool fail = false;
	int r;
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
	engine->RegisterGlobalFunction("bool Test(bool, float)", asFUNCTION(Test), asCALL_GENERIC);
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
	if( bout.buffer != "script (2, 18) : Error   : Unexpected end of file\n" )
	{
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
                       "script (5, 18) : Error   : Illegal operation on 'int&'\n"
                       "script (5, 15) : Error   : Can't implicitly convert from 'int&' to 'type&'.\n" )
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

	engine->Release();

	// Success
 	return fail;
}

} // namespace

