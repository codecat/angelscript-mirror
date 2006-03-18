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
// as_bytecodedef.h
//
// Byte code definitions
//


#ifndef AS_BYTECODEDEF_H
#define AS_BYTECODEDEF_H

#include "as_config.h"

//---------------------------------------------
// Byte code instructions

#define BC_POP		0
#define BC_PUSH		(BC_POP+1)
#define BC_SET4		(BC_POP+2)
#define BC_RD4		(BC_POP+3)
#define BC_RDSF4	(BC_POP+4)
#define BC_WRT4		(BC_POP+5)
#define BC_MOV4		(BC_POP+6)
#define BC_PSF		(BC_POP+7)	// Push stack frame
#define BC_MOVSF4   (BC_POP+8)
#define BC_SWAP4    (BC_POP+9) // Swap top two dwords
#define BC_STORE4   (BC_POP+10) // Store top dword in temp reg
#define BC_RECALL4  (BC_POP+11) // Push temp reg on stack
#define BC_ADDOFF   (BC_POP+12) // Add an offset to reference

#define BC_CALL		(BC_POP+13)  // Call function
#define BC_RET		(BC_CALL+1) // Return from function
#define BC_JMP		(BC_CALL+2)
#define BC_JZ		(BC_CALL+3)
#define BC_JNZ		(BC_CALL+4)

#define BC_TZ		(BC_CALL+5)  // Test if zero
#define BC_TNZ		(BC_TZ+1)  // Test if not zero
#define BC_TS		(BC_TZ+2)  // Test if signaled (less than zero)
#define BC_TNS		(BC_TZ+3)  // Test if not signaled (zero or greater)
#define BC_TP		(BC_TZ+4)  // Test if positive (greater than zero)
#define BC_TNP		(BC_TZ+5)  // Test if not positive (zero or less)

#define BC_ADDi		(BC_TZ+6)
#define BC_SUBi		(BC_ADDi+1)
#define BC_MULi		(BC_ADDi+2)
#define BC_DIVi		(BC_ADDi+3)
#define BC_MODi		(BC_ADDi+4)
#define BC_NEGi		(BC_ADDi+5)
#define BC_CMPi		(BC_ADDi+6)
#define BC_INCi		(BC_ADDi+7)
#define BC_DECi		(BC_ADDi+8)
#define BC_I2F		(BC_ADDi+9)	// integer to float conversion

#define BC_ADDf		(BC_ADDi+10)
#define BC_SUBf		(BC_ADDf+1)
#define BC_MULf		(BC_ADDf+2)
#define BC_DIVf		(BC_ADDf+3)
#define BC_MODf		(BC_ADDf+4)
#define BC_NEGf		(BC_ADDf+5)
#define BC_CMPf		(BC_ADDf+6)
#define BC_INCf		(BC_ADDf+7)
#define BC_DECf		(BC_ADDf+8)
#define BC_F2I		(BC_ADDf+9)	// float to integer conversion

#define BC_BNOT		(BC_ADDf+10)
#define BC_BAND		(BC_BNOT+1)
#define BC_BOR		(BC_BNOT+2)
#define BC_BXOR		(BC_BNOT+3)
#define BC_BSLL		(BC_BNOT+4)
#define BC_BSRL		(BC_BNOT+5)
#define BC_BSRA		(BC_BNOT+6)

#define BC_UI2F     (BC_BNOT+7)
#define BC_F2UI     (BC_UI2F+1)
#define BC_CMPui    (BC_UI2F+2)
#define BC_SB       (BC_UI2F+3) // Signed byte
#define BC_SW       (BC_UI2F+4) // Signed word
#define BC_UB       (BC_UI2F+5) // Unsigned byte
#define BC_UW       (BC_UI2F+6) // Unsigned word
#define BC_WRT1     (BC_UI2F+7)
#define BC_WRT2     (BC_UI2F+8)
#define BC_INCi16   (BC_UI2F+9)
#define BC_INCi8    (BC_UI2F+10)
#define BC_DECi16   (BC_UI2F+11)
#define BC_DECi8    (BC_UI2F+12)
#define BC_PUSHZERO (BC_UI2F+13)
#define BC_COPY     (BC_UI2F+14)
#define BC_PGA      (BC_UI2F+15) // Push Global Address

