/*
   AngelCode Scripting Library
   Copyright (c) 2003-2005 Andreas Jönsson

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

   Andreas Jönsson
   andreas@angelcode.com
*/


#include <assert.h>
#include <new>
#include <malloc.h>

#include "as_config.h"

#include "as_arrayobject.h"
#include "as_scriptengine.h"
#include "as_texts.h"

struct sArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

static void ArrayObjectConstructor(int elementSize, int behaviourIndex, asCArrayObject *self)
{
	new(self) asCArrayObject(0, elementSize, behaviourIndex);
}

static void ArrayObjectConstructor2(asUINT length, int elementSize, int behaviourIndex, asCArrayObject *self)
{
	new(self) asCArrayObject(length, elementSize, behaviourIndex);
}

static void ArrayObjectDestructor(asCArrayObject *self)
{
	self->~asCArrayObject();
}

static void ArrayObjectAddRef(asCArrayObject *self)
{
	self->AddRef();
}

static void ArrayObjectRelease(asCArrayObject *self)
{
	self->Release();
}

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
	return self->length();
}

static void ArrayObjectResize(asUINT size, asCArrayObject *self)
{
	self->resize(size);
}

void RegisterArrayObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType(asDEFAULT_ARRAY, sizeof(asCArrayObject), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_CONSTRUCT, "void f(int, int)", asFUNCTIONPR(ArrayObjectConstructor, (int, int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_CONSTRUCT, "void f(uint, int, int)", asFUNCTIONPR(ArrayObjectConstructor2, (asUINT, int, int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_ADDREF, "void f()", asFUNCTION(ArrayObjectAddRef), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_RELEASE, "void f()", asFUNCTION(ArrayObjectRelease), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ArrayObjectDestructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_ASSIGNMENT, "void[] &f(void[]&in)", asFUNCTION(ArrayObjectAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_INDEX, "int f(uint)", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_INDEX, "int f(uint) const", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length() const", asFUNCTION(ArrayObjectLength), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asFUNCTION(ArrayObjectResize), asCALL_CDECL_OBJLAST); assert( r >= 0 );
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

asCArrayObject::asCArrayObject(asUINT length, int elementSize, int behaviourIndex)
{
	asIScriptContext *ctx = asGetActiveContext();
	engine = (asCScriptEngine *)ctx->GetEngine();
	this->elementSize = elementSize & 0xFFFFFF;
	this->arrayType = elementSize >> 24;
	this->behaviourIndex = behaviourIndex;
	refCount = 1;

	CreateBuffer(&buffer, length);
}

int asCArrayObject::AddRef()
{
	return ++refCount;
}

int asCArrayObject::Release()
{
	int r = --refCount;
	if( r == 0 )
		delete this;
	return r;
}

asCArrayObject::~asCArrayObject()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}
}

asUINT asCArrayObject::length()
{
	return buffer->numElements;
}

void asCArrayObject::resize(asUINT numElements)
{
	sArrayBuffer *newBuffer;
	if( (arrayType & ~3) || behaviourIndex >= 0 )
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
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
		newBuffer = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+elementSize*numElements);
		newBuffer->numElements = numElements;

		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		memcpy(newBuffer->data, buffer->data, c*elementSize);
	}

	// Release the old buffer
	free(buffer);

	buffer = newBuffer;
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
		if( ((arrayType & ~3) || behaviourIndex >= 0) && !(arrayType & 1) )
			return *(void**)(buffer->data + sizeof(void*)*index);
		else
			return buffer->data + elementSize*index;
	}
}

void asCArrayObject::CreateBuffer(sArrayBuffer **buf, asUINT numElements)
{
	if( arrayType & ~3 )
	{
		*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(asCArrayObject*)*numElements);
		(*buf)->numElements = numElements;
	}
	else
	{
		if( behaviourIndex >= 0 )
		{
			*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
			(*buf)->numElements = numElements;
		}
		else
		{
			*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+elementSize*numElements);
			(*buf)->numElements = numElements;
		}
	}

	Construct(*buf, 0, numElements);
}

void asCArrayObject::DeleteBuffer(sArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	free(buf);
}

