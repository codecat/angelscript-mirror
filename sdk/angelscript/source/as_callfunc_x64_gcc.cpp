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


//
// as_callfunc.cpp
//
// These functions handle the actual calling of system functions
//

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_X64_GCC

#include "as_scriptengine.h"
#include "as_texts.h"

// The below code was contributed by niteice on May 18th, 2008

// this is based on the calling mechanism of libffcall
// note that it only works on gcc!

// asQWORD is large enough to hold a register
register asQWORD rax __asm__("rax");		// integer return 1
register asQWORD rdx __asm__("rdx");		// integer return 2
register double xmm0 __asm__("xmm0");		// float return

register asQWORD rsp __asm__("rsp");

register asQWORD rdi __asm__("rdi");
register asQWORD rsi __asm__("rsi");
register asQWORD rcx __asm__("rcx");
register asQWORD r8 __asm__("r8");
register asQWORD r9 __asm__("r9");
asQWORD iargs[6] = { rdi, rsi, rdx, rcx, r8, r9 };

register double xmm1 __asm__("xmm1");
register double xmm2 __asm__("xmm2");
register double xmm3 __asm__("xmm3");
register double xmm4 __asm__("xmm4");
register double xmm5 __asm__("xmm5");
register double xmm6 __asm__("xmm6");
register double xmm7 __asm__("xmm7");
double fargs[8] = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 };

enum argTypes { x64ENDARG = 0, x64INTARG = 1, x64FLOATARG = 2, x64DOUBLEARG = 3 };

#define X64_MAX_ARGS 32

BEGIN_AS_NAMESPACE

asDWORD GetReturnedFloat()
{
	return asDWORD(xmm0);
}

asQWORD GetReturnedDouble()
{
	return asQWORD(xmm0);
}

static asQWORD CallCDeclFunction(const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD *func)
{
	asQWORD (*call)() = (asQWORD (*)())func;
	int ii, fi;		// track integer and float arguments separately
	bool iargsUsed[6], fargsUsed[8];

	// point our stack frame at rsp first, then grow it downward as needed
	int *stackFrame = (int *)rsp;

	asMemClear(iargsUsed, sizeof(iargsUsed));
	asMemClear(fargsUsed, sizeof(fargsUsed));

	// copy the arguments into the appropriate registers or stack
	// the first 6 integer arguments go into rdi, rsi, rdc, rcx, r8, and r9
	// the first 8 float arguments go into xmm0...7
	// everything else is pushed right-to-left
	for(fi = 0, ii = 0; *pArgsType != NULL; pArgsType++)
	{
		switch(*pArgsType)
		{
			case x64ENDARG:
				break;
			case x64INTARG:
			{
				if(ii <= 6)
					iargs[ii] = pArgs[ii];
				else
				{
					stackFrame -= sizeof(int);
					*stackFrame = pArgs[ii];
				}
				ii++;
				break;
			}
			case x64FLOATARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(float);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
			case x64DOUBLEARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(double);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
		}
	}

	// call the function with the arguments
	return call();
}

// for simplicity we just stick obj at the beginning of pArgs and call cdecl
static asQWORD CallThisCallFunction(const void *obj, const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD *func)
{
	asQWORD (*call)() = (asQWORD (*)())func;
	int ii, fi;		// track integer and float arguments separately
	bool iargsUsed[6], fargsUsed[8];

	// point our stack frame at rsp first, then grow it downward as needed
	int *stackFrame = (int *)rsp;

	asMemClear(iargsUsed, sizeof(iargsUsed));
	asMemClear(fargsUsed, sizeof(fargsUsed));

	iargs[0] = *(asQWORD *)&obj;

	// copy the arguments into the appropriate registers or stack
	// the first 6 integer arguments go into rdi, rsi, rdc, rcx, r8, and r9
	// the first 8 float arguments go into xmm0...7
	// everything else is pushed right-to-left
	for(fi = 0, ii = 1; *pArgsType != NULL; pArgsType++)
	{
		switch(*pArgsType)
		{
			case x64ENDARG:
				break;
			case x64INTARG:
			{
				if(ii <= 6)
					iargs[ii] = pArgs[ii];
				else
				{
					stackFrame -= sizeof(int);
					*stackFrame = pArgs[ii];
				}
				ii++;
				break;
			}
			case x64FLOATARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(float);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
			case x64DOUBLEARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(double);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
		}
	}

	// call the function with the arguments
	return call();
}

