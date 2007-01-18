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

const char *script2 = "\
class TestObj                     \n\
{                                 \n\
    TestObj(int a) {this.a = a;}  \n\
	int a;                        \n\
}                                 \n\
void Func(TestObj obj)            \n\
{                                 \n\
    assert(obj.a == 2);           \n\
}                                 \n\
void Test()                       \n\
{                                 \n\
	Func(2);                      \n\
}                                 \n";


bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	int res = 0;
	engine->RegisterGlobalProperty("int res", &res);

	engine->ExecuteString(0, "res = cast<int>(2342.4)");
	if( res != 2342 ) 
		fail = true;

	engine->ExecuteString(0, "double tmp = 3452.4; res = cast<int>(tmp)");
	if( res != 3452 ) 
		fail = true;

	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->Build(0);

	r = engine->ExecuteString(0, "clss c; cast<intf1>(c); cast<intf2>(c);");
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "intf1 @a = clss(); cast<clss>(a).Test2(); cast<intf2>(a).Test2();");
	if( r < 0 )
		fail = true;

	// Test use of handle after invalid cast (should throw a script exception)
	r = engine->ExecuteString(0, "intf1 @a = clss(); cast<intf3>(a).Test3();");
	if( r != asEXECUTION_EXCEPTION )
		fail = true;

	// Don't permit cast operator to remove constness
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "const intf1 @a = clss(); cast<intf2>(a).Test2();");
	if( r >= 0 )
		fail = true;

	if( bout.buffer != "ExecuteString (1, 26) : Error   : No conversion from 'const intf1@&' to 'intf2@&' available.\n"
					   "ExecuteString (1, 40) : Error   : Illegal operation on 'const int'\n" )
		fail = true;

	//--------------
	// Using constructor as implicit cast operator
	// TODO:
/*	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, "Test2", script2, strlen(script2));
	r = engine->Build(0);
	if( r < 0 )
		fail = true;
*/
	engine->Release();

	// Success
 	return fail;
}

} // namespace

