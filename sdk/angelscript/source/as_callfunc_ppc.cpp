/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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


//
// as_callfunc_ppc.cpp
//
// These functions handle the actual calling of system functions
//
// This version is PPC specific
// Modified from as_callfunc_sh4.cpp by Pecan Heber June 2006
// Tweaked for compiling (only!) in 2.7.0 wip by kunitoki July 2006
//

#include <stdio.h>

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_PPC

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"

#include <stdlib.h>

BEGIN_AS_NAMESPACE

#define AS_PPC_MAX_ARGS 32
#define AS_MAX_REG_FLOATS 13
#define AS_MAX_REG_INTS 8

#define eq ==
//these register defines are logical/reference only, not used by asm()
#define sp r1
#define rFloatUsedCount r22
#define rGPRusedCount r23
#define rArgDataType r24
#define rArgTypePtr r25
#define rStackPtr r26
#define rFuncPtr r27
#define rArgsPtr r29
#define rTemp1 r30
#define rTemp2 r31

// The array used to send values to the correct places.
// Contains a byte of argTypes to indicate the register tYpe to load
// or zero if end of arguments
// The +1 is for when CallThis (object methods) is used
// Extra +1 when returning in memory
// Extra +1 in ppcArgsType to ensure zero end-of-args marker

extern "C" {
	enum argTypes { ppcENDARG, ppcINTARG, ppcFLOATARG, ppcDOUBLEARG };
	static asBYTE ppcArgsType[AS_PPC_MAX_ARGS + 1 + 1 + 1];
	static asDWORD ppcArgs[AS_PPC_MAX_ARGS + 1 + 1];
}

