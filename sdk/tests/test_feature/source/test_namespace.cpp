#include "utils.h"

namespace TestNamespace
{

bool Test()
{
	bool fail = false;
	asIScriptEngine *engine;
	int r;
	COutStream out;
	CBufferedOutStream bout;

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"int func() { return var; } \n"
			"int func2() { return var; } \n"
			"int var = 0; \n"
			"class cl { cl() {v = 0;} int v; } \n"
			"interface i {} \n"
			"enum e { e1 = 0 } \n"
			"funcdef void fd(); \n"
			// Namespaces allow same entities to be declared again
			"namespace a { \n"
			"  int func() { return var; } \n" // Should find the global var in the same namespace
			"  int func2() { return func(); } \n" // Should find the global function in the same namespace
			"  int var = 1; \n"
			"  class cl { cl() {v = 1;} int v; } \n"
			"  interface i {} \n"
			"  enum e { e1 = 1 } \n"
			"  funcdef void fd(); \n"
			// Nested namespaces are allowed
			"  namespace b { \n"
			"    int var = 2; \n"
			"  } \n"
			"} \n"
			// It's possible to specify exactly which one is wanted
			"cl gc; \n"
			"a::cl gca; \n"
			"void main() \n"
			"{ \n"
			"  assert(var == 0); \n"
			"  assert(::var == 0); \n"
			"  assert(a::var == 1); \n"
			"  assert(a::b::var == 2); \n"
			"  assert(func() == 0); \n"
			"  assert(a::func() == 1); \n"
			"  assert(func2() == 0); \n"
			"  assert(a::func2() == 1); \n"
			"  assert(e1 == 0); \n"
			"  assert(::e1 == 0); \n"
			"  assert(e::e1 == 0); \n"
			"  assert(::e::e1 == 0); \n"
			"  assert(a::e1 == 1); \n"
			"  assert(a::e::e1 == 1); \n"
			"  cl c; \n"
			"  a::cl ca; \n"
			"  assert(c.v == 0); \n"
			"  assert(ca.v == 1); \n"
			"  assert(gc.v == 0); \n"
			"  assert(gca.v == 1); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	
		// Retrieving entities should work properly with namespace
		mod->SetDefaultNamespace("a");
		asIScriptFunction *f1 = mod->GetFunctionByDecl("int func()");
		asIScriptFunction *f2 = mod->GetFunctionByDecl("int a::func()");
		asIScriptFunction *f3 = mod->GetFunctionByName("func");
		if( f1 == 0 || f1 != f2 || f1 != f3 )
			TEST_FAILED;

		int v1 = mod->GetGlobalVarIndexByName("var");
		int v2 = mod->GetGlobalVarIndexByDecl("int var");
		int v3 = mod->GetGlobalVarIndexByDecl("int a::var");
		if( v1 < 0 || v1 != v2 || v1 != v3 )
			TEST_FAILED;

		int t1 = mod->GetTypeIdByDecl("cl");
		int t2 = mod->GetTypeIdByDecl("a::cl");
		if( t1 < 0 || t1 != t2 )
			TEST_FAILED;

		// Test saving and loading 
		CBytecodeStream s("");
		mod->SaveByteCode(&s);

		asIScriptModule *mod2 = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		r = mod2->LoadByteCode(&s);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// TODO: It should be possible to inform the namespace when querying by declaration
	// TODO: It should be possible to choose whether to include namespace or not when getting declarations

	// Test registering interface with namespace
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		
		r = engine->SetDefaultNamespace("test"); assert( r >= 0 );

		r = engine->RegisterObjectType("t", 0, asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("t", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("t", "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectProperty("t", "int a", 0); assert( r >= 0 );
		int t1 = engine->GetTypeIdByDecl("t");
		int t2 = engine->GetTypeIdByDecl("test::t");
		if( t1 < 0 || t1 != t2 )
			TEST_FAILED;
		
		r = engine->RegisterInterface("i"); assert( r >= 0 );
		r = engine->RegisterInterfaceMethod("i", "void f()"); assert( r >= 0 );
		t1 = engine->GetTypeIdByDecl("test::i");
		if( t1 < 0 )
			TEST_FAILED;

		r = engine->RegisterEnum("e"); assert( r >= 0 );
		r = engine->RegisterEnumValue("e", "e1", 0); assert( r >= 0 );
		t1 = engine->GetTypeIdByDecl("test::e");
		if( t1 < 0 )
			TEST_FAILED;

		r = engine->RegisterFuncdef("void f()"); assert( r >= 0 );
		t1 = engine->GetTypeIdByDecl("test::f");
		if( t1 < 0 )
			TEST_FAILED;

		r = engine->RegisterGlobalFunction("void gf()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		asIScriptFunction *f1 = engine->GetGlobalFunctionByDecl("void test::gf()");
		asIScriptFunction *f2 = engine->GetGlobalFunctionByDecl("void gf()");
		if( f1 == 0 || f1 != f2 )
			TEST_FAILED;

		r = engine->RegisterGlobalProperty("int gp", (void*)1); assert( r >= 0 );
		int g1 = engine->GetGlobalPropertyIndexByName("gp");
		int g2 = engine->GetGlobalPropertyIndexByDecl("int gp");
		int g3 = engine->GetGlobalPropertyIndexByDecl("int test::gp");
		if( g1 < 0 || g1 != g2 || g1 != g3 )
			TEST_FAILED;

		r = engine->RegisterTypedef("td", "int"); assert( r >= 0 );
		t1 = engine->GetTypeIdByDecl("test::td");
		if( t1 < 0 )
			TEST_FAILED;

		engine->Release();
	}


	// Test accessing registered properties in different namespaces from within function in another namespace
	// http://www.gamedev.net/topic/624376-accessing-global-property-from-within-script-namespace/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		int a = 0, b = 0;
		r = engine->RegisterGlobalProperty("int a", &a);
		r = engine->SetDefaultNamespace("test");
		r = engine->RegisterGlobalProperty("int b", &b);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"namespace script { \n"
			"void func() \n"
			"{ \n"
			"  a = 1; \n"
			"} \n"
			"} \n");
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		// TODO: Should have better error message. Perhaps show variables declared in other scopes
		if( bout.buffer != "test (2, 1) : Info    : Compiling void func()\n"
						   "test (4, 3) : Error   : 'a' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		mod->AddScriptSection("test", 
			"namespace script { \n"
			"void func() \n"
			"{ \n"
			"  ::a = 1; \n"
			"  test::b = 2; \n"
			"} \n"
			"} \n");
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "script::func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( a != 1 || b != 2 )
			TEST_FAILED;

		engine->Release();		
	}

	// Success
	return fail;
}

} // namespace