#define BC_SET8     (BC_UI2F+16)
#define BC_WRT8     (BC_SET8+1)
#define BC_RD8      (BC_SET8+2)
#define BC_NEGd     (BC_SET8+3)
#define BC_INCd     (BC_SET8+4)
#define BC_DECd     (BC_SET8+5)
#define BC_ADDd     (BC_SET8+6)
#define BC_SUBd     (BC_SET8+7)
#define BC_MULd     (BC_SET8+8)
#define BC_DIVd     (BC_SET8+9)
#define BC_MODd     (BC_SET8+10)
#define BC_SWAP8    (BC_SET8+11)
#define BC_CMPd     (BC_SET8+12)
#define BC_dTOi     (BC_SET8+13)
#define BC_dTOui    (BC_SET8+14)
#define BC_dTOf     (BC_SET8+15)
#define BC_iTOd     (BC_SET8+16)
#define BC_uiTOd    (BC_SET8+17)
#define BC_fTOd     (BC_SET8+18)
#define BC_JMPP     (BC_SET8+19) // Jump relative to top stack dword
#define BC_PEID     (BC_SET8+20) // Push exception ID
#define BC_SRET4    (BC_SET8+21) // Store dword return value
#define BC_SRET8    (BC_SET8+22) // Store qword return value
#define BC_RRET4    (BC_SET8+23) // Recall dword return value
#define BC_RRET8    (BC_SET8+24) // Recall qword return value
#define BC_STR      (BC_SET8+25) // Push string address and length on stack
#define BC_JS		(BC_SET8+26) // Same as TS+JMP1 or TNS+JMP0
#define BC_JNS		(BC_SET8+27) // Same as TNS+JMP1 or TS+JMP0
#define BC_JP		(BC_SET8+28) // Same as TP+JMP1 or TNP+JMP0
#define BC_JNP		(BC_SET8+29) // Same as TNP+JMP1 or TP+JMP0
#define BC_CMPIi    (BC_SET8+30) // Same as SET4+CMPi
#define BC_CMPIui   (BC_SET8+31) // Same as SET4+CMPui
#define BC_CALLSYS  (BC_SET8+32)
#define BC_CALLBND  (BC_SET8+33)
#define BC_RDGA4    (BC_SET8+34) // Same as PGA+RD4
#define BC_MOVGA4   (BC_SET8+35) // Same as PGA+MOV4
#define BC_ADDIi    (BC_SET8+36) // Same as SET4+ADDi
#define BC_SUBIi    (BC_SET8+37) // Same as SET4+SUBi
#define BC_CMPIf    (BC_SET8+38) // Same as SET4+CMPf
#define BC_ADDIf    (BC_SET8+39) // Same as SET4+ADDf
#define BC_SUBIf    (BC_SET8+40) // Same as SET4+SUBf
#define BC_MULIi    (BC_SET8+41) // Same as SET4+MULi
#define BC_MULIf    (BC_SET8+42) // Same as SET4+MULf

#define BC_SUSPEND  (BC_MULIf+1)
#define BC_END      (BC_SUSPEND+1)
#define BC_MAXBYTECODE (BC_END+1)

// Temporary tokens, can't be output to the final program
#define BC_PSP		   246
#define BC_EID         247
#define BC_LINE	       248
#define BC_ADDDESTRASF 249
#define BC_REMDESTRASF 250
#define BC_ADDDESTRSF  251
#define BC_REMDESTRSF  252
#define BC_ADDDESTRSP  253
#define BC_REMDESTRSP  254
#define BC_LABEL	   255

