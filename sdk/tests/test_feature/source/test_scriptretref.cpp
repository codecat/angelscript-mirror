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
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

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
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "g = 0; Test() = 42; assert( g == 42 );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning reference to a global variable
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"string g;\n"
			"string &Test()\n"
			"{\n"
			"  return g;\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "Test() = '42'; assert( g == '42' );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning reference to a global variable
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"string @g;\n"
			"string@ &Test()\n"
			"{\n"
			"  return g;\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "@Test() = @'42'; assert( g == '42' );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning reference to a class member
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"class A { \n"
			"  int g;\n"
			"  int &Test()\n"
			"  {\n"
			"    return g;\n"
			"  }\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "A a; a.g = 0; a.Test() = 42; assert( a.g == 42 );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning reference to a class member
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"class A { \n"
			"  string g;\n"
			"  string &Test()\n"
			"  {\n"
			"    return g;\n"
			"  }\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "A a; a.g = ''; a.Test() = '42'; assert( a.g == '42' );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning reference to a class member
	// This should work, as the global variable is guaranteed to be there even after the function returns
	{
		bout.buffer = "";
		const char *script =
			"class A { \n"
			"  string @g;\n"
			"  string @&Test()\n"
			"  {\n"
			"    return g;\n"
			"  }\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) fail = true;
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}

		r = ExecuteString(engine, "A a; @a.Test() = @'42'; assert( a.g == '42' );", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}

	// Test returning a constant
	// This should fail to compile because the expression is not a reference
	{
		bout.buffer = "";
		const char *script =
			"int &Test()\n"
			"{\n"
			"  return 42;\n"
			"}\n";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 ) fail = true;
		if( bout.buffer != "script (1, 1) : Info    : Compiling int& Test()\n"
						   "script (3, 3) : Error   : Not a valid reference\n" )
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
						   "script19 (4, 3) : Error   : Can't return reference to local value.\n" )
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
						   "25 (1, 22) : Error   : Can't return reference to local value.\n" )
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