// Loads all data into the correct places and calls the function.
// ppcArgsType is an array containing a byte type (enum argTypes) for each argument.
// stackArgSize is the size in bytes for how much data to put on the stack frame
// -------------------------------------------------------------------
extern "C" asQWORD ppcFunc(const asDWORD* argsPtr, int StackArgSize, asDWORD func);
// -------------------------------------------------------------------
asm(""
	" .align 4\n"
	" .globl _ppcFunc\n"
	"_ppcFunc:\n"
	// setup stack
	" mflr r0 \n"
	// stmw r30, -8(sp)
	" stmw r30, -8(r1) \n"
	// stw r0, 8(sp)
	" stw r0, 8(r1) \n"
	// mr rTemp1,r4 // stacksize
	" mr r30,r4 \n" // stacksize
	// addi rTemp1,rTemp1,24 // plus link/save area standard size
	" addi r30, r30, 24 \n" // plus link/save area standard size
	// mr rTemp2, sp
	" mr r31, r1 \n"
	// sub sp, sp, rTemp1 // set our stack frame
	" sub r1, r1, r30 \n" // set our stack frame
	// stw rTemp2, 0(sp) // stow callers stack frame ptr
	" stw r31, 0(r1) \n" // stow callers stack frame ptr
	//
	// mr rFuncPtr, r5 // function ptr to call
	" mr r27, r5 \n" // function ptr to call
	// mr rArgsPtr, r3 // arguments pointer
	" mr r29, r3 \n" // arguments pointer

	// Clear some registers
	" sub r0,r0,r0 \n"
	// mr rGPRusedCount,r0 //counting of used/assigned GPR's
	" mr r23,r0 \n"
	// mr rFloadUsedCount,r0 //counting of used/assigned Float Registers
	" mr r22,r0 \n"

	// fetch address of argument types array
	// lis rArgTypePtr, ha16(ppcArgsType)
	" lis r25, ha16(ppcArgsType) \n"
	// addi rArgTypePtr, rArgTypePtr, lo16(ppcArgsType)
	" addi r25, r25, lo16(ppcArgsType) \n"

	// Load and stack registers according to type of argument
	// subi rArgTypePtr, rArgTypePtr, 1
	" subi r25, r25, 1 \n"

	"ppcNextArg: \n"
	// addi rArgTypePtr, rArgTypePtr, 1
	" addi r25, r25, 1 \n"
	// This is like switch{case:0; case:int; case:float; case:double}
	// lbz rArgDataType,0(rArgTypePtr)
	" lbz r24, 0(r25) \n"
	// mulli r0,rArgDataType,2
	" mulli r24, r24, 2 \n"
	// lis rTemp1, ha16(ppcTypeSwitch)
	" lis r30, ha16(ppcTypeSwitch) \n"
	// addi rTemp1, lo16(ppcTypeSwitch)
	" addi r30, r30, lo16(ppcTypeSwitch) \n"
	// add rTemp1, rTemp1, rArgDataType
	" add r30, r30, r24 \n"
	// mtctr rTemp1
	" mtctr r30 \n"
	" bctr \n"
	"ppcTypeSwitch: \n"
	" b ppcArgsEnd \n"
	" b ppcArgIsInteger \n"
	" b ppcArgIsFloat \n"
	" b ppcArgIsDouble \n"

	// Load and stack General Purpose registers (integer arguments)
	"ppcArgIsInteger: \n"
	// lis rTemp1,ha16(ppcLoadIntReg)
	// addi rTemp1,rTemp1,lo16(ppcLoadIntReg)
	" lis r30,ha16(ppcLoadIntReg) \n"
	" addi r30, r30, lo16(ppcLoadIntReg) \n"
	// mulli r0,rGPRusedCount,8
	" mulli r0, r23, 8 \n"
	// add rTemp1,rTemp1, r0
	" add r30, r30, r0 \n"
	// lwz r11,0(rArgsPtr)
	" lwz r11,0(r29) \n"
	// cmpwi rGPRusedCount,AS_MAX_REG_INTS \n" // can only load GPR3 through GPR10
	" cmpwi r23, 8 \n" // can only load GPR3 through GPR10
	" bgt ppcLoadIntRegUpd \n" // store in stack if GPR overflow
	// mtctr rTemp1
	" mtctr r30 \n"
	" bctr \n" // else load a GPR, then store in stack
	"ppcLoadIntReg: \n"
	" mr r3,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r4,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r5,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r6,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r7,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r8,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r9,r11 \n"
	" b ppcLoadIntRegUpd \n"
	" mr r10,r11 \n"
	" b ppcLoadIntRegUpd \n"
	"ppcLoadIntRegUpd: \n"
	// stw r11,0(rStackPtr)
	" stw r11,0(r26) \n"
	// addi rGPRusedCount,rGPRusedCount,1
	" addi r23, r23, 1 \n"
	// addi rArgsPtr,rArgsPtr,4
	" addi r29, r29, 4 \n"
	// addi rStackPtr,rStackPtr,4
	" addi r26, r26, 4 \n"
	" b ppcNextArg \n"

	// Load and stack float single arguments
	"ppcArgIsFloat: \n"
	// lis rTemp1,ha16(ppcLoadFloatReg)
	// addi rTemp1,rTemp1,lo16(ppcLoadFloatReg)
	" lis r30,ha16(ppcLoadFloatReg) \n"
	" addi r30, r30, lo16(ppcLoadFloatReg)\n"
	// mulli r0,rFloatUsedCount,8
	" mulli r0, r22 ,8 \n"
	// add rTemp1,rTemp1, r0
	" add r30, r30, r0 \n"
	// lfs f15,0(rArgsPtr)
	" lfs f15, 0(r29) \n"
	// cmpwi rFloatUsedCount,AS_MAX_REG_FLOATS // can't load more than 14 float/double regs
	" cmpwi r22, 13 \n" // can't load more than 14 float/double regs
	" bgt ppcLoadFloatRegUpd \n" // store float into stack area
	// mtctr rTemp1 \n"
	" mtctr r30 \n"
	" bctr \n" // else load reg, then store into stack area
	"ppcLoadFloatReg: \n"
	" fmr f0,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f1,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f2,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f3,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f4,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f5,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f6,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f7,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f8,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f9,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f10,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f11,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f12,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f13,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	" fmr f14,f15 \n"
	" b ppcLoadFloatRegUpd \n"
	"ppcLoadFloatRegUpd: \n"
	// stfs f15,0(rStackPtr)
	" stfs f15, 0(r26) \n"
	// addi rFloatUsedCount,1
	" addi r22, r22, 1 \n"
	// addi rGPRusedCount,1 //a float reg eats up a GPR
	" addi r23, r23, 1 \n" //a float reg eats up a GPR
	// addi rArgsPtr,4
	" addi r29, r29, 4 \n"
	// addi rStackPtr,4
	" addi r26, r26, 4 \n"
	" b ppcNextArg \n"

	// Load and stack a Double float argument
	"ppcArgIsDouble: \n"
	// lis rTemp1,ha16(ppcLoadDoubleReg)
	" lis r30, ha16(ppcLoadDoubleReg) \n"
	// addi rTemp1,lo16(ppcLoadDoubleReg)
	" addi r30, r30, lo16(ppcLoadDoubleReg)\n"
	// mulli r0,rFloatUsedCount,8 //calc branch for float reg
	" mulli r0, r22, 8 \n" //calc branch for float reg
	// add rTemp1,r0
	" add r30, r30, r0 \n"
	// lfd f15,0(rArgPtr)
	" lfd f15, 0(r29) \n"
	// cmpwi rFloatUsedCount,AS_MAX_REG_FLOATS // Can't load more than 14 float regs
	" cmpwi r22,13 \n" // Can't load more than 14 float regs
	" bgt ppcLoadDoubleRegUpd \n" // just store it into the stack
	// mtctr rTemp1
	" mtctr r30 \n"
	" bctr \n" // else load double, then store into stack
	"ppcLoadDoubleReg: \n"
	" fmr f0,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f1,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f2,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f3,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f4,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f5,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f6,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f7,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f8,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f9,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f10,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f11,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f12,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f13,f15 \n"
	" b ppcLoadDoubleRegUpd \n"
	" fmr f14,f15 \n"
	" b ppcLoadIntRegUpd \n"
	"ppcLoadDoubleRegUpd: \n"
	// stfd f14,0(rStackPtr)
	" stfd f14,0(r26) \n"
	// addi rFloatUsedCount,1
	" addi r22, r22, 1 \n"
	// addi rGPRusedCount,2 //a double float eats up two GPRs
	" addi r23, r23, 2 \n" //a double float eats up two GPRs
	// addi rArgsPtr,8
	" addi r29, r29, 8 \n"
	// addi rStackPtr,8
	" addi r26, r26, 8 \n"
	" b ppcNextArg \n" // go get next argument

	// End of arguments, registers are loaded, stack is set, call function
	"ppcArgsEnd: \n"
	// mtlr rFuncPtr
	" mtlr r27 \n"
	" bl \n"
	// function returned
	// lwz sp, 0(sp) \n" // restore callers stack
	" lwz r1, 0(r1) \n" // restore callers stack
	// lwz r0, 8(sp) \n" // fetch return link to caller
	" lwz r0, 8(r1) \n" // fetch return link to caller
	// lmw r30, -8(sp) \n" // restore staved regs
	" lmw r30, -8(r1) \n" // restore staved regs
	" blr \n" // return to caller
	"\n"
	" .align 4\n"
	"ppcArgsType:\n"
	" .long _ppcArgsType\n"
);

