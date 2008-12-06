#include "utils.h"

namespace TestRegisterType
{

#define TESTNAME "TestRegisterType"

void DummyFunc(asIScriptGeneric *) {}

bool TestRefScoped();

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
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
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
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
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
	r = engine->RegisterObjectType("val", 4, asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectType("val1", 4, asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("val1", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(DummyFunc), asCALL_GENERIC);
	r = engine->RegisterObjectType("val2", 4, asOBJ_VALUE | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
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
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
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
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
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

	// Must be possible to register float types
	r = engine->RegisterObjectType("real", sizeof(float), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_FLOAT); assert( r >= 0 );

	// It must not be possible to register a value type without defining the application type
	r = engine->RegisterObjectType("test1", 4, asOBJ_VALUE);
	if( r >= 0 ) fail = true;
	r = engine->RegisterObjectType("test2", 4, asOBJ_VALUE | asOBJ_APP_CLASS_CONSTRUCTOR);
	if( r >= 0 ) fail = true;

	engine->Release();

	// REF+SCOPED
	if( !fail ) fail = TestRefScoped();

	// TODO: It should be allowed to register the type without specifying the application type, 
	// if the engine won't use it (i.e. no native functions take or return the type by value)

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

//////////////////////////////////////

int *Scoped_Factory()
{
	return new int(42);
}

void Scoped_Release(int *p)
{
	if( p ) delete p;
}

int *Scoped_Negate(int *p)
{
	if( p ) 
		return new int(-*p);
	return 0;
}

int &Scoped_Assignment(int &a, int *p)
{
	*p = a;
	return *p;
}

int *Scoped_Add(int &a, int b)
{
	return new int(a + b);
}

bool TestRefScoped()
{
	bool fail = false;
	int r = 0;
	CBufferedOutStream bout;
 	asIScriptEngine *engine;

	// REF+SCOPED
	// This type requires a factory and a release behaviour. It cannot have the addref behaviour.
	// The intention of this type is to permit value types that have special needs for memory management,
	// for example must be aligned on 16 byte boundaries, or must use a memory pool. The type must not allow
	// object handles (except when returning a new value from registered functions).
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("scoped", 0, asOBJ_REF | asOBJ_SCOPED); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_FACTORY, "scoped @f()", asFUNCTION(Scoped_Factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_RELEASE, "void f()", asFUNCTION(Scoped_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_NEGATE, "scoped @f()", asFUNCTION(Scoped_Negate), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_ASSIGNMENT, "scoped &f(const scoped &in)", asFUNCTION(Scoped_Assignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "scoped @f(const scoped &in, int)", asFUNCTION(Scoped_Add), asCALL_CDECL); assert( r >= 0 );

	// Don't permit handles to be taken
	r = engine->ExecuteString(0, "scoped @s = null");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 8) : Error   : Object handle is not supported for this type\n"
		               "ExecuteString (1, 13) : Error   : Can't implicitly convert from 'const int@const' to 'scoped&'.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test a legal actions
	r = engine->ExecuteString(0, "scoped a");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "scoped s; scoped t = s + 10;");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Don't permit functions to be registered with handle for parameters
	bout.buffer = "";
	r = engine->RegisterGlobalFunction("void f(scoped@)", asFUNCTION(DummyFunc), asCALL_GENERIC);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "System function (1, 14) : Error   : Object handle is not supported for this type\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Don't permit functions to be registered to take type by reference (since that require handles)
	bout.buffer = "";
	r = engine->RegisterGlobalFunction("void f(scoped&)", asFUNCTION(DummyFunc), asCALL_GENERIC);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "System function (1, 14) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Permit &in
	r = engine->RegisterGlobalFunction("void f(scoped&in)", asFUNCTION(DummyFunc), asCALL_GENERIC);
	if( r < 0 ) fail = true;



	engine->Release();

	return fail;
}

} // namespace