// for simplicity we just stick obj at the beginning of pArgs and call cdecl
static asQWORD CallThisCallFunction_objLast(const void *obj, const asDWORD* pArgs, const asBYTE *pArgsType, int argSize, asDWORD *func)
{
	asQWORD (*call)() = (asQWORD (*)())func;
	int ii, fi;		// track integer and float arguments separately
	bool iargsUsed[6], fargsUsed[8];

	// point our stack frame at rsp first, then grow it downward as needed
	int *stackFrame = (int *)rsp;

	asMemClear(iargsUsed, sizeof(iargsUsed));
	asMemClear(fargsUsed, sizeof(fargsUsed));

	// copy the arguments into the appropriate registers or stack
	// the first 6 integer arguments go into rdi, rsi, rdc, rcx, r8, and r9
	// the first 8 float arguments go into xmm0...7
	// everything else is pushed right-to-left
	for(fi = 0, ii = 0; *pArgsType != NULL; pArgsType++)
	{
		switch(*pArgsType)
		{
			case x64ENDARG:
				break;
			case x64INTARG:
			{
				if(ii <= 6)
					iargs[ii] = pArgs[ii];
				else
				{
					stackFrame -= sizeof(int);
					*stackFrame = pArgs[ii];
				}
				ii++;
				break;
			}
			case x64FLOATARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(float);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
			case x64DOUBLEARG:
			{
				if(fi <= 8)
					fargs[fi] = pArgs[fi];
				else
					stackFrame -= sizeof(double);
					*stackFrame = pArgs[fi];
				fi++;
				break;
			}
		}
	}

	// call the function with the arguments
	return call();
}


// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/)
{
	// References are always returned as primitive data
	if( func->returnType.IsReference() || func->returnType.IsObjectHandle() )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = false;
	}
	// Registered types have special flags that determine how they are returned
	else if( func->returnType.IsObject() )
	{
		asDWORD objType = func->returnType.GetObjectType()->flags;
		if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_CLASS) )
		{
			if( objType & COMPLEX_MASK )
			{
				internal->hostReturnInMemory = true;
				internal->hostReturnSize = 1;
				internal->hostReturnFloat = false;
			}
			else
			{
				internal->hostReturnFloat = false;
				if( func->returnType.GetSizeInMemoryDWords() > 2 )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
				else
				{
					internal->hostReturnInMemory = false;
					internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
				}
			}
		}
		else if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_PRIMITIVE) )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = false;
		}
		else if( (objType & asOBJ_VALUE) && (objType & asOBJ_APP_FLOAT) )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = true;
		}
	}
	// Primitive types can easily be determined
	else if( func->returnType.GetSizeInMemoryDWords() > 2 )
	{
		// Shouldn't be possible to get here
		asASSERT(false);

		internal->hostReturnInMemory = true;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = false;
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 2 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 2;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttDouble, true));
	}
	else if( func->returnType.GetSizeInMemoryDWords() == 1 )
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst(asCDataType::CreatePrimitive(ttFloat, true));
	}
	else
	{
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 0;
		internal->hostReturnFloat = false;
	}

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	// Verify if the function takes any objects by value
	asUINT n;
	internal->takesObjByVal = false;
	for( n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].IsObject() && !func->parameterTypes[n].IsObjectHandle() && !func->parameterTypes[n].IsReference() )
		{
			internal->takesObjByVal = true;
			break;
		}
	}

	// Verify if the function has any registered autohandles
	internal->hasAutoHandles = false;
	for( n = 0; n < internal->paramAutoHandles.GetLength(); n++ )
	{
		if( internal->paramAutoHandles[n] )
		{
			internal->hasAutoHandles = true;
			break;
		}
	}

	return 0;
}