// puts the arguments in the correct place in the stack array. See comments above.
// ------------------------------------------------------------------
void stackArgs(const asDWORD *args, int& numIntArgs, int& numFloatArgs, int& numDoubleArgs)
// ------------------------------------------------------------------
{
	// asm("trap");
	int i;

	int argWordPos = numIntArgs + numFloatArgs + (numDoubleArgs*2) ;

	for(i = 0; i < AS_PPC_MAX_ARGS; i++)
	{
		if ( ppcArgsType[i] eq ppcENDARG )
			break;

		if( ppcArgsType[i] eq ppcFLOATARG )
		{
			// stow float
			((float*)ppcArgs)[argWordPos] = (float)(args[i]);
			numFloatArgs++;
			argWordPos++; //add one word
		}
		if ( ppcArgsType[i] eq ppcDOUBLEARG )
		{
			// stow double
			((double*)ppcArgs)[argWordPos] = (double)(args[i]);
			numDoubleArgs++;
			argWordPos+=2; //add two words
		}

		if( ppcArgsType[i] eq ppcINTARG )
		{
			// stow register
			((int*)ppcArgs)[argWordPos] = (int)(args[i]);
			numIntArgs++;
			argWordPos++;
		}
	}
}

// -------------------------------------------------------------------
asQWORD CallCDeclFunction(const asDWORD* pArgs, int argSize, asDWORD func)
// -------------------------------------------------------------------
{
	int intArgs = 0;
	int floatArgs = 0;
	int doubleArgs = 0;

	// put the arguments in the correct places in the ppcArgs array
	if(argSize > 0)
		stackArgs( pArgs, intArgs, floatArgs, doubleArgs );

	// asm(" trap\n nop\n");
	// printf("calling ppcFunc, %d %d %d %p.. %p.. %d...\n", intArgs, floatArgs, doubleArgs, (void*)func, ppcFunc, (int)ppcArgs[0]);

	//-return ppcFunc(intArgs << 2, floatArgs << 2, restArgs << 2, func);
	return ppcFunc( ppcArgs, argSize, func);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the first parameter is the object
// -------------------------------------------------------------------
asQWORD CallThisCallFunction(const void *obj, const asDWORD* pArgs, int argSize, asDWORD func )
// -------------------------------------------------------------------
{
	int intArgs = 1;
	int floatArgs = 0;
	int doubleArgs = 0;

	ppcArgs[0] = (asDWORD) obj;
	ppcArgsType[0] = ppcINTARG;

	// put the arguments in the correct places in the ppcArgs array
	if (argSize > 0)
		stackArgs( pArgs, intArgs, floatArgs, doubleArgs );

	// asm(" trap\n nop\n");
	// printf("calling from CallThisCall...\n");
	return ppcFunc( pArgs, argSize + sizeof(obj), func);
}

// This function is identical to CallCDeclFunction, with the only difference that
// the value in the last parameter is the object
// -------------------------------------------------------------------
asQWORD CallThisCallFunction_objLast(const void *obj, const asDWORD* pArgs, int argSize, asDWORD func)
// -------------------------------------------------------------------
{
	int intArgs = 0;
	int floatArgs = 0;
	int doubleArgs = 0;

	stackArgs( pArgs, intArgs, floatArgs, doubleArgs );

	int numArgs = intArgs + floatArgs + doubleArgs ;
	if( numArgs < AS_PPC_MAX_ARGS )
	{
		int argPos = intArgs + floatArgs + (doubleArgs*2/*words*/);
		ppcArgs[argPos] = (asDWORD) obj;
		ppcArgsType[numArgs] = ppcINTARG;
	}

	// asm(" trap\n nop\n");
	// printf("calling from CallThisCallFunction_objlast...\n");
	return ppcFunc( pArgs, argSize+sizeof(obj), func );
}

// This function should prepare system functions so that it will be faster to call them
// -------------------------------------------------------------------
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine)
// -------------------------------------------------------------------
{
	// asm("trap");
	// UNUSED(engine); //pecan 2006.6.8

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
		if( objType & asOBJ_CLASS )
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

#ifdef THISCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_THISCALL ||
					internal->callConv == ICC_VIRTUAL_THISCALL )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
