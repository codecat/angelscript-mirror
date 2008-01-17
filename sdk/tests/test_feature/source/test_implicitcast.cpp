#include "utils.h"

namespace TestImplicitCast
{

#define TESTNAME "TestImplicitCast"



void Type_construct0(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*a = 0;
}

void Type_construct1(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*a = *(int*)gen->GetArgPointer(0);;
}

void Type_castInt(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*(int*)gen->GetReturnPointer() = *a;
}

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

	// Two forms of casts: value cast and ref cast
	// A value cast actually constructs a new object
	// A ref cast will only reinterpret a handle, without actually constructing any object

	// Should be possible to tell AngelScript if it may use the behaviour implicitly or not
	// Since care must be taken with implicit casts, it is not allowed by default,
	// i.e. asBEHAVE_VALUE_CAST and asBEHAVE_VALUE_CAST_IMPLICIT or
	//      asBEHAVE_REF_CAST and asBEHAVE_REF_CAST_IMPLICIT

	//----------------------------------------------------------------------------
	// VALUE_CAST

	// TODO: (Test) Cast from primitive to object is an object constructor/factory
	// TODO: (Test) Cast from object to object can be either object behaviour or object constructor/factory, 
	// depending on which object registers the cast

	// TODO: (Test) Can't register casts from primitive to primitive

	// TODO: (Test) It shall not be possible to register a cast behaviour from an object to a boolean type

	// TODO: (Test) If an object has a cast to more than one matching primitive type, the cast to the 
	// closest matching type will be used, i.e. Obj has cast to int and to float. A type of 
	// int8 is requested, so the cast to int is used

	// TODO: (Implement) It shall be possible to register cast operators as explicit casts. The constructor/factory 
	// is by default an explicit cast, but shall be possible to register as implicit cast.

	// TODO: (Test) When compiling operators with non-primitives, the compiler should first look for 
	// compatible registered operator behaviours. If not found, the compiler should see if 
	// there is any cast behaviour that allow conversion of the type to a primitive type.

	// TODO: (Implement) Type constructors should be made explicit cast only, or perhaps not permit casts at all

	// Test 1
	// A class can be implicitly cast to a primitive, if registered the VALUE_CAST behaviour
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->RegisterGlobalFunction("void assert( bool )", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("type", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Type_construct0), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Type_construct1), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_VALUE_CAST, "int f()", asFUNCTION(Type_castInt), asCALL_GENERIC); assert( r >= 0 );

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "type t(5); \n"
		                         "int a = t; \n"             // conversion to primitive in assignment
								 "assert( a == 5 ); \n"
								 "assert( a + t == 10 ); \n" // conversion to primitive with math operation
								 "a -= t; \n"                // conversion to primitive with math operation
								 "assert( a == 0 ); \n"
								 "assert( t == int(5) ); \n" // conversion to primitive with comparison 
								 "type b(t); \n"             // conversion to primitive with parameter
								 "assert( 32 == (1 << t) ); \n"   // conversion to primitive with bitwise operation 
	                             "assert( (int(5) & t) == 5 ); \n" // conversion to primitive with bitwise operation
								 , &ctx);
	if( r != 0 )
	{
		if( r == 3 )
			PrintException(ctx);
		fail = true;
	}
	if( ctx ) ctx->Release();

	// Test 2
	// A class won't be converted to primitive if there is no obvious target type
	// ex: t << 1 - It is not known what type t should be converted to
	// ex: t + t - It is not known what type t should be converted to
	// ex: t < t - It is not known what type t should be converted to
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "type t(5); t << 1; ");
	if( r == 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : Illegal operation on 'type&'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	bout.buffer = "";
	r = engine->ExecuteString(0, "type t(5); t + t; ");
	if( r == 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : No matching operator that takes the types 'type&' and 'type&' found\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	bout.buffer = "";
	r = engine->ExecuteString(0, "type t(5); t < t; ");
	if( r == 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : No matching operator that takes the types 'type&' and 'type&' found\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	engine->Release();

	//-----------------------------------------------------------------
	// TODO: REFERENCE_CAST

	// It must be possible to cast an object handle to another object handle, without 
	// losing the reference to the original object. This is what will allow applications
	// to register inheritance for registered types. This should perhaps be a special 
	// behaviour, e.g. HANDLE_CAST. How to provide a cast from a base class to a derived class?
	// The base class may not know about the derived class, so it must be the derived class that 
	// registers the behaviour. 
	
	// How to provide interface functionalities to registered types? I.e. a class implements 
	// various interfaces, and a handle to one of the interfaces may be converted to a handle
	// of another interface that is implemented by the class.

	// Class A is the base class
	// Class B inherits from class A
	// A handle to B can be implicitly cast to a handle to A via the HANDLE_CAST behaviour
	// A handle to A can not be implicitly cast to a handle to B since it was registered as HANDLE_CAST_EXPLICIT
	// A handle to A that truly points to a B can be explictly cast to a handle to B via the HANDLE_CAST_EXPLICIT behaviour
	// A handle to A that truly points to an A will return a null handle when cast to B (this is detected via the dynamic_cast<> implementation of the behaviour)

	// Success
 	return fail;
}

} // namespace