//------------------------------------------------------------
// Relocation Table
extern asDWORD relocTable[BC_MAXBYTECODE];

//------------------------------------------------------------
// Instruction sizes

const int BCS_POP      = BCSIZE2;
const int BCS_PUSH     = BCSIZE2;
const int BCS_SET4     = BCSIZE4;
const int BCS_RD4      = BCSIZE0;
const int BCS_RDSF4    = BCSIZE2;
const int BCS_WRT4     = BCSIZE0;
const int BCS_MOV4     = BCSIZE0;
const int BCS_PSF      = BCSIZE2;
const int BCS_MOVSF4   = BCSIZE2;
const int BCS_SWAP4    = BCSIZE0;
const int BCS_STORE4   = BCSIZE0;
const int BCS_RECALL4  = BCSIZE0;
const int BCS_ADDOFF   = BCSIZE0;

const int BCS_CALL     = BCSIZE4;
const int BCS_RET      = BCSIZE2;
const int BCS_JMP      = BCSIZE4;
const int BCS_JZ       = BCSIZE4;
const int BCS_JNZ      = BCSIZE4;

const int BCS_TZ       = BCSIZE0;
const int BCS_TNZ      = BCSIZE0;
const int BCS_TS       = BCSIZE0;
const int BCS_TNS      = BCSIZE0;
const int BCS_TP       = BCSIZE0;
const int BCS_TNP      = BCSIZE0;

const int BCS_ADDi     = BCSIZE0;
const int BCS_SUBi     = BCSIZE0;
const int BCS_MULi     = BCSIZE0;
const int BCS_DIVi     = BCSIZE0;
const int BCS_MODi     = BCSIZE0;
const int BCS_NEGi     = BCSIZE0;
const int BCS_CMPi     = BCSIZE0;
const int BCS_INCi     = BCSIZE0;
const int BCS_DECi     = BCSIZE0;
const int BCS_I2F      = BCSIZE0;

const int BCS_ADDf     = BCSIZE0;
const int BCS_SUBf     = BCSIZE0;
const int BCS_MULf     = BCSIZE0;
const int BCS_DIVf     = BCSIZE0;
const int BCS_MODf     = BCSIZE0;
const int BCS_NEGf     = BCSIZE0;
const int BCS_CMPf     = BCSIZE0;
const int BCS_INCf     = BCSIZE0;
const int BCS_DECf     = BCSIZE0;
const int BCS_F2I      = BCSIZE0;

const int BCS_BNOT     = BCSIZE0;
const int BCS_BAND     = BCSIZE0;
const int BCS_BOR      = BCSIZE0;
const int BCS_BXOR     = BCSIZE0;
const int BCS_BSLL     = BCSIZE0;
const int BCS_BSRL     = BCSIZE0;
const int BCS_BSRA     = BCSIZE0;

const int BCS_UI2F     = BCSIZE0;
const int BCS_F2UI     = BCSIZE0;
const int BCS_CMPui    = BCSIZE0;
const int BCS_SB       = BCSIZE0;
const int BCS_SW       = BCSIZE0;
const int BCS_UB       = BCSIZE0;
const int BCS_UW       = BCSIZE0;
const int BCS_WRT1     = BCSIZE0;
const int BCS_WRT2     = BCSIZE0;
const int BCS_INCi16   = BCSIZE0;
const int BCS_INCi8    = BCSIZE0;
const int BCS_DECi16   = BCSIZE0;
const int BCS_DECi8    = BCSIZE0;
const int BCS_PUSHZERO = BCSIZE0;
const int BCS_COPY     = BCSIZE2;
const int BCS_PGA      = BCSIZE4;

