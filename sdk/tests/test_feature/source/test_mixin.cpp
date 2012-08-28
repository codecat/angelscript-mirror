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
	// TODO: mixin classes cannot inherit from other classes (at least not to start with)
	// TODO: mixin classes cannot be declared as shared or final
	// TODO: mixin classes can be declared in separate namespaces

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
		script =
			"mixin class Test {} \n"
			"mixin class Test {} \n"
			"int Test; \n";
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

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

