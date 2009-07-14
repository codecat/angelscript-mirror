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


//
// as_bytecodedef.h
//
// Byte code definitions
//


#ifndef AS_BYTECODEDEF_H
#define AS_BYTECODEDEF_H

#include "as_config.h"

BEGIN_AS_NAMESPACE

//---------------------------------------------
// Byte code instructions
enum asEBCInstr
{
	// Unsorted
	BC_POP			= 0,	// Decrease stack size
	BC_PUSH			= 1,	// Increase stack size
	BC_PshC4		= 2,	// Push constant on stack
	BC_PshV4		= 3,	// Push value in variable on stack
	BC_PSF			= 4,	// Push stack frame
	BC_SWAP4		= 5,	// Swap top two dwords
	BC_NOT			= 6,    // Boolean not operator for a variable
	BC_PshG4		= 7,	// Push value in global variable on stack
	BC_LdGRdR4	    = 8,    // Same as LDG, RDR4
	BC_CALL			= 9,	// Call function
	BC_RET			= 10,	// Return from function
	BC_JMP			= 11,

	// Conditional jumps
	BC_JZ			= 12,
	BC_JNZ			= 13,
	BC_JS			= 14,	// Same as TS+JNZ or TNS+JZ
	BC_JNS			= 15,	// Same as TNS+JNZ or TS+JZ
	BC_JP			= 16,	// Same as TP+JNZ or TNP+JZ
	BC_JNP			= 17,	// Same as TNP+JNZ or TP+JZ

	// Test value
	BC_TZ			= 18,	// Test if zero
	BC_TNZ			= 19,	// Test if not zero
	BC_TS			= 20,	// Test if signaled (less than zero)
	BC_TNS			= 21,	// Test if not signaled (zero or greater)
	BC_TP			= 22,	// Test if positive (greater than zero)
	BC_TNP			= 23,	// Test if not positive (zero or less)

	// Negate value
	BC_NEGi			= 24,
	BC_NEGf			= 25,
	BC_NEGd			= 26,

	// Increment value pointed to by address in register
	BC_INCi16		= 27,
	BC_INCi8    	= 28,
	BC_DECi16   	= 29,
	BC_DECi8    	= 30, 
	BC_INCi			= 31,
	BC_DECi			= 32,
	BC_INCf			= 33,
	BC_DECf			= 34,
	BC_INCd     	= 35,
	BC_DECd     	= 36,

	// Increment variable
	BC_IncVi		= 37,
	BC_DecVi		= 38,

	// Bitwise operations
	BC_BNOT			= 39,
	BC_BAND			= 40,
	BC_BOR			= 41,
	BC_BXOR			= 42,
	BC_BSLL			= 43,
	BC_BSRL			= 44,
	BC_BSRA			= 45,

	// Unsorted
	BC_COPY			= 46,	// Do byte-for-byte copy of object
	BC_SET8			= 47,	// Push QWORD on stack
	BC_RDS8			= 48,	// Read value from address on stack onto the top of the stack
	BC_SWAP8		= 49,

	// Comparisons
	BC_CMPd     	= 50,
	BC_CMPu			= 51,
	BC_CMPf			= 52,
	BC_CMPi			= 53,

	// Comparisons with constant value
	BC_CMPIi		= 54,
	BC_CMPIf		= 55,
	BC_CMPIu        = 56,

