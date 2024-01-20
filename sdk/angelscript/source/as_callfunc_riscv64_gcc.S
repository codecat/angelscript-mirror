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
	addi	sp,sp,-32
	.cfi_def_cfa_offset 32
	sd		ra,24(sp)
	sd		s0,16(sp)
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	addi	s0,sp,32
	.cfi_def_cfa 8,0
	sd		a0,-24(s0)
	ld		a5,-24(s0)
	jalr	a5
	nop
	ld		ra,24(sp)
	.cfi_restore 1
	ld		s0,16(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 32
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	jr		ra
	.cfi_endproc
	.size	CallRiscVFunc, .-CallRiscVFunc

#endif /* __riscv && __LP64__ */

#endif /* !AS_MAX_PORTABILITY */
