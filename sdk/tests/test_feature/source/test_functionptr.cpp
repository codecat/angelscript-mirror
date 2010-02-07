#include "utils.h"

namespace TestFunctionPtr
{

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptContext *ctx;
	asIScriptEngine *engine;
	asIScriptModule *mod;
	CBufferedOutStream bout;
	const char *script;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Test the declaration of new function signatures
	script = "funcdef void functype();\n"
	// It must be possible to declare variables of the funcdef type
		     "functype @myFunc = null;\n"
	// It must be possible to initialize the function pointer directly
//			"functype @myFunc1 = @func;\n"
			"void func() {}\n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r != 0 )
		fail = true;

	// It must not be possible to declare a non-handle variable of the funcdef type
	script = "funcdef void functype();\n"
		     "functype myFunc;\n";

	// It must not be possible to invoke the funcdef
	script = "funcdef void functype();\n"
		     "void func() { functype(); } \n";

	// Test that it is possible to declare the function signatures out of order
	// This also tests the circular reference between the function signatures
	script = "funcdef void f1(f2@) \n"
	         "funcdef void f2(f1@) \n";

	// Test that it is required to declare the variable as handle
	script = "function void f()\n"
			 "f@ ptr2; \n"           // ok
		     "f  ptr1; \n";           // fail

	// Test that it is possible to take the address of a function
	script = "f@ myFuncPtr = @func; \n"
			 "funcdef void f() \n"
	         "void func() {} \n";
			 
	// Test that is possible to call a function in a function pointer
	script = "CALLBACK@ myFuncPtr = @func;              \n"
	         "funcdef bool CALLBACK(const string &in); \n"
			 "bool func(const string &in s)             \n"
			 "{                                         \n"
			 "  return s == 'test';                     \n"
			 "}                                         \n";
	
	// ExecuteString(engine, "assert(myFuncPtr('test'))", mod);

	// Test that the function in a function pointer isn't released while the function 
	// is being executed, even though the function pointer itself is cleared
	script = "DYNFUNC@ funcPtr;        \n"
		     "funcdef void DYNFUNC(); \n"
			 "@funcPtr = @CompileDynFunc('void func() { @funcPtr = null; }'); \n";

	// It must be possible to register the function signature from the application
	// engine->RegisterFunctionSignature("void CALLBACK()");

	// It must also be possible to enumerate the registered callbacks

	// It must be possible to identify a function handle type from the type id

	// It must be possible to save the byte code with function handles

	// It must be possible enumerate the function definitions in the module, 
	// and to enumerate the parameters the function accepts

	// Test that a funcdef can't have the same name as other global entities

	// A funcdef defined in multiple modules must share the id and signature so that a function implemented 
	// in one module can be called from another module by storing the handle in the funcdef variable

	// An interface that takes a funcdef as parameter must still have its typeid shared if the funcdef can also be shared
	// If the funcdef takes an interface as parameter, it must still be shared
		
	// Must not be possible to take the address of class methods

	engine->Release();

	// Success
 	return fail;
}

} // namespace

