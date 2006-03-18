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
// as_bytecode.cpp
//
// A class for constructing the final byte code
//

#include <memory.h> // memcpy()
#include <string.h> // some compilers declare memcpy() here
#include <assert.h>
#include <stdio.h> // fopen(), fprintf(), fclose()

#include "as_config.h"
#include "as_bytecode.h"
#include "as_debug.h" // mkdir()
#include "as_array.h"
#include "as_string.h"
#include "as_module.h"
#include "as_scriptengine.h"

asCByteCode::asCByteCode()
{
	first = 0;
	last  = 0;
	largestStackUsed = -1;
}

asCByteCode::~asCByteCode()
{
	ClearAll();
}

void asCByteCode::Finalize()
{
	// verify the bytecode
	PostProcess();

	// Optimize the code (optionally)
	Optimize();

	// Resolve jumps
	ResolveJumpAddresses();

	// Build line numbers buffer
	ExtractLineNumbers();
}

void asCByteCode::ClearAll()
{
	cByteInstruction *del = first;

	while( del ) 
	{
		first = del->next;
		delete del;
		del = first;
	}

	first = 0;
	last = 0;

	lineNumbers.SetLength(0);

	largestStackUsed = -1;
}

void asCByteCode::GetVarsUsed(asCArray<int> &vars)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		if( curr->op == BC_VAR )
		{
			bool insert = true;
			for( int n = 0; n < vars.GetLength(); n++ )
			{
				if( (int)*ARG_DW(curr->arg) == vars[n] )
				{
					insert = false;
					break;
				}
			}

			if( insert )
				vars.PushLast(*ARG_DW(curr->arg));
		}

		curr = curr->next;
	}
}

bool asCByteCode::IsVarUsed(int offset)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		if( curr->op == BC_VAR )
		{
			if( (int)*ARG_DW(curr->arg) == offset )
				return true;
		}

		curr = curr->next;
	}

	return false;
}

void asCByteCode::ExchangeVar(int oldOffset, int newOffset)
{
	cByteInstruction *curr = first;
	while( curr )
	{
		if( curr->op == BC_VAR )
		{
			if( (int)*ARG_DW(curr->arg) == oldOffset )
				*ARG_DW(curr->arg) = newOffset;
		}

		curr = curr->next;
	}
}

void asCByteCode::AddPath(asCArray<cByteInstruction *> &paths, cByteInstruction *instr, int stackSize)
{
	if( instr->marked )
	{
		// Verify the size of the stack
		assert( instr->stackSize == stackSize);
	}
	else
	{
		// Add the destination to the code paths
		instr->marked = true;
		instr->stackSize = stackSize;
		paths.PushLast(instr);
	}
}

bool asCByteCode::IsCombination(cByteInstruction *curr, asBYTE bc1, asBYTE bc2)
{
	if( curr->op == bc1 && curr->next && curr->next->op == bc2 )
		return true;
	
	return false;
}

bool asCByteCode::IsCombination(cByteInstruction *curr, asBYTE bc1, asBYTE bc2, asBYTE bc3)
{
	if( curr->op == bc1 && 
		curr->next && curr->next->op == bc2 &&
		curr->next->next && curr->next->next->op == bc3 )
		return true;
	
	return false;
}

cByteInstruction *asCByteCode::ChangeFirstDeleteNext(cByteInstruction *curr, asBYTE bc)
{
	curr->op = bc;
	
	if( curr->next ) DeleteInstruction(curr->next);
	
	// Continue optimization with the instruction before the altered one
	if( curr->prev ) 
		return curr->prev;
	else
		return curr;
}

cByteInstruction *asCByteCode::DeleteFirstChangeNext(cByteInstruction *curr, asBYTE bc)
{
	assert( curr->next );
	
	cByteInstruction *instr = curr->next;
	instr->op = bc;
	
	DeleteInstruction(curr);
	
	// Continue optimization with the instruction before the altered one
	if( instr->prev ) 
		return instr->prev;
	else
		return instr;
}

void asCByteCode::InsertBefore(cByteInstruction *before, cByteInstruction *instr)
{
	assert(instr->next == 0);
	assert(instr->prev == 0);

	if( before->prev ) before->prev->next = instr;
	instr->prev = before->prev;
	before->prev = instr;
	instr->next = before;

	if( first == before ) first = instr;
}

void asCByteCode::RemoveInstruction(cByteInstruction *instr)
{
	if( instr == first ) first = first->next;
	if( instr == last ) last = last->prev;

	if( instr->prev ) instr->prev->next = instr->next;
	if( instr->next ) instr->next->prev = instr->prev;

	instr->next = 0;
	instr->prev = 0;
}