const int BCS_SET8     = BCSIZE8;
const int BCS_WRT8     = BCSIZE0;
const int BCS_RD8      = BCSIZE0;
const int BCS_NEGd     = BCSIZE0;
const int BCS_INCd     = BCSIZE0;
const int BCS_DECd     = BCSIZE0;
const int BCS_ADDd     = BCSIZE0;
const int BCS_SUBd     = BCSIZE0;
const int BCS_MULd     = BCSIZE0;
const int BCS_DIVd     = BCSIZE0;
const int BCS_MODd     = BCSIZE0;
const int BCS_SWAP8    = BCSIZE0;
const int BCS_CMPd     = BCSIZE0;
const int BCS_dTOi     = BCSIZE0;
const int BCS_dTOui    = BCSIZE0;
const int BCS_dTOf     = BCSIZE0;
const int BCS_iTOd     = BCSIZE0;
const int BCS_uiTOd    = BCSIZE0;
const int BCS_fTOd     = BCSIZE0;
const int BCS_JMPP     = BCSIZE0;
const int BCS_PEID     = BCSIZE0;
const int BCS_SRET4    = BCSIZE0;
const int BCS_SRET8    = BCSIZE0;
const int BCS_RRET4    = BCSIZE0;
const int BCS_RRET8    = BCSIZE0;
const int BCS_STR      = BCSIZE2;
const int BCS_JS       = BCSIZE4;
const int BCS_JNS      = BCSIZE4;
const int BCS_JP       = BCSIZE4;
const int BCS_JNP      = BCSIZE4;
const int BCS_CMPIi    = BCSIZE4;
const int BCS_CMPIui   = BCSIZE4;
const int BCS_CALLSYS  = BCSIZE4;
const int BCS_CALLBND  = BCSIZE4;
const int BCS_RDGA4    = BCSIZE4;
const int BCS_MOVGA4   = BCSIZE4;
const int BCS_ADDIi    = BCSIZE4;
const int BCS_SUBIi    = BCSIZE4;
const int BCS_CMPIf    = BCSIZE4;
const int BCS_ADDIf    = BCSIZE4;
const int BCS_SUBIf    = BCSIZE4;
const int BCS_MULIi    = BCSIZE4;
const int BCS_MULIf    = BCSIZE4;
const int BCS_SUSPEND  = BCSIZE0;
const int BCS_END      = BCSIZE0;

// Temporary
const int BCS_PSP      = BCSIZE2;
#ifdef BUILD_WITH_LINE_CUES
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
	BCS_ADDOFF,

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
	BCS_PEID,
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
	BCS_END,
	0,0,0,0,0,0,0,0, // 112-119
	0,0,0,0,0,0,0,0,0,0, // 120-129
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
	0,  // BC_EID
	BCS_LINE,  
	0,  // BC_ADDDESTRASF
	0,  // BC_REMDESTRASF
	0,  // BC_ADDDESTRSF
	0,  // BC_REMDESTRSF
	0,  // BC_ADDDESTRSP
	0,  // BC_REMDESTRSP
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
    -1,     // BC_ADDOFF

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
	1,		// BC_PEID
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
	0,		// BC_END
	0,0,0,0,0,0,0,0, // 112-119
	0,0,0,0,0,0,0,0,0,0, // 120-129
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
	0xFFFF, // BC_EID
	0xFFFF, // BC_LINE
	0xFFFF, // BC_ADDDESTRASF
	0xFFFF, // BC_REMDESTRASF
	0xFFFF, // BC_ADDDESTRSF
	0xFFFF, // BC_REMDESTRSF
	0xFFFF, // BC_ADDDESTRSP
	0xFFFF, // BC_REMDESTRSP
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
	{"ADDOFF"},

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
	{"PEID"},
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
	{"END"},
	{0},{0},{0},{0},{0},{0},{0},{0}, // 112-119
	{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}, // 120-129
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
	{"EID"},
	{"LINE"},
	{"ADDDESTRASF"}, 
	{"REMDESTRASF"},
	{"ADDDESTRSF"},
	{"REMDESTRSF"},
	{"ADDDESTRSP"},
	{"REMDESTRSP"},
	{"LABEL"}
};
#endif

#endif
