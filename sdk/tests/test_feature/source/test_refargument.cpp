#include "utils.h"

namespace TestRefArgument
{

#define TESTNAME "TestRefArgument"

static const char *script1 =
"void TestObjHandle(refclass &in ref)   \n"
"{                                      \n"
"   float r;                            \n"
"   test2(r);                           \n"
"   Assert(ref.id == 0xdeadc0de);       \n"
"   test(ref);                          \n"
"}                                      \n"
"void test(refclass &in ref)            \n"
"{                                      \n"
"   Assert(ref.id == 0xdeadc0de);       \n"
"}                                      \n"
"void test2(float &out ref)             \n"
"{                                      \n"
"}                                      \n";

class CRefClass
{
public:
	CRefClass() 
	{
		id = 0xdeadc0de;
	}
	~CRefClass() 
	{
	}
	CRefClass &operator=(const CRefClass &other) {id = other.id; return *this;}
	int id;
};

static void Assert(bool expr)
{
	if( !expr )
	{
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			printf("func: %s\n", engine->GetFunctionDeclaration(ctx->GetCurrentFunction()));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
		}
	}
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	r = engine->RegisterObjectType("refclass", sizeof(CRefClass), asOBJ_CLASS_CDA); assert(r >= 0);
	r = engine->RegisterObjectProperty("refclass", "int id", offsetof(CRefClass, id)); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ASSIGNMENT, "refclass &f(refclass &in)", asMETHOD(CRefClass, operator=), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL); assert( r >= 0 );

	COutStream out;

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	asIScriptContext *ctx;
	engine->CreateContext(&ctx);
	int func = engine->GetFunctionIDByName(0, "TestObjHandle"); assert(r >= 0);

	CRefClass cref;	
	r = ctx->Prepare(r); assert(r >= 0);
	ctx->SetArgObject(0, &cref);
	r = ctx->Execute();  assert(r >= 0);


	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
		printf("%s: Execution failed: %d\n", TESTNAME, r);
	}
	if( ctx ) ctx->Release();


	engine->Release();

	// Success
	return fail;
}

} // namespace