bool asCByteCode::CanBeSwapped(cByteInstruction *curr)
{
	if( !curr || !curr->next || !curr->next->next ) return false;
	if( curr->next->next->op != BC_SWAP4 ) return false;

	cByteInstruction *next = curr->next;

	if( curr->op != BC_PUSHZERO &&
		curr->op != BC_SET4 &&
		curr->op != BC_RDSF4 &&
		curr->op != BC_PSF &&
		curr->op != BC_PGA && 
		curr->op != BC_RRET4 &&
		curr->op != BC_RDGA4 )
		return false;

	if( next->op != BC_PUSHZERO &&
		next->op != BC_SET4 &&
		next->op != BC_RDSF4 &&
		next->op != BC_PSF &&
		next->op != BC_PGA && 
		next->op != BC_RRET4 &&
		next->op != BC_RDGA4 )
		return false;

	return true;
}

bool asCByteCode::MatchPattern(cByteInstruction *curr)
{
	if( !curr || !curr->next || !curr->next->next ) return false;

	if( curr->op != BC_SET4 ) return false;

	asDWORD op = curr->next->next->op;
	if( op != BC_ADDi &&
		op != BC_MULi &&
		op != BC_ADDf &&
		op != BC_MULf ) return false;

	asDWORD val = curr->next->op;
	if( val != BC_PUSHZERO &&
		val != BC_RDSF4 &&
		val != BC_PSF &&
		val != BC_PGA && 
		val != BC_RRET4 &&
		val != BC_RDGA4 )
		return false;

	return true;
}

cByteInstruction *asCByteCode::OptimizePattern(cByteInstruction *curr)
{
	asDWORD op = curr->next->next->op;

	// Delete the operator instruction
	DeleteInstruction(curr->next->next);

	// Swap the two value instructions
	cByteInstruction *instr = curr->next;
	RemoveInstruction(instr);
	InsertBefore(curr, instr);

	// Change the SET4 to immediate operator
	if( op == BC_ADDi ) curr->op = BC_ADDIi;
	if( op == BC_MULi ) curr->op = BC_MULIi;
	if( op == BC_ADDf ) curr->op = BC_ADDIf;
	if( op == BC_MULf ) curr->op = BC_MULIf;

	// Continue with the instruction before the change
	if( instr->prev ) 
		return instr->prev;
	return instr;
}

bool asCByteCode::IsComplex()
{
	cByteInstruction *instr = first;
	while( instr )
	{
		if( instr->op == BC_CALL || instr->op == BC_CALLSYS || instr->op == BC_CALLBND )
			return true;
		if( instr->op == BC_WRT1 || instr->op == BC_WRT2 || instr->op == BC_WRT4 || instr->op == BC_WRT8 )
			return true;
		if( instr->op == BC_MOV4 || instr->op == BC_MOVSF4 || instr->op == BC_MOVGA4 )
			return true;
		if( instr->op == BC_INCi || instr->op == BC_DECi || 
			instr->op == BC_INCf || instr->op == BC_DECf || 
			instr->op == BC_INCd || instr->op == BC_DECd || 
			instr->op == BC_INCi16 || instr->op == BC_DECi16 || 
			instr->op == BC_INCi8 || instr->op == BC_DECi8 )
			return true;
		if( instr->op == BC_COPY )
			return true;
		if( instr->op == BC_STR || instr->op == BC_ALLOC || instr->op == BC_FREE )
			return true;

		instr = instr->next;
	}

	return false;
}

