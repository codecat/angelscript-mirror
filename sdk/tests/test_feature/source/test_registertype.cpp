#include "utils.h"

namespace TestRegisterType
{

#define TESTNAME "TestRegisterType"

void DummyFunc() {}

bool Test()
{
	bool fail = false;
	int r = 0;
	CBufferedOutStream bout;
 	asIScriptEngine *engine;
	const char *script;

	// A type registered with asOBJ_REF must not register destructor
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 4, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : The behaviour is not compatible with the type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// A type registered with asOBJ_GC must register all gc behaviours
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("gc", 4, asOBJ_REF | asOBJ_GC); assert( r >= 0 );
	r = engine->ExecuteString(0, "");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : Type 'gc' is missing behaviours\n"
		               " (0, 0) : Error   : Invalid configuration\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// A type registered with asOBJ_VALUE must not register addref, release, and gc behaviours
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("val", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	r = engine->RegisterObjectBehaviour("val", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	r = engine->RegisterObjectBehaviour("val", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : The behaviour is not compatible with the type\n"
		               " (0, 0) : Error   : The behaviour is not compatible with the type\n"
					   " (0, 0) : Error   : The behaviour is not compatible with the type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Object types registered as ref must not be allowed to be
	// passed by value to registered functions, nor returned by value
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 4, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void f(ref)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;
	r = engine->RegisterGlobalFunction("ref f()", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref type without registered assignment behaviour won't allow the assignment
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 ); 
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_FACTORY, "ref@ f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
	r = engine->ExecuteString(0, "ref r1, r2; r1 = r2;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 18) : Error   : There is no copy operator for this type available.\n"
		               "ExecuteString (1, 16) : Error   : There is no copy operator for this type available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref type must register addref and release
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->ExecuteString(0, "ref r");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : Type 'ref' is missing behaviours\n"
		               " (0, 0) : Error   : Invalid configuration\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref type with asOBJ_NOHANDLE must not register addref, release, and factory
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF | asOBJ_NOHANDLE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC); 
	if( r != asILLEGAL_BEHAVIOUR_FOR_TYPE )
		fail = true;
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_FACTORY, "ref @f()", asFUNCTION(0), asCALL_GENERIC);
	if( bout.buffer != " (0, 0) : Error   : The behaviour is not compatible with the type\n"
		               " (0, 0) : Error   : The behaviour is not compatible with the type\n"
					   "System function (1, 5) : Error   : Object handle is not supported for this type\n")
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Value type with asOBJ_POD without registered assignment behaviour should allow bitwise copy
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE | asOBJ_POD); assert( r >= 0 );
	r = engine->ExecuteString(0, "val v1, v2; v1 = v2;");
	if( r != asEXECUTION_FINISHED )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Value type without asOBJ_POD and assignment behaviour must not allow bitwise copy
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->ExecuteString(0, "val v1, v2; v1 = v2;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 18) : Error   : There is no copy operator for this type available.\n"
		               "ExecuteString (1, 16) : Error   : There is no copy operator for this type available.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

    // Value types without asOBJ_POD must have constructor and destructor registered
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE); assert( r >= 0 );
	r = engine->RegisterObjectType("val1", 4, asOBJ_VALUE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("val1", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectType("val2", 4, asOBJ_VALUE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("val2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->ExecuteString(0, "val v1, v2; v1 = v2;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : Type 'val' is missing behaviours\n"
		               " (0, 0) : Error   : Type 'val1' is missing behaviours\n"
					   " (0, 0) : Error   : Type 'val2' is missing behaviours\n"
					   " (0, 0) : Error   : Invalid configuration\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref type must register ADDREF and RELEASE
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectType("ref1", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref1", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectType("ref2", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref2", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->ExecuteString(0, "ref @r;");
	if( r >= 0 )
		fail = true;
	if( bout.buffer != " (0, 0) : Error   : Type 'ref' is missing behaviours\n"
		               " (0, 0) : Error   : Type 'ref1' is missing behaviours\n"
					   " (0, 0) : Error   : Type 'ref2' is missing behaviours\n"
					   " (0, 0) : Error   : Invalid configuration\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref types without default factory must not be allowed to be initialized, nor must it be allowed to be passed by value in parameters or returned by value
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	script = "ref func(ref r) { ref r2; return ref(); }";
	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling ref func(ref)\n"
		               "script (1, 1) : Error   : Data type can't be 'ref'\n"
					   "script (1, 10) : Error   : Parameter type can't be 'ref'\n"
					   "script (1, 23) : Error   : Data type can't be 'ref'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// Ref types without default constructor must not be allowed to be passed by in/out reference, but must be allowed to be passed by inout reference
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	script = "void func(ref &in r1, ref &out r2, ref &inout r3) { }";
	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func(ref&in, ref&out, ref&inout)\n"
		               "script (1, 11) : Error   : Parameter type can't be 'ref&'\n"
					   "script (1, 23) : Error   : Parameter type can't be 'ref&'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();
	
	// It must not be possible to register functions that take handles of types with asOBJ_HANDLE
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("ref", 0, asOBJ_REF | asOBJ_NOHANDLE); assert( r >= 0 );
	r = engine->ExecuteString(0, "ref @r");
	if( r >= 0 )
		fail = true;
	r = engine->RegisterGlobalFunction("ref@ func()", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 5) : Error   : Object handle is not supported for this type\n"
	                   "ExecuteString (1, 6) : Error   : Data type can't be 'ref'\n"
	                   "System function (1, 4) : Error   : Object handle is not supported for this type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	engine->Release();

	// TODO:
	// Types that registers constructors/factories, must also register the default constructor/factory (unless asOBJ_POD is used)

	// TODO:
	// What about asOBJ_NOHANDLE and asEP_ALLOW_UNSAFE_REFERENCES? Should it allow &inout?

	// TODO:
    // Validate if the same behaviour is registered twice, e.g. if index
    // behaviour is registered twice with signature 'int f(int)' and error should be given

	// Success
 	return fail;
}

} // namespace

