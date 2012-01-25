#include "utils.h"

namespace TestNamespace
{

bool Test()
{
	bool fail = false;
	asIScriptEngine *engine;
	int r;
	COutStream out;

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

		// Test saving and loading 
		CBytecodeStream s("");
		mod->SaveByteCode(&s);

		asIScriptModule *mod2 = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		mod2->LoadByteCode(&s);

		r = ExecuteString(engine, "main()", mod2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// TODO: It should be possible to inform the namespace when querying by declaration
	// TODO: It should be possible to choose whether to include namespace or not when getting declarations

	// Success
	return fail;
}

} // namespace

