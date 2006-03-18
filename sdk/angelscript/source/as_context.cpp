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


//
// as_context.cpp
//
// This class handles the execution of the byte code
//

#include <math.h> // fmodf()

#include "as_config.h"
#include "as_context.h"
#include "as_scriptengine.h"
#include "as_tokendef.h"
#include "as_bytecodedef.h"
#include "as_bstr_util.h"
#include "as_texts.h"
#include "as_callfunc.h"
#include "as_module.h"
#include "as_debug.h" // mkdir()

// We need at least 2 DWORDs reserved for exception handling
// We need at least 1 DWORD reserved for calling system functions
const int RESERVE_STACK = 2;


#ifdef AS_DEBUG
// Instruction statistics
int instrCount[256];

int instrCount2[256][256];
int lastBC;

class asCDebugStats
{
public:
	asCDebugStats() 
	{
		memset(instrCount, 0, sizeof(instrCount)); 
	}

	~asCDebugStats() 
	{
		mkdir("AS_DEBUG"); 
		FILE *f = fopen("AS_DEBUG/total.txt", "at");
		if( f )
		{
			// Output instruction statistics
			fprintf(f, "\nTotal count\n");
			int n;
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] > 0 )
					fprintf(f, "%-10.10s : %.0f\n", bcName[n].name, instrCount[n]);
			}

			fprintf(f, "\nNever executed\n");
			for( n = 0; n < BC_MAXBYTECODE; n++ )
			{
				if( bcName[n].name && instrCount[n] == 0 )
					fprintf(f, "%-10.10s\n", bcName[n].name);
			}

			fclose(f);
		}
	}

	double instrCount[256];
} stats;
#endif

AS_API asIScriptContext *asGetActiveContext()
{
	asCThreadLocalData *tld = threadManager.GetLocalData();
	if( tld->activeContexts.GetLength() == 0 )
		return 0;

	return tld->activeContexts[tld->activeContexts.GetLength()-1];
}

void asPushActiveContext(asIScriptContext *ctx)
{
	asCThreadLocalData *tld = threadManager.GetLocalData();
	tld->activeContexts.PushLast(ctx);
}

void asPopActiveContext(asIScriptContext *ctx)
{
	asCThreadLocalData *tld = threadManager.GetLocalData();

	assert(tld->activeContexts.GetLength() > 0);
	assert(tld->activeContexts[tld->activeContexts.GetLength()-1] == ctx);

	tld->activeContexts.PopLast();
}

asCContext::asCContext(asCScriptEngine *engine, bool holdRef)
{
#ifdef AS_DEBUG
	memset(instrCount, 0, sizeof(instrCount));

	memset(instrCount2, 0, sizeof(instrCount2));

	lastBC = 255;
#endif
	
	holdEngineRef = holdRef;
	if( holdRef )
		engine->AddRef();
	this->engine = engine;

	status = tsUninitialized;
	stackBlockSize = 0;
	refCount = 1;
	module = 0;
	inExceptionHandler = false;

	stringFunction = 0;
	currentFunction = 0;
}

asCContext::~asCContext()
{
	DetachEngine();

	for( int n = 0; n < stackBlocks.GetLength(); n++ )
	{
		if( stackBlocks[n] )
			delete[] stackBlocks[n];
	}
	stackBlocks.SetLength(0);

	if( stringFunction )
		delete stringFunction;
}

int asCContext::AddRef()
{
	ENTERCRITICALSECTION(criticalSection);
	int r = ++refCount;
	LEAVECRITICALSECTION(criticalSection);

	return r;
}

int asCContext::Release()
{
	ENTERCRITICALSECTION(criticalSection);
	int r = --refCount;

	if( refCount == 0 )
	{
		LEAVECRITICALSECTION(criticalSection);
		delete this;
		return 0;
	}
	LEAVECRITICALSECTION(criticalSection);

	return r;
}

void asCContext::DetachEngine()
{
	if( engine == 0 ) return;

	// Abort any execution
	Abort();

	// Release module
	if( module )
	{
		module->ReleaseContextRef();
		module = 0;
	}

	// Clear engine pointer
	if( holdEngineRef )
		engine->Release();
	engine = 0;
}

asIScriptEngine *asCContext::GetEngine()
{
	return engine;
}

int asCContext::Prepare(int funcID)
{
	if( status == tsActive || status == tsSuspended )
		return asCONTEXT_ACTIVE;

	if( funcID == -1 )
	{
		// Use the previously prepared function
		if( initialFunction == 0 )
			return asNO_FUNCTION;

		currentFunction = initialFunction;
	}
	else
	{
		// Check engine pointer
		if( engine == 0 ) return asERROR;

		if( status == tsActive || status == tsSuspended )
			return asCONTEXT_ACTIVE;

		initialFunction = engine->GetScriptFunction(funcID);
		currentFunction = initialFunction;
		if( currentFunction == 0 )
			return asNO_FUNCTION;

		// Remove reference to previous module. Add reference to new module
		if( module ) module->ReleaseContextRef();
		module = engine->GetModule(funcID);
		if( module ) 
			module->AddContextRef(); 
		else 
			return asNO_MODULE;

		// Determine the minimum stack size needed
		int stackSize = currentFunction->GetSpaceNeededForArguments() + currentFunction->stackNeeded + RESERVE_STACK;

		stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

		if( stackSize != stackBlockSize )
		{
			for( int n = 0; n < stackBlocks.GetLength(); n++ )
				if( stackBlocks[n] )
					delete[] stackBlocks[n];
			stackBlocks.SetLength(0);

			stackBlockSize = stackSize;

			asDWORD *stack = new asDWORD[stackBlockSize];
			stackBlocks.PushLast(stack);
		}

		// Reserve space for the arguments and return value
		returnValueSize = currentFunction->GetSpaceNeededForReturnValue();
		argumentsSize = currentFunction->GetSpaceNeededForArguments();
		if( currentFunction->returnType.IsComplex(engine) )
			argumentsSize++;
	}

	byteCode = currentFunction->byteCode.AddressOf();

	// Reset state
	exceptionLine = -1;
	exceptionFunction = 0;
	exceptionID = 0;
	isCallingSystemFunction = false;
	doAbort = false;
	doSuspend = false;
	externalSuspendRequest = false;
	status = tsPrepared;

	// Reserve space for the arguments and return value
	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;
	
	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	return asSUCCESS;
}

