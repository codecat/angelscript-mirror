#include "utils.h"

namespace TestModule
{

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
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

	// We must release the function afterwards
	if( func )
		func->Release();

	// TODO: Make sure the section name is correct

	// TODO: It should be possible to compile a recursive function

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

