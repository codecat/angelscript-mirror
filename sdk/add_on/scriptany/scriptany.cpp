#include "scriptany.h"
#include <new>
#include <assert.h>
#include <string.h>

BEGIN_AS_NAMESPACE

// We'll use the generic interface for the constructors as we need the engine pointer
static void ScriptAnyConstructor_Generic(asIScriptGeneric *gen)
{
	asIScriptEngine *engine = gen->GetEngine();
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	new(self) CScriptAny(engine);
}

static void ScriptAnyConstructor2_Generic(asIScriptGeneric *gen)
{
	asIScriptEngine *engine = gen->GetEngine();
	void *ref = (void*)gen->GetArgAddress(0);
	int refType = gen->GetArgTypeId(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	new(self) CScriptAny(ref,refType,engine);
}

static CScriptAny &ScriptAnyAssignment(CScriptAny *other, CScriptAny *self)
{
	return *self = *other;
}

static void ScriptAny_Store(void *ref, int refTypeId, CScriptAny *self)
{
	self->Store(ref, refTypeId);
}

static void ScriptAny_Retrieve(void *ref, int refTypeId, CScriptAny *self)
{
	self->Retrieve(ref, refTypeId);
}

static void ScriptAnyAssignment_Generic(asIScriptGeneric *gen)
{
	CScriptAny *other = (CScriptAny*)gen->GetArgObject(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void ScriptAny_Store_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refTypeId = gen->GetArgTypeId(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	self->Store(ref, refTypeId);
}

static void ScriptAny_Retrieve_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refTypeId = gen->GetArgTypeId(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	self->Retrieve(ref, refTypeId);
}

static void ScriptAny_AddRef_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	self->AddRef();
}

static void ScriptAny_Release_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	self->Release();
}

static void ScriptAny_GetRefCount_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	*(int*)gen->GetReturnPointer() = self->GetRefCount();
}

static void ScriptAny_SetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	self->SetFlag();
}

static void ScriptAny_GetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	*(bool*)gen->GetReturnPointer() = self->GetFlag();
}

static void ScriptAny_EnumReferences_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->EnumReferences(engine);
}

static void ScriptAny_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	CScriptAny *self = (CScriptAny*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->ReleaseAllHandles(engine);
}

void RegisterScriptAny(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptAny_Generic(engine);
	else
		RegisterScriptAny_Native(engine);
}

static void *AnyAlloc(int)
{
	return new char[sizeof(CScriptAny)];
}

static void AnyFree(void *p)
{
	assert( p );
	delete[] (char*)p;
}