#ifdef CDECL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_CDECL ||
					internal->callConv == ICC_CDECL_OBJLAST ||
					internal->callConv == ICC_CDECL_OBJFIRST )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
#ifdef STDCALL_RETURN_SIMPLE_IN_MEMORY
				if( internal->callConv == ICC_STDCALL )
				{
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				}
#endif
			}
		}
		else if( objType == asOBJ_PRIMITIVE )
		{
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = false;
		}
		else if( objType == asOBJ_FLOAT )
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
		assert(false);

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

// -------------------------------------------------------------------
asDWORD GetReturnedFloat()
// -------------------------------------------------------------------
{
	asDWORD f;

	//asm("swc1 $f0, %0\n" : "=m"(f));
	asm(" stfs f0, %0\n" : "=m"(f));

	return f;
}


// -------------------------------------------------------------------
asQWORD GetReturnedDouble()
// -------------------------------------------------------------------
{
	asQWORD f;

	//asm("swc1 $f0, %0\n" : "=m"(f));
	asm(" stfd f0, %0\n" : "=m"(f));

	return f;
}

// -------------------------------------------------------------------
int CallSystemFunction(int id, asCContext *context, void *objectPointer)
// -------------------------------------------------------------------
{
//	asm("trap");
	memset( ppcArgsType, 0, sizeof(ppcArgsType));
	id = -id - 1;

	asCScriptEngine *engine = context->engine;
	asCScriptFunction *descr = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;

	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(-id-1, objectPointer);

	asQWORD retQW = 0;

	void *func = (void*)sysFunc->func;
	int paramSize = sysFunc->paramSize;
	asDWORD *args = context->stackPointer;
	void *retPointer = 0;
	void *obj = 0;
	asDWORD *vftable;
	int popSize = paramSize;

	context->objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());
		ppcArgs[AS_PPC_MAX_ARGS+1] = (asDWORD) retPointer;
		ppcArgsType[AS_PPC_MAX_ARGS+1] = ppcINTARG;

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
					engine->CallFree(descr->returnType.GetObjectType(), retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(int(obj) + sysFunc->baseOffset);

			// Don't keep a reference to the object pointer, as it is the
			// responsibility of the application to make sure the reference
			// is valid during the call
			// if( descr->objectType->beh.addref )
			//	engine->CallObjectMethod(obj, descr->objectType->beh.addref);
		}
	}
	assert(descr->parameterTypes.GetLength() <= AS_PPC_MAX_ARGS);

	// mark all float/double/int arguments
	for( int a = 0; a < (int)descr->parameterTypes.GetLength(); a++ ) {
		ppcArgsType[a] = ppcINTARG;
		if (descr->parameterTypes[a].IsFloatType())
			ppcArgsType[a] = ppcFLOATARG;
		if (descr->parameterTypes[a].IsDoubleType())
			ppcArgsType[a] = ppcDOUBLEARG;
	}

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
					paramSize++;
				}
				else