int asCByteCode::Optimize()
{
	cByteInstruction *instr = first;
	while( instr )
	{
		cByteInstruction *curr = instr;
		instr = instr->next;

		// XXX x, YYY y, SWAP4 -> YYY y, XXX x
		if( CanBeSwapped(curr) )
		{
			// Delete SWAP4
			DeleteInstruction(instr->next);

			// Swap instructions
			RemoveInstruction(instr);
			InsertBefore(curr, instr);

			// Continue with the previous instruction
			if( instr->prev ) instr = instr->prev;
		}
		// SET4 x, YYY y, OP -> YYY y, OPI x
		else if( MatchPattern(curr) )
			instr = OptimizePattern(curr);
		// SWAP4, OP -> OP
		else if( IsCombination(curr, BC_SWAP4, BC_ADDi) ||
				 IsCombination(curr, BC_SWAP4, BC_MULi) ||
				 IsCombination(curr, BC_SWAP4, BC_ADDf) ||
				 IsCombination(curr, BC_SWAP4, BC_MULf) )
			instr = DeleteInstruction(curr);
		// PSF x, RD4 -> RDSF4 x
		else if( IsCombination(curr, BC_PSF, BC_RD4) ) 		  
			instr = ChangeFirstDeleteNext(curr, BC_RDSF4);
		// PSF x, MOV4 -> MOVSF4 x
		else if( IsCombination(curr, BC_PSF, BC_MOV4) )  
			instr = ChangeFirstDeleteNext(curr, BC_MOVSF4);
		// PGA x, RD4 -> RDGA4 x
		else if( IsCombination(curr, BC_PGA, BC_RD4) ) 	  
			instr = ChangeFirstDeleteNext(curr, BC_RDGA4);
		// PGA x, MOV4 -> MOVGA4 x
		else if( IsCombination(curr, BC_PGA, BC_MOV4) )  
			instr = ChangeFirstDeleteNext(curr, BC_MOVGA4);
// Begin PATTERN
		// SET4 x, CMPi -> CMPIi x
		else if( IsCombination(curr, BC_SET4, BC_CMPi) )  
			instr = ChangeFirstDeleteNext(curr, BC_CMPIi);
		// SET4 x, CMPui -> CMPIui x
		else if( IsCombination(curr, BC_SET4, BC_CMPui) ) 
			instr = ChangeFirstDeleteNext(curr, BC_CMPIui);
		// SET4 x, ADDi -> ADDIi x
		else if( IsCombination(curr, BC_SET4, BC_ADDi) ) 
			instr = ChangeFirstDeleteNext(curr, BC_ADDIi);
		// SET4 x, SUBi -> SUBIi x
		else if( IsCombination(curr, BC_SET4, BC_SUBi) ) 
			instr = ChangeFirstDeleteNext(curr, BC_SUBIi);
		// SET4 x, MULi -> MULIi x
		else if( IsCombination(curr, BC_SET4, BC_MULi) ) 
			instr = ChangeFirstDeleteNext(curr, BC_MULIi);
		// SET4 x, CMPf -> CMPIf x
		else if( IsCombination(curr, BC_SET4, BC_CMPf) )  
			instr = ChangeFirstDeleteNext(curr, BC_CMPIf);
		// SET4 x, ADDf -> ADDIf x
		else if( IsCombination(curr, BC_SET4, BC_ADDf) ) 
			instr = ChangeFirstDeleteNext(curr, BC_ADDIf);
		// SET4 x, SUBf -> SUBIf x
		else if( IsCombination(curr, BC_SET4, BC_SUBf) ) 
			instr = ChangeFirstDeleteNext(curr, BC_SUBIf);
		// SET4 x, SUBf -> MULIf x
		else if( IsCombination(curr, BC_SET4, BC_MULf) ) 
			instr = ChangeFirstDeleteNext(curr, BC_MULIf);
// End PATTERN
		// RD4, POP x -> POP x
		else if( (IsCombination(curr, BC_RD4, BC_POP) ||
			      IsCombination(curr, BC_SUBIi, BC_POP) ||
				  IsCombination(curr, BC_ADDIi, BC_POP)) && *ARG_W(instr->arg) > 0 ) 
			instr = DeleteInstruction(curr);
		// POP a, RET b -> RET b
		else if( IsCombination(curr, BC_POP, BC_RET) )
		{
			// We don't combine the POP+RET because RET first restores
			// the previous stack pointer and then pops the arguments

			// Delete POP
			instr = DeleteInstruction(curr);
		}
		// SUSPEND, SUSPEND -> SUSPEND
		// LINE, LINE -> LINE
		else if( IsCombination(curr, BC_SUSPEND, BC_SUSPEND) || 
			     IsCombination(curr, BC_LINE, BC_LINE) )
		{
			// Delete one of the instructions
			instr = DeleteInstruction(curr);
		}
		// PUSH a, PUSH b -> PUSH a+b
		else if( IsCombination(curr, BC_PUSH, BC_PUSH) )
		{
			// Combine the two PUSH
			*ARG_W(instr->arg) = *ARG_W(curr->arg) + *ARG_W(instr->arg);
			// Delete current
			DeleteInstruction(curr);
			// Continue with the instruction before the one removed
			if( instr->prev ) instr = instr->prev;
		}
		// SET4 a, GETREF 0 -> PSF a
		else if( IsCombination(curr, BC_SET4, BC_GETREF) && *ARG_W(instr->arg) == 0 )
		{
			// Convert SET4 a, to PSF a
			*ARG_W(curr->arg) = (short)*ARG_DW(curr->arg);
			curr->size = BCS_PSF;
			curr->op = BC_PSF;
			DeleteInstruction(instr);
			instr = curr;
		}
		// YYY y, POP x -> POP x-1
		else if( (IsCombination(curr, BC_RDSF4, BC_POP) || 
			      IsCombination(curr, BC_RRET4, BC_POP) ||
		          IsCombination(curr, BC_SET4 , BC_POP) ||
		          IsCombination(curr, BC_PSF  , BC_POP) ||
		          IsCombination(curr, BC_PGA  , BC_POP)) && *ARG_W(instr->arg) > 0 )
		{
			// Delete current
			DeleteInstruction(curr);
			// Decrease the POP
			(*ARG_W(instr->arg))--;
			// Continue with the instruction before the one removed
			if( instr->prev ) instr = instr->prev;
		}
		// YYY y, POP x -> POP x+1
		else if( (IsCombination(curr, BC_ADDi, BC_POP) ||
		         IsCombination(curr, BC_SUBi, BC_POP)) && *ARG_W(instr->arg) > 0 )
		{
			// Delete current
			DeleteInstruction(curr);
			// Increase the POP
			(*ARG_W(instr->arg))++;
			// Continue with the instruction before the one removed
			if( instr->prev ) instr = instr->prev;
		}
		// WRT4, POP x -> MOV4, POP x-1 (POP 0 will be removed in next iteration)
		else if( IsCombination(curr, BC_WRT4, BC_POP) && *ARG_W(instr->arg) > 0 )
		{
			// Convert WRT4 to MOV4
			curr->op = BC_MOV4;
			// Decrease the POP
			(*ARG_W(instr->arg))--;
			instr = curr->prev ? curr->prev : curr;
		}
		// SET4 0 -> PUSHZERO
		else if( curr->op == BC_SET4 && *ARG_DW(curr->arg) == 0 )
		{
			// Convert to PUSHZERO
			curr->op = BC_PUSHZERO;
			curr->size = BCS_PUSHZERO;
			// continue with the previous code
			instr = curr->prev ? curr->prev : curr;
		}
		// POP 0 -> remove
		// PUSH 0 -> remove
		else if( (curr->op == BC_POP || curr->op == BC_PUSH ) && *ARG_W(curr->arg) == 0 )  
			instr = DeleteInstruction(curr);
// Begin PATTERN
		// T**; J** +x -> J** +x
		else if( IsCombination(curr, BC_TZ , BC_JZ ) || 
			     IsCombination(curr, BC_TNZ, BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JNZ);
		else if( IsCombination(curr, BC_TNZ, BC_JZ ) ||
			     IsCombination(curr, BC_TZ , BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JZ);
		else if( IsCombination(curr, BC_TS , BC_JZ ) ||
			     IsCombination(curr, BC_TNS, BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JNS);
		else if( IsCombination(curr, BC_TNS, BC_JZ ) ||
			     IsCombination(curr, BC_TS , BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JS);
		else if( IsCombination(curr, BC_TP , BC_JZ ) ||
			     IsCombination(curr, BC_TNP, BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JNP);
		else if( IsCombination(curr, BC_TNP, BC_JZ ) ||
			     IsCombination(curr, BC_TP , BC_JNZ) )
			instr = DeleteFirstChangeNext(curr, BC_JP);
// End PATTERN
		// JMP +0 -> remove
		else if( IsCombination(curr, BC_JMP, BC_LABEL) && *ARG_W(curr->arg) == *ARG_W(instr->arg) )
			instr = DeleteInstruction(curr);
	}

	return 0;
}

void asCByteCode::ExtractLineNumbers()
{
	int lastLinePos = -1;
	int lastEIDPos = -1;
	int pos = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		cByteInstruction *curr = instr;
		instr = instr->next;
		
		if( curr->op == BC_LINE )
		{
			if( lastLinePos == pos )
			{
				lineNumbers.PopLast();
				lineNumbers.PopLast();
			}

			lastLinePos = pos;
			lineNumbers.PushLast(pos);
			lineNumbers.PushLast(*(int*)ARG_DW(curr->arg));

#ifndef BUILD_WITHOUT_LINE_CUES
			// Transform BC_LINE into BC_SUSPEND
			curr->op = BC_SUSPEND;
			curr->size = BCS_SUSPEND;
			pos += curr->size;
#else
			// Delete the instruction
			DeleteInstruction(curr);
#endif
		}
		else
			pos += curr->size;
	}
}

int asCByteCode::GetSize()
{
	int size = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		size += instr->GetSize();

		instr = instr->next;
	}

	return size;
}

void asCByteCode::AddCode(asCByteCode *bc)
{
	if( bc->first )
	{
		if( first == 0 )
		{
			first = bc->first;
			last = bc->last;
			bc->first = 0;
			bc->last = 0;
		}
		else
		{
			last->next = bc->first;
			bc->first->prev = last;
			last = bc->last;
			bc->first = 0;
			bc->last = 0;
		}
	}
}

int asCByteCode::AddInstruction()
{
	cByteInstruction *instr = new cByteInstruction();
	if( first == 0 )
	{
		first = last = instr;
	}
	else
	{
		last->AddAfter(instr);
		last = instr;
	}

	return 0;
}

int asCByteCode::AddInstructionFirst()
{
	cByteInstruction *instr = new cByteInstruction();
	if( first == 0 )
	{
		first = last = instr;
	}
	else
	{
		first->AddBefore(instr);
		first = instr;
	}

	return 0;
}

void asCByteCode::Call(int instr, int funcID, int pop)
{
	if( AddInstruction() < 0 )
		return;

	last->op = instr;
	last->size = bcSize[instr];
	last->stackInc = -pop; // BC_CALL and BC_CALLBND doesn't pop the argument but when the callee returns the arguments are already popped
	*((int*)ARG_DW(last->arg)) = funcID;
}

void asCByteCode::Alloc(int instr, int objID, int funcID, int pop)
{
	if( AddInstruction() < 0 )
		return;

	last->op = instr;
	last->size = bcSize[instr];
	last->stackInc = -pop; // BC_ALLOC
	*((int*)ARG_DW(last->arg)) = objID;
	*((int*)(ARG_DW(last->arg)+1)) = funcID;
}

void asCByteCode::Ret(int pop)
{
	if( AddInstruction() < 0 )
		return;

	last->op = BC_RET;
	last->size = bcSize[BC_RET];
	last->stackInc = 0; // The instruction pops the argument, but it doesn't affect current function
	*((short*)ARG_W(last->arg)) = pop;
}

void asCByteCode::JmpP(asDWORD max)
{
	if( AddInstruction() < 0 )
		return;
	
	last->op = BC_JMPP;
	last->size = bcSize[BC_JMPP];
	last->stackInc = bcStackInc[BC_JMPP];

	// Store the largest jump that is made for PostProcess()
	*ARG_DW(last->arg) = max;
}

void asCByteCode::Label(short label)
{
	if( AddInstruction() < 0 )
		return;

	last->op = BC_LABEL;
	last->size = 0;
	last->stackInc = 0;
	*((short*) ARG_W(last->arg)) = label;
}

void asCByteCode::Line(int line, int column)
{
	if( AddInstruction() < 0 )
		return;

	last->op = BC_LINE;
	last->size = BCS_LINE;
	last->stackInc = 0;
	*((int*) ARG_DW(last->arg)) = (line & 0xFFFFF)|((column & 0xFFF)<<20);
}

void asCByteCode::Destructor(int bc, asDWORD destr, int sfOffset)
{
	if( AddInstruction() < 0 )
		return;

	last->op = (asBYTE)bc;
	last->size = 0;
	last->stackInc = 0;

	*(ARG_DW(last->arg)) = destr;
	*(ARG_DW(last->arg)+1) = sfOffset;
}

int asCByteCode::FindLabel(int label, cByteInstruction *from, cByteInstruction **dest, int *positionDelta)
{
	// Search forward
	int labelPos = -from->GetSize();

	cByteInstruction *labelInstr = from;
	while( labelInstr )
	{
		labelPos += labelInstr->GetSize();
		labelInstr = labelInstr->next;

		if( labelInstr && labelInstr->op == BC_LABEL )
		{
			if( (int)*((short*) ARG_W(labelInstr->arg)) == label )
				break;
		}
	}

	if( labelInstr == 0 )
	{
		// Search backwards
		labelPos = -from->GetSize();

		labelInstr = from;
		while( labelInstr )
		{
			labelInstr = labelInstr->prev;
			if( labelInstr )
			{
				labelPos -= labelInstr->GetSize();

				if( labelInstr->op == BC_LABEL )
				{
					if( (int)*((short*) ARG_W(labelInstr->arg)) == label )
						break;
				}
			}
		}
	}

	if( labelInstr != 0 )
	{
		if( dest ) *dest = labelInstr;
		if( positionDelta ) *positionDelta = labelPos;
		return 0;
	}

	return -1;
}

int asCByteCode::ResolveJumpAddresses()
{
	int pos = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		// The program pointer is updated as the instruction is read
		pos += instr->GetSize();

		if( instr->op == BC_JMP || 
			instr->op == BC_JZ || instr->op == BC_JNZ ||
			instr->op == BC_JS || instr->op == BC_JNS || 
			instr->op == BC_JP || instr->op == BC_JNP )
		{
			int label = *((int*) ARG_DW(instr->arg));
			int labelPosOffset;			
			int r = FindLabel(label, instr, 0, &labelPosOffset);
			if( r == 0 )
				*((int*) ARG_DW(instr->arg)) = labelPosOffset;
			else
				return -1;
		}

		instr = instr->next;
	}

	return 0;
}


cByteInstruction *asCByteCode::DeleteInstruction(cByteInstruction *instr)
{
	if( instr == 0 ) return 0;

	cByteInstruction *ret = instr->prev ? instr->prev : instr->next;
	
	RemoveInstruction(instr);

	delete instr;
	
	return ret;
}

void asCByteCode::Output(asBYTE *array)
{
	// TODO: Convert byte code to relocation address here

	asBYTE *ap = array;

	cByteInstruction *instr = first;
	while( instr )
	{
		if( instr->GetSize() > 0 )
		{
#ifdef USE_ASM_VM
			memcpy(ap, &relocTable[instr->op], 4);
#else
			memcpy(ap, &instr->op, 4);
#endif
			memcpy(ap+4, instr->arg, instr->GetSize()-4);
		}

		ap += instr->GetSize();
		instr = instr->next;
	}
}

void asCByteCode::PostProcess()
{
	if( first == 0 ) return;

	// This function will do the following
	// - Verify if there is any code that never gets executed and remove it
	// - Calculate the stack size at the position of each byte code 
	// - Calculate the largest stack needed

	largestStackUsed = 0;

	cByteInstruction *instr = first;
	while( instr )
	{
		instr->marked = false;
		instr->stackSize = -1;
		instr = instr->next;
	}

	// Add the first instruction to the list of unchecked code paths
	asCArray<cByteInstruction *> paths;
	AddPath(paths, first, 0);

	// Go through each of the code paths
	for( int p = 0; p < paths.GetLength(); ++p )
	{
		instr = paths[p];
		int stackSize = instr->stackSize;
		
		while( instr )
		{
			instr->marked = true;
			instr->stackSize = stackSize;
			stackSize += instr->stackInc;
			if( stackSize > largestStackUsed ) 
				largestStackUsed = stackSize;

			// VAR -> SET4
			if( instr->op == BC_VAR )
			{
				instr->op = BC_SET4;
			}

			// PSP -> PSF
			if( instr->op == BC_PSP )
			{
				instr->op = BC_PSF;
				*ARG_W(instr->arg) += instr->stackSize;
			}

			if( instr->op == BC_JMP )
			{
				// Find the label that we should jump to
				int label = *((int*) ARG_DW(instr->arg));
				cByteInstruction *dest;
				int r = FindLabel(label, instr, &dest, 0);
				assert( r == 0 );
				
				AddPath(paths, dest, stackSize);
				break;
			}
			else if( instr->op == BC_JZ || instr->op == BC_JNZ ||
					 instr->op == BC_JS || instr->op == BC_JNS ||
					 instr->op == BC_JP || instr->op == BC_JNP )
			{
				// Find the label that is being jumped to
				int label = *((int*) ARG_DW(instr->arg));
				cByteInstruction *dest;
				int r = FindLabel(label, instr, &dest, 0);
				assert( r == 0 );
				
				AddPath(paths, dest, stackSize);
				
				// Add both paths to the code paths
				AddPath(paths, instr->next, stackSize);
				
				break;
			}
			else if( instr->op == BC_JMPP )
			{
				// I need to know the largest value possible
				asDWORD max = *ARG_DW(instr->arg);
								
				// Add all destinations to the code paths
				cByteInstruction *dest = instr->next;
				for( asDWORD n = 0; n <= max && dest != 0; ++n )
				{
					AddPath(paths, dest, stackSize);
					dest = dest->next;
				}				
				
				break;				
			}
			else
			{
				instr = instr->next;
				if( instr == 0 || instr->marked )
					break;
			}
		}
	}
	
	// Are there any instructions that didn't get visited?
	instr = first;
	while( instr )
	{
		if( instr->marked == false )
		{
			// TODO:
			// Give warning of unvisited code

			// Remove it
			cByteInstruction *curr = instr;
			instr = instr->next;

			// TODO: Add instruction again
			DeleteInstruction(curr);
		}
		else
			instr = instr->next;
	}	
}

void asCByteCode::DebugOutput(const char *name, asCModule *module, asCScriptEngine *engine)
{
#ifdef AS_DEBUG
	mkdir("AS_DEBUG");

	asCString str = "AS_DEBUG/";
	str += name;

	FILE *file = fopen(str, "w");

	int pos = 0;
	int lineIndex = 0;
	cByteInstruction *instr = first;
	while( instr )
	{
		if( lineIndex < lineNumbers.GetLength() && lineNumbers[lineIndex] == pos )
		{
			asDWORD line = lineNumbers[lineIndex+1];
			fprintf(file, "- %d,%d -\n", line&0xFFFFF, line>>20);
			lineIndex += 2;
		}

		fprintf(file, "%5d ", pos);
		pos += instr->GetSize();

		fprintf(file, "%3d %c ", instr->stackSize, instr->marked ? '*' : ' ');

		switch( instr->op )
		{
		case BC_POP:
		case BC_PUSH:
		case BC_RET:
		case BC_RDSF4:
		case BC_MOVSF4:
		case BC_PSF:
		case BC_COPY:
		case BC_GETOBJ:
		case BC_GETOBJREF:
		case BC_GETREF:
		case BC_LOADOBJ:
		case BC_STOREOBJ:
			assert(instr->size == BCSIZE2);
			fprintf(file, "   %-8s %d\n", bcName[instr->op].name, (int)*((short*) ARG_W(instr->arg)));
			break;

		case BC_STR:
			{
				int id = *(short*) ARG_W(instr->arg);
				asBSTR *bstr = module->GetConstantBStr(id);
				assert(instr->size == BCSIZE2);
				fprintf(file, "   %-8s %d         (l:%d s:\"%.10s\")\n", bcName[instr->op].name, (int)*((short*) ARG_W(instr->arg)), asBStrLength(*bstr), *bstr);
			}
			break;

		case BC_PGA:
		case BC_RDGA4:
		case BC_MOVGA4:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %d\n", bcName[instr->op].name, (int)*ARG_DW(instr->arg));
			break;

		case BC_SET4:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s 0x%lx          (i:%d, f:%g)\n", bcName[instr->op].name, *ARG_DW(instr->arg), *((int*) ARG_DW(instr->arg)), *((float*) ARG_DW(instr->arg)));
			break;

		case BC_CMPIi:
		case BC_ADDIi:
		case BC_SUBIi:
		case BC_MULIi:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %d\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)));
			break;

		case BC_CMPIui:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %u\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)));
			break;

		case BC_CMPIf:
		case BC_ADDIf:
		case BC_SUBIf:
		case BC_MULIf:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %g\n", bcName[instr->op].name, *((float*) ARG_DW(instr->arg)));
			break;

		case BC_SET8:
			assert(instr->size == BCSIZE8);
