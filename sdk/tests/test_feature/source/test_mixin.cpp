#include "utils.h"

using namespace std;

namespace TestMixin
{

bool Test()
{
	bool fail = false;
	asIScriptEngine *engine;
	int r;
	CBufferedOutStream bout;
	const char *script;
	asIScriptModule *mod;

	// TODO: mixins are parsed as classes but cannot be instanciated, i.e. the object type doesn't exist
	// TODO: mixin class methods are compiled in the context of the inherited class, so they can access properties of the class
	// TODO: mixin class methods are ignored if the class explicitly declares its own
	// TODO: mixin class methods are included even if an inherited base class implements them (the mixin method overrides the base class' method)
	// TODO: mixin classes cannot implement constructors/destructors (at least not to start with)

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);

		// Test basic parsing of mixin classes
		{
			script =
				"mixin class Test {\n"
				"  void Method() {} \n"
				"  int Property; \n"
				"} \n";

			mod->AddScriptSection("", script);
			r = mod->Build();
			if( r < 0 )
				TEST_FAILED;
			if( bout.buffer != "" )
			{
				printf("%s", bout.buffer.c_str());
				TEST_FAILED;
			}
		}

		// Test name conflicts
		// mixin classes can be declared in namespaces to resolve conflicts
		{
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
		}

		// Mixin classes cannot be declared as 'shared' or 'final'
		{
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
		}

		// mixin classes cannot inherit from other classes
		// TODO: should be allowed to implement interfaces, and possibly inherit from classes
		//       when that is possible, the inheritance is simply transfered to the class that inherits the mixin class
		{
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
		}

		// including a mixin class adds the properties from the mixin to the class
		{
			script = 
				"mixin class Mix { \n"
				"  int prop; \n"
				"} \n"
				"class Clss : Mix { \n"
				"  void func() { prop++; } \n"
				"} \n";
			mod->AddScriptSection("", script);

			bout.buffer = "";
			r = mod->Build();
			if( r < 0 )
				TEST_FAILED;
			if( bout.buffer != "" )
			{
				printf("%s", bout.buffer.c_str());
				TEST_FAILED;
			}
		}

		// properties from mixin classes are not included if they conflict with existing properties
		{
			script = 
				"mixin class Mix { int prop; int prop2; } \n"
				"class Base { float prop; } \n"
				"class Clss : Base, Mix {} \n";
			mod->AddScriptSection("", script);

			bout.buffer = "";
			r = mod->Build();
			if( r < 0 )
				TEST_FAILED;
			if( bout.buffer != "" )
			{
				printf("%s", bout.buffer.c_str());
				TEST_FAILED;
			}

			asIObjectType *ot = mod->GetObjectTypeByName("Clss");
			if( ot == 0 )
				TEST_FAILED;

			if( ot->GetPropertyCount() != 2 )
				TEST_FAILED;
			const char *name;
			int typeId;
			if( ot->GetProperty(0, &name, &typeId) < 0 )
				TEST_FAILED;
			if( string(name) != "prop" || typeId != asTYPEID_FLOAT )
				TEST_FAILED;
		}

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

