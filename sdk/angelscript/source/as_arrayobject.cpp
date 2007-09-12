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
#include <stdlib.h>

#include "as_config.h"
#include "as_arrayobject.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_scriptstruct.h"

BEGIN_AS_NAMESPACE

struct sArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

void ArrayObjectConstructor(asCObjectType *ot, asCArrayObject *self)
{
	new(self) asCArrayObject(0, ot);
}

static void ArrayObjectConstructor2(asUINT length, asCObjectType *ot, asCArrayObject *self)
{
	new(self) asCArrayObject(length, ot);
}

#ifndef AS_MAX_PORTABILITY

static asCArrayObject &ArrayObjectAssignment(asCArrayObject *other, asCArrayObject *self)
{
	return *self = *other;
}

static void *ArrayObjectAt(asUINT index, asCArrayObject *self)
{
	return self->at(index);
}

static asUINT ArrayObjectLength(asCArrayObject *self)
{
	return self->GetElementCount();
}
static void ArrayObjectResize(asUINT size, asCArrayObject *self)
{
	self->Resize(size);
}

#else

static void ArrayObjectConstructor_Generic(asIScriptGeneric *gen)
{
	asCObjectType *ot = *(asCObjectType**)gen->GetArgPointer(0);
	asCArrayObject *obj = (asCArrayObject*)gen->GetObject();

	ArrayObjectConstructor(ot, obj);
}

static void ArrayObjectConstructor2_Generic(asIScriptGeneric *gen)
{
	asUINT length = gen->GetArgDWord(0);
	asCObjectType *ot = *(asCObjectType**)gen->GetArgPointer(1);
	asCArrayObject *obj = (asCArrayObject*)gen->GetObject();

	ArrayObjectConstructor2(length, ot, obj);
}

static void ArrayObjectAssignment_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *other = (asCArrayObject*)gen->GetArgObject(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void ArrayObjectAt_Generic(asIScriptGeneric *gen)
{
	asUINT index = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnAddress(self->at(index));
}

static void ArrayObjectLength_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnDWord(self->GetElementCount());
}

static void ArrayObjectResize_Generic(asIScriptGeneric *gen)
{
	asUINT size = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	self->Resize(size);
}

static void ArrayObject_AddRef_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->AddRef();
}

static void ArrayObject_Release_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->Release();
}

static void ArrayObject_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	*(int*)gen->GetReturnPointer() = self->GetRefCount();
}

static void ArrayObject_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->SetFlag();
}

static void ArrayObject_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	*(bool*)gen->GetReturnPointer() = self->GetFlag();
}

static void ArrayObject_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->EnumReferences(engine);
}

static void ArrayObject_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetArgPointer(0);
	self->ReleaseAllHandles(engine);
}

#endif

void RegisterArrayObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType(asDEFAULT_ARRAY, sizeof(asCArrayObject), asOBJ_CLASS_CDA | asOBJ_SCRIPT_ARRAY); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ArrayObjectConstructor, (int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int)", asFUNCTIONPR(ArrayObjectConstructor2, (asUINT, int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTIONPR(ArrayObjectConstructor, (int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int64)", asFUNCTIONPR(ArrayObjectConstructor2, (asUINT, int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ADDREF, "void f()", asMETHOD(asCArrayObject,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASE, "void f()", asMETHOD(asCArrayObject,Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ASSIGNMENT, "void[] &f(void[]&in)", asFUNCTION(ArrayObjectAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int &f(uint)", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int &f(uint) const", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length() const", asFUNCTION(ArrayObjectLength), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asFUNCTION(ArrayObjectResize), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCArrayObject,GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCArrayObject,SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCArrayObject,GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCArrayObject,EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCArrayObject,ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );
#else
#ifndef AS_64BIT_PTR
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ArrayObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int)", asFUNCTION(ArrayObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int64)", asFUNCTION(ArrayObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int64)", asFUNCTION(ArrayObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(ArrayObject_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(ArrayObject_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ASSIGNMENT, "void[] &f(void[]&in)", asFUNCTION(ArrayObjectAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int &f(uint)", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int &f(uint) const", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length() const", asFUNCTION(ArrayObjectLength_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asFUNCTION(ArrayObjectResize_Generic), asCALL_GENERIC); assert( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ArrayObject_GetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ArrayObject_SetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ArrayObject_GetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ArrayObject_EnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ArrayObject_ReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif 
}

asCArrayObject &asCArrayObject::operator=(asCArrayObject &other)
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}

	// Copy all elements from the other array
	CreateBuffer(&buffer, other.buffer->numElements);
	CopyBuffer(buffer, other.buffer);	

	return *this;
}

int asCArrayObject::CopyFrom(asIScriptArray *other)
{
	if( other == 0 ) return asINVALID_ARG;

	// Verify that the types are equal
	if( GetArrayTypeId() != other->GetArrayTypeId() )
		return asINVALID_TYPE;

	*this = *(asCArrayObject*)other;

	return 0;
}

asCArrayObject::asCArrayObject(asUINT length, asCObjectType *ot)
{
	refCount = 1;
	objType = ot;
	objType->refCount++;

	if( objType->flags & asOBJ_POTENTIAL_CIRCLE )
		objType->engine->AddScriptObjectToGC(this, objType);		

	// Determine element size
	if( objType->subType )
	{
		elementSize = 4;
	}
	else
	{
		if( objType->tokenType == ttDouble || objType->tokenType == ttInt64 || objType->tokenType == ttUInt64 )
			elementSize = 8;
		else if( objType->tokenType == ttInt || objType->tokenType == ttUInt ||
			     objType->tokenType == ttFloat )
			elementSize = 4;
		else if( objType->tokenType == ttInt16 || objType->tokenType == ttUInt16 )
			elementSize = 2;
		else
			elementSize = 1;
	}

	arrayType = objType->arrayType;

	CreateBuffer(&buffer, length);
}

asCArrayObject::~asCArrayObject()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}
	if( objType ) objType->refCount--;
}