#ifdef __GNUC__
			fprintf(file, "   %-8s 0x%llx           (i:%lld, f:%g)\n", bcName[instr->op].name, *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#else
			fprintf(file, "   %-8s 0x%I64x          (i:%I64d, f:%g)\n", bcName[instr->op].name, *ARG_QW(instr->arg), *((__int64*) ARG_QW(instr->arg)), *((double*) ARG_QW(instr->arg)));
#endif
			break;

		case BC_ALLOC:
			assert(instr->size == BCSIZE8);
			fprintf(file, "   %-8s %u, %d\n", bcName[instr->op].name, *(int*)ARG_DW(instr->arg), *(int*)(ARG_DW(instr->arg)+1));
			break;

		case BC_CALL:
		case BC_CALLSYS:
		case BC_CALLBND:
			{
				int funcID = *(int*)ARG_DW(instr->arg) | module->moduleID;
				asCString decl = engine->GetFunctionDeclaration(funcID);

				assert(instr->size == BCSIZE4);
				fprintf(file, "   %-8s %d           (%s)\n", bcName[instr->op].name, (int)*((int*) ARG_DW(instr->arg)), decl.AddressOf());
			}
			break;	

		case BC_FREE:
		case BC_REFCPY:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %d\n", bcName[instr->op].name, (int)*((int*) ARG_DW(instr->arg)));
			break;

		case BC_JMP:
		case BC_JZ:
		case BC_JNZ:
		case BC_JS:
		case BC_JNS:
		case BC_JP:
		case BC_JNP:
			assert(instr->size == BCSIZE4);
			fprintf(file, "   %-8s %+d              (d:%d)\n", bcName[instr->op].name, *((int*) ARG_DW(instr->arg)), pos+*((int*) ARG_DW(instr->arg)));
			break;

		case BC_LABEL:
			fprintf(file, "%d:\n", (int)*((short*) ARG_W(instr->arg)));
			break;

		default:
			assert(instr->size == BCSIZE0);
			if( bcName[instr->op].name )
				fprintf(file, "   %s\n", bcName[instr->op].name);
			else
			{
				assert(false);
				fprintf(file, "   Oops! I didn't recognize this code (%ld)\n", instr->op);
			}
		}

		instr = instr->next;
	}

	fclose(file);
