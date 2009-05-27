#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"


namespace TestDictionary
{

const char *script =
"void Test()                       \n"
"{                                 \n"
"  dictionary dict;                \n"
// Test integer with the dictionary
"  dict.set(\"a\", 42);            \n"
"  assert(dict.exists(\"a\"));     \n"
"  uint u = 0;                     \n"
"  dict.get(\"a\", u);             \n"
"  assert(u == 42);                \n"
"  dict.delete(\"a\");             \n"
"  assert(!dict.exists(\"a\"));    \n"
// Test string by handle
"  string a = \"t\";               \n"
"  dict.set(\"a\", @a);            \n"
"  string @b;                      \n"
"  dict.get(\"a\", @b);            \n"
"  assert(b == \"t\");             \n"
// Test string by value
"  dict.set(\"a\", a);             \n"
"  string c;                       \n"
"  dict.get(\"a\", c);             \n"
"  assert(c == \"t\");             \n"
// Test int8 with the dictionary
"  int8 q = 41;                    \n"
"  dict.set(\"b\", q);             \n"
"  dict.get(\"b\", q);             \n"
"  assert(q == 41);                \n"
// Test float with the dictionary
"  float f = 300;                  \n"
"  dict.set(\"c\", f);             \n"
"  dict.get(\"c\", f);             \n"
"  assert(f == 300);               \n"
// Test automatic conversion between int and float in the dictionary
"  int i;                          \n"
"  dict.get(\"c\", i);             \n"
"  assert(i == 300);               \n"
"  dict.get(\"b\", f);             \n"
"  assert(f == 41);                \n"
// Test booleans with the variable type
"  bool bl;                        \n"
"  dict.set(\"true\", true);       \n"
"  dict.set(\"false\", false);     \n"
"  bl = false;                     \n"
"  dict.get(\"true\", bl);         \n"
"  assert( bl == true );           \n"
"  dict.get(\"false\", bl);        \n"
"  assert( bl == false );          \n"
// Test circular reference with itself
"  dict.set(\"self\", @dict);      \n"
"}                                 \n";

// Test circular reference including a script class and the dictionary
const char *script2 = 
"class C { dictionary dict; }                \n"
"void f() { C c; c.dict.set(\"self\", @c); } \n"; 

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
 	asIScriptEngine *engine = 0;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptDictionary(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		fail = true;
	}
	ctx->Release();

	asUINT gcCurrentSize, gcTotalDestroyed, gcTotalDetected;
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
	engine->GarbageCollect();
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

	if( gcCurrentSize != 0 || gcTotalDestroyed != 1 || gcTotalDetected != 1 )
		fail = true;

	// Test circular references including a script class and the dictionary
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "f()");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
	engine->GarbageCollect();
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

	if( gcCurrentSize != 0 || gcTotalDestroyed != 3 || gcTotalDetected != 3  )
		fail = true;

	// Test invalid ref cast together with the variable argument
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "dictionary d; d.set('hello', cast<int>(4));");
	if( r >= 0 ) 
		fail = true;
	if( bout.buffer != "ExecuteString (1, 35) : Error   : Illegal target type for reference cast\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	engine->Release();

	//-------------------------
	// Test the generic interface as well
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptDictionary_Generic(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		fail = true;
	}
	ctx->Release();

	engine->Release();

	return fail;
}

} // namespace

