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

enum argTypes { x64ENDARG = 0, x64INTARG = 1, x64FLOATARG = 2, x64DOUBLEARG = 3 };

#define X64_MAX_ARGS             32
#define MAX_CALL_INT_REGISTERS    6
#define MAX_CALL_SSE_REGISTERS    8
#define CALLSTACK_MULTIPLIER      3

#define ASM_SET_INT_M( reg_string, val )                 \
	__asm__ __volatile__ (                           \
		"lea %0, %%rax\r\n"                      \
		"mov (%%rax), %" reg_string              \
		:                                        \
		: "m" ( val )                            \
        )
#define ASM_SET_FLOAT_M( reg_string, val )               \
	__asm__ __volatile__ (                           \
		"lea      %0, %%rax\t\n"                 \
		"movss    (%%rax), %" reg_string "\r\n"  \
		:                                        \
		: "m" ( val )                            \
	)
#define ASM_SET_DOUBLE_M( reg_string, val )              \
	__asm__ __volatile__ (                           \
		"lea      %0, %%rax\t\n"                 \
		"movlpd   (%%rax), %" reg_string "\r\n"  \
		:                                        \
		: "m" ( val )                            \
	)

#define PUSH_LONG( val )                                 \
	__asm__ __volatile__ (                           \
		"mov    %0, %%rax\r\n"                   \
		"push   %%rax"                           \
		:                                        \
		: "m" ( val )                            \
	)


#define ASM_GET_REG( name, dest )                        \
	__asm__ __volatile__ (                           \
		"mov  %" name ", %0"                     \
		: "=r" ( dest )                          \
	)

