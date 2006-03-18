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
// as_context_x86.cpp
//
// This class handles the execution of the code compiled for the X86 Virtual Machine
//

// The x86 assembler Virtual Machine was originally written by
// Andres Carrera on October 12th, 2004 for version 1.8.2


#include <math.h> // fmodf(), fmod()
#include <stddef.h>

#include "as_config.h"
#include "as_context.h"
#include "as_scriptengine.h"
#include "as_tokendef.h"
#include "as_bytecodedef.h"
#include "as_bstr_util.h"
#include "as_texts.h"
#include "as_callfunc.h"
#include "as_module.h"


#ifdef USE_ASM_VM

// Take the address of these functions, as MSVC6++ can't resolve
// them in inline assembler with full optimizations turned on
static void *work_around_fmod = (double (*)(double, double))fmod;

asUPtr uptr_module_GetScriptFunction = asMETHOD(asCModule,GetScriptFunction);
static void *module_GetScriptFunction = uptr_module_GetScriptFunction.func;

asUPtr uptr_context_CallScriptFunction = asMETHOD(asCContext,CallScriptFunction);
static void *context_CallScriptFunction = uptr_context_CallScriptFunction.func;

asUPtr uptr_context_PopCallState = asMETHOD(asCContext,PopCallState);
static void *context_PopCallState = uptr_context_PopCallState.func;

asUPtr uptr_module_GetConstantBStr = asMETHOD(asCModule,GetConstantBStr);
static void *module_GetConstantBStr = uptr_module_GetConstantBStr.func;

asUPtr uptr_engine_GetModule = asMETHODPR(asCScriptEngine,GetModule,(int),asCModule *);
static void *engine_GetModule = uptr_engine_GetModule.func;

// Declare _ftol
extern "C" void _ftol();

asDWORD relocTable[BC_MAXBYTECODE];

#ifdef ASM_INTEL

#define OFFSET(opCode) __asm add eax,4 \
	__asm mov ecx, offset opCode \
	__asm mov [eax], ecx

#define NEXT_EIP \
	__asm jmp dword ptr [esi]


#elif ASM_AT_N_T

#define OFFSET
#define NEXT_EIP

#endif

// Used for comparing floats and doubles with 0
const double zero = 0;
const double one = 1;

void asCContext::CreateRelocTable() 
{
	asCContext ctx(0, 0);
	ctx.ExecuteNext(true);
}

