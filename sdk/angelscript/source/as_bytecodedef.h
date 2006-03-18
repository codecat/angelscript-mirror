/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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

#define BC_POP		0
#define BC_PUSH		1
#define BC_SET4		2
#define BC_RD4		3
#define BC_RDSF4	4
#define BC_WRT4		5
#define BC_MOV4		6
#define BC_PSF		7	// Push stack frame
#define BC_MOVSF4   8
#define BC_SWAP4    9 // Swap top two dwords
#define BC_STORE4   10 // Store top dword in temp reg
#define BC_RECALL4  11 // Push temp reg on stack

#define BC_CALL		12  // Call function
#define BC_RET		13 // Return from function
#define BC_JMP		14
#define BC_JZ		15
#define BC_JNZ		16

#define BC_TZ		17  // Test if zero
#define BC_TNZ		18  // Test if not zero
#define BC_TS		19  // Test if signaled (less than zero)
#define BC_TNS		20  // Test if not signaled (zero or greater)
#define BC_TP		21  // Test if positive (greater than zero)
#define BC_TNP		22  // Test if not positive (zero or less)

#define BC_ADDi		23
#define BC_SUBi		24
#define BC_MULi		25
#define BC_DIVi		26
#define BC_MODi		27
#define BC_NEGi		28
#define BC_CMPi		29
#define BC_INCi		30
#define BC_DECi		31
#define BC_I2F		32	// integer to float conversion

#define BC_ADDf		33
#define BC_SUBf		34
#define BC_MULf		35
#define BC_DIVf		36
#define BC_MODf		37
#define BC_NEGf		38
#define BC_CMPf		39
#define BC_INCf		40
#define BC_DECf		41
#define BC_F2I		42	// float to integer conversion

#define BC_BNOT		43
#define BC_BAND		44
#define BC_BOR		45
#define BC_BXOR		46
#define BC_BSLL		47
#define BC_BSRL		48
#define BC_BSRA		49

#define BC_UI2F     50
#define BC_F2UI     51
#define BC_CMPui    52
#define BC_SB       53 // Signed byte
#define BC_SW       54 // Signed word
#define BC_UB       55 // Unsigned byte
#define BC_UW       56 // Unsigned word
#define BC_WRT1     57
#define BC_WRT2     58
#define BC_INCi16   59
#define BC_INCi8    60
#define BC_DECi16   61
#define BC_DECi8    62
#define BC_PUSHZERO 63
#define BC_COPY     64
#define BC_PGA      65 // Push Global Address

#define BC_SET8     66
#define BC_WRT8     67
#define BC_RD8      68
#define BC_NEGd     69
#define BC_INCd     70
#define BC_DECd     71
#define BC_ADDd     72
#define BC_SUBd     73
#define BC_MULd     74
#define BC_DIVd     75
#define BC_MODd     76
#define BC_SWAP8    77
#define BC_CMPd     78
#define BC_dTOi     79
#define BC_dTOui    80
#define BC_dTOf     81
#define BC_iTOd     82
#define BC_uiTOd    83
#define BC_fTOd     84
#define BC_JMPP     85 // Jump relative to top stack dword
#define BC_SRET4    86 // Store dword return value
#define BC_SRET8    87 // Store qword return value
#define BC_RRET4    88 // Recall dword return value
#define BC_RRET8    89 // Recall qword return value
#define BC_STR      90 // Push string address and length on stack
#define BC_JS		91 // Same as TS+JMP1 or TNS+JMP0
#define BC_JNS		92 // Same as TNS+JMP1 or TS+JMP0
#define BC_JP		93 // Same as TP+JMP1 or TNP+JMP0
#define BC_JNP		94 // Same as TNP+JMP1 or TP+JMP0
#define BC_CMPIi    95 // Same as SET4+CMPi
#define BC_CMPIui   96 // Same as SET4+CMPui
#define BC_CALLSYS  97
#define BC_CALLBND  98
#define BC_RDGA4    99 // Same as PGA+RD4
#define BC_MOVGA4   100 // Same as PGA+MOV4
#define BC_ADDIi    101 // Same as SET4+ADDi
#define BC_SUBIi    102 // Same as SET4+SUBi
#define BC_CMPIf    103 // Same as SET4+CMPf
#define BC_ADDIf    104 // Same as SET4+ADDf
#define BC_SUBIf    105 // Same as SET4+SUBf
#define BC_MULIi    106 // Same as SET4+MULi
#define BC_MULIf    107 // Same as SET4+MULf

