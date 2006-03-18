/*
   AngelCode Scripting Library
   Copyright (c) 2003-2004 Andreas Jönsson

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
	asDWORD refCount;	
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

void RegisterArrayObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType(asDEFAULT_ARRAY, sizeof(asCArrayObject), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_CONSTRUCT, "void f(int, int)", asFUNCTIONP(ArrayObjectConstructor, (int, int, asCArrayObject*)), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_CONSTRUCT, "void f(uint, int, int)", asFUNCTIONP(ArrayObjectConstructor2, (asUINT, int, int, asCArrayObject*)), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(ArrayObjectDestructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_ASSIGNMENT, "void*[] &f(void*[]&)", asMETHOD(asCArrayObject, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(asDEFAULT_ARRAY, asBEHAVE_INDEX, "void *f(uint)", asMETHOD(asCArrayObject, at), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length()", asMETHOD(asCArrayObject, length), asCALL_THISCALL); assert( r >= 0 );
//	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asMETHOD(asCArrayObject, resize), asCALL_THISCALL); assert( r >= 0 );
}

asCArrayObject &asCArrayObject::operator=(asCArrayObject &other)
{
	if( buffer )
	{
		if( --(buffer->refCount) == 0 )
		{
			DeleteBuffer(buffer);
			buffer = 0;
		}
	}

/*
	// Share the internal buffer
	buffer = other.buffer;
	buffer->refCount++;
*/
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
	this->arrayDimension = elementSize >> 24;
	this->behaviourIndex = behaviourIndex;

	CreateBuffer(&buffer, length);
}

asCArrayObject::~asCArrayObject()
{
	if( buffer )
	{
		if( --(buffer->refCount) == 0 )
		{
			DeleteBuffer(buffer);
			buffer = 0;
		}
	}
}

asUINT asCArrayObject::length()
{
	return buffer->numElements;
}

void asCArrayObject::resize(asUINT numElements)
{
	// Create a new buffer
	sArrayBuffer *newBuffer;
	CreateBuffer(&newBuffer, numElements);

	// Copy the objects from the old buffer to the new buffer
	CopyBuffer(newBuffer, buffer);	

	// Release the old buffer
	if( --(buffer->refCount) == 0 )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}

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
		if( arrayDimension > 1 )
			return buffer->data + sizeof(asCArrayObject)*index;
		else
			return buffer->data + elementSize*index;
	}
}

