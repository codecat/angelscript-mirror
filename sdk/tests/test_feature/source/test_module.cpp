#include "utils.h"

namespace TestModule
{

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	ctx = engine->CreateContext();
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Compile a single function
	asIScriptFunction *func = 0;
	r = mod->CompileFunction("My func", "void func() {}", 0, 0, &func);
	if( r < 0 )
		fail = true;

	// Execute the function
	r = ctx->Prepare(func->GetId());
	if( r < 0 )
		fail = true;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// The function's section name should be correct
	if( std::string(func->GetScriptSectionName()) != "My func" )
		fail = true;

	// We must release the function afterwards
	if( func )
	{
		func->Release();
		func = 0;
	}

	// It must not be allowed to include more than one function in the code
	bout.buffer = "";
	r = mod->CompileFunction("two funcs", "void func() {} void func2() {}", 0, 0, 0);
	if( r >= 0 )
		fail = true;
	r = mod->CompileFunction("no code", "", 0, 0, 0);
	if( r >= 0 )
		fail = true;
	r = mod->CompileFunction("var", "int a;", 0, 0, 0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "two funcs (0, 0) : Error   : The code must contain one and only one function\n"
					   "no code (0, 0) : Error   : The code must contain one and only one function\n"
					   "var (0, 0) : Error   : The code must contain one and only one function\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Compiling without giving the function pointer shouldn't leak memory
	r = mod->CompileFunction(0, "void func() {}", 0, 0, 0);
	if( r < 0 )
		fail = true;

	// If the code is not provided, a proper error should be given
	r = mod->CompileFunction(0,0,0,0,0);
	if( r != asINVALID_ARG )
		fail = true;

	// Don't permit recursive calls, unless the function is added to the module scope
	// TODO: It may be possible to compile a recursive function, if the application explicitly allows it
	bout.buffer = "";
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, 0, 0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != " (1, 2) : Error   : No matching signatures to 'func()'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// It should be possible to add the compiled function to the scope of the module
	if( mod->GetFunctionCount() > 0 )
		fail = true;
	r = mod->CompileFunction(0, "void func() {}", 0, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		fail = true;
	if( mod->GetFunctionCount() != 1 )
		fail = true;

	// It should be possible to remove a function from the scope of the module
	r = mod->RemoveFunction(mod->GetFunctionIdByIndex(0));
	if( r < 0 )
		fail = true;
	if( mod->GetFunctionCount() != 0 )
		fail = true;

	// Compiling recursive functions that are added to the module is OK
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		fail = true;

	// TODO: Do not allow adding functions that already exist in the module

	// TODO: Removing a function from the scope of the module shouldn't free it 
	//       immediately if it is still used by another function

	// TODO: Make sure cyclic references between functions are resolved so we don't get memory leaks


	// TODO: Maybe we can allow replacing an existing function
	// TODO: It should be possible to serialize these dynamic functions
	// TODO: The dynamic functions should also be JIT compiled
	// TODO: What should happen if a function in the module scope references another function that has 
	//       been removed from the scope but is still alive, and then the byte code for the module is saved?

	if( ctx ) 
		ctx->Release();
	engine->Release();

	// Success
	return fail;
}

} // namespace

