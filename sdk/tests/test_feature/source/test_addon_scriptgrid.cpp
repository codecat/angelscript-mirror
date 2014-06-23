#include "utils.h"
#include "../../../add_on/scriptgrid/scriptgrid.h"
#include "../../../add_on/scriptany/scriptany.h"

namespace Test_Addon_ScriptGrid
{

static const char *TESTNAME = "Test_Addon_ScriptGrid";

bool Test()
{
	RET_ON_MAX_PORT

	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
//	asIScriptContext *ctx;
	asIScriptEngine *engine;
//	asIScriptModule *mod;

	// Test grid object forcibly destroyed by garbage collector
	// http://www.gamedev.net/topic/657955-a-quite-specific-bug/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptGrid(engine);
		RegisterScriptAny(engine);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"class B {} \n"
			"class A \n"
			"{ \n"
			"	any a; \n"
			"	grid<B> t(10, 10); \n"
			"	A() \n"
			"	{ \n"
			"		a.store(@this); \n"
			"	} \n"
			"} \n"
			"array<A@> arr; \n"
			"void main() \n"
			"{ \n"
			"	arr.insertLast(@A()); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test resize
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptGrid(engine);
		RegisterStdString(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, 
			"grid<string> g; \n"
			"g.resize(1,1); \n"
			"g[0,0] = 'hello'; \n"
			"g.resize(2,2); \n"
			"assert( g[0,0] == 'hello' ); \n"
			"g[1,1] = 'there'; \n"
			"g.resize(1,1); \n"
			"assert( g.width() == 1 && g.height() == 1 ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test initialization lists
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptGrid(engine);
		RegisterStdString(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, 
			"grid<int8> g = {{1,2,3},{4,5,6},{7,8,9}}; \n"
			"assert( g[0,0] == 1 ); \n"
			"assert( g[2,2] == 9 ); \n"
			"assert( g[0,2] == 7 ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, 
			"grid<string> g = {{'1','2'},{'4','5'}}; \n"
			"assert( g[0,0] == '1' ); \n"
			"assert( g[1,1] == '5' ); \n"
			"assert( g[0,1] == '4' ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine,
			"grid<grid<int>@> g = {{grid<int> = {{1}}, grid<int> = {{2}}}, {grid<int> = {{3}}, grid<int> = {{4}}}}; \n"
			"assert( g[0,0][0,0] == 1 ); \n"
			"assert( g[1,1][0,0] == 4 ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	
		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

