/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


#include <assert.h>
#include <new>

#include "as_config.h"
#include "as_anyobject.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

void AnyObjectConstructor(asCObjectType *ot, asCAnyObject *self)
{
	new(self) asCAnyObject(ot);
}

#ifndef AS_MAX_PORTABILITY

void AnyObjectConstructor2(void *ref, int refTypeId, asCObjectType *ot, asCAnyObject *self)
{
	new(self) asCAnyObject(ref,refTypeId,ot);
}

static asCAnyObject &AnyObjectAssignment(asCAnyObject *other, asCAnyObject *self)
{
	return *self = *other;
}

static void AnyObject_Store(void *ref, int refTypeId, asCAnyObject *self)
{
	self->Store(ref, refTypeId);
}

static void AnyObject_Retrieve(void *ref, int refTypeId, asCAnyObject *self)
{
	self->Retrieve(ref, refTypeId);
}

#else

static void AnyObjectConstructor_Generic(asIScriptGeneric *gen)
{
	asCObjectType *ot = *(asCObjectType**)gen->GetArgPointer(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	new(self) asCAnyObject(ot);
}

static void AnyObjectConstructor2_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refType = gen->GetArgTypeId(0);
	asCObjectType *ot = *(asCObjectType**)gen->GetArgPointer(1);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	new(self) asCAnyObject(ref,refType,ot);
}

static void AnyObjectAssignment_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *other = (asCAnyObject*)gen->GetArgObject(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void AnyObject_Store_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refTypeId = gen->GetArgTypeId(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	self->Store(ref, refTypeId);
}

static void AnyObject_Retrieve_Generic(asIScriptGeneric *gen)
{
	void *ref = (void*)gen->GetArgAddress(0);
	int refTypeId = gen->GetArgTypeId(0);
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();

	self->Retrieve(ref, refTypeId);
}

static void AnyObject_AddRef_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	self->AddRef();
}

static void AnyObject_Release_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	self->Release();
}

static void AnyObject_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	*(int*)gen->GetReturnPointer() = self->GetRefCount();
}

static void AnyObject_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	self->SetFlag();
}

static void AnyObject_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	*(bool*)gen->GetReturnPointer() = self->GetFlag();
}

static void AnyObject_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->EnumReferences(engine);
}

static void AnyObject_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCAnyObject *self = (asCAnyObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->ReleaseAllHandles(engine);
}

#endif


void RegisterAnyObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType(ANY_TOKEN, sizeof(asCAnyObject), asOBJ_CLASS_CDA | asOBJ_SCRIPT_ANY | asOBJ_POTENTIAL_CIRCLE | asOBJ_CONTAINS_ANY); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(AnyObjectConstructor, (asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(?&in, int)", asFUNCTIONPR(AnyObjectConstructor2, (void*, int, asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTIONPR(AnyObjectConstructor, (asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(?&in, int64)", asFUNCTIONPR(AnyObjectConstructor2, (void*, int, asCObjectType*, asCAnyObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ADDREF, "void f()", asMETHOD(asCAnyObject,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASE, "void f()", asMETHOD(asCAnyObject,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(AnyObjectAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(ANY_TOKEN, "void store(?&in)", asFUNCTION(AnyObject_Store), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(ANY_TOKEN, "void retrieve(?&out) const", asFUNCTION(AnyObject_Retrieve), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCAnyObject,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCAnyObject,SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCAnyObject,GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCAnyObject,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCAnyObject,ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );
#else
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(AnyObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(?&in, int)", asFUNCTION(AnyObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(AnyObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_CONSTRUCT, "void f(?&in, int64)", asFUNCTION(AnyObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(AnyObject_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(AnyObject_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ASSIGNMENT, "any &f(any&in)", asFUNCTION(AnyObjectAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(ANY_TOKEN, "void store(?&in)", asFUNCTION(AnyObject_Store_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(ANY_TOKEN, "void retrieve(?&out) const", asFUNCTION(AnyObject_Retrieve_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(AnyObject_GetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(AnyObject_SetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(AnyObject_GetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(AnyObject_EnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->anyObjectType, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(AnyObject_ReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif
}

asCAnyObject &asCAnyObject::operator=(asCAnyObject &other)
{
	FreeObject();

	// TODO: Need to know if it is a handle or not
	valueTypeId = other.valueTypeId;
	value = other.value;
	if( valueTypeId && value )
	{
		const asCDataType *valueType = objType->engine->GetDataTypeFromTypeId(valueTypeId);

		asCObjectType *ot = valueType->GetObjectType();
		ot->engine->CallObjectMethod(value, ot->beh.addref);
		ot->refCount++;
	}

	return *this;
}

int asCAnyObject::CopyFrom(asIScriptAny *other)
{
	if( other == 0 ) return asINVALID_ARG;

	*this = *(asCAnyObject*)other;

	return 0;
}

asCAnyObject::asCAnyObject(asCObjectType *ot)
{
	objType = ot;
	refCount = 1;

	valueTypeId = 0;
	value = 0;

	// Notify the garbage collector of this object
	objType->engine->NotifyGarbageCollectorOfNewObject(this, objType->engine->GetTypeIdByDecl(0, objType->name.AddressOf()));		
}

asCAnyObject::asCAnyObject(void *ref, int refTypeId, asCObjectType *ot)
{
	objType = ot;
	refCount = 1;

	valueTypeId = 0;
	value = 0;

	// Notify the garbage collector of this object
	objType->engine->NotifyGarbageCollectorOfNewObject(this, objType->engine->GetTypeIdByDecl(0, objType->name.AddressOf()));		

	Store(ref, refTypeId);
}

asCAnyObject::~asCAnyObject()
{
	FreeObject();
}

void asCAnyObject::Store(void *ref, int refTypeId)
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

		const asCDataType *dt = objType->engine->GetDataTypeFromTypeId(valueTypeId);

		value = *(void**)ref; // We receive a reference to the handle
		asCObjectType *ot = dt->GetObjectType();
		if( ot && value )
		{
			ot->engine->CallObjectMethod(value, ot->beh.addref);
			ot->refCount++;
		}
	}
}

int asCAnyObject::Retrieve(void *ref, int refTypeId)
{
	// Verify if the value is compatible with the requested type
	bool isCompatible = false;
	if( valueTypeId == refTypeId )
		isCompatible = true;
	// Allow obj@ to be copied to const obj@
	else if( ((valueTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) == (refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR))) && // Handle to the same object type
	 	     ((refTypeId & (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) )			   // Handle to const object
		isCompatible = true;

	const asCDataType *refType   = objType->engine->GetDataTypeFromTypeId(refTypeId);
	asCObjectType *ot = refType->GetObjectType();

	// Release the old value held by the reference
	if( *(void**)ref && ot && ot->beh.release )
	{
		ot->engine->CallObjectMethod(*(void**)ref, ot->beh.release);
		*(void**)ref = 0;
	}

	if( isCompatible )
	{
		// Set the reference to the object handle
		*(void**)ref = value;
		if( value )
			ot->engine->CallObjectMethod(value, ot->beh.addref);

		return 0;
	}

	return -1;
}

int asCAnyObject::GetTypeId()
{
	return valueTypeId;
}

void asCAnyObject::FreeObject()
{
	if( value && (valueTypeId & asTYPEID_OBJHANDLE) )
	{
		const asCDataType *valueType = objType->engine->GetDataTypeFromTypeId(valueTypeId);
		asCObjectType *ot = valueType->GetObjectType();

		if( !ot->beh.release )
		{
			if( ot->beh.destruct )
				ot->engine->CallObjectMethod(value, ot->beh.destruct);

			ot->engine->CallFree(ot, value);
		}
		else
		{
			ot->engine->CallObjectMethod(value, ot->beh.release);
		}
		ot->refCount--;
	}

	value = 0;
	valueTypeId = 0;
}

void asCAnyObject::Destruct()
{
	// Call the destructor
	this->~asCAnyObject();

	// Free the memory
	userFree(this);
}

void asCAnyObject::EnumReferences(asIScriptEngine *engine)
{
	// If we're holding a reference, we'll notify the garbage collector of it
	if( value && (valueTypeId & asTYPEID_MASK_OBJECT) )
		engine->GCEnumCallback(value);
}

void asCAnyObject::ReleaseAllHandles(asIScriptEngine *engine)
{
	const asCDataType *valueType = objType->engine->GetDataTypeFromTypeId(valueTypeId);

	if( value && valueType && valueType->GetObjectType()->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		((asCScriptEngine*)engine)->CallObjectMethod(value, valueType->GetBehaviour()->release);
		value = 0;
		valueType->GetObjectType()->refCount--;
	}
}

int asCAnyObject::AddRef()
{
	// Increase counter and clear flag set by GC
	refCount = (refCount & 0x7FFFFFFF) + 1;
	return refCount;
}

int asCAnyObject::Release()
{
	// Now do the actual releasing (clearing the flag set by GC)
	refCount = (refCount & 0x7FFFFFFF) - 1;
	if( refCount == 0 )
	{
		Destruct();
		return 0;
	}

	return refCount;
}

int asCAnyObject::GetRefCount()
{
	return refCount & 0x7FFFFFFF;
}

void asCAnyObject::SetFlag()
{
	refCount |= 0x80000000;
}

bool asCAnyObject::GetFlag()
{
	return (refCount & 0x80000000) ? true : false;
}


END_AS_NAMESPACE