int asCContext::SetExecuteStringFunction(asCScriptFunction *func)
{
	// TODO: Make thread safe

	// TODO: Verify that the context isn't running

	if( stringFunction )
		delete stringFunction;

	stringFunction = func;

	return 0;
}

int asCContext::PrepareSpecial(int funcID)
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status == tsActive || status == tsSuspended )
		return asCONTEXT_ACTIVE;

	exceptionLine = -1;
	exceptionFunction = 0;

	exceptionID = 0;

	isCallingSystemFunction = false;

	if( module ) module->ReleaseContextRef();

	module = engine->GetModule(funcID);
	module->AddContextRef();

	if( (funcID & 0xFFFF) == asFUNC_STRING )
		currentFunction = stringFunction;
	else
		currentFunction = module->GetSpecialFunction(funcID & 0xFFFF);

	if( currentFunction == 0 )
		return asERROR;

	byteCode = currentFunction->byteCode.AddressOf();

	doAbort = false;
	doSuspend = false;
	externalSuspendRequest = false;
	status = tsPrepared;

	// Determine the minimum stack size needed
	int stackSize = currentFunction->stackNeeded + RESERVE_STACK;

	stackSize = stackSize > engine->initialContextStackSize ? stackSize : engine->initialContextStackSize;

	if( stackSize != stackBlockSize )
	{
		for( int n = 0; n < stackBlocks.GetLength(); n++ )
			if( stackBlocks[n] )
				delete[] stackBlocks[n];
		stackBlocks.SetLength(0);

		stackBlockSize = stackSize;
	
		asDWORD *stack = new asDWORD[stackBlockSize];
		stackBlocks.PushLast(stack);
	}

	// Reserve space for the arguments and return value
	returnValueSize = currentFunction->GetSpaceNeededForReturnValue();
	argumentsSize = currentFunction->GetSpaceNeededForArguments();

	stackFramePointer = stackBlocks[0] + stackBlockSize - argumentsSize;
	stackPointer = stackFramePointer;
	stackIndex = 0;
	
	// Set arguments to 0
	memset(stackPointer, 0, 4*argumentsSize);

	return asSUCCESS;
}


int asCContext::GetReturnValue(asDWORD *data, int count)
{
	// Verify that the stackPos is readable
	if( count > returnValueSize ) return asERROR;

	// Must be in terminated state
	if( status != tsProgramFinished ) return asCONTEXT_NOT_FINISHED;

	if( count == 1 )
		*data = *(asDWORD*)&returnVal;
	else if( count == 2 )
		*(asQWORD*)data = returnVal;
	
	return asSUCCESS;
}

int asCContext::SetArguments(int stackPos, asDWORD *data, int count)
{
	// Verify that the stackPos is writeable
	if( stackPos < 0 ) return asERROR;
	if( stackPos >= argumentsSize ) return asERROR;
	if( stackPos + count - 1 >= argumentsSize ) return asERROR;

	// Must be before execution
	if( status != tsPrepared ) return asCONTEXT_NOT_PREPARED;

	for( int c = 0; c < count; c++ )
		stackFramePointer[stackPos + c] = data[c];

	return 0;
}

int asCContext::Abort()
{
	// TODO: Make thread safe

	if( engine == 0 ) return asERROR;

	// TODO: Can't clean the stack here
	if( status == tsSuspended )
	{
		status = tsProgramAborted;
		CleanStack();
	}
	
	doSuspend = true;
	externalSuspendRequest = true;
	doAbort = true;

	return 0;
}

int asCContext::Suspend()
{
	// TODO: Make thread safe

	if( engine == 0 ) return asERROR;

	doSuspend = true;
	externalSuspendRequest = true;

	return 0;
}

int asCContext::ExecuteStep(asDWORD flag)
{
	if( flag != asEXEC_STEP_INTO &&
		flag != asEXEC_STEP_OVER ) return asINVALID_ARG;

	// Check engine pointer
	if( engine == 0 ) return asERROR;
	
	if( status != tsSuspended && status != tsPrepared )
		return asERROR;

	status = tsSuspended;

	asPushActiveContext((asIScriptContext *)this);

	if( flag == asEXEC_STEP_INTO )
	{
		doSuspend = true;
		status = tsActive;
		while( status == tsActive )
			ExecuteNext();
	}
	else if( flag == asEXEC_STEP_OVER )
	{
		// Remember at what call stack level we are
		int level = callStack.GetLength();

		while( !externalSuspendRequest && status == tsSuspended )
		{
			doSuspend = true;
			status = tsActive;
			while( status == tsActive )
				ExecuteNext();

			// If are at the same level (or lower) as we started 
			// we should suspend the execution
			if( callStack.GetLength() <= level )
				break;
		}
	}

	doSuspend = false;
	externalSuspendRequest = false;
		
	asPopActiveContext((asIScriptContext *)this);

	if( doAbort )
	{
		doAbort = false;
			
		// TODO: Cleaning the stack is also an execution thus the context is active
		// We shouldn't decrease the numActiveContexts until after this is complete
		CleanStack();
		status = tsProgramAborted;
		return asEXECUTION_ABORTED;
	}

	if( status == tsSuspended )
		return asEXECUTION_SUSPENDED;

	if( status == tsProgramFinished )
		return asEXECUTION_FINISHED;

	if( status == tsUnhandledException )
		return asEXECUTION_EXCEPTION;

	return asERROR;
}