#define BC_SUSPEND   108
#define BC_ALLOC     109
#define BC_FREE      110
#define BC_LOADOBJ   111
#define BC_STOREOBJ  112
#define BC_GETOBJ    113
#define BC_REFCPY    114
#define BC_CHKREF    115
#define BC_RD1       116
#define BC_RD2       117
#define BC_GETOBJREF 118
#define BC_GETREF    119
#define BC_SWAP48    120
#define BC_SWAP84    121
#define BC_OBJTYPE   122
#define BC_TYPEID    123
#define BC_MAXBYTECODE 124

// Temporary tokens, can't be output to the final program
#define BC_PSP		   246
#define BC_VAR         247
#define BC_LINE	       248
#define BC_LABEL	   255

//------------------------------------------------------------
// Relocation Table
extern asDWORD relocTable[BC_MAXBYTECODE];

//------------------------------------------------------------
// Instruction sizes

const int BCS_POP       = BCSIZE2;
const int BCS_PUSH      = BCSIZE2;
const int BCS_SET4      = BCSIZE4;
const int BCS_RD4       = BCSIZE0;
const int BCS_RDSF4     = BCSIZE2;
const int BCS_WRT4      = BCSIZE0;
const int BCS_MOV4      = BCSIZE0;
const int BCS_PSF       = BCSIZE2;
const int BCS_MOVSF4    = BCSIZE2;
const int BCS_SWAP4     = BCSIZE0;
const int BCS_STORE4    = BCSIZE0;
const int BCS_RECALL4   = BCSIZE0;

const int BCS_CALL      = BCSIZE4;
const int BCS_RET       = BCSIZE2;
const int BCS_JMP       = BCSIZE4;
const int BCS_JZ        = BCSIZE4;
const int BCS_JNZ       = BCSIZE4;

const int BCS_TZ        = BCSIZE0;
const int BCS_TNZ       = BCSIZE0;
const int BCS_TS        = BCSIZE0;
const int BCS_TNS       = BCSIZE0;
const int BCS_TP        = BCSIZE0;
const int BCS_TNP       = BCSIZE0;

const int BCS_ADDi      = BCSIZE0;
const int BCS_SUBi      = BCSIZE0;
const int BCS_MULi      = BCSIZE0;
const int BCS_DIVi      = BCSIZE0;
const int BCS_MODi      = BCSIZE0;
const int BCS_NEGi      = BCSIZE0;
const int BCS_CMPi      = BCSIZE0;
const int BCS_INCi      = BCSIZE0;
const int BCS_DECi      = BCSIZE0;
const int BCS_I2F       = BCSIZE0;

const int BCS_ADDf      = BCSIZE0;
const int BCS_SUBf      = BCSIZE0;
const int BCS_MULf      = BCSIZE0;
const int BCS_DIVf      = BCSIZE0;
const int BCS_MODf      = BCSIZE0;
const int BCS_NEGf      = BCSIZE0;
const int BCS_CMPf      = BCSIZE0;
const int BCS_INCf      = BCSIZE0;
const int BCS_DECf      = BCSIZE0;
const int BCS_F2I       = BCSIZE0;

const int BCS_BNOT      = BCSIZE0;
const int BCS_BAND      = BCSIZE0;
const int BCS_BOR       = BCSIZE0;
const int BCS_BXOR      = BCSIZE0;
const int BCS_BSLL      = BCSIZE0;
const int BCS_BSRL      = BCSIZE0;
const int BCS_BSRA      = BCSIZE0;

const int BCS_UI2F      = BCSIZE0;
const int BCS_F2UI      = BCSIZE0;
const int BCS_CMPui     = BCSIZE0;
const int BCS_SB        = BCSIZE0;
const int BCS_SW        = BCSIZE0;
const int BCS_UB        = BCSIZE0;
const int BCS_UW        = BCSIZE0;
const int BCS_WRT1      = BCSIZE0;
const int BCS_WRT2      = BCSIZE0;
const int BCS_INCi16    = BCSIZE0;
const int BCS_INCi8     = BCSIZE0;
const int BCS_DECi16    = BCSIZE0;
const int BCS_DECi8     = BCSIZE0;
const int BCS_PUSHZERO  = BCSIZE0;
const int BCS_COPY      = BCSIZE2;
const int BCS_PGA       = BCSIZE4;

