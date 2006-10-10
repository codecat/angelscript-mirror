#include "utils.h"

namespace TestCastOp
{

#define TESTNAME "TestCastOp"

const char *script = "\
interface intf1       \n\
{                     \n\
void Test1();         \n\
}                     \n\
interface intf2       \n\
{                     \n\
void Test2();         \n\
}                     \n\
class clss            \n\
{                     \n\
void Test1() {}       \n\
void Test2() {}       \n\
}                     \n";


bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	int res = 0;
	engine->RegisterGlobalProperty("int res", &res);

	engine->ExecuteString(0, "res = cast<int>(2342.4)");
	if( res != 2342 ) fail = true;

	engine->ExecuteString(0, "double tmp = 3452.4; res = cast<int>(tmp)");
	if( res != 3452 ) fail = true;

	// TODO: Cast between interfaces of a script class
	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->Build(0);

	engine->ExecuteString(0, "clss c; cast<intf1>(c); cast<intf2>(c);");
	engine->ExecuteString(0, "intf1 @a = clss(); cast<clss>(a); cast<intf2>(a);");

	// TODO: Don't permit cast operator to remove constness

	engine->Release();

	// Success
 	return fail;
}

} // namespace

