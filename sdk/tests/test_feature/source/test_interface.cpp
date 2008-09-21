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

// Test inheriting from another class. Should give an error since this hasn't been implemented yet
static const char *script3 =
"class A {}       \n"
"class B : A {}   \n";


// TODO: Test explicit conversion from interface to class. Should give null value if not the right class.

bool Test2();

bool Test()
{
	bool fail = false;

	if( !fail ) fail = Test2();

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
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "test()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Test calling the interface method from the application
	int typeId = engine->GetTypeIdByDecl(0, "myclass");
	asIScriptStruct *obj = (asIScriptStruct*)engine->CreateScriptObject(typeId);

	int intfTypeId = engine->GetTypeIdByDecl(0, "myintf");
	asIObjectType *type = engine->GetObjectTypeById(intfTypeId);
	int funcId = type->GetMethodIdByDecl("void test()");
	asIScriptContext *ctx = engine->CreateContext();
	r = ctx->Prepare(funcId);
	if( r < 0 ) fail = true;
	ctx->SetObject(obj);
	ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;

	intfTypeId = engine->GetTypeIdByDecl(0, "appintf");
	type = engine->GetObjectTypeById(intfTypeId);
	funcId = type->GetMethodIdByDecl("void test()");

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
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestInterface (5, 7) : Error   : Missing implementation of 'void intf::test()'\n"
					   "TestInterface (5, 23) : Warning : The interface is already implemented\n"
					   "TestInterface (9, 1) : Info    : Compiling void test(intf&inout)\n"
					   "TestInterface (11, 9) : Error   : Data type can't be 'intf'\n"
					   "TestInterface (13, 8) : Error   : There is no copy operator for this type available.\n"
					   "TestInterface (13, 6) : Error   : There is no copy operator for this type available.\n"
					   "TestInterface (15, 16) : Error   : Can't implicitly convert from 'myclass@' to 'nointf@&'.\n"
					   "TestInterface (16, 16) : Error   : Can't implicitly convert from 'intf@' to 'myclass@&'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test inheriting from another class
	bout.buffer = "";
	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestInterface (2, 11) : Error   : The identifier must be an interface\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// Success
	return fail;
}



bool Test2()
{
	bool fail = false;
	int r;
	COutStream out;

	// An interface that is declared equally in two different modules should receive the same type id
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	const char *script = "interface Simple { void function(int); }";
	engine->AddScriptSection("a", "script", script, strlen(script));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	engine->AddScriptSection("b", "script", script, strlen(script));
	r = engine->Build("b");
	if( r < 0 )
		fail = true;

	int typeA = engine->GetTypeIdByDecl("a", "Simple");
	int typeB = engine->GetTypeIdByDecl("b", "Simple");

	if( typeA != typeB )
		fail = true;

	// Test recompiling a module
	engine->AddScriptSection("a", "script", script, strlen(script));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	typeA = engine->GetTypeIdByDecl("a", "Simple");
	if( typeA != typeB )
		fail = true;

	// Test interface that references itself
	const char *script1 = "interface A { A@ f(); }";
	engine->AddScriptSection("a", "script", script1, strlen(script1));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	engine->AddScriptSection("b", "script", script1, strlen(script1));
	r = engine->Build("b");
	if( r < 0 )
		fail = true;

	int typeAA = engine->GetTypeIdByDecl("a", "A");
	int typeBA = engine->GetTypeIdByDecl("b", "A");

	if( typeAA != typeBA )
		fail = true;


	// Test with more complex interfaces
	const char *script2 = "interface A { B@ f(); } interface B { A@ f(); C@ f(); } interface C { A@ f(); }";
	engine->AddScriptSection("a", "script", script2, strlen(script2));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	engine->AddScriptSection("b", "script", script2, strlen(script2));
	r = engine->Build("b");
	if( r < 0 )
		fail = true;

	typeAA = engine->GetTypeIdByDecl("a", "A");
	int typeAB = engine->GetTypeIdByDecl("a", "B");
	int typeAC = engine->GetTypeIdByDecl("a", "C");
	
	typeBA = engine->GetTypeIdByDecl("b", "A");
	int typeBB = engine->GetTypeIdByDecl("b", "B");
	int typeBC = engine->GetTypeIdByDecl("b", "C");

	if( typeAA != typeBA ||
		typeAB != typeBB ||
		typeAC != typeBC )
		fail = true;

	// Test interfaces that are not equal
	const char *script3 = "interface A { B@ f(); } interface B { int f(); }";
	const char *script4 = "interface A { B@ f(); } interface B { float f(); }";

	engine->AddScriptSection("a", "script", script3, strlen(script3));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	engine->AddScriptSection("b", "script", script4, strlen(script4));
	r = engine->Build("b");
	if( r < 0 )
		fail = true;

	typeAA = engine->GetTypeIdByDecl("a", "A");
	typeAB = engine->GetTypeIdByDecl("a", "B");
	
	typeBA = engine->GetTypeIdByDecl("b", "A");
	typeBB = engine->GetTypeIdByDecl("b", "B");

	if( typeAA == typeBA ||
		typeAB == typeBB )
		fail = true;

	// Interfaces that uses the interfaces that are substituted must be updated
	const char *script5 = "interface A { float f(); }";
	const char *script6 = "interface B { A@ f(); }";
	engine->AddScriptSection("a", "script5", script5, strlen(script5));
	r = engine->Build("a");
	if( r < 0 )
		fail = true;

	engine->AddScriptSection("b", "script5", script5, strlen(script5));
	engine->AddScriptSection("b", "script6", script6, strlen(script6));
	r = engine->Build("b");
	if( r < 0 )
		fail = true;

	typeBA = engine->GetTypeIdByDecl("b", "A@");
	typeBB = engine->GetTypeIdByDecl("b", "B");
	asIObjectType *objType = engine->GetObjectTypeById(typeBB);
	asIScriptFunction *func = objType->GetMethodDescriptorByIndex(0);
	if( func->GetReturnTypeId() != typeBA )
		fail = true;

	engine->Release();

	// TODO: This must work for pre-compiled byte code as well, i.e. when loading the byte code 
	// the interface ids must be resolved in the same way it is for compiled scripts

	// TODO: The interfaces should be equal if they use enums declared in the 
	// scripts as well (we don't bother checking the enum values)

	return fail;
}

} // namespace