const int BCS_SET8      = BCSIZE8;
const int BCS_WRT8      = BCSIZE0;
const int BCS_RD8       = BCSIZE0;
const int BCS_NEGd      = BCSIZE0;
const int BCS_INCd      = BCSIZE0;
const int BCS_DECd      = BCSIZE0;
const int BCS_ADDd      = BCSIZE0;
const int BCS_SUBd      = BCSIZE0;
const int BCS_MULd      = BCSIZE0;
const int BCS_DIVd      = BCSIZE0;
const int BCS_MODd      = BCSIZE0;
const int BCS_SWAP8     = BCSIZE0;
const int BCS_CMPd      = BCSIZE0;
const int BCS_dTOi      = BCSIZE0;
const int BCS_dTOui     = BCSIZE0;
const int BCS_dTOf      = BCSIZE0;
const int BCS_iTOd      = BCSIZE0;
const int BCS_uiTOd     = BCSIZE0;
const int BCS_fTOd      = BCSIZE0;
const int BCS_JMPP      = BCSIZE0;
const int BCS_SRET4     = BCSIZE0;
const int BCS_SRET8     = BCSIZE0;
const int BCS_RRET4     = BCSIZE0;
const int BCS_RRET8     = BCSIZE0;
const int BCS_STR       = BCSIZE2;
const int BCS_JS        = BCSIZE4;
const int BCS_JNS       = BCSIZE4;
const int BCS_JP        = BCSIZE4;
const int BCS_JNP       = BCSIZE4;
const int BCS_CMPIi     = BCSIZE4;
const int BCS_CMPIui    = BCSIZE4;
const int BCS_CALLSYS   = BCSIZE4;
const int BCS_CALLBND   = BCSIZE4;
const int BCS_RDGA4     = BCSIZE4;
const int BCS_MOVGA4    = BCSIZE4;
const int BCS_ADDIi     = BCSIZE4;
const int BCS_SUBIi     = BCSIZE4;
const int BCS_CMPIf     = BCSIZE4;
const int BCS_ADDIf     = BCSIZE4;
const int BCS_SUBIf     = BCSIZE4;
const int BCS_MULIi     = BCSIZE4;
const int BCS_MULIf     = BCSIZE4;
const int BCS_SUSPEND   = BCSIZE0;
const int BCS_ALLOC     = BCSIZE8;
const int BCS_FREE      = BCSIZE4;
const int BCS_LOADOBJ   = BCSIZE2;
const int BCS_STOREOBJ  = BCSIZE2;
const int BCS_GETOBJ    = BCSIZE2;
const int BCS_REFCPY    = BCSIZE4;
const int BCS_CHKREF    = BCSIZE0;
const int BCS_RD1       = BCSIZE0;
const int BCS_RD2       = BCSIZE0;
const int BCS_GETOBJREF = BCSIZE2;
const int BCS_GETREF    = BCSIZE2;
const int BCS_SWAP48    = BCSIZE0;
const int BCS_SWAP84    = BCSIZE0;
const int BCS_OBJTYPE   = BCSIZE4;
const int BCS_TYPEID    = BCSIZE4;

// Temporary
const int BCS_PSP       = BCSIZE2;
const int BCS_VAR       = BCSIZE4;
#ifndef BUILD_WITHOUT_LINE_CUES
	const int BCS_LINE = BCS_SUSPEND;
#else
	const int BCS_LINE = 0;
#endif



