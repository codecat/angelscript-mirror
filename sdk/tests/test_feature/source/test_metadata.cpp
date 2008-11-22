#include "utils.h"

#pragma warning (disable:4786)
#include "../../../add_on/scriptbuilder/scriptbuilder.h"
#include <map>

namespace TestMetaData
{

const char *script = 
// Global functions can have meta data
"[ my meta data test ] void func1() {} \n"
// meta data strings can contain any tokens, and can use nested []
"[ test['hello'] ] void func2() {} \n"
// global variables can have meta data
"[ init ] int g_var = 0; \n"
// global object variable
"[ var of type myclass ] MyClass g_obj(); \n"
// class declarations can have meta data
"[ myclass ] class MyClass {} \n"
// interface declarations can have meta data
"[ myintf ] interface MyIntf {} \n";

using namespace std;



bool Test()
{
	bool fail = false;
	int r = 0;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	// Test the parse token method
	asETokenClass t = engine->ParseToken("!is");
	if( t != asTC_KEYWORD ) 
		fail = true; 

	// Compile a script with meta data strings
	CScriptBuilder builder;
	r = builder.BuildScriptFromMemory(engine, 0, script);
	if( r < 0 )
		fail = true;

	int funcId = engine->GetFunctionIDByName(0, "func1");
	string metadata = builder.GetMetadataStringForFunc(funcId);
	if( metadata != " my meta data test " )
		fail = true;

	funcId = engine->GetFunctionIDByName(0, "func2");
	metadata = builder.GetMetadataStringForFunc(funcId);
	if( metadata != " test['hello'] " )
		fail = true;

	int typeId = engine->GetTypeIdByDecl(0, "MyClass");
	metadata = builder.GetMetadataStringForType(typeId);
	if( metadata != " myclass " )
		fail = true;

	typeId = engine->GetTypeIdByDecl(0, "MyIntf");
	metadata = builder.GetMetadataStringForType(typeId);
	if( metadata != " myintf " )
		fail = true;

	int varIdx = engine->GetGlobalVarIndexByName(0, "g_var");
	metadata = builder.GetMetadataStringForVar(varIdx);
	if( metadata != " init " )
		fail = true;

	varIdx = engine->GetGlobalVarIndexByName(0, "g_obj");
	metadata = builder.GetMetadataStringForVar(varIdx);
	if( metadata != " var of type myclass " )
		fail = true;

	engine->Release();

	return fail;
}

} // namespace