#endif
}

//=============================================================================

// Decrease stack with "numDwords"
int asCByteCode::Pop(int numDwords)
{
	if( AddInstruction() < 0 )
		return 0;

	last->op = BC_POP;
	*((short*) ARG_W(last->arg)) = (short)numDwords;
	last->size = BCS_POP;
	last->stackInc = -numDwords;

	return last->stackInc;
}

// Increase stack with "numDwords"
int asCByteCode::Push(int numDwords)
{
	if( AddInstruction() < 0 )
		return 0;

	last->op = BC_PUSH;
	*((short*) ARG_W(last->arg)) = (short)numDwords;
	last->size = BCS_PUSH;
	last->stackInc = numDwords;

	return last->stackInc;
}


int asCByteCode::InsertFirstInstrDWORD(int bc, asDWORD param)
{
	assert(bcSize[bc] == BCSIZE4);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstructionFirst() < 0 )
		return 0;

	first->op = (asBYTE)bc;
	*ARG_DW(first->arg) = param;
	first->size = bcSize[bc];
	first->stackInc = bcStackInc[bc];

	return first->stackInc;
}

int asCByteCode::InsertFirstInstrQWORD(int bc, asQWORD param)
{
	assert(bcSize[bc] == BCSIZE8);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstructionFirst() < 0 )
		return 0;

	first->op = (asBYTE)bc;
	*ARG_QW(first->arg) = param;
	first->size = bcSize[bc];
	first->stackInc = bcStackInc[bc];

	return first->stackInc;
}

