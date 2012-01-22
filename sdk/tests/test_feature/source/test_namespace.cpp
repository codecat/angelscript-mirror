#include "utils.h"

namespace TestNamespace
{

bool Test()
{
	bool fail = false;
	asIScriptEngine *engine;
	int r;
	COutStream out;

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		// Nested namespaces are allowed
		const char *script =
			"void func() {} \n"
			"namespace a { \n"
			"  void func1() {} \n"
			"  namespace b { \n"
			"    void func2() {} \n"
			"  } \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

