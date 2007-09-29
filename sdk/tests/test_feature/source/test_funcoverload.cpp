#include "utils.h"

#define TESTNAME "TestFuncOverload"

static const char *script1 =
"void Test()                               \n"
"{                                         \n"
"  Obj i;                                  \n"
"  TX.Set(\"user\", i.Value());            \n"
"}                                         \n";

static const char *script2 =
"void ScriptFunc(void)                     \n"
"{                                         \n"
"}                                         \n";

class Obj
{
public:
	void *p;
	void *Value() {return p;}
	void Set(const std::string&, void *) {}
};

static Obj o;

void FuncVoid()
{
}

void FuncInt(int v)
{
}

bool TestFuncOverload()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = false;
	COutStream out;	

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	engine->RegisterObjectType("Data", sizeof(void*), asOBJ_VALUE | asOBJ_APP_PRIMITIVE);

	engine->RegisterObjectType("Obj", sizeof(Obj), asOBJ_REF);
	engine->RegisterObjectMethod("Obj", "Data &Value()", asMETHOD(Obj, Value), asCALL_THISCALL);
	engine->RegisterObjectMethod("Obj", "void Set(string &in, Data &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
	engine->RegisterObjectMethod("Obj", "void Set(string &in, string &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
	engine->RegisterGlobalProperty("Obj TX", &o);

	engine->RegisterGlobalFunction("void func()", asFUNCTION(FuncVoid), asCALL_CDECL);
	engine->RegisterGlobalFunction("void func(int)", asFUNCTION(FuncInt), asCALL_CDECL);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->Build(0);

	engine->ExecuteString(0, "func(func(3));");

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	int r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "TestFuncOverload (1, 1) : Info    : Compiling void ScriptFunc(void)\n"
                       "TestFuncOverload (1, 17) : Error   : Parameter type can't be 'void'\n" )
		fail = true;

	// Don't permit void parameters
	r = engine->RegisterGlobalFunction("void func2(void)", asFUNCTION(FuncVoid), asCALL_CDECL); assert( r < 0 );

	engine->Release();

	return fail;
}