const int bcSize[256] =
{
	BCS_POP,
	BCS_PUSH,
	BCS_SET4,
	BCS_RD4,
	BCS_RDSF4,
	BCS_WRT4,
	BCS_MOV4,
	BCS_PSF,
	BCS_MOVSF4,
	BCS_SWAP4,
	BCS_STORE4,
	BCS_RECALL4,

	BCS_CALL,
	BCS_RET,
	BCS_JMP,
	BCS_JZ,
	BCS_JNZ,

	BCS_TZ,
	BCS_TNZ,
	BCS_TS,
	BCS_TNS,
	BCS_TP,
	BCS_TNP,

	BCS_ADDi,
	BCS_SUBi,
	BCS_MULi,
	BCS_DIVi,
	BCS_MODi,
	BCS_NEGi,
	BCS_CMPi,
	BCS_INCi,
	BCS_DECi,
	BCS_I2F,

	BCS_ADDf,
	BCS_SUBf,
	BCS_MULf,
	BCS_DIVf,
	BCS_MODf,
	BCS_NEGf,
	BCS_CMPf,
	BCS_INCf,
	BCS_DECf,
	BCS_F2I,

	BCS_BNOT,
	BCS_BAND,
	BCS_BOR,
	BCS_BXOR,
	BCS_BSLL,
	BCS_BSRL,
	BCS_BSRA,

	BCS_UI2F,
	BCS_F2UI,
	BCS_CMPui,
	BCS_SB,
	BCS_SW,
	BCS_UB,
	BCS_UW,
	BCS_WRT1,
	BCS_WRT2,
	BCS_INCi16,
	BCS_INCi8,
	BCS_DECi16,
	BCS_DECi8,
	BCS_PUSHZERO,
	BCS_COPY,
	BCS_PGA,

	BCS_SET8,
	BCS_WRT8,
	BCS_RD8,
	BCS_NEGd,
	BCS_INCd,
	BCS_DECd,
	BCS_ADDd,
	BCS_SUBd,
	BCS_MULd,
	BCS_DIVd,
	BCS_MODd,
	BCS_SWAP8,
	BCS_CMPd,
	BCS_dTOi,
	BCS_dTOui,
	BCS_dTOf,
	BCS_iTOd,
	BCS_uiTOd,
	BCS_fTOd,
	BCS_JMPP,
	BCS_SRET4,
	BCS_SRET8,
	BCS_RRET4,
	BCS_RRET8,
	BCS_STR,
	BCS_JS,
	BCS_JNS,
	BCS_JP,
	BCS_JNP,
	BCS_CMPIi,
	BCS_CMPIui,
	BCS_CALLSYS,
	BCS_CALLBND,
	BCS_RDGA4,
	BCS_MOVGA4,
	BCS_ADDIi,
	BCS_SUBIi,
	BCS_CMPIf,
	BCS_ADDIf,
	BCS_SUBIf,
	BCS_MULIi,
	BCS_MULIf,
	BCS_SUSPEND,
	BCS_ALLOC,
	BCS_FREE,
	BCS_LOADOBJ,
	BCS_STOREOBJ,
	BCS_GETOBJ,
	BCS_REFCPY,
	BCS_CHKREF,
	BCS_RD1,
	BCS_RD2,
	BCS_GETOBJREF,
	BCS_GETREF, 
	BCS_SWAP48,
	BCS_SWAP84,
	BCS_OBJTYPE,
	BCS_TYPEID,
	0,0,0,0,0,0, // 124-129
	0,0,0,0,0,0,0,0,0,0, // 130-139
	0,0,0,0,0,0,0,0,0,0, // 140-149
	0,0,0,0,0,0,0,0,0,0, // 150-159
	0,0,0,0,0,0,0,0,0,0, // 160-169
	0,0,0,0,0,0,0,0,0,0, // 170-179
	0,0,0,0,0,0,0,0,0,0, // 180-189
	0,0,0,0,0,0,0,0,0,0, // 190-199
	0,0,0,0,0,0,0,0,0,0, // 200-209
	0,0,0,0,0,0,0,0,0,0, // 210-219
	0,0,0,0,0,0,0,0,0,0, // 220-229
	0,0,0,0,0,0,0,0,0,0, // 230-239
	0,0,0,0,0,0,       // 240-245
	BCS_PSP, 
	BCS_VAR,  
	BCS_LINE,  
	0,  // 249
	0,  // 250
	0,  // 251
	0,  // 252
	0,  // 253
	0,  // 254
	0,	// BC_LABEL     
};