int asCContext::Execute()
{
	// Check engine pointer
	if( engine == 0 ) return asERROR;

	if( status != tsSuspended && status != tsPrepared )
		return asERROR;

	status = tsSuspended;

	asPushActiveContext((asIScriptContext *)this);

	while( !doSuspend && status == tsSuspended )
	{
		status = tsActive;
		while( status == tsActive )
			ExecuteNext();
	}

	doSuspend = false;

	asPopActiveContext((asIScriptContext *)this);


#ifdef AS_DEBUG
	// Output instruction statistics
	mkdir("AS_DEBUG");
	FILE *f = fopen("AS_DEBUG/stats.txt", "at");
	fprintf(f, "\n");
	int n;
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name && instrCount[n] )
			fprintf(f, "%-10.10s : %d\n", bcName[n].name, instrCount[n]);
	}

	fprintf(f, "\n");
	for( n = 0; n < 256; n++ )
	{
		if( bcName[n].name )
		{
			for( int m = 0; m < 256; m++ )
			{
				if( instrCount2[n][m] )
					fprintf(f, "%-10.10s, %-10.10s : %d\n", bcName[n].name, bcName[m].name, instrCount2[n][m]);
			}
		}
	}
	fclose(f);
#endif

	if( doAbort )
	{
		doAbort = false;

		// TODO: Cleaning the stack is also an execution thus the context is active
		// We shouldn't decrease the numActiveContexts until after this is complete
		CleanStack();
		status = tsProgramAborted;
		return asEXECUTION_ABORTED;
	}

	if( status == tsSuspended )
		return asEXECUTION_SUSPENDED;

	if( status == tsProgramFinished )
		return asEXECUTION_FINISHED;

	if( status == tsUnhandledException )
		return asEXECUTION_EXCEPTION;

	return asERROR;
}

void asCContext::PushCallState()
{
	callStack.SetLength(callStack.GetLength() + 6);

	asDWORD *s = (asDWORD *)callStack.AddressOf() + callStack.GetLength() - 6;

	s[0] = (asDWORD)stackFramePointer;
	s[1] = (asDWORD)currentFunction;
	s[2] = (asDWORD)byteCode;
	s[3] = (asDWORD)stackPointer;
	s[4] = stackIndex;
	s[5] = (asDWORD)module;
}

void asCContext::PopCallState()
{
	asDWORD *s = (asDWORD *)callStack.AddressOf() + callStack.GetLength() - 6;

	stackFramePointer = (asDWORD *)s[0];
	currentFunction   = (asCScriptFunction *)s[1];
	byteCode          = (asBYTE *)s[2];
	stackPointer      = (asDWORD *)s[3];
	stackIndex        = s[4];
	module            = (asCModule *)s[5];

	callStack.SetLength(callStack.GetLength() - 6);
}

void asCContext::CallScriptFunction(asCModule *mod, asCScriptFunction *func)
{
	// Push the framepointer, functionid and programCounter on the stack
	PushCallState();

	// Verify if there is enough room in the stack block. Allocate new block if not
	asDWORD *oldStackPointer = stackPointer;
	while( stackPointer - (func->stackNeeded + RESERVE_STACK) < stackBlocks[stackIndex] )
	{
		// The size of each stack block is determined by the following formula:
		// size = stackBlockSize << index

		// Make sure we don't allocate more space than allowed
		if( engine->maximumContextStackSize )
		{
			// This test will only stop growth once it has already crossed the limit
			if( stackBlockSize * ((1 << (stackIndex+1)) - 1) > engine->maximumContextStackSize )
			{
				SetInternalException(TXT_STACK_OVERFLOW);
				return;
			}
		}

		stackIndex++;
		if( stackBlocks.GetLength() == stackIndex )
		{
			asDWORD *stack = new asDWORD[stackBlockSize << stackIndex];
			stackBlocks.PushLast(stack);
		}

		stackPointer = stackBlocks[stackIndex] + (stackBlockSize<<stackIndex) - func->GetSpaceNeededForArguments();
	}

	if( stackPointer != oldStackPointer )
	{
		// Copy the function arguments to the new stack space
		memcpy(stackPointer, oldStackPointer, 4*func->GetSpaceNeededForArguments());
	}

	// Update framepointer and programCounter
	stackFramePointer = stackPointer;
	currentFunction = func;
	exceptionID = 0;
	module = mod;

	// Set new byte code pointer
	byteCode = currentFunction->byteCode.AddressOf();
}

#ifndef USE_ASM_VM