asUINT asCArrayObject::GetElementCount()
{
	return buffer->numElements;
}

void asCArrayObject::Resize(asUINT numElements)
{
	sArrayBuffer *newBuffer;
	if( objType->subType )
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)NEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		newBuffer->numElements = numElements;

		// Copy the elements from the old buffer
		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		asDWORD **d = (asDWORD**)newBuffer->data;
		asDWORD **s = (asDWORD**)buffer->data;
		for( int n = 0; n < c; n++ )
			d[n] = s[n];
		
		if( numElements > buffer->numElements )
		{
			Construct(newBuffer, buffer->numElements, numElements);
		}
		else if( numElements < buffer->numElements )
		{
			Destruct(buffer, numElements, buffer->numElements);
		}
	}
	else
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)NEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+elementSize*numElements);
		newBuffer->numElements = numElements;

		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		memcpy(newBuffer->data, buffer->data, c*elementSize);
	}

	// Release the old buffer
	userFree(buffer);

	buffer = newBuffer;
}

int asCArrayObject::GetArrayTypeId()
{
	asCDataType dt = asCDataType::CreateObject(objType, false);
	return objType->engine->GetTypeIdFromDataType(dt);
}

int asCArrayObject::GetElementTypeId()
{
	asCDataType dt = asCDataType::CreateObject(objType, false);
	dt = dt.GetSubType();
	return objType->engine->GetTypeIdFromDataType(dt);
}

void *asCArrayObject::GetElementPointer(asUINT index)
{
	if( index >= buffer->numElements ) return 0;

	if( objType->subType && !(arrayType & 1) )
		return (void*)((size_t*)buffer->data)[index];
	else
		return buffer->data + elementSize*index;
}

void *asCArrayObject::at(asUINT index)
{
	if( index >= buffer->numElements )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException(TXT_OUT_OF_BOUNDS);
		return 0;
	}
	else
	{
		if( objType->subType && !(arrayType & 1) )
			return (void*)((size_t*)buffer->data)[index];
		else
			return buffer->data + elementSize*index;
	}
}

void asCArrayObject::CreateBuffer(sArrayBuffer **buf, asUINT numElements)
{
	if( objType->subType )
	{
		*buf = (sArrayBuffer*)NEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		(*buf)->numElements = numElements;
	}
	else
	{
		*buf = (sArrayBuffer*)NEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+elementSize*numElements);
		(*buf)->numElements = numElements;
	}

	Construct(*buf, 0, numElements);
}

void asCArrayObject::DeleteBuffer(sArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	DELETEARRAY(buf);
}

