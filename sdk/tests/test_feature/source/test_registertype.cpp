#include "utils.h"

namespace TestRegisterType
{

void DummyFunc(asIScriptGeneric *) {}

bool TestRefScoped();

bool TestHelper();

bool Test()
{
	bool fail = TestHelper();
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
		               " (0, 0) : Info    : A garbage collected type must have the addref, release, and all gc behaviours\n"
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
		               " (0, 0) : Info    : A reference type must have the addref and release behaviours\n"
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
		               " (0, 0) : Info    : A non-pod value type must have the constructor and destructor behaviours\n"
		               " (0, 0) : Error   : Type 'val1' is missing behaviours\n"
					   " (0, 0) : Info    : A non-pod value type must have the constructor and destructor behaviours\n"
					   " (0, 0) : Error   : Type 'val2' is missing behaviours\n"
					   " (0, 0) : Info    : A non-pod value type must have the constructor and destructor behaviours\n"
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
		               " (0, 0) : Info    : A reference type must have the addref and release behaviours\n"
		               " (0, 0) : Error   : Type 'ref1' is missing behaviours\n"
					   " (0, 0) : Info    : A reference type must have the addref and release behaviours\n"
					   " (0, 0) : Error   : Type 'ref2' is missing behaviours\n"
					   " (0, 0) : Info    : A reference type must have the addref and release behaviours\n"
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
					   "script (1, 23) : Error   : Data type can't be 'ref'\n"
					   "script (1, 34) : Error   : No matching signatures to 'ref()'\n"
					   "script (1, 34) : Error   : Can't implicitly convert from 'const int' to 'ref'.\n"
					   "script (1, 34) : Error   : No default constructor for object of type 'ref'.\n"
					   "script (1, 34) : Error   : There is no copy operator for this type available.\n" )
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

	// It should be allowed to register the type without specifying the application type,
	// if the engine won't use it (i.e. no native functions take or return the type by value)
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		r = engine->RegisterObjectType("test1", 4, asOBJ_VALUE | asOBJ_POD);
		if( r < 0 ) fail = true;
		r = engine->RegisterGlobalFunction("test1 f()", asFUNCTION(0), asCALL_CDECL); 
		if( r < 0 ) fail = true;
		r = engine->RegisterGlobalFunction("void f(test1)", asFUNCTION(0), asCALL_CDECL);
		if( r < 0 ) fail = true;
		r = engine->ExecuteString(0, "test1 t");
		if( r >= 0 ) fail = true;
		// TODO: These errors should really be returned immediately when registering the function
		if( bout.buffer != " (0, 0) : Info    : test1 f()\n"
			               " (0, 0) : Error   : Can't return type 'test1' by value unless the application type is informed in the registration\n"
                           " (0, 0) : Info    : void f(test1)\n"
		                   " (0, 0) : Error   : Can't pass type 'test1' by value unless the application type is informed in the registration\n"
						   " (0, 0) : Error   : Invalid configuration\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
		engine->Release();
	}

	// It must not be possible to register a value type without defining the application type
	r = engine->RegisterObjectType("test2", 4, asOBJ_VALUE | asOBJ_APP_CLASS_CONSTRUCTOR);
	if( r >= 0 ) fail = true;

	engine->Release();

	// REF+SCOPED
	if( !fail ) fail = TestRefScoped();


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

void Scoped_InRef(int &)
{
}

bool TestRefScoped()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Skipping this due to not supporting native calling conventions
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	bool fail = false;
	int r = 0;
	CBufferedOutStream bout;
 	asIScriptEngine *engine;
	asIScriptModule *mod;

