/*
   AngelCode Scripting Library
   Copyright (c) 2003-2011 Andreas Jonsson

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

/*
 * Implements the AMD64 calling convention for gcc-based 64bit Unices
 *
 * Author: Ionut "gargltk" Leonte <ileonte@bitdefender.com>
 *
 * Initial author: niteice
 */

#include "as_config.h"

#ifndef AS_MAX_PORTABILITY
#ifdef AS_X64_GCC

#include "as_scriptengine.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

enum argTypes { x64ENDARG = 0, x64INTARG = 1, x64FLOATARG = 2, x64DOUBLEARG = 3, x64VARIABLE = 4 };
typedef asQWORD ( *funcptr_t )( void );

#define X64_MAX_ARGS             32
#define MAX_CALL_INT_REGISTERS    6
#define MAX_CALL_SSE_REGISTERS    8
#define CALLSTACK_MULTIPLIER      2
#define X64_CALLSTACK_SIZE        ( X64_MAX_ARGS + MAX_CALL_SSE_REGISTERS + 3 )

// TODO: Should this really be different on Mac and other systems? Probably the Mac way is the correct one
#if defined(AS_MAC)
#define PUSH_LONG( val )                         \
	__asm__ __volatile__ (                       \
		"movq   %0, %%rax\n"                     \
		"pushq  %%rax"                           \
		:                                        \
		: "m" ( val )                            \
	)

#define POP_LONG( reg )                          \
	__asm__ __volatile__ (                       \
		"popq     %rax\n"                        \
		"movq     %rax, " reg                    \
	)


#define ASM_GET_REG( name, dest )                \
	__asm__ __volatile__ (                       \
		"movq %" name ", %0\n"                   \
		:                                        \
		: "m" ( dest )                           \
	)
#else
#define PUSH_LONG( val )                         \
	__asm__ __volatile__ (                       \
		"mov    %0, %%rax\n"                     \
		"push   %%rax"                           \
		:                                        \
		: "m" ( val )                            \
	)

#define POP_LONG( reg )                          \
	__asm__ __volatile__ (                       \
		"popq     %rax\n"                        \
		"movq     %rax, " reg                    \
	)


#define ASM_GET_REG( name, dest )                \
	__asm__ __volatile__ (                       \
		"mov  %" name ", %0\n"                   \
		:                                        \
		: "m" ( dest )                           \
	)
#endif

static asDWORD GetReturnedFloat()
{
	float   retval = 0.0f;
	asDWORD ret    = 0;

	__asm__ __volatile__ (
		"lea      %0, %%rax\n"
		"movss    %%xmm0, (%%rax)"
		: /* no output */
		: "m" (retval)
		: "%rax"
	);

	// We need to avoid implicit conversions from float to unsigned - we need
	// a bit-wise-correct-and-complete copy of the value 
	memcpy( &ret, &retval, sizeof( ret ) );

	return ( asDWORD )ret;
}

static asQWORD GetReturnedDouble()
{
	double  retval = 0.0f;
	asQWORD ret    = 0;

	__asm__ __volatile__ (
		"lea     %0, %%rax\n"
		"movlpd  %%xmm0, (%%rax)"
		: /* no optput */
		: "m" (retval)
		: "%rax"
	);

	// We need to avoid implicit conversions from double to unsigned long long - we need
	// a bit-wise-correct-and-complete copy of the value 
	memcpy( &ret, &retval, sizeof( ret ) );

	return ret;
}

