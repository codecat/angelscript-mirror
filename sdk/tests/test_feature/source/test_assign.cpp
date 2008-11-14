#include "utils.h"

namespace TestAssign
{

#define TESTNAME "TestAssign"

static const char *script1 =
"void main()                      \n"
"{                                \n"
"  uint8[] a={2,3,4,5};           \n"
"                                 \n"
"  a[1] |= 0x30;                  \n"
"  a[2] += 0x30;                  \n"
"  print(a[1]);                   \n"
"  print(a[2]);                   \n"
"  assert(a[1] == 0x33);          \n"
"  assert(a[2] == 0x34);          \n"
"}                                \n";


void print_generic(asIScriptGeneric *gen)
{
	int a = *(int*)gen->GetAddressOfArg(0);
//	printf("%d\n", a);
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void print(int)", asFUNCTION(print_generic), asCALL_GENERIC);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 0);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = engine->ExecuteString(0, "main()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		fail = true;
	}
	if( ctx ) ctx->Release();

	engine->Release();

	// Success
	return fail;
}

} // namespace

