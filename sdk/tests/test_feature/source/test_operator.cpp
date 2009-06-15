#include "utils.h"

namespace TestOperator
{


bool Test()
{
	bool fail = false;
	COutStream out;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	const char *script = 
		"class Test                         \n"
		"{                                  \n"
		"  int value;                       \n"
		"  bool opEquals(const Test &in o)  \n"
		"  {                                \n"
		"    return value == o.value;       \n"
		"  }                                \n"
		"}                                  \n"
		"void main()                        \n"
		"{                                  \n"
		"  Test a,b;                        \n"
		"  a.value = 0;                     \n"
		"  b.value = 0;                     \n"
		"  assert( a == b );                \n"
		"  assert( a.opEquals(b) );         \n"  // Same as a == b
		"}                                  \n";

	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}
	
	r = engine->ExecuteString(0, "main()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	// TODO: Test with overloads, e.g. bool opEquals(int v)
	// TODO: opEquals that don't return bool are not considered
	// TODO: Check const correctness
	// TODO: Test with inverted arguments, e.g. 0 == a
	// TODO: Test !=, i.e. !a.opEquals(b)

	// TODO: If opEquals isn't implemented the compiler should try opCmp instead

	// Success
	return fail;
}

}