static asQWORD __attribute ((__noinline__)) X64_CallFunction( const asDWORD* pArgs, const asBYTE *pArgsType, void *func )
{
	asQWORD retval      = 0;
	asQWORD ( *call )() = (asQWORD (*)())func;
	int     i           = 0;

	/* push the stack parameters */
	for ( i = MAX_CALL_INT_REGISTERS + MAX_CALL_SSE_REGISTERS; pArgsType[i] != x64ENDARG && ( i < X64_MAX_ARGS + MAX_CALL_SSE_REGISTERS + 3 ); i++ ) {
		PUSH_LONG( pArgs[i * CALLSTACK_MULTIPLIER] );
	}

	/* push integer parameters */
	for ( i = 0; i < MAX_CALL_INT_REGISTERS; i++ ) {
		PUSH_LONG( pArgs[i * CALLSTACK_MULTIPLIER] );
	}

	/* push floating point parameters */
	for ( i = MAX_CALL_INT_REGISTERS; i < MAX_CALL_INT_REGISTERS + MAX_CALL_SSE_REGISTERS; i++ ) {
		PUSH_LONG( pArgs[i * CALLSTACK_MULTIPLIER] );
	}

	/* now pop the registers in reverse order and make the call */
	POP_LONG( "%xmm7" );
	POP_LONG( "%xmm6" );
	POP_LONG( "%xmm5" );
	POP_LONG( "%xmm4" );
	POP_LONG( "%xmm3" );
	POP_LONG( "%xmm2" );
	POP_LONG( "%xmm1" );
	POP_LONG( "%xmm0" );

	POP_LONG( "%r9" );
	POP_LONG( "%r8" );
	POP_LONG( "%rcx" );
	POP_LONG( "%rdx" );
	POP_LONG( "%rsi" );
	POP_LONG( "%rdi" );

	// call the function with the arguments
	retval = call();
	return retval;
}

// returns true if the given parameter is a 'variable argument'
inline bool IsVariableArgument( asCDataType type )
{
	return ( type.GetTokenType() == ttQuestion ) ? true : false;
}

