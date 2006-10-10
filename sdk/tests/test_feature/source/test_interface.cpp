#include "utils.h"

namespace TestInterface
{

#define TESTNAME "TestInterface"

// Test implementing multiple interfaces
// Test implicit conversion from class to interface
// Test calling method of interface handle from script
// Register interface from application
static const char *script1 =
"interface myintf                                \n"
"{                                               \n"
"   void test();                                 \n"
"}                                               \n"
"class myclass : myintf, intf2, appintf          \n"
"{                                               \n"
"   myclass() {this.str = \"test\";}             \n"
"   void test() {Assert(this.str == \"test\");}  \n"
"   int func2(const string &in i)                \n"
"   {                                            \n"
"      Assert(this.str == i);                    \n"
"      return 0;                                 \n"
"   }                                            \n"
"   string str;                                  \n"
"}                                               \n"
"interface intf2                                 \n"
"{                                               \n"
"   int func2(const string &in);                 \n"
"}                                               \n"
"void test()                                     \n"
"{                                               \n"
"   myclass a;                                   \n"
"   myintf@ b = a;                               \n"
"   intf2@ c;                                    \n"
"   @c = a;                                      \n"
"   a.func2(\"test\");                           \n"
"   c.func2(\"test\");                           \n"
"   test(a);                                     \n"
"}                                               \n"
"void test(appintf@i)                            \n"
"{                                               \n"
"   i.test();                                    \n"
"}                                               \n";

// Test class that don't implement all functions of the interface.
// Test instanciating an interface. Shouldn't work.
// Test that classes don't implement the same interface twice
// Try copying an interface variable to another. Shouldn't work.
// Test implicit conversion from class to interface that is not being implemented. Should give compiler error
// Test implicit conversion from interface to class. Should give compiler error.
static const char *script2 = 
"interface intf             \n"
"{                          \n"
"    void test();           \n"
"}                          \n"
"class myclass : intf, intf \n"
"{                          \n"
"}                          \n"
"interface nointf {}        \n"
"void test(intf &i)         \n"
"{                          \n"
"   intf a;                 \n"
"   intf@ b, c;             \n"
"   b = c;                  \n"
"   myclass d;              \n"
"   nointf@ e = d;          \n"
"   myclass@f = b;          \n"
"}                          \n";


// TODO: Test explicit conversion from interface to class. Should give null value if not the right class.

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString_Generic(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// Register an interface from the application
	r = engine->RegisterInterface("appintf"); assert( r >= 0 );
	r = engine->RegisterInterfaceMethod("appintf", "void test()"); assert( r >= 0 );

	// Test working example
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Test calling the interface method from the application
	int typeId = engine->GetTypeIdByDecl(0, "myclass");
	asIScriptStruct *obj = (asIScriptStruct*)engine->CreateScriptObject(typeId);

	int intfTypeId = engine->GetTypeIdByDecl(0, "myintf");
	int funcId = engine->GetMethodIDByDecl(intfTypeId, "void test()");
	asIScriptContext *ctx = engine->CreateContext();
	r = ctx->Prepare(funcId);
	if( r < 0 ) fail = true;
	ctx->SetObject(obj);
	ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;

	intfTypeId = engine->GetTypeIdByDecl(0, "appintf");
	funcId = engine->GetMethodIDByDecl(intfTypeId, "void test()");

	r = ctx->Prepare(funcId);
	if( r < 0 ) fail = true;
	ctx->SetObject(obj);
	ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;

	if( ctx ) ctx->Release();
	if( obj ) obj->Release();

	// Test class that don't implement all functions of the interface.
	// Test instanciating an interface. Shouldn't work.
	// Test that classes don't implement the same interface twice
	// Try copying an interface variable to another. Shouldn't work.
	// Test implicit conversion from class to interface that is not being implemented. Should give compiler error
	// Test implicit conversion from interface to class. Should give compiler error.
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestInterface (5, 7) : Error   : Missing implementation of 'void intf::test()'\n"
					   "TestInterface (5, 23) : Warning : The interface is already implemented\n"
					   "TestInterface (9, 1) : Info    : Compiling void test(intf&inout)\n"
					   "TestInterface (9, 11) : Error   : Parameter type can't be 'intf&'\n"
					   "TestInterface (11, 9) : Error   : Data type can't be 'intf'\n"
					   "TestInterface (13, 8) : Error   : There is no copy operator for this type available.\n"
					   "TestInterface (13, 6) : Error   : There is no copy operator for this type available.\n"
					   "TestInterface (15, 16) : Error   : Can't implicitly convert from 'myclass@' to 'nointf@&'.\n"
					   "TestInterface (16, 16) : Error   : Can't implicitly convert from 'intf@' to 'myclass@&'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}


	engine->Release();

	// Success
	return fail;
}

} // namespace