void asCContext::ExecuteNext(bool createRelocationTable)
{
	if( createRelocationTable ) return;

	asBYTE  *l_bc = byteCode;
	asDWORD *l_sp = stackPointer;
	asDWORD *l_fp = stackFramePointer;
	asDWORD  l_tempReg = 0;

	for(;;)
	{

#ifdef AS_DEBUG
	++stats.instrCount[*l_bc];

	++instrCount[*l_bc];

	++instrCount2[lastBC][*l_bc];
	lastBC = *l_bc;
#endif

	// Remember to keep the cases in order and without 
	// gaps, because that will make the switch faster. 
	// It will be faster since only one lookup will be 
	// made to find the correct jump destination. If not
	// in order, the switch will make two lookups.
	switch(*l_bc)
	{
//--------------
// memory access functions
	case BC_POP:
		l_sp += *(asWORD*)(BCARG_W(l_bc));
		l_bc += BCS_POP;
		break;

	case BC_PUSH:
		l_sp -= *(asWORD*)(BCARG_W(l_bc));
		l_bc += BCS_PUSH;
		break;

	case BC_SET4:
		--l_sp;
		*l_sp = *(asDWORD*)(BCARG_DW(l_bc));
		l_bc += BCS_SET4;
		break;

	case BC_RD4:
		*l_sp = *(asDWORD*)(*l_sp);
		l_bc += BCS_RD4;
		break;

	case BC_RDSF4:
		--l_sp;
		*l_sp = *(l_fp - *(short*)(BCARG_W(l_bc)));
		l_bc += BCS_RDSF4;
		break;

	case BC_WRT4:
		*(asDWORD*)*l_sp = *(l_sp + 1);
		++l_sp;
		l_bc += BCS_WRT4;
		break;

	case BC_MOV4:
		*(asDWORD*)*l_sp = *(l_sp+1);
		l_sp += 2;
		l_bc += BCS_MOV4;
		break;

	case BC_PSF:
		--l_sp;
		*l_sp = asDWORD(l_fp - *(short*)(BCARG_W(l_bc)));
		l_bc += BCS_PSF;
		break;

	case BC_MOVSF4:
		*(l_fp - *(short*)(BCARG_W(l_bc))) = *l_sp;
		++l_sp;
		l_bc += BCS_MOVSF4;
		break;

	case BC_SWAP4:
		{
			asDWORD d = *l_sp;
			*l_sp = *(l_sp+1);
			*(l_sp+1) = d;
			l_bc += BCS_SWAP4;
		}
		break;

	case BC_STORE4:
		l_tempReg = *l_sp;
		l_bc += BCS_STORE4;
		break;

	case BC_RECALL4:
		--l_sp;
		*l_sp = l_tempReg;
		l_bc += BCS_RECALL4;
		break;

	case BC_ADDOFF:
		{
			int i = int(*l_sp); // The offset
			++l_sp; // Pop the value
			asDWORD *a = (asDWORD*)*l_sp; // The reference to the pointer

			if( *a == 0 ) // Check the pointer
			{
				SetInternalException(TXT_NULL_POINTER_ACCESS);
				return;
			}
			
			*l_sp = asDWORD((asBYTE*)(*a) + i);
			l_bc += BCS_ADDOFF;
		}
		break;

//----------------
// path control instructions
	case BC_CALL:
		{
			int i = *(int*)(BCARG_DW(l_bc));
			l_bc += BCS_CALL;

			assert( i >= 0 );
			assert( (i & FUNC_IMPORTED) == 0 );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			CallScriptFunction(module, module->GetScriptFunction(i));

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != tsActive )
				return;
		}
		break;

	case BC_RET:
		{
			if( callStack.GetLength() == 0 )
			{
				status = tsProgramFinished;
				return;
			}

			asWORD w = *(asWORD*)(BCARG_W(l_bc));

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Read the old framepointer, functionid, and programCounter from the stack
			PopCallState();

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// Pop arguments from stack
			l_sp += w;
		}
		break;

	case BC_JMP:
		l_bc += BCS_JMP + *(int*)(BCARG_DW(l_bc));
		break;

	case BC_JZ:
		{
			asDWORD d = *l_sp;
			if( d == 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			++l_sp;
			l_bc += BCS_JZ;
		}
		break;

	case BC_JNZ:
		{
			asDWORD d = *l_sp;
			if( d != 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			++l_sp;
			l_bc += BCS_JNZ;
		}
		break;
//--------------------
// test instructions
	case BC_TZ:
		*l_sp = (*l_sp == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc += BCS_TZ;
		break;
	case BC_TNZ:
		*l_sp = (*l_sp == 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc += BCS_TNZ;
		break;
	case BC_TS:
		*l_sp = (int(*l_sp) < 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc += BCS_TS;
		break;
	case BC_TNS:
		*l_sp = (int(*l_sp) < 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc += BCS_TNS;
		break;
	case BC_TP:
		*l_sp = (int(*l_sp) > 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
		l_bc += BCS_TP;
		break;
	case BC_TNP:
		*l_sp = (int(*l_sp) > 0 ? 0 : VALUE_OF_BOOLEAN_TRUE);
		l_bc += BCS_TNP;
		break;
//--------------------------
// int instructions
	case BC_ADDi:
		++l_sp;
		*l_sp = asDWORD(int(*l_sp) + int(*(l_sp-1)));
		l_bc += BCS_ADDi;
		break;

	case BC_SUBi:
		++l_sp;
		*l_sp = asDWORD(int(*l_sp) - int(*(l_sp-1)));
		l_bc += BCS_SUBi;
		break;

	case BC_MULi:
		++l_sp;
		*l_sp = asDWORD(int(*l_sp) * int(*(l_sp-1)));
		l_bc += BCS_MULi;
		break;

	case BC_DIVi:
		if( *l_sp == 0 )
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Raise exception
			SetInternalException(TXT_DIVIDE_BY_ZERO);
			return;
		}
		++l_sp;
		*l_sp = asDWORD(int(*l_sp) / int(*(l_sp-1)));
		l_bc += BCS_DIVi;
		break;

	case BC_MODi:
		if( *l_sp == 0 )
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Raise exception
			SetInternalException(TXT_DIVIDE_BY_ZERO);
			return;
		}
		++l_sp;
		*l_sp = asDWORD(int(*l_sp) % int(*(l_sp-1)));
		l_bc += BCS_MODi;
		break;

	case BC_NEGi:
		*l_sp = asDWORD(-int(*l_sp));
		l_bc += BCS_NEGi;
		break;

	case BC_CMPi:
		{
			++l_sp;
			int i = asDWORD(int(*l_sp) - int(*(l_sp-1)));
			if( i == 0 ) i = 0;
			else if( i < 0 ) i = -1;
			else i = 1;
			*l_sp = i;
			l_bc += BCS_CMPi;
		}
		break;

	case BC_INCi:
		++(*(int*)(*l_sp));
		l_bc += BCS_INCi;
		break;

	case BC_DECi:
		--(*(int*)(*l_sp));
		l_bc += BCS_DECi;
		break;

	case BC_I2F:
		*(float*)l_sp = float(*(int*)(l_sp));
		l_bc += BCS_UI2F;
		break;
//------------------
// float instructions
	case BC_ADDf:
		++l_sp;
		*(float*)l_sp = *(float*)l_sp + *(float*)(l_sp-1);
		l_bc += BCS_MULf;
		break;

	case BC_SUBf:
		++l_sp;
		*(float*)l_sp = *(float*)l_sp - *(float*)(l_sp-1);
		l_bc += BCS_MULf;
		break;

	case BC_MULf:
		++l_sp;
		*(float*)l_sp = *(float*)l_sp * *(float*)(l_sp-1);
		l_bc += BCS_MULf;
		break;

	case BC_DIVf:
		if( *l_sp == 0 )
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Raise exception
			SetInternalException(TXT_DIVIDE_BY_ZERO);
			return;
		}
		++l_sp;
		*(float*)l_sp = *(float*)l_sp / *(float*)(l_sp-1);
		l_bc += BCS_DIVf;
		break;

	case BC_MODf:
		if( *l_sp == 0 )
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			// Raise exception
			SetInternalException(TXT_DIVIDE_BY_ZERO);
			return;
		}
		++l_sp;
		*(float*)l_sp = fmodf(*(float*)l_sp, *(float*)(l_sp-1));
		l_bc += BCS_MODf;
		break;

	case BC_NEGf:
		*(float*)l_sp = -*(float*)l_sp;
		l_bc += BCS_NEGf;
		break;

	case BC_CMPf:
		{
			++l_sp;
			float f = *(float*)l_sp - *(float*)(l_sp-1);
			int i;
			if( f == 0 ) i = 0;
			else if( f < 0 ) i = -1;
			else i = 1;
			*l_sp = i;
			l_bc += BCS_CMPf;
		}
		break;



	case BC_INCf:
		++(*(float*)(*l_sp));
		l_bc += BCS_INCf;
		break;

	case BC_DECf:
		--(*(float*)(*l_sp));
		l_bc += BCS_DECf;
		break;

	case BC_F2I:
		*l_sp = int(*(float*)(l_sp));
		l_bc += BCS_F2I;
		break;

//--------------------
// bits instructions
	case BC_BNOT:
		*l_sp = ~*l_sp;
		l_bc += BCS_BNOT;
		break;

	case BC_BAND:
		++l_sp;
		*l_sp = *l_sp & *(l_sp-1);
		l_bc += BCS_BAND;
		break;

	case BC_BOR:
		++l_sp;
		*l_sp = *l_sp | *(l_sp-1);
		l_bc += BCS_BOR;
		break;

	case BC_BXOR:
		++l_sp;
		*l_sp = *l_sp ^ *(l_sp-1);
		l_bc += BCS_BXOR;
		break;

	case BC_BSLL:
		++l_sp;
		*l_sp = *l_sp << *(l_sp-1);
		l_bc += BCS_BSLL;
		break;

	case BC_BSRL:
		++l_sp;
		*l_sp = *l_sp >> *(l_sp-1);
		l_bc += BCS_BSRL;
		break;

	case BC_BSRA:
		++l_sp;
		*l_sp = int(*l_sp) >> *(l_sp-1);
		l_bc += BCS_BSRA;
		break;

//----------------------------------
	case BC_UI2F:
		*(float*)l_sp = float(*(unsigned int*)(l_sp));
		l_bc += BCS_UI2F;
		break;
	
	case BC_F2UI:
		*l_sp = (unsigned int)(*(float*)(l_sp));
		l_bc += BCS_F2UI;
		break;

	case BC_CMPui:
		{
			asDWORD d = asDWORD(*(l_sp+1));
			asDWORD d2 = asDWORD(*l_sp);
			l_sp++;
			int i;
			if( d == d2 ) i = 0;
			else if( d < d2 ) i = -1;
			else i = 1;
			*l_sp = asDWORD(i);
			l_bc += BCS_CMPui;
		}
		break;
		
	case BC_SB:
		*l_sp = *(char*)l_sp;
		l_bc += BCS_SB;
		break;

	case BC_SW:
		*l_sp = *(short*)l_sp;
		l_bc += BCS_SW;
		break;

	case BC_UB:
		*l_sp = *(asBYTE*)l_sp;
		l_bc += BCS_UB;
		break;

	case BC_UW:
		*l_sp = *(asWORD*)l_sp;
		l_bc += BCS_UW;
		break;

	case BC_WRT1:
		l_sp++;
		*(asBYTE*)*(l_sp-1) = *(asBYTE*)l_sp;
		l_bc += BCS_WRT1;
		break;

	case BC_WRT2:
		l_sp++;
		*(asWORD*)*(l_sp-1) = *(asWORD*)l_sp;
		l_bc += BCS_WRT2;
		break;

	case BC_INCi16:
		(*(short*)(*l_sp))++;
		l_bc += BCS_INCi16;
		break;

	case BC_INCi8:
		(*(char*)(*l_sp))++;
		l_bc += BCS_INCi8;
		break;

	case BC_DECi16:
		(*(short*)(*l_sp))--;
		l_bc += BCS_DECi16;
		break;

	case BC_DECi8:
		(*(char*)(*l_sp))--;
		l_bc += BCS_DECi8;
		break;

	case BC_PUSHZERO:
		--l_sp;
		*l_sp = 0;
		l_bc += BCS_PUSHZERO;
		break;

	case BC_COPY:
		memcpy((asDWORD*)*l_sp, (asDWORD*)*(l_sp+1), *(asWORD*)(BCARG_W(l_bc))*4);
		l_sp++;
		l_bc += BCS_COPY;
		break;

	case BC_PGA:
		{
			int i = *(int*)(BCARG_DW(l_bc));
			--l_sp;
			if( i < 0 )
				*l_sp = asDWORD(engine->globalPropAddresses[-int(i) - 1]);
			else
				*l_sp = asDWORD(module->globalMem.AddressOf() + (i & 0xFFFF));
			l_bc += BCS_PGA;
		}
		break;

	case BC_SET8:
		l_sp -= 2;
		*(asQWORD*)l_sp = *(asQWORD*)(BCARG_QW(l_bc));
		l_bc += BCS_SET8;
		break;

	case BC_WRT8:
		l_sp++;
		*(asQWORD*)*(l_sp-1) = *(asQWORD*)l_sp;
		l_bc += BCS_WRT8;
		break;

	case BC_RD8:
		*(asQWORD*)(l_sp-1) = *(asQWORD*)*l_sp;
		--l_sp;
		l_bc += BCS_RD8;
		break;

	case BC_NEGd:
		*(double*)l_sp = -*(double*)l_sp;
		l_bc += BCS_NEGd;
		break;

	case BC_INCd:
		++(*(double*)(*l_sp));
		l_bc += BCS_INCd;
		break;

	case BC_DECd:
		--(*(double*)(*l_sp));
		l_bc += BCS_DECd;
		break;

	case BC_ADDd:
		l_sp += 2;
		*(double*)l_sp = *(double*)(l_sp) + *(double*)(l_sp-2);
		l_bc += BCS_ADDd;
		break;
	
	case BC_SUBd:
		l_sp += 2;
		*(double*)l_sp = *(double*)(l_sp) - *(double*)(l_sp-2);
		l_bc += BCS_SUBd;
		break;

	case BC_MULd:
		l_sp += 2;
		*(double*)l_sp = *(double*)(l_sp) * *(double*)(l_sp-2);
		l_bc += BCS_MULd;
		break;

	case BC_DIVd:
		{
			if( *(asQWORD*)l_sp == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			
			l_sp += 2;
			*(double*)l_sp = *(double*)(l_sp) / *(double*)(l_sp-2);
			l_bc += BCS_DIVd;
		}
		break;

	case BC_MODd:
		{
			if( *(asQWORD*)l_sp == 0 )
			{
				// Need to move the values back to the context
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				// Raise exception
				SetInternalException(TXT_DIVIDE_BY_ZERO);
				return;
			}
			
			l_sp += 2;
			*(double*)l_sp = fmod(*(double*)(l_sp), *(double*)(l_sp-2));
			l_bc += BCS_MODd;
		}
		break;

	case BC_SWAP8:
		{
			asQWORD q = *(asQWORD*)l_sp;
			*(asQWORD*)l_sp = *(asQWORD*)(l_sp+2);
			*(asQWORD*)(l_sp+2) = q;
			l_bc += BCS_SWAP8;
		}
		break;

	case BC_CMPd:
		{
			double dbl = *(double*)(l_sp+2) - *(double*)(l_sp);
			l_sp += 3;
			int i;
			if( dbl == 0 ) i = 0;
			else if( dbl < 0 ) i = -1;
			else i = 1;
			*l_sp = asDWORD(i);

			l_bc += BCS_CMPd;
		}
		break;

	case BC_dTOi:
		{
			int i = int(*(double*)(l_sp++));
			*l_sp = i;
		}
		l_bc += BCS_dTOui;
		break;

	case BC_dTOui:
		{
			asUINT u = asDWORD(*(double*)(l_sp++));
			*l_sp = u;
		}
		l_bc += BCS_dTOui;
		break;

	case BC_dTOf:
		{
			float f = float(*(double*)(l_sp++));
			*(float*)l_sp = f;
		}
		l_bc += BCS_dTOf;
		break;

	case BC_iTOd:
		{
			double d = double(int(*l_sp--));
			*(double*)l_sp = d;
		}
		l_bc += BCS_iTOd;
		break;

	case BC_uiTOd:
		{
			double d = double(*l_sp--);
			*(double*)l_sp = d;
		}
		l_bc += BCS_uiTOd;
		break;

	case BC_fTOd:
		{
			double d = double(*(float*)(l_sp--));
			*(double*)l_sp = d;
		}
		l_bc += BCS_fTOd;
		break;

	case BC_JMPP:
		l_bc += BCS_JMPP + (*l_sp++)*BCS_JMP;
		break;

	case BC_PEID:
		// Push current eid value
		l_sp--;
		*l_sp = exceptionID;

		l_bc += BCS_PEID;
		break;

	case BC_SRET4:
		*(asDWORD*)&returnVal = *l_sp;
		l_sp++;
		l_bc += BCS_SRET4;
		break;

	case BC_SRET8:
		returnVal = *(asQWORD*)l_sp;
		l_sp += 2;
		l_bc += BCS_SRET8;
		break;

	case BC_RRET4:
		l_sp--;
		*l_sp = *(asDWORD*)&returnVal;
		l_bc += BCS_RRET4;
		break;

	case BC_RRET8:
		l_sp -= 2;
		*(asQWORD*)l_sp = returnVal;
		l_bc += BCS_RRET8;
		break;

// -----------------------
	case BC_STR:
		{
			// Get the string id from the argument
			asWORD w = *(asWORD*)(BCARG_W(l_bc));
			// Push the string pointer on the stack
			--l_sp;
			asBSTR *b = module->GetConstantBStr(w);
			*l_sp = *(asDWORD*)b;
			// Push the string length on the stack
			--l_sp;
			*l_sp = asBStrLength(*b);
			l_bc += BCS_STR;
		}
		break;

	case BC_JS:
		{
			int i = int(*l_sp);
			if( i < 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			l_sp++;
			l_bc += BCS_JS;
		}
		break;
	case BC_JNS:
		{
			int i = int(*l_sp);
			if( i >= 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			l_sp++;
			l_bc += BCS_JNS;
		}
		break;
	case BC_JP:
		{
			int i = int(*l_sp);
			if( i > 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			l_sp++;
			l_bc += BCS_JP;
		}
		break;
	case BC_JNP:
		{
			int i = int(*l_sp);
			if( i <= 0 )
				l_bc += *(int*)(BCARG_DW(l_bc));
			l_sp++;
			l_bc += BCS_JNP;
		}
		break;

	case BC_CMPIi:
		{
			int i = int(*l_sp) - *(int*)(BCARG_DW(l_bc));
			if( i == 0 ) i = 0;
			else if( i < 0 ) i = -1;
			else i = 1;
			*l_sp = asDWORD(i);
			l_bc += BCS_CMPIi;
		}
		break;

	case BC_CMPIui:
		{
			asDWORD d2 = *(asDWORD*)(BCARG_DW(l_bc));
			asDWORD d = asDWORD(*l_sp);
			int i;
			if( d == d2 ) i = 0;
			else if( d < d2 ) i = -1;
			else i = 1;
			*l_sp = asDWORD(i);
			l_bc += BCS_CMPIui;
		}
		break;

	case BC_CALLSYS:
		{
			// Get function ID from the argument
			int i = *(int*)(BCARG_DW(l_bc));
			assert( i < 0 );

			// Need to move the values back to the context 
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			l_sp += CallSystemFunction(i, this);

			// Update the program position after the call so that line number is correct
			l_bc += BCS_CALLSYS;

			// Should the execution be suspended?
			if( doSuspend )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				status = tsSuspended;
				return;
			}
			// An exception might have been raised
			if( status != tsActive )
			{
				byteCode = l_bc;
				stackPointer = l_sp;
				stackFramePointer = l_fp;

				return;
			}
		}
		break;

	case BC_CALLBND:
		{
			// Get the function ID from the stack
			int i = *(int*)(BCARG_DW(l_bc));
			l_bc += BCS_CALL;

			assert( i >= 0 );
			assert( i & FUNC_IMPORTED );

			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			int funcID = module->bindInformations[i&0xFFFF].importedFunction;
			if( funcID == -1 )
			{
				SetInternalException(TXT_UNBOUND_FUNCTION);
				return;
			}
			else
			{
				asCModule *callModule = engine->GetModule(funcID);
				asCScriptFunction *func = callModule->GetScriptFunction(funcID);

				CallScriptFunction(callModule, func);
			}

			// Extract the values from the context again
			l_bc = byteCode;
			l_sp = stackPointer;
			l_fp = stackFramePointer;

			// If status isn't active anymore then we must stop
			if( status != tsActive )
				return;
		}
		break;

	case BC_RDGA4:
		{
			int i = *(int*)(BCARG_DW(l_bc));
			asDWORD *a;
			if( i < 0 )
				a = (asDWORD*)(engine->globalPropAddresses[-int(i) - 1]);
			else
				a = (asDWORD*)(module->globalMem.AddressOf() + (i & 0xFFFF));
			--l_sp;
			*l_sp = *a;
			l_bc += BCS_RDGA4;
		}
		break;

	case BC_MOVGA4:
		{
			int i = *(int*)(BCARG_DW(l_bc));
			asDWORD *a;
			if( i < 0 )
				a = (asDWORD*)(engine->globalPropAddresses[-int(i) - 1]);
			else
				a = (asDWORD*)(module->globalMem.AddressOf() + (i & 0xFFFF));
			*(asDWORD*)a = *l_sp++;
			l_bc += BCS_MOVGA4;
		}
		break;
	case BC_ADDIi:
		*l_sp = asDWORD(int(asDWORD(*l_sp)) + *(int*)(BCARG_DW(l_bc)));
		l_bc += BCS_ADDIi;
		break;
	case BC_SUBIi:
		*l_sp = asDWORD(int(asDWORD(*l_sp)) - *(int*)(BCARG_DW(l_bc)));
		l_bc += BCS_SUBIi;
		break;
	case BC_CMPIf:
		{
			float f = *(float*)l_sp - *(float*)(BCARG_DW(l_bc));
			int i;
			if( f == 0 ) i = 0;
			else if( f < 0 ) i = -1;
			else i = 1;
			*l_sp = asDWORD(i);
			l_bc += BCS_CMPIf;
		}
		break;
	case BC_ADDIf:
		*(float*)l_sp = *(float*)l_sp + *(float*)(BCARG_DW(l_bc));
		l_bc += BCS_ADDIf;
		break;
	case BC_SUBIf:
		*(float*)l_sp = *(float*)l_sp - *(float*)(BCARG_DW(l_bc));
		l_bc += BCS_SUBIf;
		break;
	case BC_MULIi:
		*l_sp = asDWORD(int(asDWORD(*l_sp)) * *(int*)(BCARG_DW(l_bc)));;
		l_bc += BCS_MULIi;
		break;
	case BC_MULIf:
		*(float*)l_sp = *(float*)l_sp * *(float*)(BCARG_DW(l_bc));
		l_bc += BCS_MULIf;
		break;
	case BC_SUSPEND:
		l_bc += BCS_SUSPEND;
		if( doSuspend )
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			status = tsSuspended;
			return;
		}
		break;
	case BC_END:
		{
			// Need to move the values back to the context
			byteCode = l_bc;
			stackPointer = l_sp;
			stackFramePointer = l_fp;

			status = tsSuspended;
			return;
		}
		// Don't update the program position
		// l_bc += BCS_END;
		break;


/*
	default:
		// This Microsoft specific code allows the
		// compiler to optimize the switch case as
		// it will know that the code will never 
		// reach this point
		__assume(0);
*/	}
	}

	SetInternalException(TXT_UNRECOGNIZED_BYTE_CODE);
}

#endif // USE_ASM_VM

int asCContext::SetException(const char *descr)
{
	// Only allow this if we're executing a CALL byte code
	if( !isCallingSystemFunction ) return asERROR;

	SetInternalException(descr);

	return 0;
}

void asCContext::SetInternalException(const char *descr)
{
	if( inExceptionHandler )
	{
		assert(false); // Shouldn't happen
		return; // but if it does, at least this will not crash the application
	}

	exceptionString = descr;
	exceptionFunction = currentFunction->id;
	exceptionLine = currentFunction->GetLineNumber(byteCode - currentFunction->byteCode.AddressOf());

	// Clean up stack
	CleanStack();

	status = tsUnhandledException;
}

void asCContext::CleanStack()
{
	inExceptionHandler = true;

	// TODO: Increase the numActiveContexts when cleaning the stack

	// Run the clean up code for each of the functions called
	CleanStackFrame();

	while( callStack.GetLength() > 0 )
	{
		PopCallState();

		CleanStackFrame();
	}
	inExceptionHandler = false;
}

void asCContext::CleanStackFrame()
{
	// Find the ID for the exception location so that the cleanup function can do the right thing
	exceptionID = currentFunction->GetExceptionID(byteCode - currentFunction->byteCode.AddressOf());

	// Store state
	asBYTE *oldByteCode = byteCode;

	// Execute clean up code
	byteCode = currentFunction->cleanCode.AddressOf();

#ifdef USE_ASM_VM
	while( *(asDWORD*)byteCode != relocTable[BC_END] ) ExecuteNext();
#else
	while( *byteCode != BC_END ) ExecuteNext();
#endif

	// Restore state
	byteCode = oldByteCode;
}

int asCContext::GetExceptionLineNumber()
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	return exceptionLine;
}

int asCContext::GetExceptionFunction()
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	return exceptionFunction;
}

int asCContext::GetCurrentFunction()
{
	if( status == tsSuspended || status == tsActive )
		return currentFunction->id;

	return -1;
}

int asCContext::GetCurrentLineNumber()
{
	if( status == tsSuspended || status == tsActive )
	{
		return currentFunction->GetLineNumber(byteCode - currentFunction->byteCode.AddressOf());
	}

	return -1;
}

const char *asCContext::GetExceptionString(int *length)
{
	if( GetState() != asEXECUTION_EXCEPTION ) return 0;

	if( length ) *length = exceptionString.GetLength();

	return exceptionString.AddressOf();
}

#ifdef AS_DEPRECATED
int asCContext::GetExceptionString(char *buffer, int bufferSize)
{
	if( GetState() != asEXECUTION_EXCEPTION ) return asERROR;

	memcpy(buffer, exceptionString.AddressOf(), bufferSize < (exceptionString.GetLength() + 1) ? bufferSize : (exceptionString.GetLength() + 1));
	buffer[bufferSize-1] = 0;

	return exceptionString.GetLength();
}
#endif

int asCContext::GetState()
{
	if( status == tsSuspended )
		return asEXECUTION_SUSPENDED;

	if( status == tsActive )
		return asEXECUTION_ACTIVE;

	if( status == tsUnhandledException )
		return asEXECUTION_EXCEPTION;

	if( status == tsProgramFinished )
		return asEXECUTION_FINISHED;

	if( status == tsPrepared )
		return asEXECUTION_PREPARED;

	if( status == tsUninitialized )
		return asEXECUTION_UNINITIALIZED;

	return asERROR;
}

