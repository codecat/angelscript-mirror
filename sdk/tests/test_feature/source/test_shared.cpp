#include "utils.h"

namespace TestShared
{

bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	asIScriptEngine *engine;
	int r;

	{
		int reg;
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalProperty("int reg", &reg);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("a", 
			"interface badIntf {} \n"
			"shared interface sintf {} \n"
			"shared class T : sintf, badIntf \n" // Allow shared interface, but not non-shared interface
			"{ \n"
			"  void test() \n"
			"  { \n"
			"    var = 0; \n" // Don't allow accessing non-shared global variables
			"    gfunc(); \n" // Don't allow calling non-shared global functions
			"    reg = 1; \n" // Allow accessing registered variables
			"    assert( reg == 1 ); \n" // Allow calling registered functions
			"    badIntf @intf; \n" // Do not allow use of non-shared types in parameters/return type
			"  } \n"
			"  void f(badIntf @) {} \n" // Don't allow use of non-shared types in parameters/return type
			"} \n"
			"int var; \n"
			"void gfunc() {} \n"
			);
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 ) 
			TEST_FAILED;
		if( bout.buffer != "a (13, 3) : Error   : Shared code cannot use non-shared type 'badIntf'\n"
						   "a (3, 25) : Error   : Shared class cannot implement non-shared interface 'badIntf'\n"
						   "a (5, 3) : Info    : Compiling void T::test()\n"
						   "a (7, 5) : Error   : Shared code cannot access non-shared global variable 'var'\n"
						   "a (8, 5) : Error   : Shared code cannot call non-shared function 'void gfunc()'\n"
						   "a (11, 5) : Error   : Shared code cannot use non-shared type 'badIntf'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		engine->DiscardModule("");

		const char *validCode =
			"shared interface I {} \n"
			"shared class T : I \n"
			"{ \n"
			"  void func() {} \n"
			"  int i; \n"
			"} \n";
		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("a", validCode);
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptModule *mod2 = engine->GetModule("2", asGM_ALWAYS_CREATE);
		mod2->AddScriptSection("b", validCode);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		int t1 = mod->GetTypeIdByDecl("T");
		int t2 = mod2->GetTypeIdByDecl("T");
		if( t1 != t2 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__"1");
		mod->SaveByteCode(&stream);

		asIScriptModule *mod3 = engine->GetModule("3", asGM_ALWAYS_CREATE);
		r = mod3->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		int t3 = mod3->GetTypeIdByDecl("T");
		if( t1 != t3 )
			TEST_FAILED;

		engine->Release();
	}

	// Success
	return fail;
}



} // namespace