void asCArrayObject::Construct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	if( arrayType & 1 )
	{
		// Set all object handles to null
		asDWORD *d = (asDWORD*)(buf->data + start * sizeof(void*));
		memset(d, 0, (end-start)*sizeof(void*));
	}
	else if( objType->subType )
	{
		// Call the constructor on all objects
		asCScriptEngine *engine = objType->engine;
		asCObjectType *subType = objType->subType;
		if( subType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY) )
		{
			asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
			asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

			if( subType->flags & asOBJ_SCRIPT_STRUCT ) 
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType);
					ConstructScriptStruct(*d, subType, engine);
				}
			}
			else if( subType->flags & asOBJ_SCRIPT_ARRAY )
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType);
					ArrayObjectConstructor(subType, (asCArrayObject*)*d);
				}
			}
		}
		else
		{
			int funcIndex = subType->beh.construct;
			asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
			asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

			if( funcIndex )
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType);
					engine->CallObjectMethod(*d, funcIndex);
				}
			}
			else
			{
				for( ; d < max; d++ )
					*d = (asDWORD*)engine->CallAlloc(subType);
			}
		}
	}
}

void asCArrayObject::Destruct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	bool doDelete = true;
	if( objType->subType )
	{
		asCScriptEngine *engine = objType->engine;
		int funcIndex;
		if( objType->subType->beh.release )
		{
			funcIndex = objType->subType->beh.release;
			doDelete = false;
		}
		else
			funcIndex = objType->subType->beh.destruct;

		// Call the destructor on all of the objects
		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
		asDWORD **d   = (asDWORD**)(buf->data + start * sizeof(void*));

		if( doDelete )
		{
			if( funcIndex )
			{
				for( ; d < max; d++ )
				{
					if( *d )
					{
						engine->CallObjectMethod(*d, funcIndex);
						engine->CallFree(objType->subType, *d);
					}
				}
			}
			else
			{
				for( ; d < max; d++ )
				{
					if( *d )
						engine->CallFree(objType->subType, *d);
				}
			}
		}
		else
		{
			for( ; d < max; d++ )
			{
				if( *d )
					engine->CallObjectMethod(*d, funcIndex);
			}
		}
	}
}


void asCArrayObject::CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src)
{
	asUINT esize;
	asCScriptEngine *engine = objType->engine;
	if( arrayType & 1 )
	{
		// Copy the references and increase the reference counters
		int funcIndex = objType->subType->beh.addref;

		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

			asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
			asDWORD **d   = (asDWORD**)dst->data;
			asDWORD **s   = (asDWORD**)src->data;
			
			for( ; d < max; d++, s++ )
			{
				*d = *s;
				if( *d )
					engine->CallObjectMethod(*d, funcIndex);
			}
		}
	}
	else
	{
		esize = elementSize;
		int funcIndex = 0;
		if( objType->subType )
		{
			funcIndex = objType->subType->beh.copy;
			esize = objType->subType->size;
		}

		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
			if( objType->subType )
			{
				// Call the assignment operator on all of the objects
				asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
				asDWORD **d   = (asDWORD**)dst->data;
				asDWORD **s   = (asDWORD**)src->data;

				if( funcIndex )
				{
					for( ; d < max; d++, s++ )
						engine->CallObjectMethod(*d, *s, funcIndex);
				}
				else
				{
					for( ; d < max; d++, s++ )
						memcpy(*d, *s, esize);
				}
			}
			else
			{
				// Primitives are copied byte for byte
				memcpy(dst->data, src->data, count*esize);
			}
		}
	}
}

void asCArrayObject::Destruct()
{
	// Call destructor and free the memory
	DELETE(this, asCArrayObject);
}

void asCArrayObject::EnumReferences(asIScriptEngine *engine)
{
	// If the array is holding handles, then we need to notify the GC of them
	if( objType->subType )
	{
		void **d = (void**)buffer->data;
		for( asUINT n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
				((asCScriptEngine*)engine)->GCEnumCallback(d[n]);
		}
	}
}

void asCArrayObject::ReleaseAllHandles(asIScriptEngine *engine)
{
	asCObjectType *subType = objType->subType;
	if( subType && subType->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		void **d = (void**)buffer->data;
		for( asUINT n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
			{
				((asCScriptEngine*)engine)->CallObjectMethod(d[n], subType->beh.release);
				d[n] = 0;
			}
		}
	}
}


int asCArrayObject::AddRef()
{
	// Increase counter and clear flag set by GC
	refCount = (refCount & 0x7FFFFFFF) + 1;
	return refCount;
}

int asCArrayObject::Release()
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

int asCArrayObject::GetRefCount()
{
	return refCount & 0x7FFFFFFF;
}

void asCArrayObject::SetFlag()
{
	refCount |= 0x80000000;
}

bool asCArrayObject::GetFlag()
{
	return (refCount & 0x80000000) ? true : false;
}

END_AS_NAMESPACE
