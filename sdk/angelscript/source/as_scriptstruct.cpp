/*
   AngelCode Scripting Library
   Copyright (c) 2003-2008 Andreas Jonsson

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


#include <new>

#include "as_config.h"

#include "as_scriptengine.h"

#include "as_scriptstruct.h"
#include "as_arrayobject.h"

BEGIN_AS_NAMESPACE

// This helper function will call the default constructor, that is a script function
asIScriptStruct *ScriptStructFactory(asCObjectType *objType, asCScriptEngine *engine)
{
	asIScriptStruct *ptr = (asIScriptStruct*)engine->CallAlloc(objType);

	int funcIndex = objType->beh.construct;
	
	// Setup a context for calling the default constructor
	asIScriptContext *ctx;
	int r = engine->CreateContext(&ctx, true);
	if( r < 0 )
	{
		engine->CallFree(ptr);
		return 0;
	}
	r = ctx->Prepare(funcIndex);
	if( r < 0 )
	{
		engine->CallFree(ptr);
		ctx->Release();
		return 0;
	}
	ctx->SetObject(ptr);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		// TODO: Verify that the memory for the structure have been released already
		ctx->Release();
		return 0;
	}
	ctx->Release();	
	
	return ptr;
}

#ifdef AS_MAX_PORTABILITY

static void ScriptStruct_AddRef_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	self->AddRef();
}

static void ScriptStruct_Release_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	self->Release();
}

static void ScriptStruct_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	*(int*)gen->GetReturnPointer() = self->GetRefCount();
}

static void ScriptStruct_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	self->SetFlag();
}

static void ScriptStruct_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	*(bool*)gen->GetReturnPointer() = self->GetFlag();
}

static void ScriptStruct_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->EnumReferences(engine);
}

static void ScriptStruct_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->ReleaseAllHandles(engine);
}

#endif

void RegisterScriptStruct(asCScriptEngine *engine)
{
	// Register the default script structure behaviours
	int r;
	engine->scriptTypeBehaviours.flags = asOBJ_SCRIPT_STRUCT;
#ifndef AS_MAX_PORTABILITY
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ScriptStruct_Construct), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(ScriptStruct_Construct), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asMETHOD(asCScriptStruct,AddRef), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asMETHOD(asCScriptStruct,Release), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ASSIGNMENT, "int &f(int &in)", asFUNCTION(ScriptStruct_Assignment), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCScriptStruct,GetRefCount), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCScriptStruct,SetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCScriptStruct,GetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCScriptStruct,EnumReferences), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCScriptStruct,ReleaseAllHandles), asCALL_THISCALL); asASSERT( r >= 0 );
#else
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ScriptStruct_Construct_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(ScriptStruct_Construct_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptStruct_AddRef_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptStruct_Release_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ASSIGNMENT, "int &f(int &in)", asFUNCTION(ScriptStruct_Assignment_Generic), asCALL_GENERIC); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptStruct_GetRefCount_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptStruct_SetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptStruct_GetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptStruct_EnumReferences_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptStruct_ReleaseAllHandles_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif
}

void ScriptStruct_Construct_Generic(asIScriptGeneric *gen)
{
	asCObjectType *objType = *(asCObjectType**)gen->GetAddressOfArg(0);
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();

	ScriptStruct_Construct(objType, self);
}

void ScriptStruct_Construct(asCObjectType *objType, asCScriptStruct *self)
{
	new(self) asCScriptStruct(objType);
}

asCScriptStruct::asCScriptStruct(asCObjectType *ot)
{
	refCount.set(1);
	objType          = ot;
	objType->AddRef();
	isDestructCalled = false;

	// Notify the garbage collector of this object
	if( objType->flags & asOBJ_GC )
		objType->engine->gc.AddScriptObjectToGC(this, objType);

	// Construct all properties
	asCScriptEngine *engine = objType->engine;
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			size_t *ptr = (size_t*)(((char*)this) + prop->byteOffset);

			if( prop->type.IsObjectHandle() )
				*ptr = 0;
			else
			{
				// Allocate the object and call it's constructor
				*ptr = (size_t)AllocateObject(prop->type.GetObjectType(), engine);
			}
		}
	}
}

void asCScriptStruct::Destruct()
{
	// Call the destructor, which will also call the GCObject's destructor
	this->~asCScriptStruct();

	// Free the memory
	userFree(this);
}

asCScriptStruct::~asCScriptStruct()
{
	objType->Release();

	// The engine pointer should be available from the objectType
	asCScriptEngine *engine = objType->engine;

	// Destroy all properties
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			// Destroy the object
			void **ptr = (void**)(((char*)this) + prop->byteOffset);
			if( *ptr )
			{
				FreeObject(*ptr, prop->type.GetObjectType(), engine);
				*(asDWORD*)ptr = 0;
			}
		}
	}
}

asIScriptEngine *asCScriptStruct::GetEngine() const
{
	return objType->engine;
}

int asCScriptStruct::AddRef()
{
	// Increase counter and clear flag set by GC
	gcFlag = false;
	return refCount.atomicInc();
}

int asCScriptStruct::Release()
{
	// Clear the flag set by the GC
	gcFlag = false;

	// Call the script destructor behaviour if the reference counter is 1.
	if( refCount.get() == 1 && !isDestructCalled )
	{
		// Make sure the destructor is called once only, even if the  
		// reference count is increased and then decreased again
		isDestructCalled = true;

		// Call the destructor
		int funcIndex = objType->beh.destruct;
		if( funcIndex )
		{
			// Setup a context for calling the default constructor
			asIScriptContext *ctx;
			asCScriptEngine *engine = objType->engine;
			int r = engine->CreateContext(&ctx, true);
			if( r >= 0 )
				r = ctx->Prepare(funcIndex);
			if( r >= 0 )
			{
				ctx->SetObject(this);
				ctx->Execute();

				// There's not much to do if the execution doesn't finish, so we just ignore the result
			}
			ctx->Release();	
		}
	}

	// Now do the actual releasing (clearing the flag set by GC)
	int r = refCount.atomicDec();
	if( r == 0 )
	{
		Destruct();
		return 0;
	}

	return r;
}

asIObjectType *asCScriptStruct::GetObjectType()
{
	return objType;
}

int asCScriptStruct::GetRefCount()
{
	return refCount.get();
}

void asCScriptStruct::SetFlag()
{
	gcFlag = true;
}

bool asCScriptStruct::GetFlag()
{
	return gcFlag;
}

int asCScriptStruct::GetStructTypeId()
{
	asCDataType dt = asCDataType::CreateObject(objType, false);
	return objType->engine->GetTypeIdFromDataType(dt);
}

int asCScriptStruct::GetPropertyCount()
{
	return (int)objType->properties.GetLength();
}

int asCScriptStruct::GetPropertyTypeId(asUINT prop)
{
	if( prop >= objType->properties.GetLength() )
		return asINVALID_ARG;

	return objType->engine->GetTypeIdFromDataType(objType->properties[prop]->type);
}

const char *asCScriptStruct::GetPropertyName(asUINT prop)
{
	if( prop >= objType->properties.GetLength() )
		return 0;

	return objType->properties[prop]->name.AddressOf();
}

void *asCScriptStruct::GetPropertyPointer(asUINT prop)
{
	if( prop >= objType->properties.GetLength() )
		return 0;

	// Objects are stored by reference, so this must be dereferenced
	asCDataType *dt = &objType->properties[prop]->type;
	if( dt->IsObject() && !dt->IsObjectHandle() )
		return *(void**)(((char*)this) + objType->properties[prop]->byteOffset);

	return (void*)(((char*)this) + objType->properties[prop]->byteOffset);
}

void asCScriptStruct::EnumReferences(asIScriptEngine *engine)
{
	// We'll notify the GC of all object handles that we're holding
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			void *ptr = *(void**)(((char*)this) + prop->byteOffset);
			if( ptr )
				((asCScriptEngine*)engine)->GCEnumCallback(ptr);
		}
	}
}

void asCScriptStruct::ReleaseAllHandles(asIScriptEngine *engine)
{
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = objType->properties[n];
		if( prop->type.IsObject() && prop->type.IsObjectHandle() )
		{
			void **ptr = (void**)(((char*)this) + prop->byteOffset);
			if( *ptr )
			{
				((asCScriptEngine*)engine)->CallObjectMethod(*ptr, prop->type.GetBehaviour()->release);
				*ptr = 0;
			}
		}
	}
}

void ScriptStruct_Assignment_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *other = *(asCScriptStruct**)gen->GetAddressOfArg(0);
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();

	*self = *other;

	*(asCScriptStruct**)gen->GetReturnPointer() = self;
}

asCScriptStruct &ScriptStruct_Assignment(asCScriptStruct *other, asCScriptStruct *self)
{
	return (*self = *other);
}

asCScriptStruct &asCScriptStruct::operator=(const asCScriptStruct &other)
{
	asASSERT( objType == other.objType );

	asCScriptEngine *engine = objType->engine;

	// Copy all properties
	for( asUINT n = 0; n < objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = objType->properties[n];
		if( prop->type.IsObject() )
		{
			void **dst = (void**)(((char*)this) + prop->byteOffset);
			void **src = (void**)(((char*)&other) + prop->byteOffset);
			if( !prop->type.IsObjectHandle() )
				CopyObject(*src, *dst, prop->type.GetObjectType(), engine);
			else
				CopyHandle((asDWORD*)src, (asDWORD*)dst, prop->type.GetObjectType(), engine);
		}
		else
		{
			void *dst = ((char*)this) + prop->byteOffset;
			void *src = ((char*)&other) + prop->byteOffset;
			memcpy(dst, src, prop->type.GetSizeInMemoryBytes());
		}
	}

	return *this;
}

int asCScriptStruct::CopyFrom(asIScriptStruct *other)
{
	if( other == 0 ) return asINVALID_ARG;

	if( GetStructTypeId() != other->GetStructTypeId() )
		return asINVALID_TYPE;

	*this = *(asCScriptStruct*)other;

	return 0;
}

void *asCScriptStruct::AllocateObject(asCObjectType *objType, asCScriptEngine *engine)
{
	void *ptr = 0;

	if( objType->flags & asOBJ_SCRIPT_STRUCT )
	{
		ptr = ScriptStructFactory(objType, engine);
	}
	else if( objType->flags & asOBJ_TEMPLATE )
	{
		ptr = ArrayObjectFactory(objType);
	}
	else if( objType->flags & asOBJ_REF )
	{
		ptr = engine->CallGlobalFunctionRetPtr(objType->beh.construct);
	}
	else
	{
		ptr = engine->CallAlloc(objType);
		int funcIndex = objType->beh.construct;
		if( funcIndex )
			engine->CallObjectMethod(ptr, funcIndex);
	}

	return ptr;
}

void asCScriptStruct::FreeObject(void *ptr, asCObjectType *objType, asCScriptEngine *engine)
{
	if( !objType->beh.release )
	{
		if( objType->beh.destruct )
			engine->CallObjectMethod(ptr, objType->beh.destruct);

		engine->CallFree(ptr);
	}
	else
	{
		engine->CallObjectMethod(ptr, objType->beh.release);
	}
}

void asCScriptStruct::CopyObject(void *src, void *dst, asCObjectType *objType, asCScriptEngine *engine)
{
	int funcIndex = objType->beh.copy;

	if( funcIndex )
		engine->CallObjectMethod(dst, src, funcIndex);
	else
		memcpy(dst, src, objType->size);
}

void asCScriptStruct::CopyHandle(asDWORD *src, asDWORD *dst, asCObjectType *objType, asCScriptEngine *engine)
{
	if( *dst )
		engine->CallObjectMethod(*(void**)dst, objType->beh.release);
	*dst = *src;
	if( *dst )
		engine->CallObjectMethod(*(void**)dst, objType->beh.addref);
}

END_AS_NAMESPACE

