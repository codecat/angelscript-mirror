//
//  AngelCode Scripting Library
//  Copyright (c) 2024 Andreas Jonsson
//
//  This software is provided 'as-is', without any express or implied
//  warranty. In no event will the authors be held liable for any
//  damages arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any
//  purpose, including commercial applications, and to alter it and
//  redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented// you
//     must not claim that you wrote the original software. If you use
//     this software in a product, an acknowledgment in the product
//     documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and
//     must not be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//     distribution.
//
//  The original version of this library can be located at:
//  http://www.angelcode.com/angelscript/
//
//  Andreas Jonsson
//  andreas@angelcode.com
//


// Assembly routines for the 64bit RISC-V call convention used for Linux

// Compile with GCC/GAS

#if !defined(AS_MAX_PORTABILITY)

#if defined(__riscv) && defined(__LP64__)

	.file "as_callfunc_riscv64_gcc.S"
	.option pic
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.align 1
	.globl CallRiscVFunc
	.type CallRiscVFunc, @function
CallRiscVFunc:
	.cfi_startproc
	addi	sp,sp,-48                 // reserve bytes on stack (aligned to 16bytes)
	.cfi_def_cfa_offset 48
	sd		ra,40(sp)         // backup return address on stack, just below original stack frame pointer
	sd		s0,32(sp)         // backup frame pointer on stack, below return address
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	addi	s0,sp,48                  // load new frame pointer with the reserved space
	.cfi_def_cfa 8,0
	sd		a0,-24(s0)        // store func arg on stack, below backup of s0
	sd		a1,-32(s0)        // store retfloat arg on stack
	ld		a5,-24(s0)        // load func arg to a5
	jalr	a5                        // call the function in func arg
	nop
	ld		a5,-32(s0)        // restore retfloat
	sext.w 		a4,a5             // load lower 32bit word of retfloat into a4
	li		a5,1              // set a5 = 1
	bne		a4,a5,.L_nofloat  // jump if a4 != 1
	fsd		fa0,-32(s0)       // store the raw returned 64bit float/double value on the stack (where retfloat was)
	ld		a0,-32(s0)        // load the raw 64bit value into a0 for return
.L_nofloat:
	ld		ra,40(sp)         // restore return address from stack
	.cfi_restore 1
	ld		s0,32(sp)         // restore frame pointer from stack
	.cfi_restore 8
	.cfi_def_cfa 2, 48
	addi	sp,sp,48                  // clear reserved space from stack
	.cfi_def_cfa_offset 0
	jr		ra
	.cfi_endproc
	.size	CallRiscVFunc, .-CallRiscVFunc

#endif /* __riscv && __LP64__ */

#endif /* !AS_MAX_PORTABILITY */