#define __CALL( statement )                              \
	__asm__ __volatile__ (                           \
		"push  %rdi\r\n"                         \
		"push  %rsi\r\n"                         \
		"push  %rdx\r\n"                         \
		"push  %rcx\r\n"                         \
		"push  %r8\r\n"                          \
		"push  %r9"                              \
	);                                               \
	statement;                                       \
	__asm__ __volatile__ (                           \
		"pop   %r9\r\n"                          \
		"pop   %r8\r\n"                          \
		"pop   %rcx\r\n"                         \
		"pop   %rdx\r\n"                         \
		"pop   %rsi\r\n"                         \
		"pop   %rdi"                             \
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

static asQWORD CallCDeclFunction( const asDWORD* pArgs, const asBYTE *pArgsType, asDWORD *func )
{
	asQWORD retval      = 0;
	asQWORD ( *call )() = (asQWORD (*)())func;
	int     iReg        = 0;
	int     fReg        = 0;
	int     i           = 0;        // track integer and float arguments separately
	float   __f         = 0.0f;
	double  __d         = 0.0f;

	// the first 6 integer arguments go into rdi, rsi, rdx, rcx, r8, and r9
	// the first 8 float arguments go into xmm0...7
	// everything else is pushed right-to-left
	for( i = 0, iReg = 0, fReg = 0; *pArgsType; pArgsType++, i += CALLSTACK_MULTIPLIER ) {
		switch ( *pArgsType ) {
			case x64ENDARG: {
				break;
			}
			case x64INTARG: {
				if ( iReg < MAX_CALL_INT_REGISTERS ) {
					switch ( iReg ) {
						case 0: {
							ASM_SET_INT_M( "%rdi", pArgs[i] );
							break;
						}
						case 1: {
							ASM_SET_INT_M( "%rsi", pArgs[i] );
							break;
						}
						case 2: {
							ASM_SET_INT_M( "%rdx", pArgs[i] );
							break;
						}
						case 3: {
							ASM_SET_INT_M( "%rcx", pArgs[i] );
							break;
						}
						case 4: {
							ASM_SET_INT_M( "%r8", pArgs[i] );
							break;
						}
						case 5: {
							ASM_SET_INT_M( "%r9", pArgs[i] );
							break;
						}
					}
				} else {
					PUSH_LONG( pArgs[i] );
				}
				iReg++;
				break;
			}
			case x64FLOATARG: {
				if ( fReg < MAX_CALL_SSE_REGISTERS )	{
					__CALL( memcpy( &__f, pArgs + i, sizeof( __f ) ) );
					switch ( fReg ) {
						case 0: {
							ASM_SET_FLOAT_M( "%xmm0", __f );
							break;
						}
						case 1: {
							ASM_SET_FLOAT_M( "%xmm1", __f );
							break;
						}
						case 2: {
							ASM_SET_FLOAT_M( "%xmm2", __f );
							break;
						}
						case 3: {
							ASM_SET_FLOAT_M( "%xmm3", __f );
							break;
						}
						case 4: {
							ASM_SET_FLOAT_M( "%xmm4", __f );
							break;
						}
						case 5: {
							ASM_SET_FLOAT_M( "%xmm5", __f );
							break;
						}
						case 6: {
							ASM_SET_FLOAT_M( "%xmm6", __f );
							break;
						}
						case 7: {
							ASM_SET_FLOAT_M( "%xmm7", __f );
							break;
						}
					}
				} else {
					PUSH_LONG( pArgs[i] );
				}
				fReg++;
				break;
			}
			case x64DOUBLEARG: {
				if ( fReg < MAX_CALL_SSE_REGISTERS ) {
					__CALL( memcpy( &__d, pArgs + i, sizeof( __d ) ) );
					switch ( fReg ) {
						case 0: {
							ASM_SET_DOUBLE_M( "%xmm0", __d );
							break;
						}
						case 1: {
							ASM_SET_DOUBLE_M( "%xmm1", __d );
							break;
						}
						case 2: {
							ASM_SET_DOUBLE_M( "%xmm2", __d );
							break;
						}
						case 3: {
							ASM_SET_DOUBLE_M( "%xmm3", __d );
							break;
						}
						case 4: {
							ASM_SET_DOUBLE_M( "%xmm4", __d );
							break;
						}
						case 5: {
							ASM_SET_DOUBLE_M( "%xmm5", __d );
							break;
						}
						case 6: {
							ASM_SET_DOUBLE_M( "%xmm6", __d );
							break;
						}
						case 7: {
							ASM_SET_DOUBLE_M( "%xmm7", __d );
							break;
						}
					}
				} else {
					PUSH_LONG( pArgs[i] );
				}
				fReg++;
				break;
			}
		}
	}

	// call the function with the arguments
	retval = call();
	return retval;
}

// This function should prepare system functions so that it will be faster to call them
int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine * /*engine*/)
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
	asCScriptEngine            *engine   = context->engine;
	asCScriptFunction          *descr    = engine->scriptFunctions[id];
	asSSystemFunctionInterface *sysFunc  = engine->scriptFunctions[id]->sysFuncIntf;
	int                         callConv = sysFunc->callConv;

	if( callConv == ICC_GENERIC_FUNC || callConv == ICC_GENERIC_METHOD )
		return context->CallGeneric(id, objectPointer);

	asQWORD  retQW              = 0;
	asQWORD  retQW2             = 0;
	void    *func               = (void*)sysFunc->func;
	int      paramSize          = sysFunc->paramSize;
	asDWORD *args               = context->stackPointer;
	asDWORD *stack_pointer      = context->stackPointer;
	void    *retPointer         = 0;
	void    *obj                = 0;
