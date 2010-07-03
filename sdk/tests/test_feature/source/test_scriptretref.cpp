#include "utils.h"

namespace TestScriptRetRef
{

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	asIScriptEngine *engine;
	asIScriptModule *mod;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	RegisterScriptString(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Test returning reference to a global variable
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"int g;\n"
			"int &Test()\n"
			"{\n"
			"  return g;\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 ) fail = true;
		if( bout.buffer != "script (2, 1) : Info    : Compiling int& Test()\n"
						   "script (4, 3) : Error   : Script functions must not return references\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
	}

	// Test returning reference to a parameter
	// This should fail to compile because the origin cannot be guaranteed 
	{
		bout.buffer = "";
		const char *script19 =
			"class Object {}\n"
			"Object &Test(Object &in object)\n"
			"{\n"
			"  return object;\n"
			"}\n";
		mod->AddScriptSection("script19", script19, strlen(script19));
		r = mod->Build();
		if( r >= 0 ) fail = true;
		if( bout.buffer != "script19 (2, 1) : Info    : Compiling Object& Test(Object&in)\n"
						   "script19 (4, 3) : Error   : Script functions must not return references\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
	}

	// Test returning reference to temporary object
	// This should fail to compile because the reference is to a temporary variable
	{
		bout.buffer = "";
		const char *script25 = "string &SomeFunc() { return ''; }";
		r = mod->AddScriptSection("25", script25);
		r = mod->Build();
		if( r >= 0 ) fail = true;
		if( bout.buffer != "25 (1, 1) : Info    : Compiling string& SomeFunc()\n"
						   "25 (1, 22) : Error   : Script functions must not return references\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

