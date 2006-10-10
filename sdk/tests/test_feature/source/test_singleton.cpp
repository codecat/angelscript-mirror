#include "utils.h"
#include "stdstring.h"

using namespace std;

namespace TestSingleton
{

#define TESTNAME "TestSingleton"

int GameMgr;
int SoundMgr;

const char *script =
"void TestSingleton() \n"
"{                    \n"
"  Game.Test();       \n"
"}                    \n";

void TestMethod(asIScriptGeneric *gen)
{
	int *obj = (int*)gen->GetObject();
	assert(obj == &GameMgr || obj == &SoundMgr);
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);

	int inta = 0, intb = 0;
	r = engine->RegisterObjectType("GameMgr", 0, 0); assert(r >= 0);
	r = engine->RegisterObjectMethod("GameMgr", "void Test()", asFUNCTION(TestMethod), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalProperty("GameMgr Game", (void*)&GameMgr); assert(r >= 0);

	// Test registering another object globabl property after 
	// compiling script that uses previous global property
	engine->AddScriptSection(0, "script", script, strlen(script), 0, false);
	engine->Build(0);

	r = engine->RegisterObjectType("SoundMgr", 0, 0); assert(r >= 0);
	r = engine->RegisterObjectMethod("SoundMgr", "void Test()", asFUNCTION(TestMethod), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalProperty("SoundMgr SMgr", (void*)&SoundMgr); assert(r >= 0);

	engine->ExecuteString(0, "Game.Test()");
	engine->ExecuteString(0, "SMgr.Test()");
	
	engine->ExecuteString(0, "TestSingleton()");

	engine->Release();

	// Success
	return fail;
}

} // namespace

