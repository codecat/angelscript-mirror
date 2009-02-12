/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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

enum argTypes { x64ENDARG = 0, x64INTARG = 1, x64FLOATARG = 2, x64DOUBLEARG = 3, x64VARIABLE = 4 };
typedef asQWORD ( *funcptr_t )( void );

#define X64_MAX_ARGS             32
#define MAX_CALL_INT_REGISTERS    6
#define MAX_CALL_SSE_REGISTERS    8
#define CALLSTACK_MULTIPLIER      2
#define X64_CALLSTACK_SIZE        ( X64_MAX_ARGS + MAX_CALL_SSE_REGISTERS + 3 )

#define PUSH_LONG( val )                                 \
	__asm__ __volatile__ (                           \
		"mov    %0, %%rax\r\n"                   \
		"push   %%rax"                           \
		:                                        \
		: "m" ( val )                            \
	)

#define POP_LONG( reg )                                  \
	__asm__ __volatile__ (                           \
		"popq     %rax\r\n"                      \
		"movq     %rax, " reg                    \
	)


#define ASM_GET_REG( name, dest )                        \
	__asm__ __volatile__ (                           \
		"mov  %" name ", %0"                     \
		: "=r" ( dest )                          \
	)

BEGIN_AS_NAMESPACE

static asDWORD GetReturnedFloat()
{
	float   retval = 0.0f;
	asDWORD ret    = 0;

	__asm__ __volatile__ (
		"lea      %0, %%rax\r\n"
		"movss    %%xmm0, (%%rax)"
		: /* no output */
		: "m" (retval)
		: "%rax"
	);

	/* We need to avoid implicit conversions from float to unsigned - we need
	   a bit-wise-correct-and-complete copy of the value */
	memcpy( &ret, &retval, sizeof( ret ) );

	return ( asDWORD )ret;
}

static asQWORD GetReturnedDouble()
{
	double  retval = 0.0f;
	asQWORD ret    = 0;

	__asm__ __volatile__ (
		"lea     %0, %%rax\r\n"
		"movlpd  %%xmm0, (%%rax)"
		: /* no optput */
		: "m" (retval)
		: "%rax"
	);
	/* We need to avoid implicit conversions from double to unsigned long long - we need
	   a bit-wise-correct-and-complete copy of the value */
	memcpy( &ret, &retval, sizeof( ret ) );

	return ret;
}

static asQWORD X64_CallFunction( const asDWORD* pArgs, const asBYTE *pArgsType, void *func )
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

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction( asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/ )
{
	// References are always returned as primitive data
	if ( func->returnType.IsReference() || func->returnType.IsObjectHandle() ) {
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = false;
	} else if ( func->returnType.IsObject() ) {
		// Registered types have special flags that determine how they are returned
		asDWORD objType = func->returnType.GetObjectType()->flags;
		if ( ( objType & asOBJ_VALUE ) && ( objType & asOBJ_APP_CLASS ) ) {
			if( objType & COMPLEX_MASK ) {
				internal->hostReturnInMemory = true;
				internal->hostReturnSize = 1;
				internal->hostReturnFloat = false;
			} else {
				internal->hostReturnFloat = false;
				if( func->returnType.GetSizeInMemoryDWords() > 2 ) {
					internal->hostReturnInMemory = true;
					internal->hostReturnSize = 1;
				} else {
					internal->hostReturnInMemory = false;
					internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
				}
			}
		} else if ( ( objType & asOBJ_VALUE ) && ( objType & asOBJ_APP_PRIMITIVE ) ) {
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = false;
		} else if ( ( objType & asOBJ_VALUE ) && ( objType & asOBJ_APP_FLOAT ) ) {
			internal->hostReturnInMemory = false;
			internal->hostReturnSize = func->returnType.GetSizeInMemoryDWords();
			internal->hostReturnFloat = true;
		}
	} else if ( func->returnType.GetSizeInMemoryDWords() > 2 ) {
		// Shouldn't be possible to get here
		asASSERT(false);
	} else if ( func->returnType.GetSizeInMemoryDWords() == 2 ) {
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 2;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst( asCDataType::CreatePrimitive( ttDouble, true ) );
	} else if( func->returnType.GetSizeInMemoryDWords() == 1 ) {
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 1;
		internal->hostReturnFloat = func->returnType.IsEqualExceptConst( asCDataType::CreatePrimitive( ttFloat, true ) );
	} else {
		internal->hostReturnInMemory = false;
		internal->hostReturnSize = 0;
		internal->hostReturnFloat = false;
	}

	// Calculate the size needed for the parameters
	internal->paramSize = func->GetSpaceNeededForArguments();

	// Verify if the function takes any objects by value
	asUINT n;
	internal->takesObjByVal = false;
	for( n = 0; n < func->parameterTypes.GetLength(); n++ ) {
		if( func->parameterTypes[n].IsObject() && !func->parameterTypes[n].IsObjectHandle() && !func->parameterTypes[n].IsReference() ) {
			internal->takesObjByVal = true;
			break;
		}
	}

	// Verify if the function has any registered autohandles
	internal->hasAutoHandles = false;
	for( n = 0; n < internal->paramAutoHandles.GetLength(); n++ ) {
		if( internal->paramAutoHandles[n] ) {
			internal->hasAutoHandles = true;
			break;
		}
	}

	return 0;
}

