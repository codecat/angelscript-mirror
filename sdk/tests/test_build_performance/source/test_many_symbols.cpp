//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "script_factory.h"
#include <string>
#include <sstream>
using std::string;
using std::stringstream;

namespace TestManySymbols
{

#define TESTNAME "TestManySymbols"

enum {
    c_elem_cnt = 10000
};

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.25.0 WIP 1: 10.86 secs\n");


 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);

	printf("\nGenerating...\n");

	string script;
	create_symbol_test(c_elem_cnt, script);

	printf("\nBuilding...\n");

	double time = GetSystemTimer();

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script.c_str(), script.size(), 0);
	int r = mod->Build();

	time = GetSystemTimer() - time;

	if( r != 0 )
		printf("Build failed\n", TESTNAME);
	else
		printf("Time = %f secs\n", time);

	engine->Release();
}

} // namespace



