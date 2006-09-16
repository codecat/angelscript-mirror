#include "utils.h"
#include "stdstring.h"

using namespace std;

namespace TestSingleton
{

#define TESTNAME "TestSingleton"

int GameMgr;
int SoundMgr;

void TestMethod(int *obj)
{
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

	int dummy;
	r = engine->RegisterGlobalProperty("int a", &dummy);
	r = engine->RegisterGlobalProperty("int b", &dummy);


	r = engine->RegisterObjectType("GameMgr", 0, 0); assert(r >= 0);
	r = engine->RegisterObjectMethod("GameMgr", "void Test()", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST); assert(r >= 0);
	
	// Misc
	engine->RegisterObjectMethod("GameMgr", "void EnableFrameRateCounter(int enable)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "int GetWidth()", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "int GetHeight()", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);

	// Driver stuff
	engine->RegisterObjectMethod("GameMgr", "int SetDriverMode(int w, int h, int b)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "void SetGamma(float gamma)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "float GetGamma()", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "void EnableFog(float r, float g, float b, float start, float end)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);

	// Bitmap Management
	engine->RegisterObjectMethod("GameMgr", "void AddBitmap(string &name, string &filename)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "void RemoveBitmap(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "bool HasBitmap(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "bool DrawBitmap(string &name, int x, int y)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);

	// Actor Def Management
	engine->RegisterObjectMethod("GameMgr", "void AddActorDef(string &name, string &filename)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("GameMgr", "void RemoveActorDef(string &name, string &filename)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);

	r = engine->RegisterGlobalProperty("GameMgr Game", (void*)&GameMgr); assert(r >= 0);

	r = engine->RegisterObjectType("SoundMgr", 0, 0); assert(r >= 0);
	r = engine->RegisterObjectMethod("SoundMgr", "void Test()", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST); assert(r >= 0);

	engine->RegisterObjectMethod("SoundMgr", "int AddSound(string &name, string &filename)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int Add3DSound(string &name, string &filename)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int RemoveSound(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int HasSound(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int PlaySound(string &name, float vol, float pan, float freq, int loop)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int StopSound(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int ModifySound(string &name, float vol, float pan, float freq)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int IsSoundPlaying(string &name)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("SoundMgr", "int SetMasterVolume(float vol)", asFUNCTION(TestMethod), asCALL_CDECL_OBJLAST);

	r = engine->RegisterGlobalProperty("int a1", &dummy);
	r = engine->RegisterGlobalProperty("int b2", &dummy);
	r = engine->RegisterGlobalProperty("int a3", &dummy);
	r = engine->RegisterGlobalProperty("int b4", &dummy);
	r = engine->RegisterGlobalProperty("int a5", &dummy);
	r = engine->RegisterGlobalProperty("int b6", &dummy);
	r = engine->RegisterGlobalProperty("int a7", &dummy);
	r = engine->RegisterGlobalProperty("int b8", &dummy);

	r = engine->RegisterGlobalProperty("SoundMgr SMgr", (void*)&SoundMgr); assert(r >= 0);
	r = engine->RegisterGlobalProperty("int a9", &dummy);
	r = engine->RegisterGlobalProperty("int b10", &dummy);
	r = engine->RegisterGlobalProperty("int a11", &dummy);
	r = engine->RegisterGlobalProperty("int b12", &dummy);
	r = engine->RegisterGlobalProperty("int a13", &dummy);
	r = engine->RegisterGlobalProperty("int b14", &dummy);
	r = engine->RegisterGlobalProperty("int a15", &dummy);
	r = engine->RegisterGlobalProperty("int b16", &dummy);


	engine->ExecuteString(0, "Game.Test()");
	engine->ExecuteString(0, "SMgr.Test()");

	engine->Release();

	// Success
	return fail;
}

} // namespace