void asCContext::ExecuteNext(bool createRelocationTable)
{
	static bool isRelocationTableInitialized = false;

	if( createRelocationTable ) 
	{
		if( !isRelocationTableInitialized ) 
		{
			// Verify the member offsets used in the assembler code
#ifdef _DEBUG
			int offset;
			assert((offset = offsetof(asCContext     , engine             )) == 0x0C);
			assert((offset = offsetof(asCContext     , module             )) == 0x10);
			assert((offset = offsetof(asCContext     , status             )) == 0x14);
			assert((offset = offsetof(asCContext     , doSuspend          )) == 0x18);
			assert((offset = offsetof(asCContext     , byteCode           )) == 0x1C);
			assert((offset = offsetof(asCContext     , stackFramePointer  )) == 0x24);
			assert((offset = offsetof(asCContext     , exceptionID        )) == 0x28);
			assert((offset = offsetof(asCContext     , returnVal          )) == 0x30);
			assert((offset = offsetof(asCContext     , callStack          )) == 0x38);
			assert((offset = offsetof(asCContext     , stackPointer       )) == 0x50);
			assert((offset = offsetof(asCScriptEngine, globalPropAddresses)) == 0x74);
			assert((offset = offsetof(asCModule      , globalMem          )) == 0x134);
			//assert((offset = offsetof(asCArray<int>  , length             )) == 0x4);
#endif

#ifdef ASM_INTEL
			__asm 
			{
				mov eax, offset relocTable
				mov ecx, offset bc_POP		
				mov [eax], ecx
				OFFSET(bc_PUSH)
				OFFSET(bc_SET4)
				OFFSET(bc_RD4)
				OFFSET(bc_RDSF4)
				OFFSET(bc_WRT4)
				OFFSET(bc_MOV4)
				OFFSET(bc_PSF)
				OFFSET(bc_MOVSF4)
				OFFSET(bc_SWAP4)
				OFFSET(bc_STORE4)
				OFFSET(bc_RECALL4)
				OFFSET(bc_ADDOFF)
				OFFSET(bc_CALL)
				OFFSET(bc_RET)
				OFFSET(bc_JMP)
				OFFSET(bc_JZ)
				OFFSET(bc_JNZ)
				OFFSET(bc_TZ)
				OFFSET(bc_TNZ)
				OFFSET(bc_TS)
				OFFSET(bc_TNS)
				OFFSET(bc_TP)
				OFFSET(bc_TNP)
				OFFSET(bc_ADDi)
				OFFSET(bc_SUBi)
				OFFSET(bc_MULi)
				OFFSET(bc_DIVi)
				OFFSET(bc_MODi)
				OFFSET(bc_NEGi)
				OFFSET(bc_CMPi)
				OFFSET(bc_INCi)
				OFFSET(bc_DECi)
				OFFSET(bc_I2F)
				OFFSET(bc_ADDf)
				OFFSET(bc_SUBf)
				OFFSET(bc_MULf)
				OFFSET(bc_DIVf)
				OFFSET(bc_MODf)
				OFFSET(bc_NEGf)
				OFFSET(bc_CMPf)
				OFFSET(bc_INCf)
				OFFSET(bc_DECf)
				OFFSET(bc_F2I)
				OFFSET(bc_BNOT)
				OFFSET(bc_BAND)
				OFFSET(bc_BOR)
				OFFSET(bc_BXOR)
				OFFSET(bc_BSLL)
				OFFSET(bc_BSRL)
				OFFSET(bc_BSRA)
				OFFSET(bc_UI2F)
				OFFSET(bc_F2UI)
				OFFSET(bc_CMPui)
				OFFSET(bc_SB)
				OFFSET(bc_SW)
				OFFSET(bc_UB)
				OFFSET(bc_UW)
				OFFSET(bc_WRT1)
				OFFSET(bc_WRT2)
				OFFSET(bc_INCi16)
				OFFSET(bc_INCi8)
				OFFSET(bc_DECi16)
				OFFSET(bc_DECi8)
				OFFSET(bc_PUSHZERO)
				OFFSET(bc_COPY)
				OFFSET(bc_PGA)
				OFFSET(bc_SET8)
				OFFSET(bc_WRT8)
				OFFSET(bc_RD8)
				OFFSET(bc_NEGd)
				OFFSET(bc_INCd)
				OFFSET(bc_DECd)
				OFFSET(bc_ADDd)
				OFFSET(bc_SUBd)
				OFFSET(bc_MULd)
				OFFSET(bc_DIVd)
				OFFSET(bc_MODd)
				OFFSET(bc_SWAP8)
				OFFSET(bc_CMPd)
				OFFSET(bc_dTOi)
				OFFSET(bc_dTOui)
				OFFSET(bc_dTOf)
				OFFSET(bc_iTOd)
				OFFSET(bc_uiTOd)
				OFFSET(bc_fTOd)
				OFFSET(bc_JMPP)
				OFFSET(bc_PEID)
				OFFSET(bc_SRET4)
				OFFSET(bc_SRET8)
				OFFSET(bc_RRET4)
				OFFSET(bc_RRET8)
				OFFSET(bc_STR)
				OFFSET(bc_JS)		
				OFFSET(bc_JNS)
				OFFSET(bc_JP)
				OFFSET(bc_JNP)
				OFFSET(bc_CMPIi)
				OFFSET(bc_CMPIui)
				OFFSET(bc_CALLSYS)
				OFFSET(bc_CALLBND)
				OFFSET(bc_RDGA4)
				OFFSET(bc_MOVGA4)
				OFFSET(bc_ADDIi)
				OFFSET(bc_SUBIi)
				OFFSET(bc_CMPIf)
				OFFSET(bc_ADDIf)
				OFFSET(bc_SUBIf)
				OFFSET(bc_MULIi)
				OFFSET(bc_MULIf)
				OFFSET(bc_SUSPEND)
				OFFSET(bc_END)
			}

#elif ASM_AT_N_T
			assert(0);
#endif

			isRelocationTableInitialized = true;
		}
		return;
	}	
	
	asBYTE  *asm_byteCode;
	asDWORD *asm_stackPointer;
	asDWORD *asm_stackFramePointer;
	asDWORD  asm_tempReg;

	asQWORD temp;
	
#ifdef ASM_INTEL
	__asm 
	{		
		mov ecx, this
		// Set asm_byteCode = this->byteCode
		mov esi, [ecx+1Ch]
		mov [asm_byteCode], esi
		// Set asm_stackPointer = this->stackPointer
		mov ebx, [ecx+50h]
		mov [asm_stackPointer], ebx
		// Set asm_stackFramePointer = this->stackFramePointer
		mov eax, [ecx+24h]
		mov [asm_stackFramePointer], eax				
		NEXT_EIP	

// TESTED
bc_POP:			
		xor         ecx,ecx
        mov         cx,word ptr [esi+4]
        add         esi,6
        lea         ebx,[ebx+ecx*4]    
		NEXT_EIP		
	
// TESTED
bc_PUSH:		
		xor         edx,edx
        mov         dx,word ptr [esi+4]
        neg         edx
        add         esi,6
        lea         ebx,[ebx+edx*4]    
		NEXT_EIP
		
// TESTED
bc_SET4:
		mov         eax,dword ptr [esi+4]
		sub         ebx,4
		add         esi,8
		mov         dword ptr [ebx],eax  
		NEXT_EIP
		
// TESTED
bc_RD4:
		mov         ecx,dword ptr [ebx]
        add         esi,4
        mov         edx,dword ptr [ecx]
        mov         dword ptr [ebx],edx  
		NEXT_EIP

// TESTED
bc_RDSF4:
		movsx       eax,word ptr [esi+4]
		mov         ecx,[asm_stackFramePointer]
		sub         ebx,4
		shl         eax,2                
		sub         ecx,eax
		add         esi,6
		mov         edx,dword ptr [ecx]
		mov         dword ptr [ebx],edx  
		NEXT_EIP

// TESTED
bc_WRT4:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		mov         dword ptr [eax],ecx  
		NEXT_EIP
	
// TESTED
bc_MOV4:
		mov         edx,dword ptr [ebx]
		mov         eax,dword ptr [ebx+4]
		add         ebx,8
		add         esi,4
		mov         dword ptr [edx],eax  
		NEXT_EIP
	
// TESTED
bc_PSF:
		movsx       ecx,word ptr [esi+4]
        mov         edx,[asm_stackFramePointer]
        sub         ebx,4
        shl         ecx,2                
        sub         edx,ecx
        add         esi,6
        mov         dword ptr [ebx],edx  
		NEXT_EIP

// TESTED
bc_MOVSF4:
		movsx       eax,word ptr [esi+4]
		mov         ecx,[asm_stackFramePointer]
		mov         edx,dword ptr [ebx]
		shl         eax,2                
		sub         ecx,eax
		add         ebx,4
		add         esi,6
		mov         dword ptr [ecx],edx  
		NEXT_EIP

// TESTED
bc_SWAP4:
		mov         eax,dword ptr [ebx]
        mov         ecx,dword ptr [ebx+4]
        add         esi,4
        mov         dword ptr [ebx],ecx
        mov         dword ptr [ebx+4],eax
		NEXT_EIP

// NOT TESTED
bc_STORE4:
		mov         edx,dword ptr [ebx]
        add         esi,4
        mov         [asm_tempReg],edx
		NEXT_EIP

// NOT TESTED
bc_RECALL4:
		mov         eax,[asm_tempReg]
		sub         ebx,4
		add         esi,4
		mov         dword ptr [ebx],eax  
		NEXT_EIP

// NOT TESTED
bc_ADDOFF:
		mov         ecx,dword ptr [ebx]
		mov         eax,dword ptr [ebx+4]
		add         ebx,4
		mov         eax,dword ptr [eax]
		test        eax,eax
		je          ExceptionNullPointer
		add         eax,ecx
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

ExceptionNullPointer:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		// Raise exception
		SetInternalException(TXT_NULL_POINTER_ACCESS);
		return;
	} __asm { 

// TESTED
bc_CALL:
		mov         edi,this
		mov         ecx,[asm_stackFramePointer]
		mov         eax,dword ptr [esi+4]
		add         esi,8
		mov         dword ptr [edi+24h],ecx     // this->stackFramePointer = asm_stackPointer
		mov         ecx,dword ptr [edi+10h]     // ecx = this->module
		push        eax
		mov         dword ptr [edi+1Ch],esi     // this->byteCode = esi
		mov         dword ptr [edi+50h],ebx     // this->stackPointer = ebx
		call        module_GetScriptFunction
		mov         edx,dword ptr [edi+10h]     // edx = this->module
		push        eax
		push        edx
		mov         ecx,edi
		call        context_CallScriptFunction
		mov         eax,dword ptr [edi+24h]     // eax = this->stackFramePointer
		mov         esi,dword ptr [edi+1Ch]     // esi = this->byteCode
		mov         ebx,dword ptr [edi+50h]     // ebx = this->stackPointer
		mov         [asm_stackFramePointer],eax
		mov         eax,dword ptr [edi+14h]     // eax = this->status
		cmp         eax,3						// status != tsActive
		jne         Return
		NEXT_EIP

Return:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		return;
	} __asm { 
		
// TESTED
bc_RET:
		mov         edi,this
		mov         eax,dword ptr [edi+3Ch]     // eax = this->callStack.length
		test        eax,eax
		je          ReturnFinished
		mov         cx,word ptr [esi+4]
		mov         edx,[asm_stackFramePointer]
		mov         word ptr [temp],cx			// temp = ecx (return pop)
		mov         ecx,edi
		mov         dword ptr [edi+1Ch],esi     // this->byteCode = esi
		mov         dword ptr [edi+50h],ebx     // this->stackPointer = ebx
		mov         dword ptr [edi+24h],edx     // this->stackFramePointer = edx
		call        context_PopCallState
		mov         eax,dword ptr [edi+24h]     // eax = this->stackFramePointer
		mov         ecx,dword ptr [edi+50h]     // ecx = this->stackPointer
		mov         esi,dword ptr [edi+1Ch]     // esi = this->byteCode
		mov         [asm_stackFramePointer],eax
		mov         eax,dword ptr [temp]        // eax = temp (return pop)
		and         eax,0FFFFh                 
		lea         ebx,[ecx+eax*4]             // pop the arguments
		NEXT_EIP

ReturnFinished:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		status = tsProgramFinished;
		return;
	} __asm { 

// TESTED
bc_JMP:
		mov         edx,dword ptr [esi+4]
		lea         esi,[esi+edx+8]      
		NEXT_EIP

// TESTED
bc_JZ:
		cmp         dword ptr [ebx],0
		jne         jz_lbl
		add         esi,dword ptr [esi+4]
jz_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

// TESTED
bc_JNZ:
		cmp         dword ptr [ebx],0
		je          jnz_lbl
		add         esi,dword ptr [esi+4]
jnz_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

// NOT TESTED
bc_TZ:
		mov         ecx,dword ptr [ebx]
        xor         eax,eax
        test        ecx,ecx
        sete        al
        add         esi,4
        mov         dword ptr [ebx],eax
		NEXT_EIP

// NOT TESTED
bc_TNZ:
		mov         edx,dword ptr [ebx]
        xor         ecx,ecx
        test        edx,edx
        setne       cl
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

// TESTED
bc_TS:
		mov         ecx,dword ptr [ebx]
        xor         edx,edx
        test        ecx,ecx
        setl        dl
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

// NOT TESTED
bc_TNS:
		mov         ecx,dword ptr [ebx]
	    xor         eax,eax
	    test        ecx,ecx
	    setge       al
	    add         esi,4
	    mov         dword ptr [ebx],eax
		NEXT_EIP

// TESTED
bc_TP:
		mov         edx,dword ptr [ebx]
        xor         ecx,ecx
        test        edx,edx
        setg        cl
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

// TESTED
bc_TNP:
		mov         ecx,dword ptr [ebx]
        xor         edx,edx
        test        ecx,ecx
        setle       dl
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

// TESTED
bc_ADDi:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         ecx,eax
		add         esi,4
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

// TESTED
bc_SUBi:
		mov         ecx,dword ptr [ebx]
		mov         edx,dword ptr [ebx+4]
		add         ebx,4
		sub         edx,ecx
		add         esi,4
		mov         dword ptr [ebx],edx  
		NEXT_EIP

// TESTED
bc_MULi:
		mov         edx,dword ptr [ebx]
		add         ebx,4
		imul        edx,dword ptr [ebx]
		add         esi,4
		mov         dword ptr [ebx],edx
		NEXT_EIP

// TESTED
bc_DIVi:
		cmp         dword ptr [ebx],0
		je          ExceptionDivByZero 
		mov         eax,dword ptr [ebx+4]
		add         ebx,4
		cdq
		idiv        dword ptr [ebx-4]
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

ExceptionDivByZero:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		// Raise exception
		SetInternalException(TXT_DIVIDE_BY_ZERO);
		return;
	} __asm { 

// TESTED
bc_MODi:
		cmp         dword ptr [ebx],0
		je          ExceptionDivByZero
		mov         eax,dword ptr [ebx+4]
		add         ebx,4
		cdq
		idiv        dword ptr [ebx-4]
		add         esi,4
		mov         dword ptr [ebx],edx
		NEXT_EIP

// NOT TESTED
bc_NEGi:
		mov         eax,dword ptr [ebx]
		neg         eax
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

// TESTED
bc_CMPi:
		mov         eax,dword ptr [ebx+4]
        mov         ecx,dword ptr [ebx]
        add         ebx,4
        sub         eax,ecx
        jne         cmpi_lbl
        xor         eax,eax
        add         esi,4
        mov         dword ptr [ebx],eax
        NEXT_EIP
cmpi_lbl:
        xor         ecx,ecx
        test        eax,eax
        setge       cl
        dec         ecx
        and         ecx,0FFFFFFFEh
        inc         ecx
        add         esi,4
        mov         eax,ecx
        mov         dword ptr [ebx],eax  
        NEXT_EIP

// TESTED
bc_INCi:
		mov         eax,dword ptr [ebx]
        add         esi,4
        inc         int ptr [eax]
		NEXT_EIP

// NOT TESTED
bc_DECi:
		mov         eax,dword ptr [ebx]
        add         esi,4
        dec         int ptr [eax]
		NEXT_EIP

// NOT TESTED
bc_I2F:
		fild        dword ptr [ebx]
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_ADDf:
		fld         dword ptr [ebx]
        fadd        dword ptr [ebx+4]
        add         ebx,4
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_SUBf:
		fld         dword ptr [ebx+4]
        fsub        dword ptr [ebx]
        add         ebx,4
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_MULf:
		fld         dword ptr [ebx]
		fmul        dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		fstp        dword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_DIVf:
		cmp         dword ptr [ebx],0
		je          ExceptionDivByZero
		fld         dword ptr [ebx+4]
		fdiv        dword ptr [ebx]
		add         ebx,4
		add         esi,4
		fstp        dword ptr [ebx]
		NEXT_EIP

// TESTED
bc_MODf:
		cmp         dword ptr [ebx],0
		je          ExceptionDivByZero
		push        dword ptr [ebx]
		push        dword ptr [ebx+4]
		add         ebx,4
		call        fmodf
		add         esp,8
		add         esi,4
		fstp        dword ptr [ebx]
		NEXT_EIP

// NOT TESTED
bc_NEGf:
		fld         dword ptr [ebx]
		fchs
		fstp        dword ptr [ebx]
		add         esi,4          
		NEXT_EIP

// NOT TESTED
bc_CMPf:
		fld         dword ptr [ebx+4]
		fsub        dword ptr [ebx]
		add         ebx,4
		fcomp       dword ptr [zero]
		fnstsw      ax
		test        ah,40h
		je          cmpf_lbl1
		mov         dword ptr [ebx],0
		add         esi,4
		NEXT_EIP
cmpf_lbl1:
		test        ah,1
		je          cmpf_lbl2
		mov         dword ptr [ebx],0FFFFFFFFh
		add         esi,4
		NEXT_EIP
cmpf_lbl2:
		mov         dword ptr [ebx],1
		add         esi,4
		NEXT_EIP

// NOT TESTED
bc_INCf:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         dword ptr [eax]
		fadd        qword ptr [one]
		fstp        dword ptr [eax]
		NEXT_EIP

// NOT TESTED
bc_DECf:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         dword ptr [eax]
		fsub        qword ptr [one]
		fstp        dword ptr [eax]
		NEXT_EIP

// NOT TESTED
bc_F2I:
		fld         dword ptr [ebx]
		call        _ftol
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

// NOT TESTED
bc_BNOT:
		mov         ecx,dword ptr [ebx]
		add         esi,4
		not         ecx
		mov         dword ptr [ebx],ecx
		NEXT_EIP

// NOT TESTED
bc_BAND:
		mov         edx,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		and         ecx,edx
		add         esi,4
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

// NOT TESTED
bc_BOR:
		mov         eax,dword ptr [ebx]
        mov         ecx,dword ptr [ebx+4]
        add         ebx,4
        or          ecx,eax
        add         esi,4
        mov         dword ptr [ebx],ecx  
        NEXT_EIP

// NOT TESTED
bc_BXOR:
		mov         ecx,dword ptr [ebx]
        mov         edx,dword ptr [ebx+4]
        add         ebx,4
        xor         edx,ecx
        add         esi,4
        mov         dword ptr [ebx],edx  
        NEXT_EIP

// NOT TESTED
bc_BSLL:
		mov         ecx,dword ptr [ebx]
        mov         edx,dword ptr [ebx+4]
        add         ebx,4
        shl         edx,cl
        add         esi,4
        mov         dword ptr [ebx],edx  
		NEXT_EIP

// NOT TESTED
bc_BSRL:
		mov         ecx,dword ptr [ebx]
		mov         edx,dword ptr [ebx+4]
		add         ebx,4
		shr         edx,cl
		add         esi,4
        mov         dword ptr [ebx],edx  
		NEXT_EIP

// NOT TESTED
bc_BSRA:
		mov         edx,dword ptr [ebx+4]
		mov         ecx,dword ptr [ebx]
		add         ebx,4
		sar         edx,cl
		add         esi,4
		mov         dword ptr [ebx],edx  
		NEXT_EIP

// NOT TESTED
bc_UI2F:
		mov         eax,dword ptr [ebx]
		mov         dword ptr [temp+4],0
		mov         dword ptr [temp],eax
		add         esi,4
		fild        qword ptr [temp]
		fstp        dword ptr [ebx]
		NEXT_EIP

// NOT TESTED
bc_F2UI:
		fld         dword ptr [ebx]
		call        _ftol
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

// NOT TESTED
bc_CMPui:
		mov         eax,dword ptr [ebx+4]
        mov         ecx,dword ptr [ebx]
        add         ebx,4
        cmp         eax,ecx
        jne         cmpui_lbl
        xor         eax,eax
        add         esi,4
        mov         dword ptr [ebx],eax
        mov         dword ptr [ebp+8],esi 
        NEXT_EIP     
cmpui_lbl:
        sbb         eax,eax
        and         al,0FEh
        inc         eax
        add         esi,4
        mov         dword ptr [ebx],eax
        mov         dword ptr [ebp+8],esi 
        NEXT_EIP   

// TESTED
bc_SB:
		movsx       ecx,byte ptr [ebx]
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

// NOT TESTED
bc_SW:
		movsx       edx,word ptr [ebx]
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

// TESTED
bc_UB:
		xor         eax,eax
        add         esi,4
        mov         al,byte ptr [ebx]
        mov         dword ptr [ebx],eax  
		NEXT_EIP

// NOT TESTED
bc_UW:
		xor         ecx,ecx
        add         esi,4
        mov         cx,word ptr [ebx]
        mov         dword ptr [ebx],ecx  
		NEXT_EIP

// TESTED
bc_WRT1:
		mov         edx,dword ptr [ebx]
        mov         al,byte ptr [ebx+4]
        add         ebx,4
        add         esi,4
        mov         byte ptr [edx],al  
		NEXT_EIP

// NOT TESTED
bc_WRT2:
		mov         ecx,dword ptr [ebx]
        mov         dx,word ptr [ebx+4]
        add         ebx,4
        add         esi,4
        mov         word ptr [ecx],dx  
		NEXT_EIP

// NOT TESTED
bc_INCi16:
		mov         eax,dword ptr [ebx]
        inc         word ptr [eax]
        add         esi,4              
		NEXT_EIP

// NOT TESTED
bc_INCi8:
		mov         eax,dword ptr [ebx]
        mov         cl,byte ptr [eax]
        inc         cl
        add         esi,4
        mov         byte ptr [eax],cl  
		NEXT_EIP

// NOT TESTED
bc_DECi16:
		mov         eax,dword ptr [ebx]
        dec         word ptr [eax]
        add         esi,4              
		NEXT_EIP

// NOT TESTED
bc_DECi8:
		mov         eax,dword ptr [ebx]
        mov         cl,byte ptr [eax]
        dec         cl
        add         esi,4
        mov         byte ptr [eax],cl  
		NEXT_EIP

// TESTED
bc_PUSHZERO:
		sub         ebx,4
        add         esi,4
        mov         dword ptr [ebx],0    
		NEXT_EIP

// TESTED
bc_COPY:
		mov         eax,esi			                  // eax = byteCode
		xor         ecx,ecx                           // ecx = 0
		mov         esi,dword ptr [ebx+4]             // esi = source address
		mov         edi,dword ptr [ebx]               // edi = dest address
		mov         cx,word ptr [eax+4]               // ecx = num dwords from bytecode argument
		add         ebx,4                             // pop dest address
		rep movs    dword ptr [edi],dword ptr [esi]   // copy ecx dwords
		add         eax,6                             // move to next bytecode
		mov         esi,eax                           // esi = byteCode
		NEXT_EIP

// TESTED
bc_PGA:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		sub         ebx,4
		test        eax,eax
		jge         pga_lbl
		mov         ecx,dword ptr [edi+0Ch]
		lea         eax,[eax*4+4]
		mov         edx,dword ptr [ecx+74h]
		sub         edx,eax
		add         esi,8
		mov         eax,dword ptr [edx]
		mov         dword ptr [ebx],eax
		NEXT_EIP
pga_lbl:
		mov         ecx,dword ptr [edi+10h]
		and         eax,0FFFFh
		add         esi,8
		mov         edx,dword ptr [ecx+134h]
		lea         eax,[edx+eax*4]
		mov         dword ptr [ebx],eax
		NEXT_EIP

// TESTED
bc_SET8:
		mov         ecx,dword ptr [esi+4]
		sub         ebx,8
		add         esi,0Ch
		mov         dword ptr [ebx],ecx
		mov         edx,dword ptr [esi-4]
		mov         dword ptr [ebx+4],edx
		NEXT_EIP

// TESTED
bc_WRT8:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		mov         dword ptr [eax],ecx
		mov         edx,dword ptr [ebx+4]
		mov         dword ptr [eax+4],edx
		NEXT_EIP

// TESTED
bc_RD8:
		mov         eax,dword ptr [ebx]
        sub         ebx,4
        add         esi,4
        mov         ecx,dword ptr [eax]
        mov         dword ptr [ebx],ecx
        mov         edx,dword ptr [eax+4]
        mov         dword ptr [ebx+4],edx
		NEXT_EIP

// NOT TESTED
bc_NEGd:
		fld         qword ptr [ebx]
        fchs
        fstp        qword ptr [ebx]
        add         esi,4          
		NEXT_EIP

// NOT TESTED
bc_INCd:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         qword ptr [eax]
		fadd        qword ptr [one]
		fstp        qword ptr [eax]
		NEXT_EIP

// NOT TESTED
bc_DECd:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         qword ptr [eax]
		fsub        qword ptr [one]
		fstp        qword ptr [eax]
		NEXT_EIP

// TESTED
bc_ADDd:
		fld         qword ptr [ebx]
		fadd        qword ptr [ebx+8]
		add         ebx,8
		add         esi,4
		fstp        qword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_SUBd:
		fld         qword ptr [ebx+8]
        fsub        qword ptr [ebx]
        add         ebx,8
        add         esi,4
        fstp        qword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_MULd:
		fld         qword ptr [ebx]
		fmul        qword ptr [ebx+8]
		add         ebx,8
		add         esi,4
		fstp        qword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_DIVd:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		or          eax,ecx
		je          ExceptionDivByZero
		fld         qword ptr [ebx+8]
		fdiv        qword ptr [ebx]
		add         ebx,8
		add         esi,4
		fstp        qword ptr [ebx]
		NEXT_EIP

// TESTED
bc_MODd:
		mov         ecx,dword ptr [ebx]
		mov         eax,dword ptr [ebx+4]
		or          ecx,eax
		je          ExceptionDivByZero
		mov         edx,dword ptr [ebx+4]
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+0Ch]
		add         ebx,8
		push        edx
		push        eax
		mov         edx,dword ptr [ebx]
		push        ecx
		push        edx
		call        work_around_fmod
		fstp        qword ptr [ebx]
		add         esp,10h
		add         esi,4
		NEXT_EIP

// TESTED
bc_SWAP8:
		mov         edx,dword ptr [ebx+8]
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		mov         dword ptr [ebx],edx
		mov         edx,dword ptr [ebx+0Ch]
		add         esi,4
		mov         dword ptr [ebx+8],eax
		mov         dword ptr [ebx+4],edx
		mov         dword ptr [ebx+0Ch],ecx
		NEXT_EIP

// NOT TESTED
bc_CMPd:
		fld         qword ptr [ebx+8]
		fsub        qword ptr [ebx]
		add         ebx,0Ch
		fcomp       qword ptr [zero]
		fnstsw      ax
		test        ah,40h
		je          cmpd_lbl
		add         esi,4
		mov         dword ptr [ebx],0
		NEXT_EIP
cmpd_lbl:
		test        ah,1
		je          cmpd_lbl2
		add         esi,4
		mov         dword ptr [ebx],0FFFFFFFFh
		NEXT_EIP
cmpd_lbl2:
		mov         dword ptr [ebx],1
		add         esi,4
		NEXT_EIP

// NOT TESTED
bc_dTOi:
		fld         qword ptr [ebx]
		call        _ftol
		add         ebx,4
		mov         dword ptr [ebx],eax
		add         esi,4
		NEXT_EIP

// NOT TESTED
bc_dTOui:
		fld         qword ptr [ebx]
		call        _ftol
		add         ebx,4
		mov         dword ptr [ebx],eax
		add         esi,4
		NEXT_EIP

// NOT TESTED
bc_dTOf:
		fld         qword ptr [ebx]
        add         ebx,4
        fstp        dword ptr [ebx]
        add         esi,4          
		NEXT_EIP

// NOT TESTED
bc_iTOd:
		fild        dword ptr [ebx]
        sub         ebx,4
        add         esi,4
        fstp        qword ptr [ebx]    
		NEXT_EIP

// NOT TESTED
bc_uiTOd:
		mov         eax,dword ptr [ebx]
		mov         dword ptr [temp+4],0
		mov         dword ptr [temp],eax
		sub         ebx,4
		fild        qword ptr [temp]
		add         esi,4
		fstp        qword ptr [ebx]
		NEXT_EIP

// NOT TESTED
bc_fTOd:
		fld         dword ptr [ebx]
        sub         ebx,4
        fstp        qword ptr [ebx]
        add         esi,4          
		NEXT_EIP

// TESTED
bc_JMPP:
		mov         ecx,dword ptr [ebx]
        add         ebx,4
        lea         esi,[esi+ecx*8+4]  
		NEXT_EIP

// TESTED
bc_PEID:
		mov         edi,this
		mov         edx,dword ptr [edi+28h]
		sub         ebx,4
		add         esi,4
		mov         dword ptr [ebx],edx
		NEXT_EIP

// TESTED
bc_SRET4:
		mov         edi,this
		mov         eax,dword ptr [ebx]
		add         ebx,4
		add         esi,4
		mov         dword ptr [edi+30h],eax
		NEXT_EIP

// NOT TESTED
bc_SRET8:
		mov         edi,this
		mov         ecx,dword ptr [ebx]
		add         ebx,8
		mov         dword ptr [edi+30h],ecx
		mov         edx,dword ptr [ebx-4]
		add         esi,4
		mov         dword ptr [edi+34h],edx
		NEXT_EIP

// TESTED
bc_RRET4:
		mov         edi,this
		mov         eax,dword ptr [edi+30h]
		sub         ebx,4
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

// TESTED
bc_RRET8:
		mov         edi,this
		mov         ecx,dword ptr [edi+30h]
		sub         ebx,8
		add         esi,4
		mov         dword ptr [ebx],ecx
		mov         edx,dword ptr [edi+34h]
		mov         dword ptr [ebx+4],edx
		NEXT_EIP

// TESTED
bc_STR:
		mov         edi,this
		mov         ax,word ptr [esi+4]
		mov         ecx,dword ptr [edi+10h]
		and         eax,0FFFFh
		sub         ebx,4
		push        eax
		call        module_GetConstantBStr
		mov         eax,dword ptr [eax]
		mov         dword ptr [ebx],eax
		sub         ebx,4
		push        eax
		call        asBStrLength
		add         esp,4
		add         esi,6
		mov         dword ptr [ebx],eax
		NEXT_EIP

// NOT TESTED
bc_JS:
		cmp         dword ptr [ebx],0
		jge         js_lbl
		add         esi,dword ptr [esi+4]
js_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP
		
// NOT TESTED
bc_JNS:
		cmp         dword ptr [ebx],0
		jl          jns_lbl
		add         esi,dword ptr [esi+4]
jns_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

// NOT TESTED
bc_JP:
		cmp         dword ptr [ebx],0
		jle         jp_lbl
		add         esi,dword ptr [esi+4]
jp_lbl:
		add         ebx,4
		add         esi,8                
		NEXT_EIP

// NOT TESTED
bc_JNP:
		cmp         dword ptr [ebx],0
		jg          jnp_lbl
		add         esi,dword ptr [esi+4]
jnp_lbl:
		add         ebx,4
		add         esi,8                
		NEXT_EIP

// TESTED
bc_CMPIi:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [esi+4]
		sub         eax,ecx
		jne         cmpii_lbl
		xor         eax,eax
		add         esi,8
		mov         dword ptr [ebx],eax
		NEXT_EIP
cmpii_lbl:		
		xor         ecx,ecx
		test        eax,eax
		setge       cl
		dec         ecx
		and         ecx,0FFFFFFFEh
		inc         ecx
		add         esi,8
		mov         eax,ecx
		mov         dword ptr [ebx],eax
		NEXT_EIP

// NOT TESTED
bc_CMPIui:
		mov         eax,dword ptr [esi+4]
		mov         ecx,dword ptr [ebx]
		cmp         ecx,eax
		jne         cmpiui_lbl 
		xor         eax,eax
		add         esi,8
		mov         dword ptr [ebx],eax
		NEXT_EIP           
cmpiui_lbl:		
		sbb         eax,eax
		and         al,0FEh
		inc         eax
		add         esi,8
		mov         dword ptr [ebx],eax
		NEXT_EIP    

// TESTED
bc_CALLSYS:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		mov         edx,dword ptr [asm_stackFramePointer]
		push        edi
		push        eax
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],edx
		call        CallSystemFunction
		lea         ebx,[ebx+eax*4]
		mov         al,byte ptr [edi+18h]			// al = this->doSuspend
		add         esp,8
		add         esi,8
		test        al,al
		jne         ReturnSuspend
		cmp         dword ptr [edi+14h],3			// status != tsActive
		jne         Return
		NEXT_EIP