	BC_JMPP     	= 57,	// Jump with offset in variable
	BC_PopRPtr    	= 58,	// Pop address from stack into register
	BC_PshRPtr    	= 59,	// Push address from register on stack
	BC_STR      	= 60,	// Push string address and length on stack
	BC_CALLSYS  	= 61,
	BC_CALLBND  	= 62,
	BC_SUSPEND  	= 63,
	BC_ALLOC    	= 64,
	BC_FREE     	= 65,
	BC_LOADOBJ		= 66,
	BC_STOREOBJ  	= 67,
	BC_GETOBJ    	= 68,
	BC_REFCPY    	= 69,
	BC_CHKREF    	= 70,
	BC_GETOBJREF 	= 71,
	BC_GETREF    	= 72,
	BC_SWAP48    	= 73,
	BC_SWAP84    	= 74,
	BC_OBJTYPE   	= 75,
	BC_TYPEID    	= 76,
	BC_SetV4		= 77,	// Initialize the variable with a DWORD
	BC_SetV8		= 78,	// Initialize the variable with a QWORD
	BC_ADDSi		= 79,	// Add arg to value on stack
	BC_CpyVtoV4		= 80,	// Copy value from one variable to another
	BC_CpyVtoV8		= 81,	
	BC_CpyVtoR4     = 82,	// Copy value from variable into register
	BC_CpyVtoR8		= 83,	// Copy value from variable into register
	BC_CpyVtoG4     = 84,   // Write the value of a variable to a global variable (LDG, WRTV4)
	BC_CpyRtoV4     = 85,   // Copy the value from the register to the variable
	BC_CpyRtoV8     = 86,
	BC_CpyGtoV4     = 87,   // Copy the value of the global variable to a local variable (LDG, RDR4)
	BC_WRTV1        = 88,	// Copy value from variable to address held in register
	BC_WRTV2        = 89,
	BC_WRTV4        = 90,
	BC_WRTV8        = 91,
	BC_RDR1         = 92,	// Read value from address in register and store in variable
	BC_RDR2         = 93,
	BC_RDR4         = 94,	
	BC_RDR8         = 95,
	BC_LDG          = 96,	// Load the register with the address of the global attribute
	BC_LDV          = 97,	// Load the register with the address of the variable
	BC_PGA          = 98,
	BC_RDS4         = 99,	// Read value from address on stack onto the top of the stack
	BC_VAR          = 100,	// Push the variable offset on the stack

	// Type conversions
	BC_iTOf			= 101,
	BC_fTOi			= 102,
	BC_uTOf			= 103,
	BC_fTOu			= 104,
	BC_sbTOi		= 105,	// Signed byte
	BC_swTOi		= 106,	// Signed word
	BC_ubTOi		= 107,	// Unsigned byte
	BC_uwTOi		= 108,	// Unsigned word
	BC_dTOi     	= 109,
	BC_dTOu     	= 110,
	BC_dTOf     	= 111,
	BC_iTOd     	= 112,
	BC_uTOd     	= 113,
	BC_fTOd     	= 114,

	// Math operations
	BC_ADDi			= 115,
	BC_SUBi			= 116,
	BC_MULi			= 117,
	BC_DIVi			= 118,
	BC_MODi			= 119,
	BC_ADDf			= 120,
	BC_SUBf			= 121,
	BC_MULf			= 122,
	BC_DIVf			= 123,
	BC_MODf			= 124,
	BC_ADDd     	= 125,
	BC_SUBd     	= 126,
	BC_MULd     	= 127,
	BC_DIVd     	= 128,
	BC_MODd     	= 129,

	// Math operations with constant value
	BC_ADDIi        = 130,
	BC_SUBIi        = 131,
	BC_MULIi        = 132,
	BC_ADDIf        = 133,
	BC_SUBIf        = 134,
	BC_MULIf        = 135,

	BC_SetG4		= 136,	// Initialize the global variable with a DWORD
	BC_ChkRefS      = 137,  // Verify that the reference to the handle on the stack is not null
	BC_ChkNullV     = 138,  // Verify that the variable is not a null handle
	BC_CALLINTF  	= 139,	// Call interface method 

	BC_iTOb         = 140,
	BC_iTOw         = 141,
	BC_SetV1        = 142,
	BC_SetV2        = 143,
	BC_Cast         = 144,	// Cast handle type to another handle type