// returns true if the given parameter is a 'variable argument'
inline bool IsVariableArgument( asCDataType type )
{
	return (type.GetTokenType() == ttQuestion) ? true : false;
}

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine *engine = context->engine;
	asCScriptFunction *descr = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = engine->scriptFunctions[id]->sysFuncIntf;
	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW             = 0;
	void    *func              = (void*)sysFunc->func;
	int      paramSize         = sysFunc->paramSize;
	asDWORD *args              = context->stackPointer;
	void    *retPointer        = 0;
	void    *obj               = 0;
	asDWORD *vftable;
	int      popSize           = paramSize;
	int totalArgumentCount = 0;

	asBYTE	 argsType[X64_MAX_ARGS];
	asMemClear(argsType, sizeof(argsType));

	context->objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());

		if( sysFunc->hostReturnInMemory )
		{
			// The return is made in memory
			callConv++;
		}
	}

	if( callConv >= ICC_THISCALL )
	{
		if( objectPointer )
		{
			obj = objectPointer;
		}
		else
		{
			// The object pointer should be popped from the context stack
			popSize++;

			// Check for null pointer
			obj = (void*)*(args + paramSize);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(*(int *)(obj) + sysFunc->baseOffset);
		}
	}
	asASSERT(descr->parameterTypes.GetLength() <= X64_MAX_ARGS);

	// mark all float/double/int arguments
	int argIndex = 0;
	totalArgumentCount = (int)descr->parameterTypes.GetLength();
	for( int a = 0; a < (int)descr->parameterTypes.GetLength(); ++a, ++argIndex )
	{
		// get the base type
		argsType[argIndex] = x64INTARG;
		if( descr->parameterTypes[a].IsFloatType() && !descr->parameterTypes[a].IsReference() )
		{
			argsType[argIndex] = x64FLOATARG;
		}
		if( descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() )
		{
			argsType[argIndex] = x64DOUBLEARG;
		}
		if( descr->parameterTypes[a].GetSizeOnStackDWords() == 2 && !descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() )
		{
			argsType[argIndex] = x64INTARG;
		}

		// if it is a variable argument, account for the typeID
		if( IsVariableArgument(descr->parameterTypes[a]) )
		{
			// implicitly add another parameter (AFTER the parameter above), for the TypeID
			argsType[++argIndex] = x64INTARG;
		}
	}
	assert( argIndex == totalArgumentCount );

	asDWORD paramBuffer[64];
	if( sysFunc->takesObjByVal )
	{
		paramSize = 0;
		int spos = 0;
		int dpos = 1;

		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
			{
#ifdef COMPLEX_OBJS_PASSED_BY_REF
				if( descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK )
				{
					paramBuffer[dpos++] = args[spos++];
					++paramSize;
				}
				else
#endif
				{
					// NOTE: we may have to do endian flipping here

					// Copy the object's memory to the buffer
					memcpy( &paramBuffer[dpos], *(void**)(args+spos), descr->parameterTypes[n].GetSizeInMemoryBytes() );

					// Delete the original memory
					engine->CallFree( *(char**)(args+spos) );
					spos++;
					dpos += descr->parameterTypes[n].GetSizeInMemoryDWords();
					paramSize += descr->parameterTypes[n].GetSizeInMemoryDWords();
				}
			}
			else
			{
				// Copy the value directly
				paramBuffer[dpos++] = args[spos++];
				if( descr->parameterTypes[n].GetSizeOnStackDWords() > 1 )
				{
					paramBuffer[dpos++] = args[spos++];
				}
				paramSize += descr->parameterTypes[n].GetSizeOnStackDWords();
			}

			// if this was a variable argument parameter, then account for the implicit typeID
			if( IsVariableArgument( descr->parameterTypes[n] ) )
			{
				// the TypeID is just a DWORD
				paramBuffer[dpos++] = args[spos++];
				++paramSize;
			}
		}

		// Keep a free location at the beginning
		args = &paramBuffer[1];
	}

	// one last verification to make sure things are how we expect
	context->isCallingSystemFunction = true;
	switch( callConv )
	{
	case ICC_CDECL:
	case ICC_CDECL_RETURNINMEM:
	case ICC_STDCALL:
	case ICC_STDCALL_RETURNINMEM:
		retQW = CallCDeclFunction( args, argsType, paramSize, (asDWORD *)func );
		break;
	case ICC_THISCALL:
	case ICC_THISCALL_RETURNINMEM:
		retQW = CallThisCallFunction(obj, args, argsType, paramSize, (asDWORD *)func );
		break;
	case ICC_VIRTUAL_THISCALL:
	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
//		vftable = *(asDWORD**)obj;
//		retQW = CallThisCallFunction( obj, args, argsType, paramSize, vftable[((asDWORD *)func)>>2] );
		break;
	case ICC_CDECL_OBJLAST:
	case ICC_CDECL_OBJLAST_RETURNINMEM:
		retQW = CallThisCallFunction_objLast( obj, args, argsType, paramSize, (asDWORD *)func );
		break;
	case ICC_CDECL_OBJFIRST:
	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		retQW = CallThisCallFunction( obj, args, argsType, paramSize, (asDWORD *)&func );
		break;
	default:
		context->SetInternalException(TXT_INVALID_CALLING_CONVENTION);
	}
	context->isCallingSystemFunction = false;

#ifdef COMPLEX_OBJS_PASSED_BY_REF
	if( sysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value
		args = context->stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
		    args++;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *obj = (void*)args[spos++];
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);

				engine->CallFree(obj);
			}
			else
				spos += descr->parameterTypes[n].GetSizeInMemoryDWords();
		}
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
			context->objectRegister = (void*)(size_t)retQW;

			if( sysFunc->returnAutoHandle && context->objectRegister )
				engine->CallObjectMethod(context->objectRegister, descr->returnType.GetObjectType()->beh.addref);
		}
		else
		{
			if( !sysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
					*(asDWORD*)retPointer = (asDWORD)retQW;
				else
					*(asQWORD*)retPointer = retQW;
			}

			// Store the object in the register
			context->objectRegister = retPointer;
		}
	}
	else
	{
		// Store value in register1 register
		if( sysFunc->hostReturnFloat )
		{
			if( sysFunc->hostReturnSize == 1 )
				*(asDWORD*)&context->register1 = GetReturnedFloat();
			else
				context->register1 = GetReturnedDouble();
		}
		else if( sysFunc->hostReturnSize == 1 )
			*(asDWORD*)&context->register1 = (asDWORD)retQW;
		else
			context->register1 = retQW;
	}

	if( sysFunc->hasAutoHandles )
	{
		args = context->stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args++;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( sysFunc->paramAutoHandles[n] && args[spos] )
			{
				// Call the release method on the type
				engine->CallObjectMethod((void*)*(size_t*)&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
				args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
				spos++;
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}

	return popSize;
}

END_AS_NAMESPACE

#endif // AS_X64_GCC
#endif // AS_MAX_PORTABILITY