// TESTED
bc_CALLBND:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		mov         ecx,dword ptr [asm_stackFramePointer]
		mov         edx,dword ptr [edi+10h]
		add         esi,8
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],ecx
		mov         ecx,dword ptr [edx+11Ch]
		and         eax,0FFFFh
		mov         esi,dword ptr [ecx+eax*8+4]
		cmp         esi,0FFh
		je          ExceptionUnboundFunction
		mov         ecx,dword ptr [edi+0Ch]
		push        esi
		call        engine_GetModule
		mov         ebx,eax
		push        esi
		mov         ecx,ebx
		call        module_GetScriptFunction
		push        eax
		push        ebx
		mov         ecx,edi
		call        context_CallScriptFunction
		mov         eax,dword ptr [edi+14h]
		mov         esi,dword ptr [edi+1Ch]
		mov         edx,dword ptr [edi+24h]
		mov         ebx,dword ptr [edi+50h]
		cmp         eax,3
		mov         dword ptr [asm_stackFramePointer],edx
		jne         Return
		NEXT_EIP

ExceptionUnboundFunction:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		// Raise exception
		SetInternalException(TXT_UNBOUND_FUNCTION);
		return;
	} __asm { 


// NOT TESTED
bc_RDGA4:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		test        eax,eax
		jge         rdga4_lbl
		mov         ecx,dword ptr [edi+0Ch]          // ecx = this->engine
		lea         eax,[eax*4+4]                    
		sub         ebx,4
		mov         edx,dword ptr [ecx+74h]          // edx = engine->globalPropAddresses.array
		sub         edx,eax                          
		add         esi,8
		mov         eax,dword ptr [edx]              
		mov         eax,dword ptr [eax]              
		mov         dword ptr [ebx],eax              
		NEXT_EIP
rdga4_lbl:
		mov         ecx,dword ptr [edi+10h]          // ecx = this->module
		and         eax,0FFFFh
		sub         ebx,4
		add         esi,8
		mov         edx,dword ptr [ecx+134h]         // edx = module->globalMem.array
		lea         eax,[edx+eax*4]                  
		mov         eax,dword ptr [eax]              
		mov         dword ptr [ebx],eax              
		NEXT_EIP

// NOT TESTED
bc_MOVGA4:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		test        eax,eax
		jge         movga4_lbl
		mov         edx,dword ptr [edi+0Ch]			// edx = this->engine
		lea         ecx,[eax*4+4]
		add         ebx,4
		mov         eax,dword ptr [edx+74h]			// eax = engine->globalPropAddresses.array
		sub         eax,ecx
		mov         ecx,dword ptr [ebx-4]
		add         esi,8
		mov         eax,dword ptr [eax]
		mov         dword ptr [eax],ecx
		NEXT_EIP
movga4_lbl:
		mov         ecx,dword ptr [edi+10h]			// ecx = this->module
		and         eax,0FFFFh
		add         ebx,4
		add         esi,8
		mov         edx,dword ptr [ecx+134h]		// edx = module->globalMem.array
		mov         ecx,dword ptr [ebx-4]
		mov         dword ptr [edx+eax*4],ecx
		lea         eax,[edx+eax*4]
		NEXT_EIP

// NOT TESTED
bc_ADDIi:
		mov         edx,dword ptr [esi+4]
		mov         ecx,dword ptr [ebx]
		add         ecx,edx
		add         esi,8
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

// NOT TESTED
bc_SUBIi:
		mov         eax,dword ptr [esi+4]
		mov         ecx,dword ptr [ebx]
		sub         ecx,eax
		add         esi,8
		mov         dword ptr [ebx],ecx
		NEXT_EIP

// NOT TESTED
bc_CMPIf:
		fld         dword ptr [ebx]
		fsub        dword ptr [esi+4]
		fcomp       dword ptr [zero]
		fnstsw      ax
		test        ah,40h
		je          cmpif_lbl1
		add         esi,8
		mov         dword ptr [ebx],0
		NEXT_EIP
cmpif_lbl1:
		test        ah,1
		je          cmpif_lbl2
		add         esi,8
		mov         dword ptr [ebx],0FFFFFFFFh
		NEXT_EIP
cmpif_lbl2:
		add         esi,8
		mov         dword ptr [ebx],1
		NEXT_EIP


// NOT TESTED
bc_ADDIf:
		fld         dword ptr [esi+4]
		fadd        dword ptr [ebx]
		add         esi,8
		fstp        dword ptr [ebx]      
		NEXT_EIP

// NOT TESTED
bc_SUBIf:
		fld         dword ptr [ebx]
		fsub        dword ptr [esi+4]
		add         esi,8
		fstp        dword ptr [ebx]      
		NEXT_EIP

// NOT TESTED
bc_MULIi:
		mov         ecx,dword ptr [esi+4]
        imul        ecx,dword ptr [ebx]
        add         esi,8
        mov         dword ptr [ebx],ecx
		NEXT_EIP

// NOT TESTED
bc_MULIf:
		fld         dword ptr [esi+4]
        fmul        dword ptr [ebx]
        add         esi,8
        fstp        dword ptr [ebx]      
		NEXT_EIP

// TESTED
bc_SUSPEND:
		add         esi,4
		mov         edi,this
		mov         al,byte ptr [edi+18h]			// al = this->doSuspend
		test        al,al
		jne         ReturnSuspend
		NEXT_EIP

ReturnSuspend:
		mov [asm_byteCode], esi
		mov [asm_stackPointer], ebx
	} {
		// Need to move the values back to the context
		byteCode = asm_byteCode;
		stackPointer = asm_stackPointer;
		stackFramePointer = asm_stackFramePointer;

		status = tsSuspended;
		return;
	} __asm {

// TESTED
bc_END:
		jmp ReturnSuspend
	}

#else if ASM_AT_N_T
	assert(0);
#endif

	return;
}

#endif