#endif
				{
					// Copy the object's memory to the buffer
					memcpy(&paramBuffer[dpos], *(void**)(args+spos), descr->parameterTypes[n].GetSizeInMemoryBytes());
					// Delete the original memory
					engine->CallFree(descr->parameterTypes[n].GetObjectType(), *(char**)(args+spos));
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
					paramBuffer[dpos++] = args[spos++];
				paramSize += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}

		// Keep a free location at the beginning
		args = &paramBuffer[1];
	}

	context->isCallingSystemFunction = true;
	switch( callConv )
	{
	case ICC_CDECL:
	case ICC_CDECL_RETURNINMEM:
	case ICC_STDCALL:
	case ICC_STDCALL_RETURNINMEM:
		retQW = CallCDeclFunction( args, paramSize, (asDWORD)func );
		break;
	case ICC_THISCALL:
	case ICC_THISCALL_RETURNINMEM:
		retQW = CallThisCallFunction(obj, args, paramSize, (asDWORD)func );
		break;
	case ICC_VIRTUAL_THISCALL:
	case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		// Get virtual function table from the object pointer
		vftable = *(asDWORD**)obj;
		retQW = CallThisCallFunction( obj, args, paramSize, vftable[asDWORD(func)>>2] );
		break;
	case ICC_CDECL_OBJLAST:
	case ICC_CDECL_OBJLAST_RETURNINMEM:
		retQW = CallThisCallFunction_objLast( obj, args, paramSize, (asDWORD)func );
		break;
	case ICC_CDECL_OBJFIRST:
	case ICC_CDECL_OBJFIRST_RETURNINMEM:
		retQW = CallThisCallFunction( obj, args, paramSize, (asDWORD)func );
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
		for( int n = 0; n < (int)descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *obj = (void*)args[spos++];
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);

				engine->CallFree(descr->parameterTypes[n].GetObjectType(), obj);
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
			context->objectRegister = (void*)(asDWORD)retQW;

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
		// Store value in returnVal register
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
				engine->CallObjectMethod((void*)args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
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

#endif // AS_PPC
#endif // AS_MAX_PORTABILITY

//------------------------------------------------------------------