int asCByteCode::Instr(int bc)
{
	assert(bcSize[bc] == BCSIZE0);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op       = (asBYTE)bc;
	last->size     = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrSHORT(int bc, short param)
{
	assert(bcSize[bc] == BCSIZE2);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*((short*) ARG_W(last->arg)) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrINT(int bc, int param)
{
	assert(bcSize[bc] == BCSIZE4);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*((int*) ARG_DW(last->arg)) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrBYTE(int bc, asBYTE param)
{
	assert(bcSize[bc] == BCSIZE1);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*ARG_B(last->arg) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrDWORD(int bc, asDWORD param)
{
	assert(bcSize[bc] == BCSIZE4);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*ARG_DW(last->arg) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrQWORD(int bc, asQWORD param)
{
	assert(bcSize[bc] == BCSIZE8);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*ARG_QW(last->arg) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrWORD(int bc, asWORD param)
{
	assert(bcSize[bc] == BCSIZE2);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*ARG_W(last->arg) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrFLOAT(int bc, float param)
{
	assert(bcSize[bc] == BCSIZE4);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*((float*) ARG_DW(last->arg)) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::InstrDOUBLE(int bc, double param)
{
	assert(bcSize[bc] == BCSIZE8);
	assert(bcStackInc[bc] != 0xFFFF);

	if( AddInstruction() < 0 )
		return 0;

	last->op = (asBYTE)bc;
	*((double*) ARG_QW(last->arg)) = param;
	last->size = bcSize[bc];
	last->stackInc = bcStackInc[bc];

	return last->stackInc;
}

int asCByteCode::GetLastCode()
{
	if( last == 0 ) return -1;

	return last->op;
}

int asCByteCode::RemoveLastCode()
{
	if( last == 0 ) return -1;

	if( first == last )
	{
		delete last;
		first = 0;
		last = 0;
	}
	else
	{
		cByteInstruction *bc = last;
		last = bc->prev;

		bc->Remove();
		delete bc;
	}

	return 0;
}

//===================================================================

cByteInstruction::cByteInstruction()
{
	next = 0;
	prev = 0;

	op = 0xFF;

	size = 0;
	stackInc = 0;
}

void cByteInstruction::AddAfter(cByteInstruction *nextCode)
{
	if( next )
		next->prev = nextCode;

	nextCode->next = next;
	nextCode->prev = this;
	next = nextCode;
}

void cByteInstruction::AddBefore(cByteInstruction *prevCode)
{
	if( prev )
		prev->next = prevCode;

	prevCode->prev = prev;
	prevCode->next = this;
	prev = prevCode;
}

int cByteInstruction::GetSize()
{
	return size;
}

int cByteInstruction::GetStackIncrease()
{
	return stackInc;
}

void cByteInstruction::Remove()
{
	if( prev ) prev->next = next;
	if( next ) next->prev = prev;
	prev = 0;
	next = 0;
}

