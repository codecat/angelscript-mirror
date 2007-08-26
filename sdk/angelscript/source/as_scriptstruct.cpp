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

#include "as_scriptengine.h"

#include "as_scriptstruct.h"
#include "as_arrayobject.h"
#include "as_anyobject.h"

BEGIN_AS_NAMESPACE

// This helper function will call the default constructor, that is a script function
int ConstructScriptStruct(void *ptr, asCObjectType *objType, asCScriptEngine *engine)
{
	int funcIndex = objType->beh.construct;
	
	// Setup a context for calling the default constructor
	asIScriptContext *ctx;
	int r = engine->CreateContext(&ctx, true);
	if( r < 0 )
	{
		engine->CallFree(objType, ptr);
		return -1;
	}
	r = ctx->Prepare(funcIndex);
	if( r < 0 )
	{
		engine->CallFree(objType, ptr);
		ctx->Release();
		return -1;
	}
	ctx->SetObject(ptr);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		// The memory for the structure should have been released already
		// TODO: Verify this
		ctx->Release();
		return -1;
	}
	ctx->Release();	
	
	return 0;
}


void RegisterScriptStruct(asCScriptEngine *engine)
{
	// Register the default script structure behaviours
	int r;
	engine->scriptTypeBehaviours.flags = asOBJ_SCRIPT_STRUCT;
#ifndef AS_MAX_PORTABILITY
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ScriptStruct_Construct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(ScriptStruct_Construct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ASSIGNMENT, "int &f(void[] &in)", asFUNCTION(ScriptStruct_Assignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ScriptStruct_Construct_Generic), asCALL_GENERIC); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(ScriptStruct_Construct_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(&engine->scriptTypeBehaviours, asBEHAVE_ASSIGNMENT, "int &f(void[] &in)", asFUNCTION(ScriptStruct_Assignment_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif
}

void ScriptStruct_Construct_Generic(asIScriptGeneric *gen)
{
	asCObjectType *objType = (asCObjectType*)(size_t*)gen->GetArgPointer(0);
	asCScriptStruct *self = (asCScriptStruct*)gen->GetObject();

	ScriptStruct_Construct(objType, self);
}

void ScriptStruct_Construct(asCObjectType *objType, asCScriptStruct *self)
{
	new(self) asCScriptStruct(objType);
}

asCScriptStruct::asCScriptStruct(asCObjectType *ot)
{
	gc.Init(ot);

	isDestructCalled = false;

	// Construct all properties
	asCScriptEngine *engine = gc.objType->engine;
	for( asUINT n = 0; n < gc.objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = gc.objType->properties[n];
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
	// The engine pointer should be available from the objectType
	asCScriptEngine *engine = gc.objType->engine;

	// Destroy all properties
	for( asUINT n = 0; n < gc.objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = gc.objType->properties[n];
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

	// The GCObject's destructor will be called after this
}

int asCScriptStruct::AddRef()
{
	return gc.AddRef();
}

int asCScriptStruct::Release()
{
	// Call the script destructor behaviour if the reference counter is 1.
	if( gc.GetRefCount() == 1 && !isDestructCalled )
	{
		// Make sure the destructor is called once only, even if the  
		// reference count is increased and then decreased again
		isDestructCalled = true;

		// Call the destructor
		int funcIndex = gc.objType->beh.destruct;
		if( funcIndex )
		{
			// Setup a context for calling the default constructor
			asIScriptContext *ctx;
			asCScriptEngine *engine = gc.objType->engine;
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

	return gc.Release();
}

int asCScriptStruct::GetStructTypeId()
{
	asCDataType dt = asCDataType::CreateObject(gc.objType, false);
	return gc.objType->engine->GetTypeIdFromDataType(dt);
}

int asCScriptStruct::GetPropertyCount()
{
	// TODO: Return size_t
	return (int)gc.objType->properties.GetLength();
}

int asCScriptStruct::GetPropertyTypeId(asUINT prop)
{
	if( prop >= gc.objType->properties.GetLength() )
		return asINVALID_ARG;

	return gc.objType->engine->GetTypeIdFromDataType(gc.objType->properties[prop]->type);
}

const char *asCScriptStruct::GetPropertyName(asUINT prop)
{
	if( prop >= gc.objType->properties.GetLength() )
		return 0;

	return gc.objType->properties[prop]->name.AddressOf();
}

void *asCScriptStruct::GetPropertyPointer(asUINT prop)
{
	if( prop >= gc.objType->properties.GetLength() )
		return 0;

	// Objects are stored by reference, so this must be dereferenced
	asCDataType *dt = &gc.objType->properties[prop]->type;
	if( dt->IsObject() && !dt->IsObjectHandle() )
		return *(void**)(((char*)this) + gc.objType->properties[prop]->byteOffset);

	return (void*)(((char*)this) + gc.objType->properties[prop]->byteOffset);
}

void asCScriptStruct::EnumReferences(asIScriptEngine *engine)
{
	// We'll notify the GC of all object handles that we're holding
	for( asUINT n = 0; n < gc.objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = gc.objType->properties[n];
		if( prop->type.IsObject() )
		{
			asCGCObject *ptr = *(asCGCObject**)(((char*)this) + prop->byteOffset);
			if( ptr )
				((asCScriptEngine*)engine)->GCEnumCallback(ptr);
		}
	}
}

void asCScriptStruct::ReleaseAllHandles()
{
	for( asUINT n = 0; n < gc.objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = gc.objType->properties[n];
		if( prop->type.IsObject() && prop->type.IsObjectHandle() )
		{
			asCGCObject **ptr = (asCGCObject**)(((char*)this) + prop->byteOffset);
			if( *ptr )
			{
				(*ptr)->Release();
				*ptr = 0;
			}
		}
	}
}

void ScriptStruct_Assignment_Generic(asIScriptGeneric *gen)
{
	asCScriptStruct *other = *(asCScriptStruct**)gen->GetArgPointer(0);
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
	assert( gc.objType == other.gc.objType );

	asCScriptEngine *engine = gc.objType->engine;

	// Copy all properties
	for( asUINT n = 0; n < gc.objType->properties.GetLength(); n++ )
	{
		asCProperty *prop = gc.objType->properties[n];
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
	void *ptr;
	ptr = (void*)engine->CallAlloc(objType);

	if( objType->flags & asOBJ_SCRIPT_STRUCT )
	{
		ConstructScriptStruct(ptr, objType, engine);
	}
	else if( objType->flags & asOBJ_SCRIPT_ARRAY )
	{
		ArrayObjectConstructor(objType, (asCArrayObject*)ptr);
	}
	else if( objType->flags & asOBJ_SCRIPT_ANY )
	{
		AnyObjectConstructor(objType, (asCAnyObject*)ptr);
	}
	else
	{
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

		engine->CallFree(objType, ptr);
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
		engine->CallObjectMethod(dst, objType->beh.release);
	*dst = *src;
	if( *dst )
		engine->CallObjectMethod(dst, objType->beh.addref);
}

END_AS_NAMESPACE

