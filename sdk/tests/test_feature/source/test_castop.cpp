#include "utils.h"

namespace TestCastOp
{

#define TESTNAME "TestCastOp"

const char *script = "\
interface intf1            \n\
{                          \n\
  void Test1();            \n\
}                          \n\
interface intf2            \n\
{                          \n\
  void Test2();            \n\
}                          \n\
interface intf3            \n\
{                          \n\
  void Test3();            \n\
}                          \n\
class clss : intf1, intf2  \n\
{                          \n\
  void Test1() {}          \n\
  void Test2() {}          \n\
}                          \n";


bool Test()
{
	bool fail = false;
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

	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->Build(0);

	engine->ExecuteString(0, "clss c; cast<intf1>(c); cast<intf2>(c);");
	engine->ExecuteString(0, "intf1 @a = clss(); cast<clss>(a).Test2(); cast<intf2>(a).Test2();");

	// Test use of handle after invalid cast (should throw a script exception)
	engine->ExecuteString(0, "intf1 @a = clss(); cast<intf3>(a).Test3();");

	// TODO: Don't permit cast operator to remove constness

	engine->Release();

	// Success
 	return fail;
}

} // namespace

