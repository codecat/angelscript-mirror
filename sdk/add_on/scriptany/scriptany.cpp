#include "scriptany.h"
#include <new>
#include <assert.h>
#include <string.h>

BEGIN_AS_NAMESPACE

// We'll use the generic interface for the factories as we need the engine pointer
static void ScriptAnyFactory_Generic(asIScriptGeneric *gen)
{
	asIScriptEngine *engine = gen->GetEngine();

	*(CScriptAny**)gen->GetReturnPointer() = new CScriptAny(engine);
}

static void ScriptAnyFactory2_Generic(asIScriptGeneric *gen)
{
	asIScriptEngine *engine = gen->GetEngine();
	void *ref = (void*)gen->GetArgAddress(0);
	int refType = gen->GetArgTypeId(0);

	*(CScriptAny**)gen->GetReturnPointer() = new CScriptAny(ref,refType,engine);
}

static CScriptAny &ScriptAnyAssignment(CScriptAny *other, CScriptAny *self)
{
	return *self = *other;
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

static void ScriptAny_StoreInt_Generic(asIScriptGeneric *gen)
{
	asINT64 *ref = (asINT64*)gen->GetArgAddress(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	self->Store(*ref);
}

static void ScriptAny_StoreFlt_Generic(asIScriptGeneric *gen)
{
	double *ref = (double*)gen->GetArgAddress(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	self->Store(*ref);
}

static void ScriptAny_Retrieve_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refTypeId = gen->GetArgTypeId(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	*(bool*)gen->GetReturnPointer() = self->Retrieve(ref, refTypeId);
}

static void ScriptAny_RetrieveInt_Generic(asIScriptGeneric *gen)
{
	asINT64 *ref = (asINT64*)gen->GetArgAddress(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	*(bool*)gen->GetReturnPointer() = self->Retrieve(*ref);
}

static void ScriptAny_RetrieveFlt_Generic(asIScriptGeneric *gen)
{
	double *ref = (double*)gen->GetArgAddress(0);
	CScriptAny *self = (CScriptAny*)gen->GetObject();

	*(bool*)gen->GetReturnPointer() = self->Retrieve(*ref);
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

void RegisterScriptAny_Native(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("any", sizeof(CScriptAny), asOBJ_REF | asOBJ_GC); assert( r >= 0 );

	// We'll use the generic interface for the constructor as we need the engine pointer
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f()", asFUNCTION(ScriptAnyFactory_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(?&in)", asFUNCTION(ScriptAnyFactory2_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptAny,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptAny,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(ScriptAnyAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(?&in)", asMETHODPR(CScriptAny,Store,(void*,int),void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(int64&in)", asMETHODPR(CScriptAny,Store,(asINT64&),void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(double&in)", asMETHODPR(CScriptAny,Store,(double&),void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(?&out)", asMETHODPR(CScriptAny,Retrieve,(void*,int),bool), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(int64&out)", asMETHODPR(CScriptAny,Retrieve,(asINT64&),bool), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(double&out)", asMETHODPR(CScriptAny,Retrieve,(double&),bool), asCALL_THISCALL); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CScriptAny,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CScriptAny,SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CScriptAny,GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CScriptAny,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CScriptAny,ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );
}

void RegisterScriptAny_Generic(asIScriptEngine *engine)
{
	int r;
	r = engine->RegisterObjectType("any", sizeof(CScriptAny), asOBJ_REF | asOBJ_GC); assert( r >= 0 );

	// We'll use the generic interface for the constructor as we need the engine pointer
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f()", asFUNCTION(ScriptAnyFactory_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_FACTORY, "any@ f(?&in)", asFUNCTION(ScriptAnyFactory2_Generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptAny_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptAny_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(ScriptAnyAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(?&in)", asFUNCTION(ScriptAny_Store_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(int64&in)", asFUNCTION(ScriptAny_StoreInt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "void store(double&in)", asFUNCTION(ScriptAny_StoreFlt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(?&out) const", asFUNCTION(ScriptAny_Retrieve_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(int64&out) const", asFUNCTION(ScriptAny_RetrieveInt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("any", "bool retrieve(double&out) const", asFUNCTION(ScriptAny_RetrieveFlt_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptAny_GetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptAny_SetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptAny_GetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptAny_EnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("any", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptAny_ReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );
}


CScriptAny &CScriptAny::operator=(CScriptAny &other)
{
	FreeObject();

	value.typeId = other.value.typeId;
	if( value.typeId & asTYPEID_OBJHANDLE )
	{
		// For handles, copy the pointer and increment the reference count
		value.valueObj = other.value.valueObj;
		engine->AddRefScriptObject(value.valueObj, value.typeId);
	}
	else if( value.typeId & asTYPEID_MASK_OBJECT )
	{
		// Create a copy of the object
		value.valueObj = engine->CreateScriptObjectCopy(other.value.valueObj, value.typeId);
	}
	else
	{
		// Primitives can be copied directly
		value.valueInt = other.value.valueInt;
	}

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

	value.typeId = 0;
	value.valueInt = 0;

	// Notify the garbage collector of this object
	engine->NotifyGarbageCollectorOfNewObject(this, engine->GetTypeIdByDecl(0, "any"));		
}

CScriptAny::CScriptAny(void *ref, int refTypeId, asIScriptEngine *engine)
{
	this->engine = engine;
	refCount = 1;

	value.typeId = 0;
	value.valueInt = 0;

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

	value.typeId = refTypeId;
	if( value.typeId & asTYPEID_OBJHANDLE )
	{
		// We're receiving a reference to the handle, so we need to dereference it
		value.valueObj = *(void**)ref;
		engine->AddRefScriptObject(value.valueObj, value.typeId);
	}
	else if( value.typeId & asTYPEID_MASK_OBJECT )
	{
		// Create a copy of the object
		// We need to dereference the reference, as we receive a pointer to a pointer to the object
		value.valueObj = engine->CreateScriptObjectCopy(*(void**)ref, value.typeId);
	}
	else
	{
		// Primitives can be copied directly
		value.valueInt = 0;

		// Copy the primitive value
		// We receive a pointer to the value.
		int size = engine->GetSizeOfPrimitiveType(value.typeId);
		memcpy(&value.valueInt, ref, size);
	}
}

void CScriptAny::Store(double &ref)
{
	int typeId = engine->GetTypeIdByDecl(0, "double");
	Store(&ref, typeId);
}

void CScriptAny::Store(asINT64 &ref)
{
	int typeId = engine->GetTypeIdByDecl(0, "int64");
	Store(&ref, typeId);
}


bool CScriptAny::Retrieve(void *ref, int refTypeId)
{
	if( refTypeId & asTYPEID_OBJHANDLE )
	{
		// Is the handle type compatible with the stored value?

		// A handle can be retrieved if the stored type is a handle of same or compatible type
		// or if the stored type is an object that implements the interface that the handle refer to.
		if( (value.typeId & asTYPEID_MASK_OBJECT) && 
			engine->IsHandleCompatibleWithObject(value.valueObj, value.typeId, refTypeId) )
		{
			engine->AddRefScriptObject(value.valueObj, value.typeId);
			*(void**)ref = value.valueObj;

			return true;
		}
	}
	else if( refTypeId & asTYPEID_MASK_OBJECT )
	{
		// Is the object type compatible with the stored value?

		// Copy the object into the given reference
		if( value.typeId == refTypeId )
		{
			engine->CopyScriptObject(*(void**)ref, value.valueObj, value.typeId);

			return true;
		}
	}
	else
	{
		// Is the primitive type compatible with the stored value?

		if( value.typeId == refTypeId )
		{
			int size = engine->GetSizeOfPrimitiveType(refTypeId);
			memcpy(ref, &value.valueInt, size);
			return true;
		}

		// We know all numbers are stored as either int64 or double, since we register overloaded functions for those
		int intTypeId = engine->GetTypeIdByDecl(0, "int64");
		int fltTypeId = engine->GetTypeIdByDecl(0, "double");
		if( value.typeId == intTypeId && refTypeId == fltTypeId )
		{
			*(double*)ref = double(value.valueInt);
			return true;
		}
		else if( value.typeId == fltTypeId && refTypeId == intTypeId )
		{
			*(asINT64*)ref = asINT64(value.valueFlt);
			return true;
		}
	}

	return false;
}

bool CScriptAny::Retrieve(asINT64 &value)
{
	int typeId = engine->GetTypeIdByDecl(0, "int64");
	return Retrieve(&value, typeId);
}

bool CScriptAny::Retrieve(double &value)
{
	int typeId = engine->GetTypeIdByDecl(0, "double");
	return Retrieve(&value, typeId);
}

int CScriptAny::GetTypeId()
{
	return value.typeId;
}

void CScriptAny::FreeObject()
{
    // If it is a handle or a ref counted object, call release
	if( value.typeId & asTYPEID_MASK_OBJECT )
	{
		// Let the engine release the object
		engine->ReleaseScriptObject(value.valueObj, value.typeId);
		value.valueObj = 0;
		value.typeId = 0;
	}

    // For primitives, there's nothing to do
}


void CScriptAny::EnumReferences(asIScriptEngine *engine)
{
	// If we're holding a reference, we'll notify the garbage collector of it
	if( value.valueObj && (value.typeId & asTYPEID_MASK_OBJECT) )
		engine->GCEnumCallback(value.valueObj);
}

void CScriptAny::ReleaseAllHandles(asIScriptEngine * /*engine*/)
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
		delete this;
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