const int bcStackInc[256] =
{
	0xFFFF,	// BC_POP
	0xFFFF,	// BC_PUSH
	1,		// BC_SET4
	0,		// BC_RD4
	1,		// BC_RDSF4
	-1,		// BC_WRT4
	-2,		// BC_MOV4
	1,		// BC_PSF
	-1,		// BC_MOVSF4
	0,      // BC_SWAP4
	0,      // BC_STORE4
	1,      // BC_RECALL4

	0xFFFF,	// BC_CALL
	0xFFFF,	// BC_RET
	0,		// BC_JMP
	-1,		// BC_JZ
	-1,		// BC_JNZ

	0,		// BC_TZ
	0,		// BC_TNZ
	0,		// BC_TS
	0,		// BC_TNS
	0,		// BC_TP
	0,		// BC_TNP

	-1,		// BC_ADDi
	-1,		// BC_SUBi
	-1,		// BC_MULi
	-1,		// BC_DIVi
	-1,		// BC_MODi
	0,		// BC_NEGi
	-1,		// BC_CMPi
	0,		// BC_INCi
	0,		// BC_DECi
	0,		// BC_I2F

	-1,		// BC_ADDf
	-1,		// BC_SUBf
	-1,		// BC_MULf
	-1,		// BC_DIVf
	-1,		// BC_MODf
	0,		// BC_NEGf
	-1,		// BC_CMPf
	0,		// BC_INCf
	0,		// BC_DECf
	0,		// BC_F2I

	0,		// BC_BNOT
	-1,		// BC_BAND
	-1,		// BC_BOR
	-1,		// BC_BXOR
	-1,		// BC_BSLL
	-1,		// BC_BSRL
	-1,		// BC_BSRA
    
	0,      // BC_UI2F
	0,		// BC_F2UI
	-1,		// BC_CMPui
	0,		// BC_SB
	0,		// BC_SW
	0,		// BC_UB
	0,		// BC_UW
	-1,		// BC_WRT1
	-1,		// BC_WRT2
	0,		// BC_INCi16
	0,		// BC_INCi8
	0,		// BC_DECi16
	0,		// BC_DECi8
	1,		// BC_PUSHZERO
	-1,		// BC_COPY
	1,		// BC_PGA

	2,		// BC_SET8
	-1,		// BC_WRT8
	1,		// BC_RD8
	0,		// BC_NEGd
	0,		// BC_INCd
	0,		// BC_DECd
	-2,		// BC_ADDd
	-2,		// BC_SUBd
	-2,		// BC_MULd
	-2,		// BC_DIVd
	-2,		// BC_MODd
	0,		// BC_SWAP8
	-3,		// BC_CMPd
	-1,		// BC_dTOi
	-1,		// BC_dTOui
	-1,		// BC_dTOf
	1,		// BC_iTOd
	1,		// BC_uiTOd
	1,		// BC_fTOd
	-1,		// BC_JMPP
	-1,		// BC_SRET4
	-2,		// BC_SRET8
	1,		// BC_RRET4
	2,		// BC_RRET8
	2,		// BC_STR
	-1,     // BC_JS
	-1,     // BC_JNS
	-1,     // BC_JP
	-1,     // BC_JNP
	0,		// BC_CMPIi
	0,		// BC_CMPIui
	0xFFFF,	// BC_CALLSYS
	0xFFFF,	// BC_CALLBND
	1,		// BC_RDGA4
	-1,		// BC_MOVGA4
	0,		// BC_ADDIi
	0,		// BC_SUBIi
	0,		// BC_CMPIf
	0,		// BC_ADDIf
	0,		// BC_SUBIf
	0,		// BC_MULIi
	0,		// BC_MULIf
	0,		// BC_SUSPEND
	0xFFFF,	// BC_ALLOC
	-1,		// BC_FREE
	0,      // BC_LOADOBJ
	0,		// BC_STOREOBJ
	0,		// BC_GETOBJ
	-1,     // BC_REFCPY
	0,		// BC_CHKREF
	0,		// BC_RD1
	0,		// BC_RD2
	0,		// BC_GETOBJREF
	0,      // BC_GETREF
	0,      // BC_SWAP48
	0,      // BC_SWAP84
	1,		// BC_OBJTYPE
	1,      // BC_TYPEID
	0,0,0,0,0,0, // 124-129
	0,0,0,0,0,0,0,0,0,0, // 130-139
	0,0,0,0,0,0,0,0,0,0, // 140-149
	0,0,0,0,0,0,0,0,0,0, // 150-159
	0,0,0,0,0,0,0,0,0,0, // 160-169
	0,0,0,0,0,0,0,0,0,0, // 170-179
	0,0,0,0,0,0,0,0,0,0, // 180-189
	0,0,0,0,0,0,0,0,0,0, // 190-199
	0,0,0,0,0,0,0,0,0,0, // 200-209
	0,0,0,0,0,0,0,0,0,0, // 210-219
	0,0,0,0,0,0,0,0,0,0, // 220-229
	0,0,0,0,0,0,0,0,0,0, // 230-239
	0,0,0,0,0,0,       // 240-245
	1,		// BC_PSP
	1,      // BC_VAR
	0xFFFF, // BC_LINE
	0, // 249
	0, // 250
	0, // 251
	0, // 252
	0, // 253
	0, // 254
	0xFFFF,	// BC_LABEL
};

