/*
   AngelCode Scripting Library
   Copyright (c) 2003-2005 Andreas Jönsson

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
static void *global_fmod = (double (*)(double, double))fmod;
static void *global_CallSystemFunction = (int (*)(int, asCContext*, void*))CallSystemFunction;

asUPtr uptr_module_GetScriptFunction = asMETHOD(asCModule,GetScriptFunction);
static void *module_GetScriptFunction = uptr_module_GetScriptFunction.func;

asUPtr uptr_context_CallScriptFunction = asMETHOD(asCContext,CallScriptFunction);
static void *context_CallScriptFunction = uptr_context_CallScriptFunction.func;

asUPtr uptr_context_PopCallState = asMETHOD(asCContext,PopCallState);
static void *context_PopCallState = uptr_context_PopCallState.func;

asUPtr uptr_module_GetConstantBStr = asMETHOD(asCModule,GetConstantBStr);
static void *module_GetConstantBStr = uptr_module_GetConstantBStr.func;

asUPtr uptr_engine_GetModule = asMETHODPR(asCScriptEngine,GetModule,(int),asCModule*);
static void *engine_GetModule = uptr_engine_GetModule.func;

asUPtr uptr_engine_CallAlloc = asMETHODPR(asCScriptEngine,CallAlloc,(int),void*);
static void *engine_CallAlloc = uptr_engine_CallAlloc.func;

asUPtr uptr_engine_CallFree = asMETHODPR(asCScriptEngine,CallFree,(int,void*),void);
static void *engine_CallFree = uptr_engine_CallFree.func;

asUPtr uptr_engine_CallObjectMethod = asMETHODPR(asCScriptEngine,CallObjectMethod,(void*,int),void);
static void *engine_CallObjectMethod = uptr_engine_CallObjectMethod.func;

asUPtr uptr_context_CallLineCallback = asMETHOD(asCContext,CallLineCallback);
static void *context_CallLineCallback = uptr_context_CallLineCallback.func;


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
			assert((offset = offsetof(asCContext      , engine             )) == 0x0C);
			assert((offset = offsetof(asCContext      , module             )) == 0x10);
			assert((offset = offsetof(asCContext      , status             )) == 0x14);
			assert((offset = offsetof(asCContext      , doSuspend          )) == 0x18);
			assert((offset = offsetof(asCContext      , byteCode           )) == 0x1C);
			assert((offset = offsetof(asCContext      , stackFramePointer  )) == 0x24);
			assert((offset = offsetof(asCContext      , returnVal          )) == 0x30);
			assert((offset = offsetof(asCContext      , callStack          )) == 0x38);
			assert((offset = offsetof(asCContext      , stackPointer       )) == 0x50);
			assert((offset = offsetof(asCContext      , objectRegister     )) == 0x80);
			assert((offset = offsetof(asCContext      , objectType         )) == 0x84);
			assert((offset = offsetof(asCContext      , lineCallback       )) == 0x90);
			assert((offset = offsetof(asCScriptEngine , globalPropAddresses)) == 0x6C);
			assert((offset = offsetof(asCScriptEngine , allObjectTypes     )) == 0x78);
			assert((offset = offsetof(asCModule       , globalMem          )) == 0x168);
			assert((offset = offsetof(asCModule       , bindInformations   )) == 0x150);
			assert((offset = offsetof(asCObjectType   , beh                )) == 0x3C);
			assert((offset = offsetof(asSTypeBehaviour, release            )) == 0x10);

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
				OFFSET(bc_ALLOC)
				OFFSET(bc_FREE)
				OFFSET(bc_LOADOBJ)
				OFFSET(bc_STOREOBJ)
				OFFSET(bc_GETOBJ)
				OFFSET(bc_REFCPY)
				OFFSET(bc_CHKREF)
				OFFSET(bc_RD1)
				OFFSET(bc_RD2)
				OFFSET(bc_GETOBJREF)
				OFFSET(bc_GETREF)
				OFFSET(bc_SWAP48)
				OFFSET(bc_SWAP84)

			}

#elif ASM_AT_N_T
			assert(0);
#endif

			isRelocationTableInitialized = true;
		}
		return;
	}	
	
	asBYTE  *l_bc;  // byte code
	asDWORD *l_sp;  // stack pointer
	asDWORD *l_fp;  // frame pointer

	// Temporary storage for STORE4/RECALL4
	asDWORD  asm_tempReg;

	// Temporary variables used within the instructions
	asQWORD temp1;
	asQWORD temp2;
	asQWORD temp3;
	
#ifdef ASM_INTEL
	__asm 
	{		
		mov ecx, this
		// Set l_bc = this->byteCode
		mov esi, [ecx+1Ch]
		mov [l_bc], esi
		// Set l_sp = this->stackPointer
		mov ebx, [ecx+50h]
		mov [l_sp], ebx
		// Set l_fp = this->stackFramePointer
		mov eax, [ecx+24h]
		mov [l_fp], eax				
		NEXT_EIP	

bc_POP:			
		xor         ecx,ecx
        mov         cx,word ptr [esi+4]
        add         esi,6
        lea         ebx,[ebx+ecx*4]    
		NEXT_EIP		
	
bc_PUSH:		
		xor         edx,edx
        mov         dx,word ptr [esi+4]
        neg         edx
        add         esi,6
        lea         ebx,[ebx+edx*4]    
		NEXT_EIP
		
bc_SET4:
		mov         eax,dword ptr [esi+4]
		sub         ebx,4
		add         esi,8
		mov         dword ptr [ebx],eax  
		NEXT_EIP
		
bc_RD4:
		mov         ecx,dword ptr [ebx]
        add         esi,4
        mov         edx,dword ptr [ecx]
        mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_RDSF4:
		movsx       eax,word ptr [esi+4]
		mov         ecx,[l_fp]
		sub         ebx,4
		shl         eax,2                
		sub         ecx,eax
		add         esi,6
		mov         edx,dword ptr [ecx]
		mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_WRT4:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		mov         dword ptr [eax],ecx  
		NEXT_EIP
	
bc_MOV4:
		mov         edx,dword ptr [ebx]
		mov         eax,dword ptr [ebx+4]
		add         ebx,8
		add         esi,4
		mov         dword ptr [edx],eax  
		NEXT_EIP
	
bc_PSF:
		movsx       ecx,word ptr [esi+4]
        mov         edx,[l_fp]
        sub         ebx,4
        shl         ecx,2                
        sub         edx,ecx
        add         esi,6
        mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_MOVSF4:
		movsx       eax,word ptr [esi+4]
		mov         ecx,[l_fp]
		mov         edx,dword ptr [ebx]
		shl         eax,2                
		sub         ecx,eax
		add         ebx,4
		add         esi,6
		mov         dword ptr [ecx],edx  
		NEXT_EIP

bc_SWAP4:
		mov         eax,dword ptr [ebx]
        mov         ecx,dword ptr [ebx+4]
        add         esi,4
        mov         dword ptr [ebx],ecx
        mov         dword ptr [ebx+4],eax
		NEXT_EIP

bc_STORE4:
		mov         edx,dword ptr [ebx]
        add         esi,4
        mov         [asm_tempReg],edx
		NEXT_EIP

bc_RECALL4:
		mov         eax,[asm_tempReg]
		sub         ebx,4
		add         esi,4
		mov         dword ptr [ebx],eax  
		NEXT_EIP

bc_CALL:
		mov         edi,this
		mov         ecx,[l_fp]
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
		mov         [l_fp],eax
		mov         eax,dword ptr [edi+14h]     // eax = this->status
		cmp         eax,3						// status != tsActive
		jne         Return
		NEXT_EIP

bc_RET:
		mov         edi,this
		mov         eax,dword ptr [edi+3Ch]     // eax = this->callStack.length
		test        eax,eax
		je          ReturnFinished
		mov         cx,word ptr [esi+4]
		mov         edx,[l_fp]
		mov         word ptr [temp1],cx			// temp = ecx (return pop)
		mov         ecx,edi
		mov         dword ptr [edi+1Ch],esi     // this->byteCode = esi
		mov         dword ptr [edi+50h],ebx     // this->stackPointer = ebx
		mov         dword ptr [edi+24h],edx     // this->stackFramePointer = edx
		call        context_PopCallState
		mov         eax,dword ptr [edi+24h]     // eax = this->stackFramePointer
		mov         ecx,dword ptr [edi+50h]     // ecx = this->stackPointer
		mov         esi,dword ptr [edi+1Ch]     // esi = this->byteCode
		mov         [l_fp],eax
		mov         eax,dword ptr [temp1]       // eax = temp (return pop)
		and         eax,0FFFFh                 
		lea         ebx,[ecx+eax*4]             // pop the arguments
		NEXT_EIP


bc_JMP:
		mov         edx,dword ptr [esi+4]
		lea         esi,[esi+edx+8]      
		NEXT_EIP

bc_JZ:
		cmp         dword ptr [ebx],0
		jne         jz_lbl
		add         esi,dword ptr [esi+4]
jz_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

bc_JNZ:
		cmp         dword ptr [ebx],0
		je          jnz_lbl
		add         esi,dword ptr [esi+4]
jnz_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

bc_TZ:
		mov         ecx,dword ptr [ebx]
        xor         eax,eax
        test        ecx,ecx
        sete        al
        add         esi,4
        mov         dword ptr [ebx],eax
		NEXT_EIP

bc_TNZ:
		mov         edx,dword ptr [ebx]
        xor         ecx,ecx
        test        edx,edx
        setne       cl
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

bc_TS:
		mov         ecx,dword ptr [ebx]
        xor         edx,edx
        test        ecx,ecx
        setl        dl
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

bc_TNS:
		mov         ecx,dword ptr [ebx]
	    xor         eax,eax
	    test        ecx,ecx
	    setge       al
	    add         esi,4
	    mov         dword ptr [ebx],eax
		NEXT_EIP

bc_TP:
		mov         edx,dword ptr [ebx]
        xor         ecx,ecx
        test        edx,edx
        setg        cl
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

bc_TNP:
		mov         ecx,dword ptr [ebx]
        xor         edx,edx
        test        ecx,ecx
        setle       dl
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

bc_ADDi:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         ecx,eax
		add         esi,4
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

bc_SUBi:
		mov         ecx,dword ptr [ebx]
		mov         edx,dword ptr [ebx+4]
		add         ebx,4
		sub         edx,ecx
		add         esi,4
		mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_MULi:
		mov         edx,dword ptr [ebx]
		add         ebx,4
		imul        edx,dword ptr [ebx]
		add         esi,4
		mov         dword ptr [ebx],edx
		NEXT_EIP

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

bc_NEGi:
		mov         eax,dword ptr [ebx]
		neg         eax
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

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

bc_INCi:
		mov         eax,dword ptr [ebx]
        add         esi,4
        inc         int ptr [eax]
		NEXT_EIP

bc_DECi:
		mov         eax,dword ptr [ebx]
        add         esi,4
        dec         int ptr [eax]
		NEXT_EIP

bc_I2F:
		fild        dword ptr [ebx]
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

bc_ADDf:
		fld         dword ptr [ebx]
        fadd        dword ptr [ebx+4]
        add         ebx,4
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

bc_SUBf:
		fld         dword ptr [ebx+4]
        fsub        dword ptr [ebx]
        add         ebx,4
        add         esi,4
        fstp        dword ptr [ebx]      
		NEXT_EIP

bc_MULf:
		fld         dword ptr [ebx]
		fmul        dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		fstp        dword ptr [ebx]      
		NEXT_EIP

bc_DIVf:
		cmp         dword ptr [ebx],0
		je          ExceptionDivByZero
		fld         dword ptr [ebx+4]
		fdiv        dword ptr [ebx]
		add         ebx,4
		add         esi,4
		fstp        dword ptr [ebx]
		NEXT_EIP

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

bc_NEGf:
		fld         dword ptr [ebx]
		fchs
		fstp        dword ptr [ebx]
		add         esi,4          
		NEXT_EIP

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

bc_INCf:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         dword ptr [eax]
		fadd        qword ptr [one]
		fstp        dword ptr [eax]
		NEXT_EIP

bc_DECf:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         dword ptr [eax]
		fsub        qword ptr [one]
		fstp        dword ptr [eax]
		NEXT_EIP

bc_F2I:
		fld         dword ptr [ebx]
		call        _ftol
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

bc_BNOT:
		mov         ecx,dword ptr [ebx]
		add         esi,4
		not         ecx
		mov         dword ptr [ebx],ecx
		NEXT_EIP

bc_BAND:
		mov         edx,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		and         ecx,edx
		add         esi,4
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

bc_BOR:
		mov         eax,dword ptr [ebx]
        mov         ecx,dword ptr [ebx+4]
        add         ebx,4
        or          ecx,eax
        add         esi,4
        mov         dword ptr [ebx],ecx  
        NEXT_EIP

bc_BXOR:
		mov         ecx,dword ptr [ebx]
        mov         edx,dword ptr [ebx+4]
        add         ebx,4
        xor         edx,ecx
        add         esi,4
        mov         dword ptr [ebx],edx  
        NEXT_EIP

bc_BSLL:
		mov         ecx,dword ptr [ebx]
        mov         edx,dword ptr [ebx+4]
        add         ebx,4
        shl         edx,cl
        add         esi,4
        mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_BSRL:
		mov         ecx,dword ptr [ebx]
		mov         edx,dword ptr [ebx+4]
		add         ebx,4
		shr         edx,cl
		add         esi,4
        mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_BSRA:
		mov         edx,dword ptr [ebx+4]
		mov         ecx,dword ptr [ebx]
		add         ebx,4
		sar         edx,cl
		add         esi,4
		mov         dword ptr [ebx],edx  
		NEXT_EIP

bc_UI2F:
		mov         eax,dword ptr [ebx]
		mov         dword ptr [temp1+4],0
		mov         dword ptr [temp1],eax
		add         esi,4
		fild        qword ptr [temp1]
		fstp        dword ptr [ebx]
		NEXT_EIP

bc_F2UI:
		fld         dword ptr [ebx]
		call        _ftol
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

bc_CMPui:
		mov         eax,dword ptr [ebx+4]
        mov         ecx,dword ptr [ebx]
        add         ebx,4
        cmp         eax,ecx
        jne         cmpui_lbl
        xor         eax,eax
        add         esi,4
        mov         dword ptr [ebx],eax
        NEXT_EIP     
cmpui_lbl:
        sbb         eax,eax
        and         al,0FEh
        inc         eax
        add         esi,4
        mov         dword ptr [ebx],eax
        NEXT_EIP   

bc_SB:
		movsx       ecx,byte ptr [ebx]
        add         esi,4
        mov         dword ptr [ebx],ecx
		NEXT_EIP

bc_SW:
		movsx       edx,word ptr [ebx]
        add         esi,4
        mov         dword ptr [ebx],edx
		NEXT_EIP

bc_UB:
		xor         eax,eax
        add         esi,4
        mov         al,byte ptr [ebx]
        mov         dword ptr [ebx],eax  
		NEXT_EIP

bc_UW:
		xor         ecx,ecx
        add         esi,4
        mov         cx,word ptr [ebx]
        mov         dword ptr [ebx],ecx  
		NEXT_EIP

bc_WRT1:
		mov         edx,dword ptr [ebx]
        mov         al,byte ptr [ebx+4]
        add         ebx,4
        add         esi,4
        mov         byte ptr [edx],al  
		NEXT_EIP

bc_WRT2:
		mov         ecx,dword ptr [ebx]
        mov         dx,word ptr [ebx+4]
        add         ebx,4
        add         esi,4
        mov         word ptr [ecx],dx  
		NEXT_EIP

bc_INCi16:
		mov         eax,dword ptr [ebx]
        inc         word ptr [eax]
        add         esi,4              
		NEXT_EIP

bc_INCi8:
		mov         eax,dword ptr [ebx]
        mov         cl,byte ptr [eax]
        inc         cl
        add         esi,4
        mov         byte ptr [eax],cl  
		NEXT_EIP

bc_DECi16:
		mov         eax,dword ptr [ebx]
        dec         word ptr [eax]
        add         esi,4              
		NEXT_EIP

bc_DECi8:
		mov         eax,dword ptr [ebx]
        mov         cl,byte ptr [eax]
        dec         cl
        add         esi,4
        mov         byte ptr [eax],cl  
		NEXT_EIP

bc_PUSHZERO:
		sub         ebx,4
        add         esi,4
        mov         dword ptr [ebx],0    
		NEXT_EIP

bc_COPY:
		mov         dword ptr [temp1],esi
		mov         eax,dword ptr [ebx]
		mov         edx,dword ptr [ebx+4]
		add         ebx,4
		mov         dword ptr [temp2],eax
		test        edx,edx
		je          ExceptionNullPointerAccess
		test        eax,eax
		je          ExceptionNullPointerAccess
		mov         eax,dword ptr [temp1]
		xor         ecx,ecx
		mov         edi,dword ptr [temp2]
		mov         esi,edx
		mov         cx,word ptr [eax+4]
		shl         ecx,2
		mov         edx,ecx
		shr         ecx,2
		rep movs    dword ptr [edi],dword ptr [esi]
		mov         ecx,edx
		and         ecx,3
		add         eax,6
		rep movs    byte ptr [edi],byte ptr [esi]
		mov         esi,eax
		NEXT_EIP

bc_PGA:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		sub         ebx,4
		test        eax,eax
		jge         pga_lbl
		mov         ecx,dword ptr [edi+0Ch]
		lea         eax,[eax*4+4]
		mov         edx,dword ptr [ecx+6Ch]   // globalPropAddress
		sub         edx,eax
		add         esi,8
		mov         eax,dword ptr [edx]
		mov         dword ptr [ebx],eax
		NEXT_EIP
pga_lbl:
		mov         ecx,dword ptr [edi+10h]
		and         eax,0FFFFh
		add         esi,8
		mov         edx,dword ptr [ecx+168h]   // module::globalMem
		lea         eax,[edx+eax*4]
		mov         dword ptr [ebx],eax
		NEXT_EIP

bc_SET8:
		mov         ecx,dword ptr [esi+4]
		sub         ebx,8
		add         esi,0Ch
		mov         dword ptr [ebx],ecx
		mov         edx,dword ptr [esi-4]
		mov         dword ptr [ebx+4],edx
		NEXT_EIP

bc_WRT8:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		add         ebx,4
		add         esi,4
		mov         dword ptr [eax],ecx
		mov         edx,dword ptr [ebx+4]
		mov         dword ptr [eax+4],edx
		NEXT_EIP

bc_RD8:
		mov         eax,dword ptr [ebx]
        sub         ebx,4
        add         esi,4
        mov         ecx,dword ptr [eax]
        mov         dword ptr [ebx],ecx
        mov         edx,dword ptr [eax+4]
        mov         dword ptr [ebx+4],edx
		NEXT_EIP

bc_NEGd:
		fld         qword ptr [ebx]
        fchs
        fstp        qword ptr [ebx]
        add         esi,4          
		NEXT_EIP

bc_INCd:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         qword ptr [eax]
		fadd        qword ptr [one]
		fstp        qword ptr [eax]
		NEXT_EIP

bc_DECd:
		mov         eax,dword ptr [ebx]
		add         esi,4
		fld         qword ptr [eax]
		fsub        qword ptr [one]
		fstp        qword ptr [eax]
		NEXT_EIP

bc_ADDd:
		fld         qword ptr [ebx]
		fadd        qword ptr [ebx+8]
		add         ebx,8
		add         esi,4
		fstp        qword ptr [ebx]      
		NEXT_EIP

bc_SUBd:
		fld         qword ptr [ebx+8]
        fsub        qword ptr [ebx]
        add         ebx,8
        add         esi,4
        fstp        qword ptr [ebx]      
		NEXT_EIP

bc_MULd:
		fld         qword ptr [ebx]
		fmul        qword ptr [ebx+8]
		add         ebx,8
		add         esi,4
		fstp        qword ptr [ebx]      
		NEXT_EIP

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
		call        global_fmod
		fstp        qword ptr [ebx]
		add         esp,10h
		add         esi,4
		NEXT_EIP

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

bc_dTOi:
		fld         qword ptr [ebx]
		call        _ftol
		add         ebx,4
		mov         dword ptr [ebx],eax
		add         esi,4
		NEXT_EIP

bc_dTOui:
		fld         qword ptr [ebx]
		call        _ftol
		add         ebx,4
		mov         dword ptr [ebx],eax
		add         esi,4
		NEXT_EIP

bc_dTOf:
		fld         qword ptr [ebx]
        add         ebx,4
        fstp        dword ptr [ebx]
        add         esi,4          
		NEXT_EIP

bc_iTOd:
		fild        dword ptr [ebx]
        sub         ebx,4
        add         esi,4
        fstp        qword ptr [ebx]    
		NEXT_EIP

bc_uiTOd:
		mov         eax,dword ptr [ebx]
		mov         dword ptr [temp1+4],0
		mov         dword ptr [temp1],eax
		sub         ebx,4
		fild        qword ptr [temp1]
		add         esi,4
		fstp        qword ptr [ebx]
		NEXT_EIP

bc_fTOd:
		fld         dword ptr [ebx]
        sub         ebx,4
        fstp        qword ptr [ebx]
        add         esi,4          
		NEXT_EIP

bc_JMPP:
		mov         ecx,dword ptr [ebx]
        add         ebx,4
        lea         esi,[esi+ecx*8+4]  
		NEXT_EIP

bc_SRET4:
		mov         edi,this
		mov         eax,dword ptr [ebx]
		add         ebx,4
		add         esi,4
		mov         dword ptr [edi+30h],eax
		NEXT_EIP

bc_SRET8:
		mov         edi,this
		mov         ecx,dword ptr [ebx]
		add         ebx,8
		mov         dword ptr [edi+30h],ecx
		mov         edx,dword ptr [ebx-4]
		add         esi,4
		mov         dword ptr [edi+34h],edx
		NEXT_EIP

bc_RRET4:
		mov         edi,this
		mov         eax,dword ptr [edi+30h]
		sub         ebx,4
		add         esi,4
		mov         dword ptr [ebx],eax
		NEXT_EIP

bc_RRET8:
		mov         edi,this
		mov         ecx,dword ptr [edi+30h]
		sub         ebx,8
		add         esi,4
		mov         dword ptr [ebx],ecx
		mov         edx,dword ptr [edi+34h]
		mov         dword ptr [ebx+4],edx
		NEXT_EIP

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

bc_JS:
		cmp         dword ptr [ebx],0
		jge         js_lbl
		add         esi,dword ptr [esi+4]
js_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP
		
bc_JNS:
		cmp         dword ptr [ebx],0
		jl          jns_lbl
		add         esi,dword ptr [esi+4]
jns_lbl:
		add         ebx,4
		add         esi,8
		NEXT_EIP

bc_JP:
		cmp         dword ptr [ebx],0
		jle         jp_lbl
		add         esi,dword ptr [esi+4]
jp_lbl:
		add         ebx,4
		add         esi,8                
		NEXT_EIP

bc_JNP:
		cmp         dword ptr [ebx],0
		jg          jnp_lbl
		add         esi,dword ptr [esi+4]
jnp_lbl:
		add         ebx,4
		add         esi,8                
		NEXT_EIP

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

bc_CALLSYS:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		xor         ecx,ecx
		mov         edx,dword ptr [l_fp]
		push        ecx
		push        edi
		push        eax
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],edx
		call        global_CallSystemFunction
		lea         ebx,[ebx+eax*4]
		mov         al,byte ptr [edi+18h]			// al = this->doSuspend
		add         esp,0Ch
		add         esi,8
		test        al,al
		jne         ReturnSuspend
		cmp         dword ptr [edi+14h],3			// status != tsActive
		jne         Return
		NEXT_EIP

bc_CALLBND:
		mov         edi,this
		mov         eax,dword ptr [esi+4]          // i
		mov         ecx,dword ptr [l_fp]
		mov         edx,dword ptr [edi+10h]        // module
		add         esi,8
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],ecx
		mov         ecx,dword ptr [edx+150h]       // module->bindInformations
		and         eax,0FFFFh
		mov         esi,dword ptr [ecx+eax*8+4]    // module->bindInformations[i&0xFFFF].importedFunction
		cmp         esi,0FFFFFFFFh
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
		mov         dword ptr [l_fp],edx
		jne         Return
		NEXT_EIP

bc_RDGA4:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		test        eax,eax
		jge         rdga4_lbl
		mov         ecx,dword ptr [edi+0Ch]          // ecx = this->engine
		lea         eax,[eax*4+4]                    
		sub         ebx,4
		mov         edx,dword ptr [ecx+6Ch]          // edx = engine->globalPropAddresses.array
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
		mov         edx,dword ptr [ecx+168h]         // edx = module->globalMem.array
		lea         eax,[edx+eax*4]                  
		mov         eax,dword ptr [eax]              
		mov         dword ptr [ebx],eax              
		NEXT_EIP

bc_MOVGA4:
		mov         edi,this
		mov         eax,dword ptr [esi+4]
		test        eax,eax
		jge         movga4_lbl
		mov         edx,dword ptr [edi+0Ch]			// edx = this->engine
		lea         ecx,[eax*4+4]
		add         ebx,4
		mov         eax,dword ptr [edx+6Ch]			// eax = engine->globalPropAddresses.array
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
		mov         edx,dword ptr [ecx+168h]		// edx = module->globalMem.array
		mov         ecx,dword ptr [ebx-4]
		mov         dword ptr [edx+eax*4],ecx
		lea         eax,[edx+eax*4]
		NEXT_EIP

bc_ADDIi:
		mov         edx,dword ptr [esi+4]
		mov         ecx,dword ptr [ebx]
		add         ecx,edx
		add         esi,8
		mov         dword ptr [ebx],ecx  
		NEXT_EIP

bc_SUBIi:
		mov         eax,dword ptr [esi+4]
		mov         ecx,dword ptr [ebx]
		sub         ecx,eax
		add         esi,8
		mov         dword ptr [ebx],ecx
		NEXT_EIP

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

bc_ADDIf:
		fld         dword ptr [esi+4]
		fadd        dword ptr [ebx]
		add         esi,8
		fstp        dword ptr [ebx]      
		NEXT_EIP

bc_SUBIf:
		fld         dword ptr [ebx]
		fsub        dword ptr [esi+4]
		add         esi,8
		fstp        dword ptr [ebx]      
		NEXT_EIP

bc_MULIi:
		mov         ecx,dword ptr [esi+4]
        imul        ecx,dword ptr [ebx]
        add         esi,8
        mov         dword ptr [ebx],ecx
		NEXT_EIP

bc_MULIf:
		fld         dword ptr [esi+4]
        fmul        dword ptr [ebx]
        add         esi,8
        fstp        dword ptr [ebx]      
		NEXT_EIP

bc_SUSPEND:
		mov         ecx,this
		mov         al,byte ptr [ecx+90h]           // al = this->lineCallback
		test        al,al
		je          suspend_lbl
		mov         eax,[l_fp] 
		mov         dword ptr [ecx+1Ch],esi
		mov         dword ptr [ecx+50h],ebx
		mov         dword ptr [ecx+24h],eax
		call        context_CallLineCallback
		mov         ecx,this
suspend_lbl:
		add         esi,4
		mov         al,byte ptr [ecx+18h]			// al = this->doSuspend
		test        al,al
		jne         ReturnSuspend
		NEXT_EIP

bc_ALLOC:
		mov         edi,this
		mov         eax,dword ptr [esi+4]           // objTypeIdx
		mov         edx,dword ptr [esi+8]           // func
		push        eax
		mov         dword ptr [temp1],edx           // store func
		mov         ecx,dword ptr [edi+0Ch]         // engine
		call        engine_CallAlloc
		mov         dword ptr [temp2],eax           // store mem
		mov         eax,dword ptr [temp1]           // get func
		test        eax,eax
		je          alloc_lbl
		mov         ecx,dword ptr [temp2]           // get mem
		mov         edx,dword ptr [temp1]           // get func
		mov         eax,[l_fp] 
		push        ecx
		push        edi
		push        edx
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],eax
		call        global_CallSystemFunction       // f(func, this, mem)
		add         esp,0Ch
		lea         ebx,[ebx+eax*4]                 // l_sp += pop size
alloc_lbl:
		mov         eax,dword ptr [ebx]             // a = *l_sp
		add         ebx,4
		test        eax,eax
		je          alloc_lbl2
		mov         ecx,dword ptr [temp2]           // get mem
		mov         dword ptr [eax],ecx             // *a = mem
alloc_lbl2:
		mov         cl,byte ptr [edi+18h]           // doSuspend
		add         esi,0Ch
		test        cl,cl
		jne         ReturnSuspend
		cmp         dword ptr [edi+14h],3           // status != tsActive
		jne         alloc_lbl3
		NEXT_EIP
alloc_lbl3:
		mov         ecx,dword ptr [temp2]           // get mem
		push        eax                             // store eax
		push        ecx
		mov         eax,dword ptr [esi-8]           // objTypeIdx
        push        eax
		mov         ecx,dword ptr [edi+0Ch]         // engine
		call        engine_CallFree                 
		pop         eax                             // *a = 0
		mov         dword ptr [eax], 0  
		jmp         Return

bc_FREE:
		mov         edi,this
		mov         eax,dword ptr [ebx]
		add         ebx,4
		test        eax,eax
		mov         dword ptr [temp1],eax
		je          free_lbl
		mov         edx,eax
		cmp         dword ptr [edx],0
		je          free_lbl
		mov         ecx,dword ptr [edi+0Ch]      // engine
		mov         eax,dword ptr [esi+4]        // objTypeIndex
		mov         edx,dword ptr [ecx+78h]      // engine->allObjectTypes
		mov         eax,dword ptr [edx+eax*4]    // [objTypeIndex]
		mov         edx,[l_fp]
		add         eax,3Ch                      // ->beh
		mov         dword ptr [edi+1Ch],esi
		mov         dword ptr [edi+50h],ebx
		mov         dword ptr [edi+24h],edx
		mov         edx,dword ptr [eax+10h]      // beh->release
		test        edx,edx
		je          free_lbl2
		mov         eax,dword ptr [temp1]
		push        edx
		mov         edx,dword ptr [eax]
		push        edx
		call        engine_CallObjectMethod
		mov         edx,dword ptr [temp1]
		add         esi,8
		mov         dword ptr [edx],0
		NEXT_EIP
free_lbl2:
		mov         eax,dword ptr [eax+4]
		test        eax,eax
		je          free_lbl3
		push        eax
		mov         eax,dword ptr [temp1]
		mov         edx,dword ptr [eax]
		push        edx
		call        engine_CallObjectMethod
free_lbl3:
		mov         eax,dword ptr [temp1]
		mov         ecx,dword ptr [eax]
		push        ecx
		mov         eax,dword ptr [esi+4]        // objTypeIndex
		push        eax
		mov         ecx,dword ptr [edi+0Ch]      // engine
		call        engine_CallFree
		mov         edx,dword ptr [temp1]
		mov         dword ptr [edx],0
free_lbl:
		add         esi,8
		NEXT_EIP
		   
bc_LOADOBJ:
		mov         edi,this
		movsx       eax,word ptr [esi+4]
		shl         eax,2
		mov         ecx,eax
		mov         eax,[l_fp]
		sub         eax,ecx
		mov         dword ptr [edi+84h],0    // objectType = 0
		add         esi,6
		mov         edx,dword ptr [eax]
		mov         dword ptr [edi+80h],edx  // objectRegister = *a
		mov         dword ptr [eax],0
		NEXT_EIP

bc_STOREOBJ:
		mov         edi,this
		movsx       eax,word ptr [esi+4]
		mov         ecx,[l_fp]
		mov         edx,dword ptr [edi+80h]  // *a = objectRegister
		shl         eax,2
		sub         ecx,eax
		add         esi,6
		mov         dword ptr [ecx],edx
		mov         dword ptr [edi+80h],0    // objectRegister = 0
		NEXT_EIP

bc_GETOBJ:
		xor         eax,eax
		mov         ax,word ptr [esi+4]
		mov         edx,dword ptr [ebx+eax*4]
		lea         ecx,[ebx+eax*4]
		mov         eax,dword ptr [l_fp]
		shl         edx,2
		sub         eax,edx
		add         esi,6
		mov         edx,dword ptr [eax]
		mov         dword ptr [ecx],edx
		mov         dword ptr [eax],0
		NEXT_EIP

bc_REFCPY:
		mov         edi,this
		mov         ecx,dword ptr [edi+0Ch]      // engine
		mov         edx,dword ptr [esi+4]
		add         ebx,4
		mov         eax,dword ptr [ecx+78h]      // engine->allObjectTypes.array
		mov         eax,dword ptr [eax+edx*4]    // + objTypeIdx
		mov         edx,dword ptr [ebx]
		add         eax,3Ch                      // ->beh
		mov         dword ptr [temp1],edx
		mov         dword ptr [temp2],eax
		mov         eax,dword ptr [ebx-4]
		mov         edx,dword ptr [l_fp]
		mov         dword ptr [temp3],eax
		mov         dword ptr [edi+1Ch],esi      // byteCode
		mov         dword ptr [edi+50h],ebx      // stackPointer
		mov         dword ptr [edi+24h],edx      // stackFramePointer
		mov         eax,dword ptr [eax]          // *d
		test        eax,eax                      // != 0
		je          refcpy_lbl
		mov         edx,dword ptr [temp2]        // beh
		mov         edx,dword ptr [edx+10h]      // beh->release
		push        edx
		push        eax
		call        engine_CallObjectMethod
refcpy_lbl:
		mov         eax,dword ptr [temp1]        // s
		test        eax,eax                      // != 0
		je          refcpy_lbl2
		mov         eax,dword ptr [temp2]        // beh
		mov         edx,dword ptr [temp1]        // s
		mov         ecx,dword ptr [eax+0Ch]      // beh->addref
		push        ecx                          
		mov         ecx,dword ptr [edi+0Ch]      // engine
		push        edx                          
		call        engine_CallObjectMethod
refcpy_lbl2:
		mov         eax,dword ptr [temp3]      
		mov         ecx,dword ptr [temp1]
		add         esi,8
		mov         dword ptr [eax],ecx          // *d = s
		NEXT_EIP

bc_CHKREF:
		cmp         dword ptr [ebx],0
		je          ExceptionNullPointerAccess
		add         esi,4
		NEXT_EIP

bc_RD1:
		mov         edx,dword ptr [ebx]
		xor         eax,eax
		add         esi,4
		mov         al,byte ptr [edx]
		mov         dword ptr [ebx],eax
		NEXT_EIP

bc_RD2:
		mov         ecx,dword ptr [ebx]
		xor         edx,edx
		add         esi,4
		mov         dx,word ptr [ecx]
		mov         dword ptr [ebx],edx
		NEXT_EIP

bc_GETOBJREF:
		xor         eax,eax
		mov         edx,dword ptr [l_fp]
		mov         ax,word ptr [esi+4]
		mov         ecx,dword ptr [ebx+eax*4]
		lea         eax,[ebx+eax*4]
		shl         ecx,2
		sub         edx,ecx
		add         esi,6
		mov         ecx,dword ptr [edx]
		mov         dword ptr [eax],ecx
		NEXT_EIP
bc_GETREF:
		xor         edx,edx
		mov         dx,word ptr [esi+4]
		mov         ecx,dword ptr [ebx+edx*4]
		lea         eax,[ebx+edx*4]
		mov         edx,dword ptr [l_fp]
		shl         ecx,2
		sub         edx,ecx
		add         esi,6
		mov         dword ptr [eax],edx
		NEXT_EIP
bc_SWAP48:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		mov         edx,dword ptr [ebx+8]
		mov         dword ptr [ebx], ecx
		mov         dword ptr [ebx+4], edx
		mov         dword ptr [ebx+8], eax
		add         esi,4
		NEXT_EIP
bc_SWAP84:
		mov         eax,dword ptr [ebx]
		mov         ecx,dword ptr [ebx+4]
		mov         edx,dword ptr [ebx+8]
		mov         dword ptr [ebx], edx
		mov         dword ptr [ebx+4], eax
		mov         dword ptr [ebx+8], ecx
		add         esi,4
		NEXT_EIP

ExceptionDivByZero:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		// Raise exception
		SetInternalException(TXT_DIVIDE_BY_ZERO);
		return;
	} __asm { 

ExceptionUnboundFunction:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		// Raise exception
		SetInternalException(TXT_UNBOUND_FUNCTION);
		return;
	} __asm { 
		
ExceptionNullPointerAccess:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		// Raise exception
		SetInternalException(TXT_NULL_POINTER_ACCESS);
		return;
	} __asm { 
		
		
ReturnSuspend:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		status = tsSuspended;
		return;
	} __asm {
		
		
ReturnFinished:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		status = tsProgramFinished;
		return;
	} __asm { 
		
Return:
		mov [l_bc], esi
		mov [l_sp], ebx
	} {
		// Need to move the values back to the context
		byteCode = l_bc;
		stackPointer = l_sp;
		stackFramePointer = l_fp;

		return;
	} __asm { 
	
		
	}

#else if ASM_AT_N_T
	assert(0);
#endif

	return;
}

#endif


