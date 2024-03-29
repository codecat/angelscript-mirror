/*
   AngelCode Scripting Library
   Copyright (c) 2024 Andreas Jonsson

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
// as_callfunc_riscv64.cpp
//
// These functions handle the actual calling of system functions  
// on the 64bit RISC-V call convention used for Linux
//
// ref: https://riscv.org/wp-content/uploads/2017/05/riscv-spec-v2.2.pdf
//

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_RISCV64

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"
#include "as_context.h"

BEGIN_AS_NAMESPACE

// retfloat == 0: the called function doesn't return a float value
// retfloat == 1: the called function returns a float/double value
// argValues is an array with all the values, the first 8 values will go to a0-a7 registers, the next 8 values will go to fa0-fa7 registers, and the remaining goes to the stack
// numRegularValues holds the number of regular values to put in a0-a7 registers
// numFloatValues hold the number of float values to put in fa0-fa7 registers
// numStackValues hold the number of values to push on the stack
struct asDBLQWORD { asQWORD qw1, qw2; };
extern "C" asDBLQWORD CallRiscVFunc(asFUNCTION_t func, int retfloat, asQWORD *argValues, int numRegularValues, int numFloatValues, int numStackValues);

asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *descr, void *obj, asDWORD *args, void *retPointer, asQWORD &retQW2, void *secondObject)
{
	asCScriptEngine *engine = context->m_engine;
	const asSSystemFunctionInterface *const sysFunc = descr->sysFuncIntf;
	const asCDataType &retType = descr->returnType;
	const asCTypeInfo *const retTypeInfo = retType.GetTypeInfo();
	asFUNCTION_t func = sysFunc->func;
	int callConv = sysFunc->callConv;

	// TODO: retrieve correct function pointer to call (e.g. from virtual function table, auxiliary pointer, etc)

	// Prepare the values that will be sent to the native function
	// a0-a7 used for non-float values
	// fa0-fa7 used for float values
	// if more than 8 float values and there is space left in regular registers then those are used
	// rest of the values are pushed on the stack
	const int maxRegularRegisters = 8;
	const int maxFloatRegisters = 8;
	const int maxValuesOnStack = 48 - maxRegularRegisters - maxFloatRegisters;
	asQWORD argValues[maxRegularRegisters + maxFloatRegisters + maxValuesOnStack];
	asQWORD* stackValues = argValues + maxRegularRegisters + maxFloatRegisters;
	
	int numRegularRegistersUsed = 0;
	int numFloatRegistersUsed = 0;
	int numStackValuesUsed = 0;

	// A function returning an object by value must give the
	// address of the memory to initialize as the first argument
	if (sysFunc->hostReturnInMemory)
	{
		// Set the return pointer as the first argument
		argValues[numRegularRegistersUsed++] = (asQWORD)retPointer;
	}

	asUINT argsPos = 0;
	for (asUINT n = 0; n < descr->parameterTypes.GetLength(); n++)
	{
		const asCDataType& parmType = descr->parameterTypes[n];
		const asUINT parmDWords = parmType.GetSizeOnStackDWords();

		if (parmType.IsReference() || parmType.IsObjectHandle() || parmType.IsIntegerType() || parmType.IsUnsignedType() || parmType.IsBooleanType() )
		{
			// pointers, integers, and booleans go to regular registers

			if (parmType.GetTokenType() == ttQuestion)
			{
				// Copy the reference and type id as two separate arguments
				if (numRegularRegistersUsed < maxRegularRegisters)
				{
					argValues[numRegularRegistersUsed] = *(asQWORD*)&args[argsPos];
					numRegularRegistersUsed++;
				}
				else if (numStackValuesUsed < maxValuesOnStack)
				{
					stackValues[numStackValuesUsed] = *(asQWORD*)&args[argsPos];
					numStackValuesUsed++;
				}
				else
				{
					// Oops, we ran out of space in the argValues array!
					// TODO: This should be validated as the function is registered
					asASSERT(false);
				}

				if (numRegularRegistersUsed < maxRegularRegisters)
				{
					argValues[numRegularRegistersUsed] = (asQWORD)args[argsPos + AS_PTR_SIZE];
					numRegularRegistersUsed++;
				}
				else if (numStackValuesUsed < maxValuesOnStack)
				{
					stackValues[numStackValuesUsed] = (asQWORD)args[argsPos + AS_PTR_SIZE];
					numStackValuesUsed++;
				}
				else
				{
					// Oops, we ran out of space in the argValues array!
					// TODO: This should be validated as the function is registered
					asASSERT(false);
				}
			}
			else if (numRegularRegistersUsed < maxRegularRegisters)
			{
				if (parmDWords == 1)
					argValues[numRegularRegistersUsed] = (asQWORD)args[argsPos];
				else
					argValues[numRegularRegistersUsed] = *(asQWORD*)&args[argsPos];
				numRegularRegistersUsed++;
			}
			else if (numStackValuesUsed < maxValuesOnStack)
			{
				// The values on the stack are QWORD aligned
				if( parmDWords == 1 )
					stackValues[numStackValuesUsed] = (asQWORD)args[argsPos];
				else
					stackValues[numStackValuesUsed] = *(asQWORD*)&args[argsPos];
				numStackValuesUsed++;
			}
			else
			{
				// Oops, we ran out of space in the argValues array!
				// TODO: This should be validated as the function is registered
				asASSERT(false);
			}
		}
		else if (parmType.IsFloatType() || parmType.IsDoubleType())
		{
			// floats and doubles goes to the float registers
			// if there are more float/double args than registers, and there are still regular registers available then use those
			if (numFloatRegistersUsed < maxFloatRegisters)
			{
				if (parmDWords == 1)
					argValues[maxRegularRegisters + numFloatRegistersUsed] = 0xFFFFFFFF00000000ull | (asQWORD)args[argsPos];
				else
					argValues[maxRegularRegisters + numFloatRegistersUsed] = *(asQWORD*)&args[argsPos];
				numFloatRegistersUsed++;
			}
			else if (numRegularRegistersUsed < maxRegularRegisters)
			{
				if (parmDWords == 1)
					argValues[numRegularRegistersUsed] = 0xFFFFFFFF00000000ull | (asQWORD)args[argsPos];
				else
					argValues[numRegularRegistersUsed] = *(asQWORD*)&args[argsPos];
				numRegularRegistersUsed++;
			}
			else if (numStackValuesUsed < maxValuesOnStack)
			{
				// The values on the stack are QWORD aligned
				if (parmDWords == 1)
					stackValues[numStackValuesUsed] = 0xFFFFFFFF00000000ull | (asQWORD)args[argsPos];
				else
					stackValues[numStackValuesUsed] = *(asQWORD*)&args[argsPos];
				numStackValuesUsed++;
			}
			else
			{
				// Oops, we ran out of space in the argValues array!
				// TODO: This should be validated as the function is registered
				asASSERT(false);
			}
		}
		else if (parmType.IsObject())
		{
			if (parmType.GetTypeInfo()->flags & COMPLEX_MASK)
			{
				// complex object types are passed by address
				if (numRegularRegistersUsed < maxRegularRegisters)
				{
					argValues[numRegularRegistersUsed] = *(asQWORD*)&args[argsPos];
					numRegularRegistersUsed++;
				}
				else if (numStackValuesUsed < maxValuesOnStack)
				{
					stackValues[numStackValuesUsed] = *(asQWORD*)&args[argsPos];
					numStackValuesUsed++;
				}
				else
				{
					// Oops, we ran out of space in the argValues array!
					// TODO: This should be validated as the function is registered
					asASSERT(false);
				}
			}
			else
			{
				// simple object types are passed in registers
				// TODO: what if part of the structure fits in registers but not the other part? would part of the object be pushed on the stack?
				// TODO: what of large objects? are they passed by value in registers/stack? Or by reference?
				const asUINT sizeInMemoryDWords = parmType.GetSizeInMemoryDWords();
				const asUINT parmQWords = (sizeInMemoryDWords >> 1) + (sizeInMemoryDWords & 1);

				if ((maxRegularRegisters - numRegularRegistersUsed) > parmQWords)
				{
					if (sizeInMemoryDWords == 1)
						argValues[numRegularRegistersUsed] = (asQWORD) * *(asDWORD**)&args[argsPos];
					else
						memcpy(&argValues[numRegularRegistersUsed], *(void**)&args[argsPos], sizeInMemoryDWords * 4);
					numRegularRegistersUsed += parmQWords;
				}
				else if ((maxValuesOnStack - numStackValuesUsed) > parmQWords)
				{
					if (sizeInMemoryDWords == 1)
						stackValues[numStackValuesUsed] = (asQWORD) * *(asDWORD**)&args[argsPos];
					else
						memcpy(&stackValues[numStackValuesUsed], *(void**)&args[argsPos], sizeInMemoryDWords * 4);
					numStackValuesUsed += parmQWords;
				}
				else
				{
					// Oops, we ran out of space in the argValues array!
					// TODO: This should be validated as the function is registered
					asASSERT(false);
				}
			}
		}

		argsPos += parmDWords;
	}

	int retfloat = sysFunc->hostReturnFloat ? 1 : 0;

	// Integer values are returned in a0 and a1, allowing simple structures with up to 128bits to be returned in registers
	asDBLQWORD ret = CallRiscVFunc(func, retfloat, argValues, numRegularRegistersUsed, numFloatRegistersUsed, numStackValuesUsed);
	retQW2 = ret.qw2;

	// Special case for returning a struct with two floats. C++ will return this in fa0:fa1. These needs to be compacted into a single qword
	if (retfloat && retTypeInfo && !(retTypeInfo->flags & asOBJ_APP_CLASS_ALIGN8) && retTypeInfo->flags & asOBJ_APP_CLASS_ALLFLOATS)
	{
		ret.qw1 &= 0xFFFFFFFF;
		ret.qw1 |= (retQW2 << 32);
	}

	return ret.qw1;
}

END_AS_NAMESPACE

#endif // AS_RISCV64
#endif // AS_MAX_PORTABILITY




