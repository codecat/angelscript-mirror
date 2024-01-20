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


#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_RISCV64

#include "as_callfunc.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_tokendef.h"
#include "as_context.h"

BEGIN_AS_NAMESPACE

extern "C" asQWORD CallRiscVFunc(asFUNCTION_t func);

asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *descr, void *obj, asDWORD *args, void *retPointer, asQWORD &retQW2, void *secondObject)
{
	asCScriptEngine *engine = context->m_engine;
	const asSSystemFunctionInterface *const sysFunc = descr->sysFuncIntf;
	const asCDataType &retType = descr->returnType;
	const asCTypeInfo *const retTypeInfo = retType.GetTypeInfo();
	asFUNCTION_t func = sysFunc->func;
	int callConv = sysFunc->callConv;
	asQWORD       retQW     = 0;
	
	// TODO: prepare arguments for loading into CPU registers
	// TODO: retrieve correct function pointer to call (e.g. from virtual function table, auxiliary pointer, etc)
	// TODO: read return value from float register if needed

	retQW = CallRiscVFunc(func);

	return retQW;
}

END_AS_NAMESPACE

#endif // AS_RISCV64
#endif // AS_MAX_PORTABILITY




