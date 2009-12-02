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
	r = mod->CompileFunction("My func", "void func() {}", 0, &func);
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
	r = mod->CompileFunction("two funcs", "void func() {} void func2() {}", 0, 0);
	if( r >= 0 )
		fail = true;
	r = mod->CompileFunction("no code", "", 0, 0);
	if( r >= 0 )
		fail = true;
	r = mod->CompileFunction("var", "int a;", 0, 0);
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
	r = mod->CompileFunction(0, "void func() {}", 0, 0);
	if( r < 0 )
		fail = true;

	// If the code is not provided, a proper error should be given
	r = mod->CompileFunction(0,0,0,0);
	if( r != asINVALID_ARG )
		fail = true;

	// TODO: It should be possible to compile a recursive function
//	r = mod->CompileFunction(0, "void func() { func(); }", 0, 0);
//	if( r < 0 )
//		fail = true;

	// TODO: It should be possible to add the compiled function to the scope of the module

	// TODO: It should be possible to remove a function from the scope of the module

	// TODO: Maybe we can allow replacing an existing function

	// TODO: It should be possible to serialize these dynamic functions

	// TODO: Removing a function from the scope of the module shouldn't free it immediately if it is still used

	if( ctx ) 
		ctx->Release();
	engine->Release();

	// Success
	return fail;
}

} // namespace