void asCArrayObject::CreateBuffer(sArrayBuffer **buf, asUINT numElements)
{
	if( arrayDimension > 1 )
	{
		*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(asCArrayObject)*numElements);
		(*buf)->refCount = 1;
		(*buf)->numElements = numElements;

		if( numElements > 0 )
		{
			// Call the constructor on all objects
			int funcIndex = engine->defaultArrayObjectBehaviour->construct;
			if( funcIndex )
			{
				asSSystemFunctionInterface *i = engine->systemFunctionInterfaces[-funcIndex-1];
				asBYTE *max = (*buf)->data + (*buf)->numElements * sizeof(asCArrayObject);
				asBYTE *d = (*buf)->data;

				int esize = elementSize | ((arrayDimension-1)<<24);
				void (*f)(int, int, void *) = (void (*)(int, int, void *))(i->func);
				for( ; d < max; d += sizeof(asCArrayObject) )
					f(esize, behaviourIndex, d);
			}
		}
	}
	else
	{
		*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+elementSize*numElements);
		(*buf)->refCount = 1;
		(*buf)->numElements = numElements;

		if( behaviourIndex >= 0 && numElements > 0 )
		{
			// Call the constructor on all objects
			int funcIndex = engine->typeBehaviours[behaviourIndex]->construct;
			if( funcIndex )
			{
				asSSystemFunctionInterface *i = engine->systemFunctionInterfaces[-funcIndex-1];
				asBYTE *max = (*buf)->data + (*buf)->numElements * elementSize;
				asBYTE *d = (*buf)->data;

#ifdef __GNUC__
				/*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
				void (*f)(void *) = (void (*)(void *))(i->func);
				for( ; d < max; d += elementSize )
					f(d);
#else
				if( i->callConv == ICC_THISCALL )
				{

					asUPtr p;
					p.func = (void (*)())(i->func);
					void (asCUnknownClass::*f)() = p.mthd;
					for( ; d < max; d += elementSize )
						(((asCUnknownClass*)d)->*f)();
				}
				else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
				{
					void (*f)(void *) = (void (*)(void *))(i->func);
					for( ; d < max; d += elementSize )
						f(d);
				}
#endif
			}
		}
	}
}

void asCArrayObject::DeleteBuffer(sArrayBuffer *buf)
{
	asUINT esize;
	asSSystemFunctionInterface *i = 0;
	if( arrayDimension > 1 )
	{
		// We need to destroy default array objects
		int funcIndex = engine->defaultArrayObjectBehaviour->destruct;
		i = engine->systemFunctionInterfaces[-funcIndex-1];

		esize = sizeof(asCArrayObject);
	}
	else
	{
		esize = elementSize;
		if( behaviourIndex >= 0 )
		{
			int funcIndex = engine->typeBehaviours[behaviourIndex]->destruct;
			if( funcIndex )
			{
				i = engine->systemFunctionInterfaces[-funcIndex-1];
			}
		}
	}

	if( i && buf->numElements > 0 )
	{
		// Call the destructor on all of the objects
		asBYTE *max = buf->data + buf->numElements * esize;
		asBYTE *d = buf->data;

#ifdef __GNUC__
		/*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
		void (*f)(void *) = (void (*)(void *))(i->func);
		for( ; d < max; d += esize )
			f(d);
#else
		if( i->callConv == ICC_THISCALL )
		{
			asUPtr p;
			p.func = (void (*)())(i->func);
			void (asCUnknownClass::*f)() = p.mthd;
			for( ; d < max; d += esize )
				(((asCUnknownClass*)d)->*f)();
		}
		else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
		{
			void (*f)(void *) = (void (*)(void *))(i->func);
			for( ; d < max; d += esize )
				f(d);
		}
#endif
	}

	// Free the buffer
	free(buf);
}

// Some declarations used to be able to call the 
// simple function pointer as a class method
class asCBasicClass {};
typedef void (asCBasicClass::*asBASICMETHOD_t)();
union asUPtr2
{
	asFUNCTION_t    func;
	asBASICMETHOD_t mthd;
};

void asCArrayObject::CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src)
{
	asUINT esize;
	asSSystemFunctionInterface *i = 0;
	if( arrayDimension > 1 )
	{
		// We need to copy default array objects
		int funcIndex = engine->defaultArrayObjectBehaviour->copy;
		i = engine->systemFunctionInterfaces[-funcIndex-1];

		esize = sizeof(asCArrayObject);
	}
	else
	{
		esize = elementSize;
		if( behaviourIndex >= 0 )
		{
			int funcIndex = engine->typeBehaviours[behaviourIndex]->copy;
			if( funcIndex )
			{
				i = engine->systemFunctionInterfaces[-funcIndex-1];
			}
		}
	}

	if( dst->numElements > 0 && src->numElements > 0 )
	{
		int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
		if( i )
		{
			// Call the assignment operator on all of the objects
			asBYTE *max = dst->data + count * esize;
			asBYTE *d = dst->data;
			asBYTE *s = src->data;

			// I believe the assignment operator cannot be a virtual method
			assert( i->callConv != ICC_VIRTUAL_THISCALL );

#ifdef __GNUC__
			if( i->callConv == ICC_CDECL_OBJLAST )
			{
				void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
				for( ; d < max; d += esize, s += esize )
					f(s, d);			
			}
			else /*if( i->callConv == ICC_CDECL_OBJFIRST || i->callConv == ICC_THISCALL )*/
			{
				void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
				for( ; d < max; d += esize, s += esize )
					f(d, s);			
			}
#else
			if( i->callConv == ICC_THISCALL )
			{
				asUPtr2 p;
				p.func = (void (*)())(i->func);
				void (asCBasicClass::*f)(void *) = (void (asCBasicClass::*)(void *))(p.mthd);
				for( ; d < max; d += esize, s += esize )
				{
					(((asCBasicClass*)d)->*f)(s);
				}
			}
			else if( i->callConv == ICC_CDECL_OBJLAST )
			{
				void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
				for( ; d < max; d += esize, s += esize )
					f(s, d);			
			}
			else /*if( i->callConv == ICC_CDECL_OBJFIRST )*/
			{
				void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
				for( ; d < max; d += esize, s += esize )
					f(d, s);			
			}
#endif
			return;
		}

		// Default copy byte for byte
		memcpy(dst->data, src->data, count*esize);
	}
}