// 	asDWORD *vftable            = NULL;
	int      popSize            = paramSize;
	int      totalArgumentCount = 0;
	int      n                  = 0;
	int      base_n             = 0;
	int      a                  = 0;
	int      param_pre          = 0;
	int      param_post         = 0;

	asBYTE	 argsType[X64_MAX_ARGS + 3] = { 0 };
	asBYTE   argsSet[X64_MAX_ARGS + 3]  = { 0 };

	context->objectType = descr->returnType.GetObjectType();
	if( descr->returnType.IsObject() && !descr->returnType.IsReference() && !descr->returnType.IsObjectHandle() ) {
		// Allocate the memory for the object
		retPointer = engine->CallAlloc( descr->returnType.GetObjectType() );

		if( sysFunc->hostReturnInMemory ) {
			// The return is made in memory
			callConv++;
		}
	}

	// mark all float/double/int arguments
	int argIndex = 0;
	totalArgumentCount = (int)descr->parameterTypes.GetLength();

	if ( totalArgumentCount > X64_MAX_ARGS ) {
		context->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
		return 0;
	}

	for( a = 0; a < (int)descr->parameterTypes.GetLength(); ++a, ++argIndex ) {
		// get the base type
		argsType[argIndex] = x64INTARG;
		if( descr->parameterTypes[a].IsFloatType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64FLOATARG;
		}
		if( descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64DOUBLEARG;
		}
		if( descr->parameterTypes[a].GetSizeOnStackDWords() == 2 && !descr->parameterTypes[a].IsDoubleType() && !descr->parameterTypes[a].IsReference() ) {
			argsType[argIndex] = x64INTARG;
		}

		// if it is a variable argument, account for the typeID
		if( IsVariableArgument( descr->parameterTypes[a] ) ) {
			// implicitly add another parameter (AFTER the parameter above), for the TypeID
			argsType[++argIndex] = x64INTARG;
		}
	}
	assert( argIndex == totalArgumentCount );

	asASSERT( descr->parameterTypes.GetLength() <= X64_MAX_ARGS );

	asDWORD paramBuffer[CALLSTACK_MULTIPLIER * ( X64_MAX_ARGS + 3 )];
	memset( paramBuffer, 0, sizeof( paramBuffer ) );

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
// 		obj = ( void * )( *( int * )( obj ) + sysFunc->baseOffset );
// 		printf( ">>>>> Got adjusted OBJ_PTR -> %p\n", obj );
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

	for( n = 0; n < ( int )( param_pre + totalArgumentCount + param_post ); n++ ) {
		int copy_count = 0;
		if ( n >= param_pre && n < ( int )( param_pre + totalArgumentCount ) ) {
			copy_count = descr->parameterTypes[n - param_pre].GetSizeOnStackDWords();
		}
		if ( copy_count > CALLSTACK_MULTIPLIER ) {
			context->SetInternalException( TXT_INVALID_CALLING_CONVENTION );
			return 0;
		}
		if ( copy_count ) {
			memcpy( paramBuffer + n * CALLSTACK_MULTIPLIER, stack_pointer, copy_count * sizeof( asDWORD ) );
			stack_pointer += copy_count;
		}
	}

	asDWORD tempBuff[CALLSTACK_MULTIPLIER * ( X64_MAX_ARGS + 3 )] = { 0 };
	asBYTE  tempType[X64_MAX_ARGS + 3] = { 0 };
	int     used_int_regs = 0;
	int     used_sse_regs = 0;
	base_n = 0;
	for ( n = 0; ( n < X64_MAX_ARGS + 3 ) && ( used_int_regs < MAX_CALL_INT_REGISTERS ); n++ ) {
		if ( argsType[n] == x64INTARG ) {
			argsSet[n] = 1;
			tempType[base_n] = argsType[n];
			memcpy( tempBuff + base_n * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
			used_int_regs++;
		}
	}
	for ( n = 0; ( n < X64_MAX_ARGS + 3 ) && ( used_sse_regs < MAX_CALL_SSE_REGISTERS ); n++ ) {
		if ( argsType[n] == x64FLOATARG || argsType[n] == x64DOUBLEARG ) {
			argsSet[n] = 1;
			tempType[base_n] = argsType[n];
			memcpy( tempBuff + base_n * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
			used_sse_regs++;
		}
	}
	for ( n = X64_MAX_ARGS + 2; n >= 0; n-- ) {
		if ( argsType[n] != x64ENDARG && !argsSet[n] ) {
			argsSet[n] = 1;
			tempType[base_n] = argsType[n];
			memcpy( tempBuff + base_n * CALLSTACK_MULTIPLIER, paramBuffer + n * CALLSTACK_MULTIPLIER, CALLSTACK_MULTIPLIER * sizeof( asDWORD ) );
			base_n++;
		}
	}

	context->isCallingSystemFunction = true;
	retQW = CallCDeclFunction( tempBuff, tempType, ( asDWORD * )func );
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
