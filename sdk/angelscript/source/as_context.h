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
// as_context.h
//
// This class handles the execution of the byte code
//


#ifndef AS_CONTEXT_H
#define AS_CONTEXT_H

#include "as_config.h"
#include "as_thread.h"
#include "as_array.h"
#include "as_string.h"
#include "as_types.h"

class asCScriptFunction;
class asCScriptEngine;
class asCModule;

class asCContext : public asIScriptContext
{
public:
	asCContext(asCScriptEngine *engine, bool holdRef);
	virtual ~asCContext();

	// Memory management
	int  AddRef();
	int  Release();

	asIScriptEngine *GetEngine();

	int  Prepare(int functionID);
	int  PrepareSpecial(int functionID);
	
	int  Execute();
	int  ExecuteStep(asDWORD flag);
	int  Abort();
	int  Suspend();

	int  SetArguments(int stackPos, asDWORD *data, int count);
	int  GetReturnValue(asDWORD *data, int count);

	int  GetState();

	int  GetCurrentLineNumber();
	int  GetCurrentFunction();

	int  GetExceptionLineNumber();
	int  GetExceptionFunction();
	const char *GetExceptionString(int *length);

#ifdef AS_DEPRECATED
	int  GetExceptionString(char *buffer, int bufferSize);
#endif

	int  SetException(const char *descr);

	int  SetExecuteStringFunction(asCScriptFunction *func);

//protected:
	friend class asCScriptEngine;
	friend int CallSystemFunction(int id, asCContext *context);
#ifdef USE_ASM_VM
	friend void _setInternalException(asCContext& context, int s);

	static void CreateRelocTable(void);
#endif

	void DetachEngine();

	void ExecuteNext(bool createRelocTable = false);
	void CleanStack();
	void CleanStackFrame();

	void PushCallState();
	void PopCallState();
	void CallScriptFunction(asCModule *mod, asCScriptFunction *func);

	void SetInternalException(const char *descr);

	// Must be protected for multiple accesses
	int refCount;

	bool holdEngineRef;
	asCScriptEngine *engine;
	asCModule *module;

	int status;
	bool doSuspend;
	bool doAbort;
	bool externalSuspendRequest;
	bool isCallingSystemFunction;

	asBYTE *byteCode;

	asCScriptFunction *currentFunction;
	asDWORD *stackFramePointer;
	int exceptionID;

	asDWORD tempReg;
	asQWORD returnVal;

	asCArray<int> callStack;
	asCArray<asDWORD *> stackBlocks;
	asDWORD *stackPointer;
	int stackBlockSize; 
	int stackIndex;

	bool inExceptionHandler;
	asCString exceptionString;
	int exceptionFunction;
	int exceptionLine;

	int returnValueSize;
	int argumentsSize;

	// String function
	asCScriptFunction *stringFunction;
	
	asCScriptFunction *initialFunction;

	DECLARECRITICALSECTION(criticalSection);
};

enum eContextState
{
	tsUninitialized,
	tsPrepared,
	tsSuspended,
	tsActive,
	tsProgramFinished,
	tsProgramAborted,
	tsUnhandledException,
};


#endif
