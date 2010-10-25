#include "utils.h"

namespace TestConfigAccess
{
          
const char * const TESTNAME = "TestConfigAccess";

static void Func(asIScriptGeneric *)
{
}

static void TypeAdd(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	int *b = (int*)gen->GetArgAddress(0);

	gen->SetReturnDWord(*a + *b);
}

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	COutStream out;

	float val;

	//------------
	// Test global properties
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("float val", &val); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	// Make sure the default access is granted
	r = ExecuteString(engine, "val = 1.3f"); 
	if( r < 0 )
		TEST_FAILED;

	// Make sure the default access can be turned off
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->SetConfigGroupModuleAccess("group", asALL_MODULES, false); assert( r >= 0 );

	r = ExecuteString(engine, "val = 1.0f");
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : 'val' is not declared\n"
/*		               "ExecuteString (1, 5) : Error   : Reference is read-only\n"
		               "ExecuteString (1, 5) : Error   : Not a valid lvalue\n"*/)
		TEST_FAILED;

	// Make sure the default access can be overridden
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->SetConfigGroupModuleAccess("group", "ExecuteString", true); assert( r >= 0 );

	r = ExecuteString(engine, "val = 1.0f");
	if( r < 0 )
		TEST_FAILED;

	engine->Release();

	//----------
	// Test global functions
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void Func()", asFUNCTION(Func), asCALL_GENERIC); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", "ExecuteString", false); assert( r >= 0 );

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "Func()");
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : No matching signatures to 'Func()'\n" )
		TEST_FAILED;

	r = engine->SetConfigGroupModuleAccess("group", "ExecuteString", true); assert( r >= 0 );

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = ExecuteString(engine, "Func()");
	if( r < 0 )
		TEST_FAILED;

	engine->Release();

	//------------
	// Test object types
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", "ExecuteString", false); assert( r >= 0 );

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "mytype a");
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "ExecuteString (1, 1) : Error   : Type 'mytype' is not available for this module\n")
		TEST_FAILED;

	engine->Release();

	//------------
	// Test class methods in different config groups
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );

	r = engine->BeginConfigGroup("group"); assert( r >= 0 );
	r = engine->RegisterObjectMethod("mytype", "mytype opAdd(mytype &in)", asFUNCTION(TypeAdd), asCALL_GENERIC); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->SetConfigGroupModuleAccess("group", 0, false); assert( r >= 0 );

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "mytype a; a + a;");

	// TODO: It should be possible to disallow individual class methods
//	if( r >= 0 )
//		TEST_FAILED;

//	if( bout.buffer != "ExecuteString (1, 13) : Error   : No matching operator that takes the types 'mytype&' and 'mytype&' found\n")
//		TEST_FAILED;

	engine->Release();

	// Success
	return fail;
}

}