	// REF+SCOPED
	// This type requires a factory and a release behaviour. It cannot have the addref behaviour.
	// The intention of this type is to permit value types that have special needs for memory management,
	// for example must be aligned on 16 byte boundaries, or must use a memory pool. The type must not allow
	// object handles (except when returning a new value from registered functions).
	bout.buffer = "";
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterStdString(engine);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("scoped", 0, asOBJ_REF | asOBJ_SCOPED); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_FACTORY, "scoped @f()", asFUNCTION(Scoped_Factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_RELEASE, "void f()", asFUNCTION(Scoped_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("scoped", "scoped @opNeg()", asFUNCTION(Scoped_Negate), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("scoped", "scoped &opAssign(const scoped &in)", asFUNCTION(Scoped_Assignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("scoped", "scoped @opAdd(int)", asFUNCTION(Scoped_Add), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

	// Enumerate the objects behaviours
	asIObjectType *ot = engine->GetObjectTypeById(engine->GetTypeIdByDecl("scoped"));
	if( ot->GetBehaviourCount() != 1 )
		fail = true;
	asEBehaviours beh;
	ot->GetBehaviourByIndex(0, &beh);
	if( beh != asBEHAVE_RELEASE )
		fail = true;

	// Must be possible to determine type id for scoped types with handle
	asIScriptFunction *func = engine->GetFunctionDescriptorById(ot->GetFactoryIdByIndex(0));
	int typeId = func->GetReturnTypeId();
	if( typeId != engine->GetTypeIdByDecl("scoped@") )
		fail = true;

	// Don't permit handles to be taken
	r = engine->ExecuteString(0, "scoped @s = null");
	if( r >= 0 ) fail = true;
	// TODO: The second message is a consequence of the first error, and should ideally not be shown
	if( sizeof(void*) == 4 )
	{
		if( bout.buffer != "ExecuteString (1, 8) : Error   : Object handle is not supported for this type\n"
						   "ExecuteString (1, 13) : Error   : Can't implicitly convert from '<null handle>' to 'scoped&'.\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
	}
	else
	{
		if( bout.buffer != "ExecuteString (1, 8) : Error   : Object handle is not supported for this type\n"
						   "ExecuteString (1, 13) : Error   : Can't implicitly convert from '<null handle>' to 'scoped&'.\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
	}

	// Test a legal actions
	r = engine->ExecuteString(0, "scoped a");
	if( r != asEXECUTION_FINISHED ) fail = true;

	bout.buffer = "";
	r = engine->ExecuteString(0, "scoped s; scoped t = s + 10;");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test a compiler assert failure reported by Jeff Slutter on 2009-04-02
	bout.buffer = "";
	const char *script =
		"SetObjectPosition( GetWorldPositionByName() ); \n";

	r = engine->RegisterGlobalFunction("const scoped @GetWorldPositionByName()", asFUNCTION(Scoped_Factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void SetObjectPosition(scoped &in)", asFUNCTION(Scoped_InRef), asCALL_CDECL); assert( r >= 0 );
	
	r = engine->ExecuteString(0, script);
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// It must be possible to include the scoped type as member in script class
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", "class A { scoped s; }");
	r = mod->Build();
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "A a; scoped s; a.s = s;");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}


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

///////////////////////////////
//

// TODO: This code should eventually be moved to an standard add-on, e.g. ScriptRegistrator

// AngelScript is pretty good at validating what is registered by the application, however,
// there are somethings that AngelScript just can't be aware of. This is an attempt to remedy
// that by adding extra validations through the use of templates.

// Template for getting information on types
template<typename T>
struct CTypeInfo
{
	static const char *GetTypeName()
	{
	    // Unknown type
#ifdef _MSC_VER
        // GNUC won't let us compile at all if this is here
	    int ERROR_UnknownType[-1];
#endif
	    return 0;
    };
	static bool IsReference() { return false; }
};

// Mapping the C++ type 'int' to the AngelScript type 'int'
template<>
struct CTypeInfo<int>
{
	static const char *GetTypeName() { return "int"; }
	static bool IsReference() { return false; }
};

template<>
struct CTypeInfo<int&>
{
	static const char *GetTypeName() { return "int"; }
	static bool IsReference() { return true; }
};

template<>
struct CTypeInfo<int*>
{
	static const char *GetTypeName() { return "int"; }
	static bool IsReference() { return true; }
};

// Mapping the C++ type 'std::string' to the AngelScript type 'string'
template<>
struct CTypeInfo<std::string>
{
	static const char *GetTypeName() { return "string"; }
	static bool IsReference() { return false; }
};

template<>
struct CTypeInfo<std::string&>
{
	static const char *GetTypeName() { return "string"; }
	static bool IsReference() { return true; }
};

template<>
struct CTypeInfo<std::string*>
{
	static const char *GetTypeName() { return "string@"; }
	static bool IsReference() { return false; }
};

template<>
struct CTypeInfo<std::string**>
{
	static const char *GetTypeName() { return "string@"; }
	static bool IsReference() { return true; }
};

template<>
struct CTypeInfo<std::string*&>
{
	static const char *GetTypeName() { return "string@"; }
	static bool IsReference() { return true; }
};

// Template for verifying a parameter
template<typename A1>
struct CParamValidator
{
	static int Validate(asIScriptFunction *descr, int arg)
	{
		asDWORD flags;
		int t1 = descr->GetParamTypeId(arg, &flags);
		int t2 = descr->GetEngine()->GetTypeIdByDecl(CTypeInfo<A1>::GetTypeName());
		if( t1 != t2 )
			return -1;

		// Verify if the type is properly declared as reference / non-reference
		if( flags == asTM_NONE && CTypeInfo<A1>::IsReference() )
			return -1;

		return 0;
	}
};

// Template for registering a function
template<typename A1>
int RegisterGlobalFunction(asIScriptEngine *e, const char *decl, void (*f)(A1), asDWORD callConv)
{
	int r = e->RegisterGlobalFunction(decl, asFUNCTION(f), callConv);
	assert( r >= 0 );

	if( r >= 0 )
	{
		// TODO: Can write messages to the message callback in the engine instead of testing through asserts

		asIScriptFunction *descr = e->GetFunctionDescriptorById(r);

		// Verify the parameter count
		assert( descr->GetParamCount() == 1 );

		// Verify the parameter types
		assert( CParamValidator<A1>::Validate(descr, 0) >= 0 );
	}

	return r;
}

template<typename A1, typename A2>
int RegisterGlobalFunction(asIScriptEngine *e, const char *decl, void (*f)(A1, A2), asDWORD callConv)
{
	int r = e->RegisterGlobalFunction(decl, asFUNCTION(f), callConv);
	assert( r >= 0 );

	if( r >= 0 )
	{
		asIScriptFunction *descr = e->GetFunctionDescriptorById(r);

		// Verify the parameter count
		assert( descr->GetParamCount() == 2 );

		// Verify the parameter types
		assert( CParamValidator<A1>::Validate(descr, 0) >= 0 );
		assert( CParamValidator<A2>::Validate(descr, 1) >= 0 );
	}

	return r;
}

void func1(int) {}
void func2(std::string &) {}
void func3(std::string *) {}
void func4(int *) {}
void func5(int &) {}
void func6(std::string **) {}
void func7(std::string *&) {}
void func8(int, std::string &) {}
void func9(std::string &, int) {}

bool TestHelper()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Skipping this due to not supporting native calling conventions
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	// TODO: Add validation of return type

	RegisterGlobalFunction(engine, "void func1(int)", func1, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func2(string &in)", func2, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func3(string @)", func3, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func4(int &in)", func4, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func5(int &out)", func5, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func6(string @&out)", func6, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func7(string @&out)", func7, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func8(int, string &)", func8, asCALL_CDECL);
	RegisterGlobalFunction(engine, "void func9(string &, int)", func9, asCALL_CDECL);

	engine->Release();

	return fail;
}


} // namespace

