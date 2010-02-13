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
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Test the declaration of new function signatures
	script = "funcdef void functype();\n"
	// It must be possible to declare variables of the funcdef type
		     "functype @myFunc = null;\n"
	// It must be possible to initialize the function pointer directly
			 "functype @myFunc1 = @func;\n"
		 	 "void func() {}\n"
	// It must be possible to compare the function pointer with another
	         "void main() { \n"
			 "  assert( myFunc1 !is null ); \n"
			 "  assert( myFunc1 is func ); \n"
	// It must be possible to call a function through the function pointer
	    	 "  myFunc(); \n"
			 "} \n";
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r != 0 )
		fail = true;
	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
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

	// If the function referred to when taking a function pointer is removed from the module,
	// the code must not be invalidated. After removing func() from the module, it must still 
	// be possible to execute func2()
	script = "funcdef void FUNC(); \n"
	         "void func() {} \n";
	         "void func2() { FUNC@ f = @func; f(); } \n";




	engine->Release();

	// Success
 	return fail;
}

} // namespace