	BC_i64TOi       = 145,
	BC_uTOi64       = 146,
	BC_iTOi64       = 147,
	BC_fTOi64       = 148,
	BC_dTOi64       = 149,
	BC_fTOu64       = 150,
	BC_dTOu64       = 151,
	BC_i64TOf       = 152,
	BC_u64TOf       = 153,
	BC_i64TOd       = 154,
	BC_u64TOd       = 155,
	BC_NEGi64       = 156,
	BC_INCi64       = 157,
	BC_DECi64       = 158,
	BC_BNOT64       = 159,

	BC_ADDi64   	= 160,
	BC_SUBi64   	= 161,
	BC_MULi64   	= 162,
	BC_DIVi64		= 163,
	BC_MODi64		= 164,
	BC_BAND64		= 165,
	BC_BOR64		= 166,
	BC_BXOR64		= 167,
	BC_BSLL64		= 168,
	BC_BSRL64		= 169,
	BC_BSRA64		= 170,
	BC_CMPi64       = 171,
	BC_CMPu64       = 172,
	
	BC_ChkNullS     = 173,
	BC_ClrHi        = 174,

	BC_MAXBYTECODE  = 175,

	// Temporary tokens, can't be output to the final program
	BC_PSP			= 253,
	BC_LINE			= 254,
	BC_LABEL		= 255
};

// TODO: This is internal, and shouldn't be moved to public header
#ifdef AS_64BIT_PTR
	#define BC_RDSPTR BC_RDS8
#else
	#define BC_RDSPTR BC_RDS4
#endif


//------------------------------------------------------------
// Instruction types
enum asEBCType
{
	BCTYPE_INFO         = 0,
	BCTYPE_NO_ARG       = 1,
	BCTYPE_W_ARG        = 2,
	BCTYPE_wW_ARG       = 3,
	BCTYPE_DW_ARG       = 4,
	BCTYPE_rW_DW_ARG    = 5,
	BCTYPE_QW_ARG       = 6,
	BCTYPE_DW_DW_ARG    = 7,
	BCTYPE_wW_rW_rW_ARG = 8,
	BCTYPE_wW_QW_ARG    = 9,
	BCTYPE_wW_rW_ARG    = 10,
	BCTYPE_rW_ARG       = 11,
	BCTYPE_wW_DW_ARG    = 12,
	BCTYPE_wW_rW_DW_ARG = 13,
	BCTYPE_rW_rW_ARG    = 14,
	BCTYPE_W_rW_ARG     = 15,
	BCTYPE_wW_W_ARG     = 16,
	BCTYPE_W_DW_ARG     = 17,
	BCTYPE_QW_DW_ARG    = 18
};

// TODO: This is internal, and shouldn't be moved to public header
#ifndef AS_64BIT_PTR
	#define BCTYPE_PTR_ARG    BCTYPE_DW_ARG
	#define BCTYPE_PTR_DW_ARG BCTYPE_DW_DW_ARG
#else
	#define BCTYPE_PTR_ARG    BCTYPE_QW_ARG
	#define BCTYPE_PTR_DW_ARG BCTYPE_QW_DW_ARG
#endif

//--------------------------------------------
// Instruction info
struct asSBCInfo
{
	asEBCInstr  bc;
	asEBCType   type;
	int         stackInc;
	const char *name;
};

#define asBCINFO(b,t,s) {BC_##b, BCTYPE_##t, s, "##b##"}
#define asBCINFO_DUMMY(b) {asEBCInstr(b), BCTYPE_INFO, 0, "BC_##b##"}

#ifndef AS_PTR_SIZE
	#ifdef AS_64BIT_PTR
		#define AS_PTR_SIZE 2
	#else
		#define AS_PTR_SIZE (sizeof(void*)/4)
	#endif
#endif

