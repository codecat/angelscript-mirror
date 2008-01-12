#include "utils.h"

namespace TestImplicitCast
{

#define TESTNAME "TestImplicitCast"


bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	engine->Release();

	// Two types, value cast, and ref cast
	// A value cast actually constructs a new object
	// A ref cast will only reinterpret a handle, without actually constructing any object
	// Should be possible to tell AngelScript if it may use the behaviour implicitly or not
	// Since care must be taken with implicit casts, it is not allowed by default,
	// i.e. asBEHAVE_VALUE_CAST and asBEHAVE_VALUE_CAST_IMPLICIT or
	//      asBEHAVE_REF_CAST and asBEHAVE_REF_CAST_IMPLICIT
	//
	// Type constructors should be made explicit cast only, or perhaps not permit casts at all

	// ---

	// Cast from object to primitive is an object behaviour
	// Cast from primitive to object is an object constructor/factory
	// Cast from object to object can be either object behaviour or object constructor/factory, 
	// depending on which object registers the cast

	// Can't register casts from primitive to primitive

	// It shall be possible to register a cast behaviour that permits the implicit cast of an 
	// object to a primitive type

	// It shall not be possible to register a cast behaviour from an object to a boolean type

	// If an object has a cast to more than one matching primitive type, the cast to the 
	// closest matching type will be used, i.e. Obj has cast to int and to float. A type of 
	// int8 is requested, so the cast to int is used

	// It shall be possible to register cast operators as explicit casts. The constructor/factory 
	// is by default an explicit cast, but shall be possible to register as implicit cast.

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