void RegisterScriptAny_Native(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("any", sizeof(CScriptAny), asOBJ_CLASS_CDA); assert( r >= 0 );

	// We'll use the generic interface for the constructor as we need the engine pointer
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ScriptAnyConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_CONSTRUCT, "void f(?&in)", asFUNCTION(ScriptAnyConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptAny,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptAny,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(ScriptAnyAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(?&in)", asFUNCTION(ScriptAny_Store), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void retrieve(?&out) const", asFUNCTION(ScriptAny_Retrieve), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CScriptAny,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CScriptAny,SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CScriptAny,GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CScriptAny,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CScriptAny,ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ALLOC, "any &f(uint)", asFUNCTION(AnyAlloc), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FREE, "void f(any &in)", asFUNCTION(AnyFree), asCALL_CDECL); assert( r >= 0 );
}

void RegisterScriptAny_Generic(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("any", sizeof(CScriptAny), asOBJ_CLASS_CDA); assert( r >= 0 );

	// We'll use the generic interface for the constructor as we need the engine pointer
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ScriptAnyConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_CONSTRUCT, "void f(?&in)", asFUNCTION(ScriptAnyConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptAny_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptAny_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(ScriptAnyAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(?&in)", asFUNCTION(ScriptAny_Store_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void retrieve(?&out) const", asFUNCTION(ScriptAny_Retrieve_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptAny_GetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptAny_SetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptAny_GetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptAny_EnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptAny_ReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ALLOC, "any &f(uint)", asFUNCTION(AnyAlloc), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FREE, "void f(any &in)", asFUNCTION(AnyFree), asCALL_CDECL); assert( r >= 0 );
}


CScriptAny &CScriptAny::operator=(CScriptAny &other)
{
	FreeObject();

	// TODO: Need to know if it is a handle or not
	valueTypeId = other.valueTypeId;
	value = other.value;
	if( valueTypeId && value )
		engine->AddRefScriptObject(value, valueTypeId);

	return *this;
}

int CScriptAny::CopyFrom(CScriptAny *other)
{
	if( other == 0 ) return asINVALID_ARG;

	*this = *(CScriptAny*)other;

	return 0;
}

CScriptAny::CScriptAny(asIScriptEngine *engine)
{
	this->engine = engine;
	refCount = 1;

	valueTypeId = 0;
	value = 0;

	// Notify the garbage collector of this object
	engine->NotifyGarbageCollectorOfNewObject(this, engine->GetTypeIdByDecl(0, "any"));		
}

CScriptAny::CScriptAny(void *ref, int refTypeId, asIScriptEngine *engine)
{
	this->engine = engine;
	refCount = 1;

	valueTypeId = 0;
	value = 0;

	// Notify the garbage collector of this object
	engine->NotifyGarbageCollectorOfNewObject(this, engine->GetTypeIdByDecl(0, "any"));		

	Store(ref, refTypeId);
}

CScriptAny::~CScriptAny()
{
	FreeObject();
}

void CScriptAny::Store(void *ref, int refTypeId)
{
	FreeObject();

	// TODO: Improve this to support non-handles and primitives
	// Only accept null and object handles
	if( refTypeId && (refTypeId & asTYPEID_OBJHANDLE) == 0 )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Store received a type that is not an object handle");
	}
	else
	{
		valueTypeId = refTypeId;
		value = *(void**)ref; // We receive a reference to the handle

		if( valueTypeId && value )
			engine->AddRefScriptObject(value, valueTypeId);
	}
}

int CScriptAny::Retrieve(void *ref, int refTypeId)
{
	// Verify if the value is compatible with the requested type
	bool isCompatible = false;
	if( valueTypeId == refTypeId )
		isCompatible = true;
	// Allow obj@ to be copied to const obj@
	else if( ((valueTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) == (refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR))) && // Handle to the same object type
	 	     ((refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) )			   // Handle to const object
		isCompatible = true;

	// Release the old value held by the reference
	if( *(void**)ref && refTypeId )
	{
		engine->ReleaseScriptObject(*(void**)ref, refTypeId);
		*(void**)ref = 0;
	}

	if( isCompatible )
	{
		// Set the reference to the object handle
		*(void**)ref = value;
		if( value )
			engine->AddRefScriptObject(value, valueTypeId);

		return 0;
	}

	return -1;
}

int CScriptAny::GetTypeId()
{
	return valueTypeId;
}

void CScriptAny::FreeObject()
{
	if( value && (valueTypeId & asTYPEID_OBJHANDLE) )
		engine->ReleaseScriptObject(value, valueTypeId);

	value = 0;
	valueTypeId = 0;
}


void CScriptAny::EnumReferences(asIScriptEngine *engine)
{
	// If we're holding a reference, we'll notify the garbage collector of it
	if( value && (valueTypeId & asTYPEID_MASK_OBJECT) )
		engine->GCEnumCallback(value);
}

void CScriptAny::ReleaseAllHandles(asIScriptEngine *engine)
{
	FreeObject();
}

int CScriptAny::AddRef()
{
	// Increase counter and clear flag set by GC
	refCount = (refCount & 0x7FFFFFFF) + 1;
	return refCount;
}

int CScriptAny::Release()
{
	// Now do the actual releasing (clearing the flag set by GC)
	refCount = (refCount & 0x7FFFFFFF) - 1;
	if( refCount == 0 )
	{
		this->~CScriptAny();
		AnyFree(this);
		return 0;
	}

	return refCount;
}

int CScriptAny::GetRefCount()
{
	return refCount & 0x7FFFFFFF;
}

void CScriptAny::SetFlag()
{
	refCount |= 0x80000000;
}

bool CScriptAny::GetFlag()
{
	return (refCount & 0x80000000) ? true : false;
}


END_AS_NAMESPACE