void asCArrayObject::Construct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	if( arrayType & 1 )
	{
		// Set all object handles to null
		asDWORD *d = (asDWORD*)(buf->data + start * sizeof(void*));
		memset(d, 0, (end-start)*sizeof(void*));
	}
	else if( arrayType & ~3 )
	{
		// Call the constructor on all objects
		int funcIndex = engine->defaultArrayObjectType->beh.construct;

		asSSystemFunctionInterface *i = engine->systemFunctionInterfaces[-funcIndex-1];
		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(asCArrayObject*));
		asDWORD **d = (asDWORD**)(buf->data + start * sizeof(asCArrayObject*));

		int esize = elementSize | ((arrayType>>2)<<24);
		void (*f)(int, int, void *) = (void (*)(int, int, void *))(i->func);
		for( ; d < max; d++ )
		{
			*d = (asDWORD*)engine->CallAlloc(engine->defaultArrayObjectType->idx);
			f(esize, behaviourIndex, *d);
		}
	}
	else
	{
		if( behaviourIndex >= 0 )
		{
			// Call the constructor on all objects
			int funcIndex = engine->allObjectTypes[behaviourIndex]->beh.construct;
			asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
			asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

			if( funcIndex )
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(behaviourIndex);
					engine->CallObjectMethod(*d, funcIndex);
				}
			}
			else
			{
				for( ; d < max; d++ )
					*d = (asDWORD*)engine->CallAlloc(behaviourIndex);
			}
		}
	}
}

void asCArrayObject::Destruct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	asUINT esize;
	if( arrayType & ~3 )
	{
		// We need to destroy default array objects
		int funcIndex = engine->defaultArrayObjectType->beh.release;

		// Call the destructor on all of the objects
		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(asCArrayObject*));
		asDWORD **d   = (asDWORD**)(buf->data + start * sizeof(asCArrayObject*));

		for( ; d < max; d++ )
			if( *d )
				engine->CallObjectMethod(*d, funcIndex);
	}
	else
	{
		esize = elementSize;
		bool doDelete = true;
		if( behaviourIndex >= 0 )
		{
			int funcIndex;
			if( engine->allObjectTypes[behaviourIndex]->beh.release )
			{
				funcIndex = engine->allObjectTypes[behaviourIndex]->beh.release;
				doDelete = false;
			}
			else
				funcIndex = engine->allObjectTypes[behaviourIndex]->beh.destruct;

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
							engine->CallFree(behaviourIndex, *d);
						}
					}
				}
				else
				{
					for( ; d < max; d++ )
					{
						if( *d )
							engine->CallFree(behaviourIndex, *d);
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
}


void asCArrayObject::CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src)
{
	asUINT esize;
	if( arrayType & ~3 )
	{
		if( arrayType & 1 )
		{
			// Copy the references and increase the reference counters
			int funcIndex = engine->defaultArrayObjectType->beh.addref;

			if( dst->numElements > 0 && src->numElements > 0 )
			{
				int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

				asDWORD **max = (asDWORD**)(dst->data + count * sizeof(asCArrayObject*));
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
			// We need to copy default array objects
			int funcIndex = engine->defaultArrayObjectType->beh.copy;

			if( dst->numElements > 0 && src->numElements > 0 )
			{
				int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

				// Call the assignment operator on all of the objects
				asDWORD **max = (asDWORD**)(dst->data + count * sizeof(asCArrayObject*));
				asDWORD **d   = (asDWORD**)dst->data;
				asDWORD **s   = (asDWORD**)src->data;

				for( ; d < max; d++, s++ )
					engine->CallObjectMethod(*d, *s, funcIndex);			
			}
		}
	}
	else
	{
		if( arrayType & 1 )
		{
			// Copy the references and increase the reference counters
			int funcIndex = engine->allObjectTypes[behaviourIndex]->beh.addref;

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
			if( behaviourIndex >= 0 )
				funcIndex = engine->allObjectTypes[behaviourIndex]->beh.copy;

			if( dst->numElements > 0 && src->numElements > 0 )
			{
				int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
				if( behaviourIndex >= 0 )
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
}

