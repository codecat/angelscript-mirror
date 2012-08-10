//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "script_factory.h"
#include <string>
#include <sstream>
using std::string;
using std::stringstream;

namespace TestBigArrays
{

#define TESTNAME "TestBigArray"

enum {
    c_elem_cnt = 100000,
    c_array_cnt = 2
};

void print( const string & )
{
}

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.25.0 WIP 1: 4.04 secs\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);

	printf("\nGenerating...\n");

	string script;
	create_array_test(c_array_cnt, c_elem_cnt, script);

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