struct sByteCodeName
{
	char *name;
};

#ifdef AS_DEBUG
const sByteCodeName bcName[256] =
{
	{"POP"},
	{"PUSH"},
	{"SET4"},
	{"RD4"},
	{"RDSF4"},
	{"WRT4"},
	{"MOV4"},
	{"PSF"},
	{"MOVSF4"},
	{"SWAP4"},
	{"STORE4"},
	{"RECALL4"},

	{"CALL"},
	{"RET"},
	{"JMP"},
	{"JZ"},
	{"JNZ"},

	{"TZ"},
	{"TNZ"},
	{"TS"},
	{"TNS"},
	{"TP"},
	{"TNP"},

	{"ADDi"},
	{"SUBi"},
	{"MULi"},
	{"DIVi"},
	{"MODi"},
	{"NEGi"},
	{"CMPi"},
	{"INCi"},
	{"DECi"},
	{"I2F"},

	{"ADDf"},
	{"SUBf"},
	{"MULf"},
	{"DIVf"},
	{"MODf"},
	{"NEGf"},
	{"CMPf"},
	{"INCf"},
	{"DECf"},
	{"F2I"},

	{"BNOT"},
	{"BAND"},
	{"BOR"},
	{"BXOR"},
	{"BSLL"},
	{"BSRL"},
	{"BSRA"},

    {"UI2F"},
	{"F2UI"},
	{"CMPui"},
	{"SB"},
	{"SW"},
	{"UB"},
	{"UW"},
	{"WRT1"},
	{"WRT2"},
	{"INCi16"},
	{"INCi8"},
	{"DECi16"},
	{"DECi8"},
	{"PUSHZERO"},
	{"COPY"},
	{"PGA"},
	{"SET8"},
	{"WRT8"},
	{"RD8"},
	{"NEGd"},
	{"INCd"},
	{"DECd"},
	{"ADDd"},
	{"SUBd"},
	{"MULd"},
	{"DIVd"},
	{"MODd"},
	{"SWAP8"},
	{"CMPd"},
	{"dTOi"},
	{"dTOui"},
	{"dTOf"},
	{"iTOd"},
	{"uiTOd"},
	{"fTOd"},
	{"JMPP"},
	{"SRET4"},
	{"SRET8"},
	{"RRET4"},
	{"RRET8"},
	{"STR"},
	{"JS"},
	{"JNS"},
	{"JP"},
	{"JNP"},
	{"CMPIi"},
	{"CMPIui"},
	{"CALLSYS"},
	{"CALLBND"},
	{"RDGA4"},
	{"MOVGA4"},
	{"ADDIi"},
	{"SUBIi"},
	{"CMPIf"},
	{"ADDIf"},
	{"SUBIf"},
	{"MULIi"},
	{"MULIf"},
	{"SUSPEND"},
	{"ALLOC"},
	{"FREE"},
	{"LOADOBJ"},
	{"STOREOBJ"},
	{"GETOBJ"},
	{"REFCPY"},
	{"CHKREF"},
	{"RD1"},
	{"RD2"},
	{"GETOBJREF"},
	{"GETREF"}, 
	{"SWAP48"},
	{"SWAP84"},
	{"OBJTYPE"},
	{"TYPEID"},
	{0},{0},{0},{0},{0},{0}, // 124-129
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 130-139
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 140-149
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 150-159
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 160-169
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 170-179
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 180-189
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 190-199
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 200-209
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 210-219
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 220-229
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 230-239
	{0},{0},{0},{0},{0},{0},	     // 240-245
	{"PSP"},
	{"VAR"},
	{"LINE"},
	{0}, 
	{0},
	{0},
	{0},
	{0},
	{0},
	{"LABEL"}
};
#endif

END_AS_NAMESPACE

#endif