const asSBCInfo asBCInfo[256] =
{
	asBCINFO(POP,		W_ARG,			0xFFFF),
	asBCINFO(PUSH,		W_ARG,			0xFFFF),
	asBCINFO(PshC4,		DW_ARG,			1),
	asBCINFO(PshV4,		rW_ARG,			1),
	asBCINFO(PSF,		rW_ARG,			AS_PTR_SIZE),
	asBCINFO(SWAP4,		NO_ARG,			0),
	asBCINFO(NOT,		rW_ARG,			0),
	asBCINFO(PshG4,		W_ARG,			1),
	asBCINFO(LdGRdR4,	wW_W_ARG,		0),
	asBCINFO(CALL,		DW_ARG,			0xFFFF),
	asBCINFO(RET,		W_ARG,			0xFFFF),
	asBCINFO(JMP,		DW_ARG,			0),
	asBCINFO(JZ,		DW_ARG,			0),
	asBCINFO(JNZ,		DW_ARG,			0),
	asBCINFO(JS,		DW_ARG,			0),
	asBCINFO(JNS,		DW_ARG,			0),
	asBCINFO(JP,		DW_ARG,			0),
	asBCINFO(JNP,		DW_ARG,			0),
	asBCINFO(TZ,		NO_ARG,			0),
	asBCINFO(TNZ,		NO_ARG,			0),
	asBCINFO(TS,		NO_ARG,			0),
	asBCINFO(TNS,		NO_ARG,			0),
	asBCINFO(TP,		NO_ARG,			0),
	asBCINFO(TNP,		NO_ARG,			0),
	asBCINFO(NEGi,		rW_ARG,			0),
	asBCINFO(NEGf,		rW_ARG,			0),
	asBCINFO(NEGd,		rW_ARG,			0),
	asBCINFO(INCi16,	NO_ARG,			0),
	asBCINFO(INCi8,		NO_ARG,			0),
	asBCINFO(DECi16,	NO_ARG,			0),
	asBCINFO(DECi8,		NO_ARG,			0),
	asBCINFO(INCi,		NO_ARG,			0),
	asBCINFO(DECi,		NO_ARG,			0),
	asBCINFO(INCf,		NO_ARG,			0),
	asBCINFO(DECf,		NO_ARG,			0),
	asBCINFO(INCd,		NO_ARG,			0),
	asBCINFO(DECd,		NO_ARG,			0),
	asBCINFO(IncVi,		rW_ARG,			0),
	asBCINFO(DecVi,		rW_ARG,			0),
	asBCINFO(BNOT,		rW_ARG,			0),
	asBCINFO(BAND,		wW_rW_rW_ARG,	0),
	asBCINFO(BOR,		wW_rW_rW_ARG,	0),
	asBCINFO(BXOR,		wW_rW_rW_ARG,	0),
	asBCINFO(BSLL,		wW_rW_rW_ARG,	0),
	asBCINFO(BSRL,		wW_rW_rW_ARG,	0),
	asBCINFO(BSRA,		wW_rW_rW_ARG,	0),
	asBCINFO(COPY,		W_ARG,			-AS_PTR_SIZE),
	asBCINFO(SET8,		QW_ARG,			2),
	asBCINFO(RDS8,		NO_ARG,			2-AS_PTR_SIZE),
	asBCINFO(SWAP8,		NO_ARG,			0),
	asBCINFO(CMPd,		rW_rW_ARG,		0),
	asBCINFO(CMPu,		rW_rW_ARG,		0),
	asBCINFO(CMPf,		rW_rW_ARG,		0),
	asBCINFO(CMPi,		rW_rW_ARG,		0),
	asBCINFO(CMPIi,		rW_DW_ARG,		0),
	asBCINFO(CMPIf,		rW_DW_ARG,		0),
	asBCINFO(CMPIu,		rW_DW_ARG,		0),
	asBCINFO(JMPP,		rW_ARG,			0),
	asBCINFO(PopRPtr,	NO_ARG,			-AS_PTR_SIZE),
	asBCINFO(PshRPtr,	NO_ARG,			AS_PTR_SIZE),
	asBCINFO(STR,		W_ARG,			1+AS_PTR_SIZE),
	asBCINFO(CALLSYS,	DW_ARG,			0xFFFF),
	asBCINFO(CALLBND,	DW_ARG,			0xFFFF),
	asBCINFO(SUSPEND,	NO_ARG,			0),
	asBCINFO(ALLOC,		PTR_DW_ARG,		0xFFFF),
	asBCINFO(FREE,		PTR_ARG,		-AS_PTR_SIZE),
	asBCINFO(LOADOBJ,	rW_ARG,			0),
	asBCINFO(STOREOBJ,	wW_ARG,			0),
	asBCINFO(GETOBJ,	W_ARG,			0),
	asBCINFO(REFCPY,	PTR_ARG,		-AS_PTR_SIZE),
	asBCINFO(CHKREF,	NO_ARG,			0),
	asBCINFO(GETOBJREF,	W_ARG,			0),
	asBCINFO(GETREF,	W_ARG,			0),
	asBCINFO(SWAP48,	NO_ARG,			0),
	asBCINFO(SWAP84,	NO_ARG,			0),
	asBCINFO(OBJTYPE,	PTR_ARG,		AS_PTR_SIZE),
	asBCINFO(TYPEID,	DW_ARG,			1),
	asBCINFO(SetV4,		wW_DW_ARG,		0),
	asBCINFO(SetV8,		wW_QW_ARG,		0),
	asBCINFO(ADDSi,		DW_ARG,			0),
	asBCINFO(CpyVtoV4,	wW_rW_ARG,		0),
	asBCINFO(CpyVtoV8,	wW_rW_ARG,		0),
	asBCINFO(CpyVtoR4,	rW_ARG,			0),
	asBCINFO(CpyVtoR8,	rW_ARG,			0),
	asBCINFO(CpyVtoG4,	W_rW_ARG,		0),
	asBCINFO(CpyRtoV4,	wW_ARG,			0),
	asBCINFO(CpyRtoV8,	wW_ARG,			0),
	asBCINFO(CpyGtoV4,	wW_W_ARG,		0),
	asBCINFO(WRTV1,		rW_ARG,			0),
	asBCINFO(WRTV2,		rW_ARG,			0),
	asBCINFO(WRTV4,		rW_ARG,			0),
	asBCINFO(WRTV8,		rW_ARG,			0),
	asBCINFO(RDR1,		wW_ARG,			0),
	asBCINFO(RDR2,		wW_ARG,			0),
	asBCINFO(RDR4,		wW_ARG,			0),
	asBCINFO(RDR8,		wW_ARG,			0),
	asBCINFO(LDG,		W_ARG,			0),
	asBCINFO(LDV,		rW_ARG,			0),
	asBCINFO(PGA,		W_ARG,			AS_PTR_SIZE),
	asBCINFO(RDS4,		NO_ARG,			1-AS_PTR_SIZE),
	asBCINFO(VAR,		rW_ARG,			AS_PTR_SIZE),
	asBCINFO(iTOf,		rW_ARG,			0),
	asBCINFO(fTOi,		rW_ARG,			0),
	asBCINFO(uTOf,		rW_ARG,			0),
	asBCINFO(fTOu,		rW_ARG,			0),
	asBCINFO(sbTOi,		rW_ARG,			0),
	asBCINFO(swTOi,		rW_ARG,			0),
	asBCINFO(ubTOi,		rW_ARG,			0),
	asBCINFO(uwTOi,		rW_ARG,			0),
	asBCINFO(dTOi,		wW_rW_ARG,		0),
	asBCINFO(dTOu,		wW_rW_ARG,		0),
	asBCINFO(dTOf,		wW_rW_ARG,		0),
	asBCINFO(iTOd,		wW_rW_ARG,		0),
	asBCINFO(uTOd,		wW_rW_ARG,		0),
	asBCINFO(fTOd,		wW_rW_ARG,		0),
	asBCINFO(ADDi,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBi,		wW_rW_rW_ARG,	0),
	asBCINFO(MULi,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVi,		wW_rW_rW_ARG,	0),
	asBCINFO(MODi,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDf,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBf,		wW_rW_rW_ARG,	0),
	asBCINFO(MULf,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVf,		wW_rW_rW_ARG,	0),
	asBCINFO(MODf,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDd,		wW_rW_rW_ARG,	0),
	asBCINFO(SUBd,		wW_rW_rW_ARG,	0),
	asBCINFO(MULd,		wW_rW_rW_ARG,	0),
	asBCINFO(DIVd,		wW_rW_rW_ARG,	0),
	asBCINFO(MODd,		wW_rW_rW_ARG,	0),
	asBCINFO(ADDIi,		wW_rW_DW_ARG,	0),
	asBCINFO(SUBIi,		wW_rW_DW_ARG,	0),
	asBCINFO(MULIi,		wW_rW_DW_ARG,	0),
	asBCINFO(ADDIf,		wW_rW_DW_ARG,	0),
	asBCINFO(SUBIf,		wW_rW_DW_ARG,	0),
	asBCINFO(MULIf,		wW_rW_DW_ARG,	0),
	asBCINFO(SetG4,		W_DW_ARG,		0),
	asBCINFO(ChkRefS,	NO_ARG,			0),
	asBCINFO(ChkNullV,	rW_ARG,			0),
	asBCINFO(CALLINTF,	DW_ARG,			0xFFFF),
	asBCINFO(iTOb,		rW_ARG,			0),
	asBCINFO(iTOw,		rW_ARG,			0),
	asBCINFO(SetV1,		wW_DW_ARG,		0),
	asBCINFO(SetV2,		wW_DW_ARG,		0),
	asBCINFO(Cast,		DW_ARG,			-AS_PTR_SIZE),
	asBCINFO(i64TOi,	wW_rW_ARG,		0),
	asBCINFO(uTOi64,	wW_rW_ARG,		0),
	asBCINFO(iTOi64,	wW_rW_ARG,		0),
	asBCINFO(fTOi64,	wW_rW_ARG,		0),
	asBCINFO(dTOi64,	rW_ARG,			0),
	asBCINFO(fTOu64,	wW_rW_ARG,		0),
	asBCINFO(dTOu64,	rW_ARG,			0),
	asBCINFO(i64TOf,	wW_rW_ARG,		0),
	asBCINFO(u64TOf,	wW_rW_ARG,		0),
	asBCINFO(i64TOd,	rW_ARG,			0),
	asBCINFO(u64TOd,	rW_ARG,			0),
	asBCINFO(NEGi64,	rW_ARG,			0),
	asBCINFO(INCi64,	NO_ARG,			0),
	asBCINFO(DECi64,	NO_ARG,			0),
	asBCINFO(BNOT64,	rW_ARG,			0),
	asBCINFO(ADDi64,	wW_rW_rW_ARG,	0),
	asBCINFO(SUBi64,	wW_rW_rW_ARG,	0),
	asBCINFO(MULi64,	wW_rW_rW_ARG,	0),
	asBCINFO(DIVi64,	wW_rW_rW_ARG,	0),
	asBCINFO(MODi64,	wW_rW_rW_ARG,	0),
	asBCINFO(BAND64,	wW_rW_rW_ARG,	0),
	asBCINFO(BOR64,		wW_rW_rW_ARG,	0),
	asBCINFO(BXOR64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSLL64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSRL64,	wW_rW_rW_ARG,	0),
	asBCINFO(BSRA64,	wW_rW_rW_ARG,	0),
	asBCINFO(CMPi64,	rW_rW_ARG,		0),
	asBCINFO(CMPu64,	rW_rW_ARG,		0),
	asBCINFO(ChkNullS,	W_ARG,			0),
	asBCINFO(ClrHi,		NO_ARG,			0),

	asBCINFO_DUMMY(175),
	asBCINFO_DUMMY(176),
	asBCINFO_DUMMY(177),
	asBCINFO_DUMMY(178),
	asBCINFO_DUMMY(179),
	asBCINFO_DUMMY(180),
	asBCINFO_DUMMY(181),
	asBCINFO_DUMMY(182),
	asBCINFO_DUMMY(183),
	asBCINFO_DUMMY(184),
	asBCINFO_DUMMY(185),
	asBCINFO_DUMMY(186),
	asBCINFO_DUMMY(187),
	asBCINFO_DUMMY(188),
	asBCINFO_DUMMY(189),
	asBCINFO_DUMMY(190),
	asBCINFO_DUMMY(191),
	asBCINFO_DUMMY(192),
	asBCINFO_DUMMY(193),
	asBCINFO_DUMMY(194),
	asBCINFO_DUMMY(195),
	asBCINFO_DUMMY(196),
	asBCINFO_DUMMY(197),
	asBCINFO_DUMMY(198),
	asBCINFO_DUMMY(199),
	asBCINFO_DUMMY(200),
	asBCINFO_DUMMY(201),
	asBCINFO_DUMMY(202),
	asBCINFO_DUMMY(203),
	asBCINFO_DUMMY(204),
	asBCINFO_DUMMY(205),
	asBCINFO_DUMMY(206),
	asBCINFO_DUMMY(207),
	asBCINFO_DUMMY(208),
	asBCINFO_DUMMY(209),
	asBCINFO_DUMMY(210),
	asBCINFO_DUMMY(211),
	asBCINFO_DUMMY(212),
	asBCINFO_DUMMY(213),
	asBCINFO_DUMMY(214),
	asBCINFO_DUMMY(215),
	asBCINFO_DUMMY(216),
	asBCINFO_DUMMY(217),
	asBCINFO_DUMMY(218),
	asBCINFO_DUMMY(219),
	asBCINFO_DUMMY(220),
	asBCINFO_DUMMY(221),
	asBCINFO_DUMMY(222),
	asBCINFO_DUMMY(223),
	asBCINFO_DUMMY(224),
	asBCINFO_DUMMY(225),
	asBCINFO_DUMMY(226),
	asBCINFO_DUMMY(227),
	asBCINFO_DUMMY(228),
	asBCINFO_DUMMY(229),
	asBCINFO_DUMMY(230),
	asBCINFO_DUMMY(231),
	asBCINFO_DUMMY(232),
	asBCINFO_DUMMY(233),
	asBCINFO_DUMMY(234),
	asBCINFO_DUMMY(235),
	asBCINFO_DUMMY(236),
	asBCINFO_DUMMY(237),
	asBCINFO_DUMMY(238),
	asBCINFO_DUMMY(239),
	asBCINFO_DUMMY(240),
	asBCINFO_DUMMY(241),
	asBCINFO_DUMMY(242),
	asBCINFO_DUMMY(243),
	asBCINFO_DUMMY(244),
	asBCINFO_DUMMY(245),
	asBCINFO_DUMMY(246),
	asBCINFO_DUMMY(247),
	asBCINFO_DUMMY(248),
	asBCINFO_DUMMY(249),
	asBCINFO_DUMMY(250),
	asBCINFO_DUMMY(251),
	asBCINFO_DUMMY(252),

	asBCINFO(PSP,		W_ARG,			AS_PTR_SIZE),
	asBCINFO(LINE,		INFO,			0xFFFF),
	asBCINFO(LABEL,		INFO,			0xFFFF)
};

//-----------------------------------------------------
// Macros to access bytecode instruction arguments

#define asBC_DWORDARG(x)  (asDWORD(*(x+1)))
#define asBC_INTARG(x)    (int(*(x+1)))
#define asBC_QWORDARG(x)  (*(asQWORD*)(x+1))
#define asBC_FLOATARG(x)  (*(float*)(x+1))
#define asBC_PTRARG(x)    (asPTRWORD(*(x+1)))

#define asBC_WORDARG0(x)  (*(((asWORD*)x)+1))
#define asBC_WORDARG1(x)  (*(((asWORD*)x)+2))

#define asBC_SWORDARG0(x) (*(((short*)x)+1))
#define asBC_SWORDARG1(x) (*(((short*)x)+2))
#define asBC_SWORDARG2(x) (*(((short*)x)+3))

END_AS_NAMESPACE

#endif