asQWORD CallSystemFunctionNative(asCContext *context, asCScriptFunction *descr, void *obj, asDWORD *args, void *retPointer, asQWORD &retQW2)
{
	asSSystemFunctionInterface *sysFunc            = descr->sysFuncIntf;
	int                         callConv           = sysFunc->callConv;

	asQWORD                     retQW              = 0;
	void                       *func               = ( void * )sysFunc->func;
	asDWORD                    *stack_pointer      = args;
	funcptr_t                  *vftable            = NULL;
	int                         totalArgumentCount = 0;
	int                         n                  = 0;
	int                         base_n             = 0;
	int                         a                  = 0;
	int                         param_pre          = 0;
	int                         param_post         = 0;
	int                         argIndex           = 0;
	int                         argumentCount      = 0;

	asDWORD  tempBuff[CALLSTACK_MULTIPLIER * X64_CALLSTACK_SIZE] = { 0 };
	asBYTE   tempType[X64_CALLSTACK_SIZE] = { 0 };

	asDWORD  paramBuffer[CALLSTACK_MULTIPLIER * X64_CALLSTACK_SIZE] = { 0 };
	asBYTE	 argsType[X64_CALLSTACK_SIZE] = { 0 };

	asBYTE   argsSet[X64_CALLSTACK_SIZE]  = { 0 };

	if( sysFunc->hostReturnInMemory ) {
		// The return is made in memory
		callConv++;
	}

	argumentCount = ( int )descr->parameterTypes.GetLength();
	assert( argumentCount <= X64_MAX_ARGS );

	// TODO: optimize: argsType should be computed in PrepareSystemFunction
	for( a = 0; a < argumentCount; ++a, ++argIndex ) {
		// get the base type
		argsType[argIndex] = x64INTARG;
		if ( descr->parameterTypes[a].IsFloatType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64FLOATARG;
		}
		if ( descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64DOUBLEARG;
		}
		if ( descr->parameterTypes[a].GetSizeOnStackDWords() == 2 && !descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64INTARG;
		}

		if ( IsVariableArgument( descr->parameterTypes[a] ) ) {
			argsType[argIndex] = x64VARIABLE;
		}
	}
	assert( argIndex == argumentCount );

	for ( a = 0; a < argumentCount && totalArgumentCount <= X64_MAX_ARGS; a++ ) {
		switch ( argsType[a] ) {
			case x64ENDARG:
			case x64INTARG:
			case x64FLOATARG:
			case x64DOUBLEARG: {
				if ( totalArgumentCount < X64_MAX_ARGS )
					tempType[totalArgumentCount++] = argsType[a];
				break;
			}
			case x64VARIABLE: {
				if ( totalArgumentCount < X64_MAX_ARGS )
					tempType[totalArgumentCount++] = x64VARIABLE;
				if ( totalArgumentCount < X64_MAX_ARGS )
					tempType[totalArgumentCount++] = x64INTARG;
				break;
			}
		}
	}

	assert( totalArgumentCount <= X64_MAX_ARGS );
	if ( totalArgumentCount > argumentCount ) {
		memcpy( argsType, tempType, totalArgumentCount );
	}
	memset( tempType, 0, sizeof( tempType ) );

	// TODO: This should be checked in PrepareSystemFunction
#ifndef COMPLEX_OBJS_PASSED_BY_REF
	if( sysFunc->takesObjByVal ) {
		/* I currently know of no way we can predict register usage for passing complex
		   objects by value when the compiler does not pass them by reference instead. I
		   will quote the example from the AMD64 ABI to demonstrate this:

		   (http://www.x86-64.org/documentation/abi.pdf - page 22)

		------------------------------ BEGIN EXAMPLE -------------------------------

		Let us consider the following C code:

		typedef struct {
			int a, b;
			double d;
		} structparm;

		structparm s;
		int e, f, g, h, i, j, k;
		long double ld;
		double m, n;

		extern void func (int e, int f,
			structparm s, int g, int h,
			long double ld, double m,
			double n, int i, int j, int k);

		func (e, f, s, g, h, ld, m, n, i, j, k);

		Register allocation for the call:
		--------------------------+--------------------------+-------------------
		General Purpose Registers | Floating Point Registers | Stack Frame Offset
		--------------------------+--------------------------+-------------------
		 %rdi: e                  | %xmm0: s.d               | 0:  ld
		 %rsi: f                  | %xmm1: m                 | 16: j
		 %rdx: s.a,s.b            | %xmm2: n                 | 24: k
		 %rcx: g                  |                          |
		 %r8:  h                  |                          |
		 %r9:  i                  |                          |
		--------------------------+--------------------------+-------------------
		*/

		context->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
		return 0;
	}
#endif

	if ( obj && ( callConv == ICC_VIRTUAL_THISCALL || callConv == ICC_VIRTUAL_THISCALL_RETURNINMEM ) ) {
		vftable = *( ( funcptr_t ** )obj );
		func    = ( void * )vftable[( asQWORD )func >> 3];
	}

	switch ( callConv ) {
		case ICC_CDECL_RETURNINMEM:
		case ICC_STDCALL_RETURNINMEM: {
			if ( totalArgumentCount ) {
				memmove( argsType + 1, argsType, totalArgumentCount );
			}
			memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
			argsType[0] = x64INTARG;
			base_n = 1;

			param_pre = 1;

			break;
		}
		case ICC_THISCALL:
		case ICC_VIRTUAL_THISCALL:
		case ICC_CDECL_OBJFIRST: {
			if ( totalArgumentCount ) {
				memmove( argsType + 1, argsType, totalArgumentCount );
			}
			memcpy( paramBuffer, &obj, sizeof( obj ) );
			argsType[0] = x64INTARG;

			param_pre = 1;

			break;
		}
		case ICC_THISCALL_RETURNINMEM:
		case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		case ICC_CDECL_OBJFIRST_RETURNINMEM: {
			if ( totalArgumentCount ) {
				memmove( argsType + 2, argsType, totalArgumentCount );
			}
			memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
			memcpy( paramBuffer + CALLSTACK_MULTIPLIER, &obj, sizeof( &obj ) );
			argsType[0] = x64INTARG;
			argsType[1] = x64INTARG;

			param_pre = 2;

			break;
		}
		case ICC_CDECL_OBJLAST: {
			memcpy( paramBuffer + totalArgumentCount * CALLSTACK_MULTIPLIER, &obj, sizeof( obj ) );
			argsType[totalArgumentCount] = x64INTARG;

			param_post = 1;

			break;
		}
		case ICC_CDECL_OBJLAST_RETURNINMEM: {
			if ( totalArgumentCount ) {
				memmove( argsType + 1, argsType, totalArgumentCount );
			}
			memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
			argsType[0] = x64INTARG;
			memcpy( paramBuffer + ( totalArgumentCount + 1 ) * CALLSTACK_MULTIPLIER, &obj, sizeof( obj ) );
			argsType[totalArgumentCount + 1] = x64INTARG;

			param_pre = 1;
			param_post = 1;

			break;
		}
		default: {
			base_n = 0;
			break;
		}
	}

	int adjust = 0;
	for( n = 0; n < ( int )( param_pre + totalArgumentCount + param_post ); n++ ) {
		int copy_count = 0;
		if ( n >= param_pre && n < ( int )( param_pre + totalArgumentCount ) ) {
			copy_count = descr->parameterTypes[n - param_pre - adjust].GetSizeOnStackDWords();

			if ( argsType[n] == x64VARIABLE ) {
				adjust += 1;
				argsType[n] = x64INTARG;
				n += 1;
			}
		}
		if ( copy_count > CALLSTACK_MULTIPLIER ) {
			if ( copy_count > CALLSTACK_MULTIPLIER + 1 ) {
				context->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
				return 0;
			}

			memcpy( paramBuffer + ( n - 1 ) * CALLSTACK_MULTIPLIER, stack_pointer, AS_PTR_SIZE * sizeof( asDWORD ) );
			stack_pointer += AS_PTR_SIZE;
			memcpy( paramBuffer + n * CALLSTACK_MULTIPLIER, stack_pointer, sizeof( asDWORD ) );
			stack_pointer += 1;
		} else {
			if ( copy_count ) {
				memcpy( paramBuffer + n * CALLSTACK_MULTIPLIER, stack_pointer, copy_count * sizeof( asDWORD ) );
				stack_pointer += copy_count;
			}
		}
	}

	// If we are returning a object not by reference, we need to make the
	// pointer to the space allocated to the object the first parameter.
	if( descr->returnType.IsObject() && ( descr->returnType.GetObjectType()->flags & asOBJ_APP_CLASS_CA ) == asOBJ_APP_CLASS_CA &&
		!descr->returnType.IsReference() && !sysFunc->hostReturnInMemory )
	{
		if ( totalArgumentCount )
		{
			memmove( paramBuffer + CALLSTACK_MULTIPLIER, paramBuffer, ( CALLSTACK_MULTIPLIER * ( X64_CALLSTACK_SIZE - 1 ) ) );
			memmove( argsType + 1, argsType, X64_CALLSTACK_SIZE - 1 );
		}
		memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
		argsType[ 0 ] = x64INTARG;
	}

	/*
	 * Q: WTF is going on here !?
	 *
	 * A: The idea is to pre-arange the parameters so that X64_CallFunction() can do
	 * it's little magic which must work regardless of how the compiler decides to
	 * allocate registers. Basically:
	 * - the first MAX_CALL_INT_REGISTERS entries in tempBuff and tempType will
	 *   contain the values/types of the x64INTARG parameters - that is the ones who
	 *   go into the registers. If the function has less then MAX_CALL_INT_REGISTERS
	 *   integer parameters then the last entries will be set to 0
	 * - the next MAX_CALL_SSE_REGISTERS entries will contain the float/double arguments
	 *   that go into the floating point registers. If the function has less than
	 *   MAX_CALL_SSE_REGISTERS floating point parameters then the last entries will
	 *   be set to 0
	 * - index MAX_CALL_INT_REGISTERS + MAX_CALL_SSE_REGISTERS marks the start of the
	 *   parameters which will get passed on the stack. These are added to the array
	 *   in reverse order so that X64_CallFunction() can simply push them to the stack
	 *   without the need to perform further tests
	 */
	int     used_int_regs = 0;
	int     used_sse_regs = 0;
	int     idx           = 0;
	base_n = 0;
	for ( n = 0; ( n < X64_CALLSTACK_SIZE ) && ( used_int_regs < MAX_CALL_INT_REGISTERS ); n++ ) {
		if ( argsType[n] == x64INTARG ) {
			idx = base_n;
			argsSet[n] = 1;
			tempType[idx] = argsType[n];
			memcpy( tempBuff + idx * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
			used_int_regs++;
		}
	}
	base_n = 0;
	for ( n = 0; ( n < X64_CALLSTACK_SIZE ) && ( used_sse_regs < MAX_CALL_SSE_REGISTERS ); n++ ) {
		if ( argsType[n] == x64FLOATARG || argsType[n] == x64DOUBLEARG ) {
			idx = MAX_CALL_INT_REGISTERS + base_n;
			argsSet[n] = 1;
			tempType[idx] = argsType[n];
			memcpy( tempBuff + idx * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
			used_sse_regs++;
		}
	}
	base_n = 0;
	for ( n = X64_CALLSTACK_SIZE - 1; n >= 0; n-- ) {
		if ( argsType[n] != x64ENDARG && !argsSet[n] ) {
			idx = MAX_CALL_INT_REGISTERS + MAX_CALL_SSE_REGISTERS + base_n;
			argsSet[n] = 1;
			tempType[idx] = argsType[n];
			memcpy( tempBuff + idx * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
		}
	}

	context->isCallingSystemFunction = true;
	retQW = X64_CallFunction( tempBuff, tempType, ( asDWORD * )func );
	ASM_GET_REG( "%rdx", retQW2 );
	context->isCallingSystemFunction = false;

	// If the return is a float value we need to get the value from the FP register
	if( sysFunc->hostReturnFloat )
	{
		if( sysFunc->hostReturnSize == 1 )
			*(asDWORD*)&retQW = GetReturnedFloat();
		else
			retQW = GetReturnedDouble();
	}

	return retQW;
}

int CallSystemFunction(int id, asCContext *context, void *objectPointer)
{
	asCScriptEngine            *engine  = context->engine;
	asCScriptFunction          *descr   = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc = descr->sysFuncIntf;

	int callConv = sysFunc->callConv;
	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW             = 0;
	asQWORD  retQW2            = 0;
	asDWORD *args              = context->regs.stackPointer;
	void    *retPointer        = 0;
	void    *obj               = 0;
	int      popSize           = sysFunc->paramSize;

	context->regs.objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() )
	{
		// Allocate the memory for the object
		retPointer = engine->CallAlloc(descr->returnType.GetObjectType());
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
			popSize += AS_PTR_SIZE;

			// Check for null pointer
			obj = (void*)*(size_t*)(args);
			if( obj == 0 )
			{
				context->SetInternalException(TXT_NULL_POINTER_ACCESS);
				if( retPointer )
					engine->CallFree(retPointer);
				return 0;
			}

			// Add the base offset for multiple inheritance
			obj = (void*)(size_t(obj) + sysFunc->baseOffset);

			// Skip the object pointer
			args += AS_PTR_SIZE;
		}
	}

	retQW = CallSystemFunctionNative(context, descr, obj, args, sysFunc->hostReturnInMemory ? retPointer : 0, retQW2);

#ifdef COMPLEX_OBJS_PASSED_BY_REF
	if( sysFunc->takesObjByVal )
	{
		// Need to free the complex objects passed by value
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args += AS_PTR_SIZE;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( descr->parameterTypes[n].IsObject() &&
				!descr->parameterTypes[n].IsReference() &&
				(descr->parameterTypes[n].GetObjectType()->flags & COMPLEX_MASK) )
			{
				void *obj = (void*)*(size_t*)&args[spos];
				spos += AS_PTR_SIZE;
				asSTypeBehaviour *beh = &descr->parameterTypes[n].GetObjectType()->beh;
				if( beh->destruct )
					engine->CallObjectMethod(obj, beh->destruct);

				engine->CallFree(obj);
			}
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}
#endif

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() )
	{
		if( descr->returnType.IsObjectHandle() )
		{
#if defined(AS_BIG_ENDIAN) && AS_PTR_SIZE == 1
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;
#endif

			context->regs.objectRegister = (void*)(size_t)retQW;

			if( sysFunc->returnAutoHandle && context->regs.objectRegister )
				engine->CallObjectMethod(context->regs.objectRegister, descr->returnType.GetObjectType()->beh.addref);
		}
		else
		{
			if( !sysFunc->hostReturnInMemory )
			{
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 )
				{
#if defined(AS_BIG_ENDIAN) && AS_PTR_SIZE == 1
					// Since we're treating the system function as if it is returning a QWORD we are
					// actually receiving the value in the high DWORD of retQW.
					retQW >>= 32;
#endif

					*(asDWORD*)retPointer = (asDWORD)retQW;
				}
				else if( sysFunc->hostReturnSize == 2 )
					*(asQWORD*)retPointer = retQW;
				else if( sysFunc->hostReturnSize == 3 )
				{
					*(asQWORD*)retPointer         = retQW;
					*(((asDWORD*)retPointer) + 2) = (asDWORD)retQW2;
				}
				else // if( sysFunc->hostReturnSize == 4 )
				{
					*(asQWORD*)retPointer         = retQW;
					*(((asQWORD*)retPointer) + 1) = retQW2;
				}
			}

			// Store the object in the register
			context->regs.objectRegister = retPointer;
		}
	}
	else
	{
		// Store value in value register
		if( sysFunc->hostReturnSize == 1 )
		{
#if defined(AS_BIG_ENDIAN)
			// Since we're treating the system function as if it is returning a QWORD we are
			// actually receiving the value in the high DWORD of retQW.
			retQW >>= 32;

			// due to endian issues we need to handle return values, that are
			// less than a DWORD (32 bits) in size, special
			int numBytes = descr->returnType.GetSizeInMemoryBytes();
			if( descr->returnType.IsReference() ) numBytes = 4;
			switch( numBytes )
			{
			case 1:
				{
					// 8 bits
					asBYTE *val = (asBYTE*)&context->regs.valueRegister;
					val[0] = (asBYTE)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
					val[4] = 0;
					val[5] = 0;
					val[6] = 0;
					val[7] = 0;
				}
				break;
			case 2:
				{
					// 16 bits
					asWORD *val = (asWORD*)&context->regs.valueRegister;
					val[0] = (asWORD)retQW;
					val[1] = 0;
					val[2] = 0;
					val[3] = 0;
				}
				break;
			default:
				{
					// 32 bits
					asDWORD *val = (asDWORD*)&context->regs.valueRegister;
					val[0] = (asDWORD)retQW;
					val[1] = 0;
				}
				break;
			}
#else
			*(asDWORD*)&context->regs.valueRegister = (asDWORD)retQW;
#endif
		}
		else
			context->regs.valueRegister = retQW;
	}

	// Release autohandles in the arguments
	if( sysFunc->hasAutoHandles )
	{
		args = context->regs.stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer )
			args += AS_PTR_SIZE;

		int spos = 0;
		for( asUINT n = 0; n < descr->parameterTypes.GetLength(); n++ )
		{
			if( sysFunc->paramAutoHandles[n] && *(size_t*)&args[spos] != 0 )
			{
				// Call the release method on the type
				engine->CallObjectMethod((void*)*(size_t*)&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release);
				*(size_t*)&args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() )
				spos += AS_PTR_SIZE;
			else
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
		}
	}

	return popSize;
}

END_AS_NAMESPACE

#endif // AS_X64_GCC
#endif // AS_MAX_PORTABILITY
