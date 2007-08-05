#include "utils.h"

namespace TestGetArgPtr
{

#define TESTNAME "TestGetArgPtr"

static const char *script1 = 
"int test(int a, string b, string @c, float &in d)     \n"
"{                                                     \n"
"  assert(a == 3);                                     \n"
"  assert(b == \"tst\");                               \n"
"  assert(c == \"42\");                                \n"
"  assert(d == 3.14f);                                 \n"
"  return 107;                                         \n"
"}                                                     \n"
"string test2()                                        \n"
"{                                                     \n"
"  return \"tst\";                                     \n"
"}                                                     \n";

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	int func = engine->GetFunctionIDByName(0, "test");
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(func);

	*(int*)ctx->GetArgPointer(0) = 3;
	*(asCScriptString**)ctx->GetArgPointer(1) = new asCScriptString("tst");
	*(asCScriptString**)ctx->GetArgPointer(2) = new asCScriptString("42"); // We don't have to add a reference, because we don't want to keep a copy
	float pi = 3.14f;
	*(float**)ctx->GetArgPointer(3) = &pi;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED ) fail = true;

	if( *(int*)ctx->GetReturnPointer() != 107 ) fail = true;

	func = engine->GetFunctionIDByName(0, "test2");
	ctx->Prepare(func);

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED ) fail = true;

	if( (*(asCScriptString**)ctx->GetReturnPointer())->buffer != "tst" ) fail = true;

	ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