// returns true if the given parameter is a 'variable argument'
inline bool IsVariableArgument( asCDataType type )
{
	return ( type.GetTokenType() == ttQuestion ) ? true : false;
}

int CallSystemFunction( int id, asCContext *context, void *objectPointer )
{
	asCScriptEngine            *engine             = context->engine;
	asCScriptFunction          *descr              = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc            = engine->scriptFunctions[id]->sysFuncIntf;
	int                         callConv           = sysFunc->callConv;

	asQWORD                     retQW              = 0;
	asQWORD                     retQW2             = 0;
	void                       *func               = ( void * )sysFunc->func;
	int                         paramSize          = sysFunc->paramSize;
	asDWORD                    *args               = context->stackPointer;
	asDWORD                    *stack_pointer      = context->stackPointer;
	void                       *retPointer         = 0;
	void                       *obj                = 0;
	funcptr_t                  *vftable            = NULL;
	int                         popSize            = paramSize;
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

	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD ) {
		return context->CallGeneric( id, objectPointer );
	}

	context->objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() ) {
		// Allocate the memory for the object
		retPointer = engine->CallAlloc( descr->returnType.GetObjectType() );

		if( sysFunc->hostReturnInMemory ) {
			// The return is made in memory
			callConv++;
		}
	}

	argumentCount = ( int )descr->parameterTypes.GetLength();
	assert( argumentCount <= X64_MAX_ARGS );

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

	if( sysFunc->takesObjByVal ) {
		context->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
		return 0;
	}

	obj = objectPointer;
	if ( !obj && callConv >= ICC_THISCALL ) {
		// The object pointer should be popped from the context stack
		popSize += PTR_SIZE;

		// Check for null pointer
		obj = ( void * )( *( ( asQWORD * )( args ) ) );
		stack_pointer += PTR_SIZE;
		if( !obj ) {
			context->SetInternalException( TXT_NULL_POINTER_ACCESS );
			if( retPointer ) {
				engine->CallFree( retPointer );
			}
			return 0;
		}

		// Add the base offset for multiple inheritance
		obj = ( void * )( ( asQWORD )obj + sysFunc->baseOffset );
	}

	if ( obj && ( callConv == ICC_VIRTUAL_THISCALL || callConv == ICC_VIRTUAL_THISCALL_RETURNINMEM ) ) {
		vftable = *( ( funcptr_t ** )obj );
		func    = ( void * )vftable[( asQWORD )func >> 3];
	}

	switch ( callConv ) {
		case ICC_CDECL_RETURNINMEM:
		case ICC_STDCALL_RETURNINMEM: {
			memmove( paramBuffer + CALLSTACK_MULTIPLIER, paramBuffer, CALLSTACK_MULTIPLIER * totalArgumentCount * sizeof( asDWORD ) );
			memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
			memmove( argsType + 1, argsType, totalArgumentCount );
			argsType[0] = x64INTARG;
			base_n = 1;
			param_pre = 1;
			break;
		}
		case ICC_THISCALL:
		case ICC_THISCALL_RETURNINMEM:
		case ICC_VIRTUAL_THISCALL:
		case ICC_VIRTUAL_THISCALL_RETURNINMEM:
		case ICC_CDECL_OBJFIRST:
		case ICC_CDECL_OBJFIRST_RETURNINMEM: {
			bool           is_complex = ( descr->returnType.GetObjectType() && descr->returnType.GetObjectType()->size > 16 );
			unsigned short add_count  = ( retPointer && is_complex ? 2 : 1 );
			memmove( paramBuffer + add_count * CALLSTACK_MULTIPLIER, paramBuffer, CALLSTACK_MULTIPLIER * totalArgumentCount * sizeof( asDWORD ) );
			memmove( argsType + add_count, argsType, totalArgumentCount );
			if ( retPointer && is_complex ) {
				memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
				memcpy( paramBuffer + CALLSTACK_MULTIPLIER, &obj, sizeof( &obj ) );
				argsType[0] = x64INTARG;
				argsType[1] = x64INTARG;
			} else {
				memcpy( paramBuffer, &obj, sizeof( obj ) );
				argsType[0] = x64INTARG;
			}
			param_pre = add_count;
			break;
		}
		case ICC_CDECL_OBJLAST:
		case ICC_CDECL_OBJLAST_RETURNINMEM: {
			if ( retPointer ) {
				memcpy( argsType + 1, argsType, totalArgumentCount );
				memcpy( paramBuffer, &retPointer, sizeof( retPointer ) );
				argsType[0] = x64INTARG;
				memcpy( paramBuffer + ( totalArgumentCount + 1 ) * CALLSTACK_MULTIPLIER, &obj, sizeof( obj ) );
				argsType[totalArgumentCount + 1] = x64INTARG;

				param_pre = 1;
			} else {
				memcpy( paramBuffer + totalArgumentCount * CALLSTACK_MULTIPLIER, &obj, sizeof( obj ) );
				argsType[totalArgumentCount] = x64INTARG;
			}

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

			memcpy( paramBuffer + ( n - 1 ) * CALLSTACK_MULTIPLIER, stack_pointer, PTR_SIZE * sizeof( asDWORD ) );
			stack_pointer += PTR_SIZE;
			memcpy( paramBuffer + n * CALLSTACK_MULTIPLIER, stack_pointer, sizeof( asDWORD ) );
			stack_pointer += 1;
		} else {
			if ( copy_count ) {
				memcpy( paramBuffer + n * CALLSTACK_MULTIPLIER, stack_pointer, copy_count * sizeof( asDWORD ) );
				stack_pointer += copy_count;
			}
		}
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

	// Store the returned value in our stack
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() ) {
		if( descr->returnType.IsObjectHandle() ) {
			context->objectRegister = ( void * )( size_t )retQW;

			if( sysFunc->returnAutoHandle && context->objectRegister ) {
				engine->CallObjectMethod( context->objectRegister, descr->returnType.GetObjectType()->beh.addref );
			}
		} else {
			if( !sysFunc->hostReturnInMemory ) {
				// Copy the returned value to the pointer sent by the script engine
				if( sysFunc->hostReturnSize == 1 ) {
					*( asDWORD * )retPointer = ( asDWORD )retQW;
				} else {
					*( asQWORD * )retPointer = retQW;
				}
			} else {
				if ( descr->returnType.GetObjectType()->size == 16 && ( descr->returnType.GetObjectType()->flags & asOBJ_APP_CLASS_D ) != asOBJ_APP_CLASS_D ) {
					/* special case where the first eightbyte is returned in %rax and the second
					   eightbyte in %rdx */
					*( asQWORD * )retPointer             = retQW;
					*( ( ( asQWORD * )retPointer ) + 1 ) = retQW2;
				}
			}

			// Store the object in the register
			context->objectRegister = retPointer;
		}
	} else {
		// Store value in register1 register
		if( sysFunc->hostReturnFloat ) {
			if( sysFunc->hostReturnSize == 1 ) {
				*(asDWORD*)&context->register1 = GetReturnedFloat();
			} else {
				context->register1 = GetReturnedDouble();
			}
		} else if ( sysFunc->hostReturnSize == 1 ) {
			*( asDWORD * )&context->register1 = ( asDWORD )retQW;
		} else {
			context->register1 = retQW;
		}
	}

	if( sysFunc->hasAutoHandles ) {
		args = context->stackPointer;
		if( callConv >= ICC_THISCALL && !objectPointer ) {
			args++;
		}

		int spos = 0;
		for( n = 0; n < ( int )descr->parameterTypes.GetLength(); n++ ) {
			if( sysFunc->paramAutoHandles[n] && args[spos] ) {
				// Call the release method on the type
				engine->CallObjectMethod( ( void * )*( size_t * )&args[spos], descr->parameterTypes[n].GetObjectType()->beh.release );
				args[spos] = 0;
			}

			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].IsObjectHandle() && !descr->parameterTypes[n].IsReference() ) {
				spos++;
			} else {
				spos += descr->parameterTypes[n].GetSizeOnStackDWords();
			}
		}
	}

	return popSize;
}

END_AS_NAMESPACE

#endif // AS_X64_GCC
#endif // AS_MAX_PORTABILITY
