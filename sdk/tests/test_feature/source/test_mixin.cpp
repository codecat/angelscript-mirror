#include "utils.h"

namespace TestMixin
{

bool Test()
{
	bool fail = false;
	asIScriptEngine *engine;
	int r;
	CBufferedOutStream bout;

	// TODO: mixins are parsed as classes but cannot be instanciated, i.e. the object type doesn't exist
	// TODO: mixins can be inherited from, which will include methods and properties from the mixin into the class
	// TODO: mixin class methods are compiled in the context of the inherited class, so they can access properties of the class
	// TODO: mixin class properties are ignored if the class explicitly declares its own
	// TODO: mixin class methods are ignored if the class explicitly declares its own
	// TODO: mixin classes cannot implement constructors/destructors (at least not to start with)

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		// Test basic parsing of mixin classes
		const char *script =
			"mixin class Test {\n"
			"  void Method() {} \n"
			"  int Property; \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Test name conflicts
		// mixin classes can be declared in namespaces to resolve conflicts
		script =
			"mixin class Test {} \n"
			"mixin class Test {} \n"
			"int Test; \n"
			"namespace A { \n"
			"  mixin class Test {} \n"
			"} \n";
		mod->AddScriptSection("", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != " (2, 13) : Error   : Name conflict. 'Test' is a mixin class.\n"
		                   " (3, 5) : Error   : Name conflict. 'Test' is a mixin class.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Mixin classes cannot be declared as 'shared' or 'final'
		script =
			"mixin shared final class Test {} \n";
		mod->AddScriptSection("", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != " (1, 7) : Error   : Mixin class cannot be declared as 'shared'\n"
		                   " (1, 14) : Error   : Mixin class cannot be declared as 'final'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// mixin classes cannot inherit from other classes
		// TODO: should be allowed to implement interfaces, and possibly inherit from classes
		//       when that is possible, the inheritance is simply transfered to the class that inherits the mixin class
		script =
			"interface Intf {} \n"
			"class Clss {} \n"
			"mixin class Test : Intf, Clss {} \n";
		mod->AddScriptSection("", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != " (3, 20) : Error   : Mixin class cannot inherit from classes or implement interfaces\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

