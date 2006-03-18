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
// as_compiler.cpp
//
// The class that does the actual compilation of the functions
//

#include <math.h> // fmodf()

#include "as_config.h"
#include "as_compiler.h"
#include "as_tokendef.h"
#include "as_tokenizer.h"
#include "as_string_util.h"
#include "as_texts.h"


asCCompiler::asCCompiler()
{
	builder = 0;
	script = 0;

	variables = 0;
	isProcessingDeferredOutParams = false;
	noCodeOutput = 0;
}

asCCompiler::~asCCompiler()
{
	while( variables )
	{
		asCVariableScope *var = variables;
		variables = variables->parent;

		delete var;
	}
}

void asCCompiler::Reset(asCBuilder *builder, asCScriptCode *script)
{
	this->builder = builder;
	this->engine = builder->engine;
	this->script = script;

	hasCompileErrors = false;

	nextLabel = 0;
	breakLabels.SetLength(0);
	continueLabels.SetLength(0);

	byteCode.ClearAll();
	objVariableTypes.SetLength(0);
	objVariablePos.SetLength(0);

	globalExpression = false;
}

int asCCompiler::CompileFunction(asCBuilder *builder, asCScriptCode *script, asCScriptNode *func)
{
	Reset(builder, script);

	int stackPos = 0;

	// Reserve a label for the cleanup code
	nextLabel++;

	// Add the first variable scope, which the parameters and 
	// variables declared in the outermost statement block is 
	// part of.
	AddVariableScope();

	//----------------------------------------------
	// Examine return type

	asCDataType returnType = builder->CreateDataTypeFromNode(func->firstChild, script);
	returnType = builder->ModifyDataTypeFromNode(returnType, func->firstChild->next, 0);

	//----------------------------------------------
	// Declare parameters
	// Find first parameter
	asCScriptNode *node = func->firstChild;
	while( node && node->nodeType != snParameterList ) 
		node = node->next;

	// Register parameters from last to first, otherwise they will be destroyed in the wrong order
	asCVariableScope vs(0);

	if( node ) node = node->firstChild;
	while( node )
	{
		// Get the parameter type
		asCDataType type = builder->CreateDataTypeFromNode(node, script);

		type = builder->ModifyDataTypeFromNode(type, node->next, 0);

		// Is the data type allowed?
		if( type.GetSizeOnStackDWords() == 0 || type.isReference && type.GetSizeInMemoryBytes() == 0 )
		{
			asCString str;
			str.Format(TXT_PARAMETER_CANT_BE_s, (const char *)type.Format());
			Error(str, node);
		}

		// If the parameter has a name then declare it as variable
		node = node->next->next;
		if( node && node->nodeType == snIdentifier )
		{
			GETSTRING(name, &script->code[node->tokenPos], node->tokenLength);

			if( vs.DeclareVariable(name, type, stackPos) < 0 )
				Error(TXT_PARAMETER_ALREADY_DECLARED, node);

			node = node->next;
		}
		else
			vs.DeclareVariable("", type, stackPos);

		// Move to next parameter
		stackPos -= type.GetSizeOnStackDWords();
	}

	int n;
	for( n = vs.variables.GetLength() - 1; n >= 0; n-- )
	{
		variables->DeclareVariable(vs.variables[n]->name, vs.variables[n]->type, vs.variables[n]->stackOffset);
	}

	// Is the return type allowed?
	if( (returnType.GetSizeOnStackDWords() == 0 && returnType != asCDataType(ttVoid, false, false) )   || returnType.isReference && returnType.GetSizeInMemoryBytes() == 0 )
	{
		asCString str;
		str.Format(TXT_RETURN_CANT_BE_s, (const char *)returnType.Format());
		Error(str, node);
	}

	variables->DeclareVariable("return", returnType, stackPos);

	//--------------------------------------------
	// Compile the statement block
	bool hasReturn;
	asCByteCode bc;
	LineInstr(&bc, func->lastChild->tokenPos);
	CompileStatementBlock(func->lastChild, false, &hasReturn, &bc);
	LineInstr(&bc, func->lastChild->tokenPos + func->lastChild->tokenLength);

	// Make sure there is a return in all paths (if not return type is void)
	if( returnType != asCDataType(ttVoid, false, false) )
	{
		if( hasReturn == false )
			Error(TXT_NOT_ALL_PATHS_RETURN, func->lastChild);
	}

	//------------------------------------------------
	// Concatenate the bytecode
	// Count total variable size
	int varSize = GetVariableOffset(variableAllocations.GetLength()) - 1;
	byteCode.Push(varSize);

	// Add the code for the statement block
	byteCode.AddCode(&bc);

	// Deallocate all local variables
	for( n = variables->variables.GetLength() - 1; n >= 0; n-- )
	{
		sVariable *v = variables->variables[n];
		if( v->stackOffset > 0 )
		{
			// Call variables destructors
			if( v->name != "return" && v->name != "return address" )
				CompileDestructor(v->type, v->stackOffset, &byteCode);

			DeallocateVariable(v->stackOffset);
		}
	}
	
	// This is the label that return statements jump to 
	// in order to exit the function
	byteCode.Label(0);

	// Call destructors for function parameters
	for( n = variables->variables.GetLength() - 1; n >= 0; n-- )
	{
		sVariable *v = variables->variables[n];
		if( v->stackOffset <= 0 )
		{
			// Call variable destructors here, for variables not yet destroyed
			if( v->name != "return" && v->name != "return address" )
				CompileDestructor(v->type, v->stackOffset, &byteCode);
		}

		// Do not deallocate parameters
	}

	// If there are compile errors, there is no reason to build the final code
	if( hasCompileErrors ) return -1;

	// At this point there should be no variables allocated
	assert(variableAllocations.GetLength() == freeVariables.GetLength());

	// Remove the variable scope
	RemoveVariableScope();

	byteCode.Pop(varSize);
	
	byteCode.Ret(-stackPos);

	// Finalize the bytecode
	byteCode.Finalize();

	// Compile the list of object variables for the exception handler
	for( n = 0; n < variableAllocations.GetLength(); n++ )
	{
		if( variableAllocations[n].IsObject() && !variableAllocations[n].isReference )
		{
			objVariableTypes.PushLast(variableAllocations[n].objectType);
			objVariablePos.PushLast(GetVariableOffset(n));
		}
	}

	if( hasCompileErrors ) return -1;

	return 0;
}

void asCCompiler::DetermineDefaultArrayArgs(asCDataType &type, asDWORD *subType, asDWORD *size_arrayType)
{
	// Determine how many dimensions of the default array type there is
	// Stop counting if an overloaded array type is found
	asCDataType sub = type;
	int arrayDimensions = 0;
	while( sub.IsDefaultArrayType(engine) )
	{
		arrayDimensions++;
		sub = sub.GetSubType(engine);
	}

	// Determine the information about the default array types
	int at = type.arrayType;
	at &= ~((~0) << (arrayDimensions*2));

	*subType = engine->GetObjectTypeIndex(sub.objectType);
	*size_arrayType = sub.GetSizeInMemoryBytes() | (at << 24);
}

void asCCompiler::DefaultConstructor(asCByteCode *bc, asCDataType &type)
{
	assert(!type.isExplicitHandle);

	int func = 0;
	asSTypeBehaviour *beh = engine->GetBehaviour(&type);
	if( beh ) func = beh->construct;

	if( type.IsDefaultArrayType(engine) )
	{
		asDWORD subType;
		asDWORD size_arrayType;
		DetermineDefaultArrayArgs(type, &subType, &size_arrayType);

		// The default array object needs some extra data
		bc->InstrDWORD(BC_SET4, subType); // Behaviour index
		bc->InstrDWORD(BC_SET4, size_arrayType);  // Element size and arraydimensions

		bc->Alloc(BC_ALLOC, engine->GetObjectTypeIndex(type.objectType), func, 3);
	}
	else
	{
		bc->Alloc(BC_ALLOC, engine->GetObjectTypeIndex(type.objectType), func, 1);
	}
}

void asCCompiler::CompileConstructor(asCDataType &type, int offset, asCByteCode *bc)
{
	// Call constructor for the data type
	if( type.IsObject() && !type.isExplicitHandle )
	{
		bc->InstrINT(BC_VAR, offset);
		bc->InstrWORD(BC_GETREF, 0);
		DefaultConstructor(bc, type);
	}
}

void asCCompiler::CompileDestructor(asCDataType &type, int offset, asCByteCode *bc)
{
	if( !type.isReference )
	{
		// Call destructor for the data type
		if( type.IsObject() )
		{
			int objTypeIdx = engine->GetObjectTypeIndex(type.objectType);

			// Free the memory
			bc->InstrINT(BC_VAR, offset);
			bc->InstrWORD(BC_GETREF, 0);
			bc->InstrINT(BC_FREE, objTypeIdx);
		}
	}
}

void asCCompiler::LineInstr(asCByteCode *bc, int pos)
{
	int r, c;
	script->ConvertPosToRowCol(pos, &r, &c);
	bc->Line(r, c);
}

void asCCompiler::CompileStatementBlock(asCScriptNode *block, bool ownVariableScope, bool *hasReturn, asCByteCode *bc)
{
	*hasReturn = false;
	bool isFinished = false;
	bool hasWarned = false;

	if( ownVariableScope )
		AddVariableScope();

	asCScriptNode *node = block->firstChild;
	while( node )
	{
		if( !hasWarned && (*hasReturn || isFinished) )
		{
			hasWarned = true;
			Warning(TXT_UNREACHABLE_CODE, node);
		}

		if( node->nodeType == snBreak || node->nodeType == snContinue )
			isFinished = true;

		asCByteCode statement;
		if( node->nodeType == snDeclaration )
			CompileDeclaration(node, &statement);
		else
			CompileStatement(node, hasReturn, &statement);

		LineInstr(bc, node->tokenPos);
		bc->AddCode(&statement);

		if( !hasCompileErrors )
			assert( tempVariables.GetLength() == 0 );
		
		node = node->next;
	}

	if( ownVariableScope )
	{

		// Deallocate variables in this block, in reverse order
		for( int n = variables->variables.GetLength() - 1; n >= 0; n-- )
		{
			sVariable *v = variables->variables[n];

			// Call variable destructors here, for variables not yet destroyed
			// If the block is terminated with a break, continue, or 
			// return the variables are already destroyed
			if( !isFinished && !*hasReturn )
				CompileDestructor(v->type, v->stackOffset, bc);

			// Don't deallocate function parameters
			if( v->stackOffset > 0 )
				DeallocateVariable(v->stackOffset);
		}

		RemoveVariableScope();
	}
}

int asCCompiler::CompileGlobalVariable(asCBuilder *builder, asCScriptCode *script, asCScriptNode *node, sGlobalVariableDescription *gvar)
{
	Reset(builder, script);
	globalExpression = true;

	// Add a variable scope (even though variables can't be declared)
	AddVariableScope();

	asSExprContext ctx;

	// Compile the expression
	if( node && node->nodeType == snArgList )
	{
		// Make sure that it is a registered type, and that it isn't a pointer
		if( gvar->datatype.objectType == 0 || gvar->datatype.isExplicitHandle )
		{
			Error(TXT_MUST_BE_OBJECT, node);
		}
		else
		{
			// Compile the arguments
			int argCount;
			
			asCArray<asSExprContext *> args;
			CompileArgumentList(node, args, &gvar->datatype);
			
			asCArray<asCTypeInfo> argTypes;
			argCount = args.GetLength();
			
			int n;
			for( n = 0; n < argCount; n++ )
				argTypes.PushLast(args[n]->type);

			// Find all constructors
			asCArray<int> funcs;
			asSTypeBehaviour *beh = engine->GetBehaviour(&gvar->datatype);
			if( beh )
				funcs = beh->constructors;

			asCString str = gvar->datatype.Format();
			MatchFunctions(funcs, argTypes, node, str);
			
			if( funcs.GetLength() == 1 )
			{
				// TODO: This reference is open while evaluating the arguments. We must fix this
				ctx.bc.InstrINT(BC_PGA, gvar->index);

				PrepareFunctionCall(funcs[0], node, &ctx.bc, args);
				MoveArgsToStack(funcs[0], &ctx.bc, args, false);

				for( n = 0; n < argCount; n++ )
					argTypes[n] = args[n]->type;

				PerformFunctionCall(funcs[0], &ctx, true, &argTypes, node);
			}

			// Cleanup
			for( n = 0; n < argCount; n++ )
				if( args[n] ) delete args[n];
		}
	}
	else
	{
		// Call constructor for all data types
		if( gvar->datatype.IsObject() && !gvar->datatype.isExplicitHandle )
		{
			ctx.bc.InstrINT(BC_PGA, gvar->index);
			DefaultConstructor(&ctx.bc, gvar->datatype);
		}

		if( node )
		{
			asSExprContext expr;
			CompileAssignment(node, &expr);

			asCTypeInfo ltype;
			ltype.Set(gvar->datatype);
			ltype.dataType.isReference = true;
			if( !ltype.dataType.isExplicitHandle )
				ltype.dataType.isReadOnly = false;
			// TODO: use a different flag to define read-only handles
			ltype.stackOffset = -1;

			// If left expression resolves into a registered type
			// check if the assignment operator is overloaded, and check 
			// the type of the right hand expression. If none is found
			// the default action is a direct copy if it is the same type
			// and a simple assignment.
			asSTypeBehaviour *beh = 0;
			if( !ltype.dataType.isExplicitHandle ) 
				beh = engine->GetBehaviour(&ltype.dataType);
			bool assigned = false;
			if( beh )
			{
				// Find the matching overloaded operators
				int op = ttAssignment;
				asCArray<int> ops;
				int n;
				for( n = 0; n < beh->operators.GetLength(); n += 2 )
				{
					if( op == beh->operators[n] )
						ops.PushLast(beh->operators[n+1]);
				}

				asCArray<int> match;
				MatchArgument(ops, match, &expr.type, 0);

				if( match.GetLength() > 0 )
					assigned = true;

				if( match.GetLength() == 1 )
				{
					// If it is an array, both sides must have the same subtype
					if( ltype.dataType.arrayType > 0 )
						if( !ltype.dataType.IsEqualExceptRefAndConst(expr.type.dataType) )
							Error(TXT_BOTH_MUST_BE_SAME, node);

					asCScriptFunction *descr = engine->systemFunctions[-match[0] - 1];

					// Add code for arguments
					MergeExprContexts(&ctx, &expr);

					PrepareArgument(&descr->parameterTypes[0], &expr, node, true, bool(descr->inOutFlags[0] & 1));
					MergeExprContexts(&ctx, &expr);

					if( descr->parameterTypes[0].isReference )
					{
						if( descr->parameterTypes[0].IsObject() && !descr->parameterTypes[0].isExplicitHandle )
							ctx.bc.InstrWORD(BC_GETOBJREF, 0);
						else
							ctx.bc.InstrWORD(BC_GETREF, 0);
					}
					else if( descr->parameterTypes[0].IsObject() )
					{
						ctx.bc.InstrWORD(BC_GETOBJ, 0);

						// The temporary variable must not be freed as it will no longer hold an object
						DeallocateVariable(expr.type.stackOffset);
						expr.type.isTemporary = false;
					}

					// Add the code for the object
					ctx.bc.InstrINT(BC_PGA, gvar->index);
					ctx.bc.Instr(BC_RD4);

					asCArray<asCTypeInfo> argTypes;
					argTypes.PushLast(expr.type);
					PerformFunctionCall(match[0], &ctx, false, &argTypes);

					ctx.bc.Pop(ctx.type.dataType.GetSizeOnStackDWords());
				}
				else if( match.GetLength() > 1 )
				{
					Error(TXT_MORE_THAN_ONE_MATCHING_OP, node);
				}
			}

			if( !assigned )
			{
				PrepareForAssignment(&ltype.dataType, &expr.type, &expr.bc, node);

				// Add expression code to bytecode
				MergeExprContexts(&ctx, &expr);

				// Add byte code for storing value of expression in variable
				ctx.bc.InstrINT(BC_PGA, gvar->index);

				PerformAssignment(&ltype, &ctx.bc, node->prev);

				// Release temporary variables used by expression
				ReleaseTemporaryVariable(expr.type, &ctx.bc);

				ctx.bc.Pop(expr.type.dataType.GetSizeOnStackDWords());
			}

		}
	}

	// Concatenate the bytecode
	int varSize = GetVariableOffset(variableAllocations.GetLength()) - 1;
	byteCode.Push(varSize);
	byteCode.AddCode(&ctx.bc);
	
	// Deallocate variables in this block, in reverse order
	for( int n = variables->variables.GetLength() - 1; n >= 0; --n )
	{
		sVariable *v = variables->variables[n];

		// Call variable destructors here, for variables not yet destroyed
		CompileDestructor(v->type, v->stackOffset, &byteCode);

		DeallocateVariable(v->stackOffset);
	}

	if( hasCompileErrors ) return -1;

	// At this point there should be no variables allocated
	assert(variableAllocations.GetLength() == freeVariables.GetLength());

	// Remove the variable scope again
	RemoveVariableScope();

	byteCode.Pop(varSize);

	return 0;
}

void asCCompiler::PrepareArgument(asCDataType *paramType, asSExprContext *ctx, asCScriptNode *node, bool isFunction, bool isInRef)
{
	asCDataType dt = *paramType;

	// Need to protect arguments by reference 
	if( isFunction && dt.isReference )
	{


		// Allocate a temporary variable of the same type as the argument
		dt.isReference = false;
		dt.isReadOnly = false;

		int offset;
		if( isInRef )
		{
			IsVariableInitialized(&ctx->type, node);

			// If the argument already is a temporary 
			// variable we don't need to allocate another
			if( !ctx->type.isTemporary )
			{
				// Make sure the variable is not used in the expression
				asCArray<int> vars;
				ctx->bc.GetVarsUsed(vars);
				offset = AllocateVariableNotIn(dt, true, vars);

				// Allocate and construct the temporary object
				asCByteCode tmpBC;
				CompileConstructor(dt, offset, &tmpBC);

				// Insert the code before the expression code
				tmpBC.AddCode(&ctx->bc);
				ctx->bc.AddCode(&tmpBC);

				// Assign the evaluated expression to the temporary variable
				PrepareForAssignment(&dt, &ctx->type, &ctx->bc, node);

				dt.isReference = true;
				asCTypeInfo type;
				type.Set(dt);
				type.isTemporary = true;
				type.stackOffset = offset;

				ctx->bc.InstrINT(BC_VAR, offset);
				ctx->bc.InstrWORD(BC_GETREF, 0);

				PerformAssignment(&type, &ctx->bc, node);

				ctx->bc.Pop(ctx->type.dataType.GetSizeOnStackDWords());

				ReleaseTemporaryVariable(ctx->type, &ctx->bc);

				ctx->type = type;

				ctx->bc.InstrINT(BC_VAR, offset);
				ctx->bc.InstrWORD(BC_GETREF, 0);
				if( dt.IsObject() && !dt.isExplicitHandle ) 
					ctx->bc.Instr(BC_RD4);

				if( paramType->isReadOnly )
					ctx->type.dataType.isReadOnly = true;
			}
		}
		else
		{
			// Make sure the variable is not used in the expression
			asCArray<int> vars;
			ctx->bc.GetVarsUsed(vars);
			offset = AllocateVariableNotIn(dt, true, vars);

			// Allocate and construct the temporary object
			asCByteCode tmpBC;
			CompileConstructor(dt, offset, &tmpBC);

			// Insert the code before the expression code
			tmpBC.AddCode(&ctx->bc);
			ctx->bc.AddCode(&tmpBC);


			dt.isReference = (!dt.IsObject() || dt.isExplicitHandle);
			asCTypeInfo type;
			type.Set(dt);
			type.isTemporary = true;
			type.stackOffset = offset;

			// Must release any temporary variables even though they won't be used
			ReleaseTemporaryVariable(ctx->type, &ctx->bc);

			ctx->type = type;

			ctx->bc.InstrINT(BC_VAR, offset);
			ctx->bc.InstrWORD(BC_GETREF, 0);
			if( dt.IsObject() && !dt.isExplicitHandle ) 
				ctx->bc.Instr(BC_RD4);

		}


		// After the function returns the temporary variable will   
		// be assigned to the expression, if it is a valid lvalue
	}
	else
	{
		IsVariableInitialized(&ctx->type, node);

		if( dt.IsObject() )
		{
			if( !dt.isReference )
			{
				// Objects passed by value must be placed in temporary variables
				// so that they are guaranteed to not be referenced anywhere else
				PrepareTemporaryObject(node, ctx);

				// The ímplicit conversion shouldn't convert the object to 
				// non-reference yet. It will be dereferenced just before the call.
				// Otherwise the object might be missed by the exception handler.
				dt.isReference = true;
			}
			else
			{	
				// An object passed by reference should place the pointer to 
				// the object on the stack.
				dt.isReference = false;
			}
		}

		// Implicitly convert primitives to the parameter type
		ImplicitConversion(&ctx->bc, dt, &ctx->type, node, false);
	}

	// Don't put any pointer on the stack yet
	if( paramType->isReference || paramType->IsObject() )
	{
		ctx->bc.Pop(1);
		ctx->bc.InstrINT(BC_VAR, ctx->type.stackOffset);

		ProcessDeferredOutParams(ctx);
	}
}

void asCCompiler::PrepareFunctionCall(int funcID, asCScriptNode *argListNode, asCByteCode *bc, asCArray<asSExprContext *> &args)
{
	// When a match has been found, compile the final byte code using correct parameter types
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	// Add code for arguments
	asCScriptNode *arg = argListNode->lastChild;
	int n;
	for( n = args.GetLength()-1; n >= 0; n-- )
	{
		asSExprContext e;

		// Reference parameters whose value won't be used don't evaluate the expression
		if( !descr->parameterTypes[n].isReference || (descr->inOutFlags[n] & 1) )
		{
			MergeExprContexts(&e, args[n]);
		}
		else
		{
			// Discard the deferred output parameters
			for( int d = 0; d < args[n]->deferredOutParams.GetLength(); d++ )
				DeallocateVariable(args[n]->deferredOutParams[d].argType.stackOffset);
			args[n]->deferredOutParams.SetLength(0);
		}

		e.type = args[n]->type;
		PrepareArgument(&descr->parameterTypes[n], &e, arg, true, bool(descr->inOutFlags[n] & 1));
		args[n]->type = e.type;
		bc->AddCode(&e.bc);

		if( arg )
			arg = arg->prev;
	}
}

void asCCompiler::MoveArgsToStack(int funcID, asCByteCode *bc, asCArray<asSExprContext *> &args, bool addOneToOffset)
{
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	int offset = 0;
	if( addOneToOffset )
		offset += 1;

	// Move the objects that are sent by value to the stack just before the call
	for( int n = 0; n < descr->parameterTypes.GetLength(); n++ )
	{
		if( descr->parameterTypes[n].isReference )
		{
			if( descr->parameterTypes[n].IsObject() && !descr->parameterTypes[n].isExplicitHandle )
				bc->InstrWORD(BC_GETOBJREF, offset);
			else
				bc->InstrWORD(BC_GETREF, offset);
		}
		else if( descr->parameterTypes[n].IsObject() )
		{
			bc->InstrWORD(BC_GETOBJ, offset);

			// The temporary variable must not be freed as it will no longer hold an object
			DeallocateVariable(args[n]->type.stackOffset);
			args[n]->type.isTemporary = false;
		}

		offset += descr->parameterTypes[n].GetSizeOnStackDWords();
	}
}

void asCCompiler::CompileArgumentList(asCScriptNode *node, asCArray<asSExprContext*> &args, asCDataType *type)
{
	assert(node->nodeType == snArgList);

	// Count arguments
	asCScriptNode *arg = node->firstChild;
	int argCount = 0;
	while( arg )
	{
		argCount++;
		arg = arg->next;
	}

	// Default array takes two extra parameters
	if( type && type->IsDefaultArrayType(engine) )
	{
		argCount += 2;	
	}

	// Prepare the arrays
	args.SetLength(argCount);
	int n;
	for( n = 0; n < argCount; n++ )
		args[n] = 0;

	n = argCount-1;
	if( type && type->IsDefaultArrayType(engine) )
	{
		asDWORD subType;
		asDWORD size_arrayType;
		DetermineDefaultArrayArgs(*type, &subType, &size_arrayType);

		// Pass base types behaviour index and size, also pass the array dimension
		args[n] = new asSExprContext;
		args[n]->bc.InstrDWORD(BC_SET4, subType); // Behaviour index
		args[n]->type.Set(asCDataType(ttInt, false, false));
		n--;
		args[n] = new asSExprContext;
		args[n]->bc.InstrDWORD(BC_SET4, size_arrayType); // Element size
		args[n]->type.Set(asCDataType(ttInt, false, false));
		n--;
	}

	// Compile the arguments in reverse order (as they will be pushed on the stack)
	arg = node->lastChild;
	while( arg )
	{
		asSExprContext expr;
		CompileAssignment(arg, &expr);

		args[n] = new asSExprContext;
		MergeExprContexts(args[n], &expr);
		args[n]->type = expr.type;

		n--;
		arg = arg->prev;
	}
}

void asCCompiler::MatchFunctions(asCArray<int> &funcs, asCArray<asCTypeInfo> &argTypes, asCScriptNode *node, const char *name, bool isConstMethod)
{
	int n;
	if( funcs.GetLength() > 0 )
	{
		// Check the number of parameters in the found functions
		for( n = 0; n < funcs.GetLength(); ++n )
		{
			asCScriptFunction *desc = builder->GetFunctionDescription(funcs[n]);

			if( desc->parameterTypes.GetLength() != argTypes.GetLength() )
			{
				// remove it from the list
				if( n == funcs.GetLength()-1 )
					funcs.PopLast();
				else
					funcs[n] = funcs.PopLast();
				n--;
			}
		}

		// Match functions with the parameters, and discard those that do not match
		asCArray<int> matchingFuncs = funcs;

		for( n = 0; n < argTypes.GetLength(); ++n )
		{
			asCArray<int> tempFuncs;
			MatchArgument(funcs, tempFuncs, &argTypes[n], n);

			// Intersect the found functions with the list of matching functions
			for( int f = 0; f < matchingFuncs.GetLength(); f++ )
			{
				int c;
				for( c = 0; c < tempFuncs.GetLength(); c++ )
				{
					if( matchingFuncs[f] == tempFuncs[c] )
						break;
				}

				// Was the function a match?
				if( c == tempFuncs.GetLength() )
				{
					// No, remove it from the list
					if( f == matchingFuncs.GetLength()-1 )
						matchingFuncs.PopLast();
					else
						matchingFuncs[f] = matchingFuncs.PopLast();
					f--;
				}
			}
		}

		funcs = matchingFuncs;
	}

	if( !isConstMethod )
		FilterConst(funcs);

	if( funcs.GetLength() != 1 )
	{
		// Build a readable string of the function with parameter types
		asCString str = name;
		str += "(";
		if( argTypes.GetLength() )
			str += argTypes[0].dataType.Format();
		for( n = 1; n < argTypes.GetLength(); n++ )
			str += ", " + argTypes[n].dataType.Format();
		str += ")";

		if( isConstMethod )
			str += " const";

		if( funcs.GetLength() == 0 )
			str.Format(TXT_NO_MATCHING_SIGNATURES_TO_s, (const char *)str);
		else
			str.Format(TXT_MULTIPLE_MATCHING_SIGNATURES_TO_s, (const char *)str);

		Error(str, node);
	}
}

void asCCompiler::CompileDeclaration(asCScriptNode *decl, asCByteCode *bc)
{
	// Get the data type
	asCDataType type = builder->CreateDataTypeFromNode(decl->firstChild, script);

	// Declare all variables in this declaration
	asCScriptNode *node = decl->firstChild->next;
	while( node )
	{
		// Is the type allowed?
		if( type.GetSizeOnStackDWords() == 0 || 
			(type.IsObject() && !type.isExplicitHandle && type.GetSizeInMemoryBytes() == 0) )
		{
			asCString str;
			// TODO: Change to "'type' cannot be declared as variable"
			str.Format(TXT_DATA_TYPE_CANT_BE_s, (const char *)type.Format());
			Error(str, node);
		}

		// Get the name of the identifier
		GETSTRING(name, &script->code[node->tokenPos], node->tokenLength);

		// Verify that the name isn't used by a dynamic data type
		if( engine->GetObjectType(name) != 0 )
		{
			asCString str;
			str.Format(TXT_ILLEGAL_VARIABLE_NAME_s, (const char *)name);
			Error(str, node);
		}

		int offset = AllocateVariable(type, false);
		if( variables->DeclareVariable(name, type, offset) < 0 )
		{
			asCString str;
			str.Format(TXT_s_ALREADY_DECLARED, (const char *)name);
			Error(str, node);
		}

		node = node->next;
		if( node && node->nodeType == snArgList )
		{
			// Make sure that it is a registered type, and that is isn't a pointer
			if( type.objectType == 0 || type.isExplicitHandle )
			{
				Error(TXT_MUST_BE_OBJECT, node);
			}
			else
			{
				// Compile the arguments
				asCArray<asCTypeInfo> argTypes;
				asCArray<asSExprContext *> args;
				int argCount;
				
				CompileArgumentList(node, args, &type);
				argCount = args.GetLength();

				int n;
				for( n = 0; n < argCount; n++ )
					argTypes.PushLast(args[n]->type);

				// Find all constructors
				asCArray<int> funcs;
				asSTypeBehaviour *beh = engine->GetBehaviour(&type);
				if( beh )
					funcs = beh->constructors;

				asCString str = type.Format();
				MatchFunctions(funcs, argTypes, node, str);

				if( funcs.GetLength() == 1 )
				{
					asSExprContext ctx;

					sVariable *v = variables->GetVariable(name);
					ctx.bc.InstrINT(BC_VAR, v->stackOffset);

					PrepareFunctionCall(funcs[0], node, &ctx.bc, args);
					MoveArgsToStack(funcs[0], &ctx.bc, args, false);

					int offset = 0;
					for( n = 0; n < argCount; n++ )
					{
						argTypes[n] = args[n]->type;
						offset += args[n]->type.dataType.GetSizeOnStackDWords();
					}

					ctx.bc.InstrWORD(BC_GETREF, offset);

					PerformFunctionCall(funcs[0], &ctx, true, &argTypes, node);

					bc->AddCode(&ctx.bc);
				}

				// Cleanup
				for( n = 0; n < argCount; n++ )
					if( args[n] ) delete args[n];
			}

			node = node->next;
		}
		else
		{
			asSExprContext ctx;

			// Call the default constructor here
			CompileConstructor(type, offset, &ctx.bc);
			
			// Is the variable initialized?
			if( node && node->nodeType == snAssignment )
			{
				// TODO: We can use a copy constructor here

				asCTypeInfo ltype;
				ltype.Set(type);
				ltype.dataType.isReference = true;
				// Allow initialization of constant variables
				if( !ltype.dataType.isExplicitHandle )
					ltype.dataType.isReadOnly = false;
				// TODO: explicit handles have another flag for defining read-only handles

				// Compile the expression
				asSExprContext expr;
				CompileAssignment(node, &expr);

				// If left expression resolves into a registered type
				// check if the assignment operator is overloaded, and check 
				// the type of the right hand expression. If none is found
				// the default action is a direct copy if it is the same type
				// and a simple assignment.
				asSTypeBehaviour *beh = 0;
				if( !ltype.dataType.isExplicitHandle ) 
					beh = engine->GetBehaviour(&ltype.dataType);
				bool assigned = false;
				if( beh )
				{
					// Find the matching overloaded operators
					int op = ttAssignment;
					asCArray<int> ops;
					int n;
					for( n = 0; n < beh->operators.GetLength(); n += 2 )
					{
						if( op == beh->operators[n] )
							ops.PushLast(beh->operators[n+1]);
					}

					asCArray<int> match;
					MatchArgument(ops, match, &expr.type, 0);

					if( match.GetLength() > 0 )
						assigned = true;

					if( match.GetLength() == 1 )
					{
						// If it is an array, both sides must have the same subtype
						if( ltype.dataType.arrayType > 0 )
							if( !ltype.dataType.IsEqualExceptRefAndConst(expr.type.dataType) )
								Error(TXT_BOTH_MUST_BE_SAME, node);

						asCScriptFunction *descr = engine->systemFunctions[-match[0] - 1];

						// Add code for arguments
						MergeExprContexts(&ctx, &expr);

						PrepareArgument(&descr->parameterTypes[0], &expr, node, true, bool(descr->inOutFlags[0] & 1));
						MergeExprContexts(&ctx, &expr);

						if( descr->parameterTypes[0].isReference )
						{
							if( descr->parameterTypes[0].IsObject() && !descr->parameterTypes[0].isExplicitHandle )
								ctx.bc.InstrWORD(BC_GETOBJREF, 0);
							else
								ctx.bc.InstrWORD(BC_GETREF, 0);
						}
						else if( descr->parameterTypes[0].IsObject() )
						{
							ctx.bc.InstrWORD(BC_GETOBJ, 0);

							// The temporary variable must not be freed as it will no longer hold an object
							DeallocateVariable(expr.type.stackOffset);
							expr.type.isTemporary = false;
						}

						// Add the code for the object
						sVariable *v = variables->GetVariable(name);
						ltype.stackOffset = (short)v->stackOffset;
						ctx.bc.InstrINT(BC_VAR, v->stackOffset);
						ctx.bc.InstrWORD(BC_GETREF, 0);
						ctx.bc.Instr(BC_RD4);

						asCArray<asCTypeInfo> argTypes;
						argTypes.PushLast(expr.type);
						PerformFunctionCall(match[0], &ctx, false, &argTypes);

						ctx.bc.Pop(ctx.type.dataType.GetSizeOnStackDWords());
					}
					else if( match.GetLength() > 1 )
					{
						Error(TXT_MORE_THAN_ONE_MATCHING_OP, node);
					}
				}

				if( !assigned )
				{
					PrepareForAssignment(&ltype.dataType, &expr.type, &expr.bc, node);

					// Add expression code to bytecode
					MergeExprContexts(&ctx, &expr);

					// Add byte code for storing value of expression in variable
					sVariable *v = variables->GetVariable(name);
					ctx.bc.InstrINT(BC_VAR, v->stackOffset);
					ctx.bc.InstrWORD(BC_GETREF, 0);
					ltype.stackOffset = (short)v->stackOffset;

					PerformAssignment(&ltype, &ctx.bc, node->prev);

					// Release temporary variables used by expression
					ReleaseTemporaryVariable(expr.type, &ctx.bc);

					ctx.bc.Pop(expr.type.dataType.GetSizeOnStackDWords());
				}

				node = node->next;
			}

			bc->AddCode(&ctx.bc);

			// TODO: Can't this leave deferred output params without being compiled?
		}
	}
}

void asCCompiler::CompileStatement(asCScriptNode *statement, bool *hasReturn, asCByteCode *bc)
{
	*hasReturn = false;

	if( statement->nodeType == snStatementBlock )
		CompileStatementBlock(statement, true, hasReturn, bc);
	else if( statement->nodeType == snIf )
		CompileIfStatement(statement, hasReturn, bc);
	else if( statement->nodeType == snFor )
		CompileForStatement(statement, bc);
	else if( statement->nodeType == snWhile )
		CompileWhileStatement(statement, bc);
	else if( statement->nodeType == snDoWhile )
		CompileDoWhileStatement(statement, bc);
	else if( statement->nodeType == snExpressionStatement )
		CompileExpressionStatement(statement, bc);
	else if( statement->nodeType == snBreak )
		CompileBreakStatement(statement, bc);
	else if( statement->nodeType == snContinue )
		CompileContinueStatement(statement, bc);
	else if( statement->nodeType == snSwitch )
		CompileSwitchStatement(statement, hasReturn, bc);
	else if( statement->nodeType == snReturn )
	{
		CompileReturnStatement(statement, bc);
		*hasReturn = true;
	}
}

void asCCompiler::CompileSwitchStatement(asCScriptNode *snode, bool *hasReturn, asCByteCode *bc)
{
	// Reserve label for break statements
	int breakLabel = nextLabel++;
	breakLabels.PushLast(breakLabel);

	// Add a variable scope that will be used by CompileBreak
	// to know where to stop deallocating variables
	AddVariableScope(true, false);

	//---------------------------
	// Compile the switch expression
	//-------------------------------

	// Compile the switch expression
	asSExprContext expr;
	CompileAssignment(snode->firstChild, &expr);
	PrepareOperand(&expr, snode->firstChild);

	// Verify that the expression is a primitive type
	if( !expr.type.dataType.IsIntegerType() && !expr.type.dataType.IsUnsignedType() )
	{
		Error(TXT_SWITCH_MUST_BE_INTEGRAL, snode->firstChild);
		return;
	}

	// Store the value in a temporary variable so that it can be used several times
	asCTypeInfo type;
	type.Set(expr.type.dataType);
	// Make sure the data type is 32 bit
	if( type.dataType.IsIntegerType() ) type.dataType.tokenType = ttInt;
	if( type.dataType.IsUnsignedType() ) type.dataType.tokenType = ttUInt;
	type.dataType.isReference = false;
	type.dataType.isReadOnly = false;
	int offset = AllocateVariable(type.dataType, true);
	type.isTemporary = true;
	type.stackOffset = offset;
	
	PrepareForAssignment(&type.dataType, &expr.type, &expr.bc, snode->firstChild);

	expr.bc.InstrINT(BC_VAR, offset);
	expr.bc.InstrWORD(BC_GETREF, 0);
	type.dataType.isReference = true;

	PerformAssignment(&type, &expr.bc, snode->firstChild);

	// Release temporary variable used by expression
	ReleaseTemporaryVariable(expr.type, &expr.bc);

	expr.bc.Pop(expr.type.dataType.GetSizeOnStackDWords());

	//-------------------------------
	// Determine case values and labels
	//--------------------------------

	// Remember the first label so that we can later pass the  
	// correct label to each CompileCase()
	int firstCaseLabel = nextLabel;
	int defaultLabel = 0;

	asCArray<int> caseValues;
	asCArray<int> caseLabels;

	// Compile all case comparisons and make them jump to the right label
	asCScriptNode *cnode = snode->firstChild->next;
	while( cnode )
	{
		// Each case should have a constant expression
		if( cnode->firstChild && cnode->firstChild->nodeType == snExpression )
		{
			// Compile expression
			asSExprContext c;
			CompileExpression(cnode->firstChild, &c);

			// Verify that the result is a constant
			if( !c.type.isConstant )
				Error(TXT_SWITCH_CASE_MUST_BE_CONSTANT, cnode->firstChild);

			// Verify that the result is an integral number
			if( !c.type.dataType.IsIntegerType() && !c.type.dataType.IsUnsignedType() )
				Error(TXT_SWITCH_MUST_BE_INTEGRAL, cnode->firstChild);

			// Store constant for later use
			caseValues.PushLast(c.type.intValue);

			// Reserve label for this case
			caseLabels.PushLast(nextLabel++);
		}
		else
		{
			// Is default the last case?
			if( cnode->next )
			{
				Error(TXT_DEFAULT_MUST_BE_LAST, cnode);
				break;
			}

			// Reserve label for this case
			defaultLabel = nextLabel++;
		}

		cnode = cnode->next;
	}

	if( defaultLabel == 0 )
		defaultLabel = breakLabel;

	//---------------------------------
    // Output the optimized case comparisons  
	// with jumps to the case code
	//------------------------------------

	// Sort the case values by increasing value. Do the sort together with the labels
	// A simple bubble sort is sufficient since we don't expect a huge number of values
	for( int fwd = 1; fwd < caseValues.GetLength(); fwd++ )
	{
		for( int bck = fwd - 1; bck >= 0; bck-- )
		{
			int bckp = bck + 1;
			if( caseValues[bck] > caseValues[bckp] )
			{
				// Swap the values in both arrays
				int swap = caseValues[bckp];
				caseValues[bckp] = caseValues[bck];
				caseValues[bck] = swap;

				swap = caseLabels[bckp];
				caseLabels[bckp] = caseLabels[bck];
				caseLabels[bck] = swap;
			}
			else 
				break;
		}
	}

	// Find ranges of consecutive numbers
	asCArray<int> ranges;
	ranges.PushLast(0);
	int n;
	for( n = 1; n < caseValues.GetLength(); ++n )
	{
		// We can join numbers that are less than 5 numbers  
		// apart since the output code will still be smaller
		if( caseValues[n] > caseValues[n-1] + 5 )
			ranges.PushLast(n);
	}

	// If the value is larger than the largest case value, jump to default
	expr.bc.InstrINT(BC_VAR, offset);
	expr.bc.InstrWORD(BC_GETREF, 0);
	expr.bc.Instr(BC_RD4);
	expr.bc.InstrINT(BC_SET4, caseValues[caseValues.GetLength()-1]);
	expr.bc.Instr(BC_CMPi);
	expr.bc.Instr(BC_TP);
	expr.bc.InstrINT(BC_JNZ, defaultLabel);

	// TODO: We could possible optimize this even more by doing a 
	// binary search instead of a linear search through the ranges

	// For each range
	int range;
	for( range = 0; range < ranges.GetLength(); range++ )
	{
		// Find the largest value in this range
		int maxRange = caseValues[ranges[range]]; 
		int index = ranges[range];
		for( ; (index < caseValues.GetLength()) && (caseValues[index] <= maxRange + 5); index++ )
			maxRange = caseValues[index];
		
		// If there are only 2 numbers then it is better to compare them directly
		if( index - ranges[range] > 2 )
		{
			// If the value is smaller than the smallest case value in the range, jump to default
			expr.bc.InstrINT(BC_VAR, offset);
			expr.bc.InstrWORD(BC_GETREF, 0);
			expr.bc.Instr(BC_RD4);
			expr.bc.InstrINT(BC_SET4, caseValues[ranges[range]]);
			expr.bc.Instr(BC_CMPi);
			expr.bc.Instr(BC_TS);
			expr.bc.InstrINT(BC_JNZ, defaultLabel);

			int nextRangeLabel = nextLabel++;
			// If this is the last range we don't have to make this test
			if( range < ranges.GetLength() - 1 )
			{
				// If the value is larger than the largest case value in the range, jump to the next range
				expr.bc.InstrINT(BC_VAR, offset);
				expr.bc.InstrWORD(BC_GETREF, 0);
				expr.bc.Instr(BC_RD4);
				expr.bc.InstrINT(BC_SET4, maxRange);
				expr.bc.Instr(BC_CMPi);
				expr.bc.Instr(BC_TP);
				expr.bc.InstrINT(BC_JNZ, nextRangeLabel);
			}

			// Jump forward according to the value
			expr.bc.InstrINT(BC_VAR, offset);
			expr.bc.InstrWORD(BC_GETREF, 0);
			expr.bc.Instr(BC_RD4);
			expr.bc.InstrINT(BC_SET4, caseValues[ranges[range]]);
			expr.bc.Instr(BC_SUBi);
			expr.bc.JmpP(maxRange - caseValues[ranges[range]]);

			// Add the list of jumps to the correct labels (any holes, jump to default)
			index = ranges[range];
			for( n = caseValues[index]; n <= maxRange; n++ )
			{
				if( caseValues[index] == n )
					expr.bc.InstrINT(BC_JMP, caseLabels[index++]);
				else
					expr.bc.InstrINT(BC_JMP, defaultLabel);
			}

			expr.bc.Label(nextRangeLabel);
		}
		else
		{
			// Simply make a comparison with each value
			int n;
			for( n = ranges[range]; n < index; ++n )
			{
				expr.bc.InstrINT(BC_VAR, offset);
				expr.bc.InstrWORD(BC_GETREF, 0);
				expr.bc.Instr(BC_RD4);
				expr.bc.InstrINT(BC_SET4, caseValues[n]);
				expr.bc.Instr(BC_CMPi);
				expr.bc.Instr(BC_TZ);
				expr.bc.InstrINT(BC_JNZ, caseLabels[n]);
			}
		}
	}

	// Catch any value that falls trough
	expr.bc.InstrINT(BC_JMP, defaultLabel);

	// Release the temporary variable previously stored
	ReleaseTemporaryVariable(type, &expr.bc);

	//----------------------------------
    // Output case implementations
	//----------------------------------

	// Compile case implementations, each one with the label before it
	cnode = snode->firstChild->next;
	while( cnode )
	{
		// Each case should have a constant expression
		if( cnode->firstChild && cnode->firstChild->nodeType == snExpression )
		{
			expr.bc.Label(firstCaseLabel++);

			CompileCase(cnode->firstChild->next, &expr.bc);
		}
		else
		{
			expr.bc.Label(defaultLabel);

			// Is default the last case?
			if( cnode->next )
			{
				// We've already reported this error
				break;
			}

			CompileCase(cnode->firstChild, &expr.bc);
		}

		cnode = cnode->next;
	}	

	//--------------------------------

	bc->AddCode(&expr.bc);

	// Add break label
	bc->Label(breakLabel);

	breakLabels.PopLast();
	RemoveVariableScope();
}

void asCCompiler::CompileCase(asCScriptNode *node, asCByteCode *bc)
{
	bool isFinished = false;
	bool hasReturn = false;
	while( node )
	{
		if( hasReturn || isFinished )
		{
			Warning(TXT_UNREACHABLE_CODE, node);
			break;
		}

		if( node->nodeType == snBreak || node->nodeType == snContinue )
			isFinished = true;

		asCByteCode statement;
		CompileStatement(node, &hasReturn, &statement);

		LineInstr(bc, node->tokenPos);
		bc->AddCode(&statement);

		if( !hasCompileErrors )
			assert( tempVariables.GetLength() == 0 );
		
		node = node->next;
	}
}

void asCCompiler::CompileIfStatement(asCScriptNode *inode, bool *hasReturn, asCByteCode *bc)
{
	// We will use one label for the if statement
	// and possibly another for the else statement
	int afterLabel = nextLabel++;

	// Compile the expression
	asSExprContext expr;
	CompileAssignment(inode->firstChild, &expr);
	PrepareOperand(&expr, inode->firstChild);
	if( !expr.type.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
		Error(TXT_EXPR_MUST_BE_BOOL, inode->firstChild);

	if( !expr.type.isConstant )
	{
		// Add byte code from the expression
		bc->AddCode(&expr.bc);

		// Add a test
		bc->InstrINT(BC_JZ, afterLabel);
	}
	else if( expr.type.dwordValue == 0 )
	{
		// Jump to the else case
		bc->InstrINT(BC_JMP, afterLabel);

		// TODO: Should we warn?
	}

	// Compile the if statement
	bool hasReturn1;
	asCByteCode ifBC;
	CompileStatement(inode->firstChild->next, &hasReturn1, &ifBC);

	// Add the byte code
	LineInstr(bc, inode->firstChild->next->tokenPos);
	bc->AddCode(&ifBC);

	// Do we have an else statement?
	if( inode->firstChild->next != inode->lastChild )
	{
		int afterElse = 0;
		if( !hasReturn1 )
		{
			afterElse = nextLabel++;

			// Add jump to after the else statement
			bc->InstrINT(BC_JMP, afterElse);
		}

		// Add label for the else statement
		bc->Label((short)afterLabel);

		bool hasReturn2;
		asCByteCode elseBC;
		CompileStatement(inode->lastChild, &hasReturn2, &elseBC);

		// Add byte code for the else statement
		LineInstr(bc, inode->lastChild->tokenPos);
		bc->AddCode(&elseBC);

		if( !hasReturn1 )
		{
			// Add label for the end of else statement
			bc->Label((short)afterElse);
		}

		// The if statement only has return if both alternatives have
		*hasReturn = hasReturn1 && hasReturn2;
	}
	else
	{
		// Add label for the end of if statement
		bc->Label((short)afterLabel);
		*hasReturn = false;
	}
}

void asCCompiler::CompileForStatement(asCScriptNode *fnode, asCByteCode *bc)
{
	// Add a variable scope that will be used by CompileBreak/Continue to know where to stop deallocating variables
	AddVariableScope(true, true);

	// We will use three labels for the for loop
	int beforeLabel = nextLabel++;
	int afterLabel = nextLabel++;
	int continueLabel = nextLabel++;

	continueLabels.PushLast(continueLabel);
	breakLabels.PushLast(afterLabel);

	//---------------------------------------
	// Compile the initialization statement
	asCByteCode initBC;
	asSExprContext expr;

	if( fnode->firstChild->nodeType == snDeclaration )
		CompileDeclaration(fnode->firstChild, &initBC);
	else
		CompileExpressionStatement(fnode->firstChild, &initBC);

	//-----------------------------------
	// Compile the condition statement
	asCScriptNode *second = fnode->firstChild->next;
	if( second->firstChild )
	{
		CompileAssignment(second->firstChild, &expr);
		PrepareOperand(&expr, second);
		if( !expr.type.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
			Error(TXT_EXPR_MUST_BE_BOOL, second);

		// If expression is false exit the loop
		expr.bc.InstrINT(BC_JZ, afterLabel);
	}

	//---------------------------
	// Compile the increment statement
	asSExprContext nextBC;
	asCScriptNode *third = second->next;
	if( third->nodeType == snAssignment )
	{
		CompileAssignment(third, &nextBC);
		expr.type = nextBC.type;

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(expr.type, &nextBC.bc);

		// Pop the value from the stack
		nextBC.bc.Pop(expr.type.dataType.GetSizeOnStackDWords()); 
	}

	//------------------------------
	// Compile loop statement
	bool hasReturn;
	asCByteCode forBC;
	CompileStatement(fnode->lastChild, &hasReturn, &forBC);

	//-------------------------------
	// Join the code pieces
	bc->AddCode(&initBC);
	bc->Label((short)beforeLabel);

	// Add a suspend bytecode inside the loop to guarantee  
	// that the application can suspend the execution
	bc->Instr(BC_SUSPEND);

	bc->AddCode(&expr.bc);
	LineInstr(bc, fnode->lastChild->tokenPos);
	bc->AddCode(&forBC);
	bc->Label((short)continueLabel);
	bc->AddCode(&nextBC.bc);
	bc->InstrINT(BC_JMP, beforeLabel);
	bc->Label((short)afterLabel);

	continueLabels.PopLast();
	breakLabels.PopLast();

	// Deallocate variables in this block, in reverse order
	for( int n = variables->variables.GetLength() - 1; n >= 0; n-- )
	{
		sVariable *v = variables->variables[n];

		// Call variable destructors here, for variables not yet destroyed
		CompileDestructor(v->type, v->stackOffset, bc);

		// Don't deallocate function parameters
		if( v->stackOffset > 0 )
			DeallocateVariable(v->stackOffset);
	}

	RemoveVariableScope();
}

void asCCompiler::CompileWhileStatement(asCScriptNode *wnode, asCByteCode *bc)
{
	// Add a variable scope that will be used by CompileBreak/Continue to know where to stop deallocating variables
	AddVariableScope(true, true);

	// We will use two labels for the while loop
	int beforeLabel = nextLabel++;
	int afterLabel = nextLabel++;

	continueLabels.PushLast(beforeLabel);
	breakLabels.PushLast(afterLabel);

	// Add label before the expression
	bc->Label((short)beforeLabel);

	// Compile expression
	asSExprContext expr;
	CompileAssignment(wnode->firstChild, &expr);
	PrepareOperand(&expr, wnode->firstChild);
	if( !expr.type.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
		Error(TXT_EXPR_MUST_BE_BOOL, wnode->firstChild);

	// Add byte code for the expression
	bc->AddCode(&expr.bc);

	// Jump to end of statement if expression is false
	bc->InstrINT(BC_JZ, afterLabel);

	// Add a suspend bytecode inside the loop to guarantee  
	// that the application can suspend the execution
	bc->Instr(BC_SUSPEND);

	// Compile statement
	bool hasReturn;
	asCByteCode whileBC;
	CompileStatement(wnode->lastChild, &hasReturn, &whileBC);

	// Add byte code for the statement
	LineInstr(bc, wnode->lastChild->tokenPos);
	bc->AddCode(&whileBC);

	// Jump to the expression
	bc->InstrINT(BC_JMP, beforeLabel);

	// Add label after the statement
	bc->Label((short)afterLabel);

	continueLabels.PopLast();
	breakLabels.PopLast();

	RemoveVariableScope();
}

void asCCompiler::CompileDoWhileStatement(asCScriptNode *wnode, asCByteCode *bc)
{
	// Add a variable scope that will be used by CompileBreak/Continue to know where to stop deallocating variables
	AddVariableScope(true, true);
	
	// We will use two labels for the while loop
	int beforeLabel = nextLabel++;
	int beforeTest = nextLabel++;
	int afterLabel = nextLabel++;

	continueLabels.PushLast(beforeTest);
	breakLabels.PushLast(afterLabel);

	// Add label before the statement
	bc->Label((short)beforeLabel);

	// Compile statement
	bool hasReturn;
	asCByteCode whileBC;
	CompileStatement(wnode->firstChild, &hasReturn, &whileBC);

	// Add byte code for the statement
	LineInstr(bc, wnode->firstChild->tokenPos);
	bc->AddCode(&whileBC);

	// Add label before the expression
	bc->Label((short)beforeTest);

	// Add a suspend bytecode inside the loop to guarantee  
	// that the application can suspend the execution
	bc->Instr(BC_SUSPEND);

	// Add a line instruction
	LineInstr(bc, wnode->lastChild->tokenPos);

	// Compile expression
	asSExprContext expr;
	CompileAssignment(wnode->lastChild, &expr);
	PrepareOperand(&expr, wnode->lastChild);
	if( !expr.type.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
		Error(TXT_EXPR_MUST_BE_BOOL, wnode->firstChild);

	// Add byte code for the expression
	bc->AddCode(&expr.bc);

	// Jump to next iteration if expression is true
	bc->InstrINT(BC_JNZ, beforeLabel);

	// Add label after the statement
	bc->Label((short)afterLabel);

	continueLabels.PopLast();
	breakLabels.PopLast();

	RemoveVariableScope();
}

void asCCompiler::CompileBreakStatement(asCScriptNode *node, asCByteCode *bc)
{
	if( breakLabels.GetLength() == 0 )
	{
		Error(TXT_INVALID_BREAK, node);
		return;
	}

	// Add destructor calls for all variables that will go out of scope
	asCVariableScope *vs = variables;
	while( !vs->isBreakScope ) 
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
			CompileDestructor(vs->variables[n]->type, vs->variables[n]->stackOffset, bc);

		vs = vs->parent;
	} 

	bc->InstrINT(BC_JMP, breakLabels[breakLabels.GetLength()-1]);
}

void asCCompiler::CompileContinueStatement(asCScriptNode *node, asCByteCode *bc)
{
	if( continueLabels.GetLength() == 0 )
	{
		Error(TXT_INVALID_CONTINUE, node);
		return;
	}

	// Add destructor calls for all variables that will go out of scope
	asCVariableScope *vs = variables;
	while( !vs->isContinueScope )
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
			CompileDestructor(vs->variables[n]->type, vs->variables[n]->stackOffset, bc);

		vs = vs->parent;
	}	

	bc->InstrINT(BC_JMP, continueLabels[continueLabels.GetLength()-1]);
}

void asCCompiler::CompileExpressionStatement(asCScriptNode *enode, asCByteCode *bc)
{
	if( enode->firstChild )
	{
		// Compile the expression 
		asSExprContext expr;
		CompileAssignment(enode->firstChild, &expr);

		// Pop the value from the stack
		expr.bc.Pop(expr.type.dataType.GetSizeOnStackDWords()); 

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(expr.type, &expr.bc);

		ProcessDeferredOutParams(&expr);

		bc->AddCode(&expr.bc);
	}
}

void asCCompiler::PrepareTemporaryObject(asCScriptNode *node, asSExprContext *ctx)
{
	// If the object already is stored in temporary variable then nothing needs to be done
	if( ctx->type.isTemporary ) return;

	// Allocate temporary variable
	asCDataType dt = ctx->type.dataType;
	dt.isReference = false;
	if( !dt.isExplicitHandle )
		dt.isReadOnly = false;
	if( ctx->type.IsNullConstant() )
	{
		dt.isReadOnly = false;
		dt.isExplicitHandle = false;
		ctx->type.dataType.isExplicitHandle = false;
	}
	int offset = AllocateVariable(dt, true);

	// Allocate and construct the temporary object
	CompileConstructor(dt, offset, &ctx->bc);

	// Assign the object to the temporary variable
	asCTypeInfo lvalue;
	dt.isReference = true;
	lvalue.Set(dt);
	lvalue.isTemporary = true;
	lvalue.stackOffset = offset;
	lvalue.isVariable = true;

	PrepareForAssignment(&lvalue.dataType, &ctx->type, &ctx->bc, node);

	ctx->bc.InstrINT(BC_VAR, offset);
	ctx->bc.InstrWORD(BC_GETREF, 0);
	PerformAssignment(&lvalue, &ctx->bc, node);

	// Pop the original reference
	ctx->bc.Pop(1);

	// Push the reference to the temporary variable on the stack
	ctx->bc.InstrINT(BC_VAR, offset);
	ctx->bc.InstrWORD(BC_GETREF, 0);
	lvalue.dataType.isReference = true;

	ctx->type = lvalue;
}

void asCCompiler::CompileReturnStatement(asCScriptNode *rnode, asCByteCode *bc)
{
	// Get return type and location
	sVariable *v = variables->GetVariable("return");
	if( v->type.GetSizeOnStackDWords() > 0 )
	{
		// Is there an expression?
		if( rnode->firstChild )
		{
			// Compile the expression
			asSExprContext expr;
			CompileAssignment(rnode->firstChild, &expr);

			// Prepare the value for assignment
			IsVariableInitialized(&expr.type, rnode->firstChild);

			PrepareArgument(&v->type, &expr, rnode->firstChild);

			if( v->type.IsObject() )
			{
				// Pop the reference to the temporary variable again
				expr.bc.Pop(1);

				// Load the object pointer into the object register
				expr.bc.InstrSHORT(BC_LOADOBJ, expr.type.stackOffset);

				// The temporary variable must not be freed as it will no longer hold an object
				DeallocateVariable(expr.type.stackOffset);
				expr.type.isTemporary = false;
			}
			else
			{
				if( v->type.GetSizeOnStackDWords() == 1 )
					expr.bc.Instr(BC_SRET4);
				else
					expr.bc.Instr(BC_SRET8);
			}

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(expr.type, &expr.bc);

			bc->AddCode(&expr.bc);
		}
		else
			Error(TXT_MUST_RETURN_VALUE, rnode);
	}
	else
		if( rnode->firstChild )
			Error(TXT_CANT_RETURN_VALUE, rnode);

	// Call destructor on all variables except for the function parameters
	asCVariableScope *vs = variables;
	while( vs )
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
			if( vs->variables[n]->stackOffset > 0 )
				CompileDestructor(vs->variables[n]->type, vs->variables[n]->stackOffset, bc);

		vs = vs->parent;
	}	

	// Jump to the end of the function
	bc->InstrINT(BC_JMP, 0);
}

void asCCompiler::AddVariableScope(bool isBreakScope, bool isContinueScope)
{
	variables = new asCVariableScope(variables);
	variables->isBreakScope    = isBreakScope;
	variables->isContinueScope = isContinueScope;
}

void asCCompiler::RemoveVariableScope()
{
	if( variables )
	{
		asCVariableScope *var = variables;
		variables = variables->parent;
		delete var;
	}
}

void asCCompiler::Error(const char *msg, asCScriptNode *node)
{
	asCString str;

	int r, c;
	script->ConvertPosToRowCol(node->tokenPos, &r, &c);

	builder->WriteError(script->name, msg, r, c);

	hasCompileErrors = true;
}

void asCCompiler::Warning(const char *msg, asCScriptNode *node)
{
	asCString str;

	int r, c;
	script->ConvertPosToRowCol(node->tokenPos, &r, &c);

	builder->WriteWarning(script->name, msg, r, c);
}

int asCCompiler::RegisterConstantBStr(const char *bstr, int len)
{
	return builder->RegisterConstantBStr(bstr, len);
}

int asCCompiler::AllocateVariable(const asCDataType &type, bool isTemporary)
{
	asCDataType t(type);

	if( t.IsPrimitive() && t.GetSizeInMemoryDWords() == 1 )
		t.tokenType = ttInt;

	if( t.IsPrimitive() && t.GetSizeInMemoryDWords() == 2 )
		t.tokenType = ttDouble;

	// Find a free location with the same type
	for( int n = 0; n < freeVariables.GetLength(); n++ )
	{
		int slot = freeVariables[n];
		if( variableAllocations[slot] == t )
		{
			if( n != freeVariables.GetLength() - 1 )
				freeVariables[n] = freeVariables.PopLast();
			else
				freeVariables.PopLast();

			// We can't return by slot, must count variable sizes
			int offset = GetVariableOffset(slot);

			if( isTemporary )
				tempVariables.PushLast(offset);

			return offset;
		}
	}

	variableAllocations.PushLast(type);

	int offset = GetVariableOffset(variableAllocations.GetLength()-1);

	if( isTemporary )
		tempVariables.PushLast(offset);

	return offset;
}

int asCCompiler::AllocateVariableNotIn(const asCDataType &type, bool isTemporary, asCArray<int> &vars)
{
	asCDataType t(type);

	if( t.IsPrimitive() && t.GetSizeInMemoryDWords() == 1 )
		t.tokenType = ttInt;

	if( t.IsPrimitive() && t.GetSizeInMemoryDWords() == 2 )
		t.tokenType = ttDouble;

	// Find a free location with the same type
	for( int n = 0; n < freeVariables.GetLength(); n++ )
	{
		int slot = freeVariables[n];
		if( variableAllocations[slot] == t )
		{
			// We can't return by slot, must count variable sizes
			int offset = GetVariableOffset(slot);

			// Verify that it is not in the list of used variables
			bool isUsed = false;
			for( int m = 0; m < vars.GetLength(); m++ )
			{
				if( offset == vars[m] )
				{
					isUsed = true;
					break;
				}
			}

			if( !isUsed )
			{
				if( n != freeVariables.GetLength() - 1 )
					freeVariables[n] = freeVariables.PopLast();
				else
					freeVariables.PopLast();

				if( isTemporary )
					tempVariables.PushLast(offset);

				return offset;
			}
		}
	}

	variableAllocations.PushLast(type);

	int offset = GetVariableOffset(variableAllocations.GetLength()-1);

	if( isTemporary )
		tempVariables.PushLast(offset);

	return offset;
}

int asCCompiler::GetVariableOffset(int varIndex)
{
	// Return offset to the last dword on the stack
	int varOffset = 1;
	for( int n = 0; n < varIndex; n++ )
		varOffset += variableAllocations[n].GetSizeOnStackDWords();

	if( varIndex < variableAllocations.GetLength() )
	{
		int size = variableAllocations[varIndex].GetSizeOnStackDWords();
		if( size > 1 )
			varOffset += size-1;
	}

	return varOffset;
}

int asCCompiler::GetVariableSlot(int offset)
{
	int varOffset = 1;
	for( int n = 0; n < variableAllocations.GetLength(); n++ )
	{
		varOffset += -1 + variableAllocations[n].GetSizeOnStackDWords();
		if( varOffset == offset )
		{
			return n;
		}
		varOffset++;
	}

	return -1;
}

void asCCompiler::DeallocateVariable(int offset)
{
	// Remove temporary variable
	int n;
	for( n = 0; n < tempVariables.GetLength(); n++ )
	{
		if( offset == tempVariables[n] )
		{
			if( n == tempVariables.GetLength()-1 )
				tempVariables.PopLast();
			else
				tempVariables[n] = tempVariables.PopLast();
			break;
		}
	}

	n = GetVariableSlot(offset);
	if( n != -1 )
	{
		freeVariables.PushLast(n);
		return;
	}

	// We might get here if the variable was implicitly declared  
	// because it was use before a formal declaration, in this case
	// the offset is 0x7FFF

	assert(offset == 0x7FFF);
}

void asCCompiler::ReleaseTemporaryVariable(asCTypeInfo &t, asCByteCode *bc)
{
	if( t.isTemporary )
	{
		if( bc )
		{
			// We need to call the destructor on the true variable type
			int n = GetVariableSlot(t.stackOffset);
			asCDataType dt = variableAllocations[n];

			// Call destructor
			CompileDestructor(dt, t.stackOffset, bc);
		}

		DeallocateVariable(t.stackOffset);
		t.isTemporary = false;
	}
}

void asCCompiler::Dereference(asSExprContext *ctx, bool generateCode)
{
	if( ctx->type.dataType.isReference )
	{
		if( ctx->type.dataType.IsObject() )
		{
			ctx->type.dataType.isReference = false;
			if( generateCode ) ctx->bc.Instr(BC_RD4);
			if( ctx->type.dataType.isObjectHandle )
			{
				if( generateCode ) ctx->bc.Instr(BC_CHKREF);
				ctx->type.dataType.isObjectHandle = false;
			}
		}
		else
		{
			if( ctx->type.dataType.GetSizeInMemoryDWords() == 1 )
			{
				ctx->type.dataType.isReference = false;
				ctx->type.dataType.isReadOnly = true;

				if( generateCode )
				{
					if( ctx->type.dataType.IsIntegerType() || ctx->type.dataType.IsEqualExceptRefAndConst(asCDataType(ttBool, false, false)) )
					{
						if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
						{
							ctx->bc.Instr(BC_RD1);
							ctx->bc.Instr(BC_SB);
						}
						else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
						{
							ctx->bc.Instr(BC_RD2);
							ctx->bc.Instr(BC_SW);
						} 
						else
							ctx->bc.Instr(BC_RD4);
					}
					else
					{
						if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
						{
							ctx->bc.Instr(BC_RD1);
							ctx->bc.Instr(BC_UB);
						}
						else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
						{
							ctx->bc.Instr(BC_RD2);
							ctx->bc.Instr(BC_UW);
						}
						else
							ctx->bc.Instr(BC_RD4);
					}
				}
			}
			else if( ctx->type.dataType.GetSizeInMemoryDWords() == 2 )
			{
				ctx->type.dataType.isReference = false;
				ctx->type.dataType.isReadOnly = true;

				if( generateCode ) ctx->bc.Instr(BC_RD8);
			}
			else
			{
				assert(false);
			}

			// If the variable dereferenced was a temporary variable we need to release it
			if( generateCode ) 
			{
				ReleaseTemporaryVariable(ctx->type, &ctx->bc);

				ProcessDeferredOutParams(ctx);
			}
		}
	}
}


bool asCCompiler::IsVariableInitialized(asCTypeInfo *type, asCScriptNode *node)
{
	// Temporary variables are assumed to be initialized
	if( type->isTemporary ) return true;

	// Unreferenced values are said to be initialized
	if( !type->dataType.isReference ) return true;

	// Verify that it is a variable
	if( !type->isVariable ) return true;

	// Find the variable
	sVariable *v = variables->GetVariableByOffset(type->stackOffset);
	assert(v);

	if( v->isInitialized ) return true;

	// Complex types don't need this test
	if( v->type.IsObject() ) return true;

	// Mark as initialized so that the user will not be bothered again
	v->isInitialized = true;

	// Write warning
	asCString str;
	str.Format(TXT_s_NOT_INITIALIZED, (const char *)v->name);
	Warning(str, node);

	return false;
}

void asCCompiler::PrepareOperand(asSExprContext *ctx, asCScriptNode *node)
{
	// Check if the variable is initialized (if it indeed is a variable)
	IsVariableInitialized(&ctx->type, node);

	asCDataType to = ctx->type.dataType;
	to.isReference = false;

	ImplicitConversion(&ctx->bc, to, &ctx->type, node, false);

	ProcessDeferredOutParams(ctx);
}

void asCCompiler::PrepareForAssignment(asCDataType *lvalue, asCTypeInfo *type, asCByteCode *bc, asCScriptNode *node)
{
	asCByteCode prep;

	if( lvalue->isExplicitHandle )
	{
		if( !type->dataType.isExplicitHandle )
		{
			Error(TXT_NEED_TO_BE_A_HANDLE, node);
		}
	}

	if( lvalue->IsObject() )
	{
		// We need the object pointer
		asCDataType to = *lvalue;
		to.isReference = false;
		ImplicitConversion(bc, to, type, node, false);
	}
	else
	{
		// A primitive type requires a non-reference
		asCDataType to = *lvalue;		
		to.isReference = false;
		ImplicitConversion(bc, to, type, node, false);
	}

	// Check data type
	if( !lvalue->IsEqualExceptRefAndConst(type->dataType) )
	{
		asCString str;
		str.Format(TXT_CANT_IMPLICITLY_CONVERT_s_TO_s, (const char*)type->dataType.Format(), (const char *)lvalue->Format());
		Error(str, node);
	}

	// If the assignment will be made with the copy behaviour then the rvalue must not be a reference
	if( lvalue->IsObject() )
		assert(!type->dataType.isReference);

	// Make sure the rvalue is initialized if it is a variable
	IsVariableInitialized(type, node);

	if( prep.GetLastCode() != -1 )
		bc->AddCode(&prep);
}

bool asCCompiler::IsLValue(asCTypeInfo &type)
{
	if( type.dataType.isReadOnly ) return false;
	if( !type.dataType.IsObject() && !type.dataType.isReference ) return false;
	if( type.isTemporary ) return false;
	return true;
}

void asCCompiler::PerformAssignment(asCTypeInfo *lvalue, asCByteCode *bc, asCScriptNode *node)
{
	if( !lvalue->dataType.IsObject() )
	{
		if( lvalue->dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( !lvalue->dataType.isReference )
		{
			Error(TXT_NOT_VALID_REFERENCE, node);
			return;
		}

		int s = lvalue->dataType.GetSizeInMemoryBytes();
		if( s == 1 )
			bc->Instr(BC_WRT1);
		else if( s == 2 )
			bc->Instr(BC_WRT2);
		else if( s == 4 )
			bc->Instr(BC_WRT4);
		else if( s == 8 )
			bc->Instr(BC_WRT8);
	}
	else if( !lvalue->dataType.isExplicitHandle )
	{
		if( lvalue->dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		asSExprContext ctx;
		ctx.type = *lvalue;
		Dereference(&ctx, true);
		*lvalue = ctx.type;
		bc->AddCode(&ctx.bc);

		// TODO: Can't this leave deferred output params unhandled?

		asSTypeBehaviour *beh = engine->GetBehaviour(&lvalue->dataType);
		if( beh->copy )
		{
			// Call the copy operator
			bc->Call(BC_CALLSYS, (asDWORD)beh->copy, 2);
			bc->Instr(BC_RRET4);
		}
		else
		{
			// Default copy operator
			if( lvalue->dataType.GetSizeInMemoryDWords() == 0 )
			{
				Error(TXT_NO_DEFAULT_COPY_OP, node);
			}

			// Copy larger data types from a reference
			bc->InstrWORD(BC_COPY, (asWORD)lvalue->dataType.GetSizeInMemoryDWords()); 
		}
	}
	else
	{
		if( lvalue->dataType.isConstHandle )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( !lvalue->dataType.isReference )
		{
			Error(TXT_NOT_VALID_REFERENCE, node);
			return;
		}

		asUINT objTypeIdx = engine->GetObjectTypeIndex(lvalue->dataType.objectType);
		bc->InstrDWORD(BC_REFCPY, objTypeIdx);
	}

	// Mark variable as initialized
	if( variables )
	{
		sVariable *v = variables->GetVariableByOffset(lvalue->stackOffset);
		if( v ) v->isInitialized = true;
	}
}

void asCCompiler::ImplicitConversion(asCByteCode *bc, const asCDataType &to, asCTypeInfo *from, asCScriptNode *node, bool isExplicit)
{
	// Convert null to any object type handle
	if( from->IsNullConstant() )
	{
		if( to.IsObject() && to.isExplicitHandle )
		{
			from->dataType.isReadOnly = to.isReadOnly;
			from->dataType.tokenType = to.tokenType;
			from->dataType.extendedType = to.extendedType;
			from->dataType.objectType = to.objectType;
			from->dataType.arrayType = to.arrayType;
		}
	}

	// Start by implicitly converting constant values
	if( from->isConstant ) 
		ImplicitConversionConstant(bc, to, from, node, isExplicit);

	// After the constant value has been converted we have the following possibilities
	
	if( !from->dataType.isReference )
	{
		// The value is on the stack

		if( from->dataType.arrayType == 0 && to.arrayType == 0 )
		{
			// int8 -> int16 -> int32
			// uint8 -> uint16 -> uint32
			// bits8 -> bits16 -> bits32
			int s = to.GetSizeInMemoryBytes();
			if( s != from->dataType.GetSizeInMemoryBytes() )
			{
				if( from->dataType.IsIntegerType() )
				{
					if( s == 1 )
						from->dataType.tokenType = ttInt8;
					else if( s == 2 )
						from->dataType.tokenType = ttInt16;
					else if( s == 4 )
						from->dataType.tokenType = ttInt;
				}
				else if( from->dataType.IsUnsignedType() )
				{
					if( s == 1 )
						from->dataType.tokenType = ttUInt8;
					else if( s == 2 )
						from->dataType.tokenType = ttUInt16;
					else if( s == 4 )
						from->dataType.tokenType = ttUInt;
				}
				else if( from->dataType.IsBitVectorType() )
				{
					if( s == 1 )
						from->dataType.tokenType = ttBits8;
					else if( s == 2 )
						from->dataType.tokenType = ttBits16;
					else if( s == 4 )
						from->dataType.tokenType = ttBits;
				}
			}

			// Allow implicit conversion between numbers
			if( to.IsIntegerType() )
			{
				if( from->dataType.IsUnsignedType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
				else if( from->dataType.IsFloatType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_F2I);
				}
				else if( from->dataType.IsDoubleType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_dTOi);
				}
				else if( from->dataType.IsBitVectorType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
			}
			else if( to.IsUnsignedType() )
			{
				if( from->dataType.IsIntegerType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
				else if( from->dataType.IsFloatType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_F2UI);
				}
				else if( from->dataType.IsDoubleType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_dTOui);
				}
				else if( from->dataType.IsBitVectorType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
			}
			else if( to.IsBitVectorType() )
			{
				if( from->dataType.IsIntegerType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
				else if( from->dataType.IsUnsignedType() )
				{
					from->dataType.tokenType = to.tokenType;
				}
			}
			else if( to.IsFloatType() )
			{
				if( from->dataType.IsIntegerType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_I2F);
				}
				else if( from->dataType.IsUnsignedType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_UI2F);
				}
				else if( from->dataType.IsDoubleType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_dTOf);
				}
			}
			else if( to.IsDoubleType() )
			{
				if( from->dataType.IsIntegerType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_iTOd);
				}
				else if( from->dataType.IsUnsignedType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_uiTOd);
				}
				else if( from->dataType.IsFloatType() )
				{
					from->dataType.tokenType = to.tokenType;
					if( bc ) bc->Instr(BC_fTOd);
				}
			}

			// Primitive types on the stack, can be const or non-const
			if( from->dataType.IsPrimitive() )
				from->dataType.isReadOnly = to.isReadOnly;
		}
	}

	// If the target is a handle to a const object, then automacally convert the from type to const
	if( to.isExplicitHandle && to.isReadOnly && !from->dataType.isReadOnly )
		from->dataType.isReadOnly = true;

	if( !to.isReference )
	{
		if( from->dataType.isReference )
		{
			asSExprContext ctx;
			ctx.type = *from;
			Dereference(&ctx, bc != 0);
			*from = ctx.type;
			if( bc )
				bc->AddCode(&ctx.bc);

			// TODO: Can't this leave unhandled deferred output params?

			if( from->dataType.IsPrimitive() )
			{
				// Try once more, since the base type may be different
				ImplicitConversion(bc, to, from, node, isExplicit);
			}
		}

		if( to.isExplicitHandle )
		{
			// If the rvalue is a handle to a const object, then the lvalue must also be a handle to a const object
			if( from->dataType.isReadOnly && !to.isReadOnly )
			{
				if( isExplicit )
				{
					assert(node);
					asCString str;
					str.Format(TXT_CANT_IMPLICITLY_CONVERT_s_TO_s, (const char*)from->dataType.Format(), (const char *)to.Format());
					Error(str, node);
				}
			}
		}
	}
	else
	{
		if( from->dataType.isReference )
		{
			// A reference to a non-const can be converted to a reference to a const
			if( to.isReadOnly ) 
				from->dataType.isReadOnly = true;
			else if( from->dataType.isReadOnly )
			{
				// A reference to a const can be converted to a reference to a non-const by putting the object in a temporary variable
				from->dataType.isReadOnly = false;

				if( bc )
				{
					assert(!from->isTemporary);

					// Allocate a temporary variable
					int offset = AllocateVariable(from->dataType, true);
					from->isTemporary = true;
					from->stackOffset = (short)offset;

					CompileConstructor(from->dataType, offset, bc);

					asSExprContext rctx;
					rctx.type = *from;
					rctx.bc.AddCode(bc);
					asSExprContext lctx;
					lctx.type = *from;
					lctx.bc.InstrSHORT(BC_PSF, offset);
					asSExprContext ctx;
					DoAssignment(&ctx, &lctx, &rctx, node, node, ttAssignment, node);

					bc->AddCode(&ctx.bc);
					*from = ctx.type;
				}
			}
		}
		else
		{
			// A non-reference can be converted to a reference, 
			// by putting the value in a temporary variable
			from->dataType.isReference = true;

			// Since it is a new temporary variable it doesn't have to be const
			from->dataType.isReadOnly = to.isReadOnly;

			if( bc )
			{
				// Allocate a temporary variable
				int offset = AllocateVariable(from->dataType, true);
				from->isTemporary = true;
				from->stackOffset = (short)offset;

				CompileConstructor(from->dataType, offset, bc);

				// Do the copy based on size
				if( from->dataType.GetSizeInMemoryDWords() == 1 )
				{
					bc->InstrINT(BC_VAR, offset);
					bc->InstrWORD(BC_GETREF, 0);
					bc->Instr(BC_MOV4);
				}
				else if( from->dataType.GetSizeInMemoryDWords() == 2 )
				{
					bc->InstrINT(BC_VAR, offset);
					bc->InstrWORD(BC_GETREF, 0);
					bc->Instr(BC_WRT8);
					bc->Pop(2);
				}
				else
				{
					// Shouldn't be possible
					assert(false);
				}

				// Put the address to the variable on the stack
				bc->InstrINT(BC_VAR, offset);
				bc->InstrWORD(BC_GETREF, 0);
			}
		}
	}
}

void asCCompiler::ImplicitConversionConstant(asCByteCode *bc, const asCDataType &to, asCTypeInfo *from, asCScriptNode *node, bool isExplicit)
{
	assert(from->isConstant);
	
	// TODO: node should be the node of the value that is
	// converted (not the operator that provokes the implicit
	// conversion)

	// If the base type is correct there is no more to do
	if( to.IsEqualExceptRefAndConst(from->dataType) ) return;

	// References cannot be constants
	if( from->dataType.isReference ) return;

	// Arrays can't be constants
	if( to.arrayType > 0 ) return;

	if( to.IsIntegerType() )
	{
		// Float constants can be implicitly converted to int
		if( from->dataType.IsFloatType() )
		{
			float fc = from->floatValue;
			int ic = int(fc);

			if( float(ic) != fc )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_g_d_g, fc, ic, float(ic));
				if( !isExplicit && node ) Warning(str, node);
			}


			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrINT(BC_SET4, ic);
			}

			from->dataType = asCDataType(ttInt, true, false);
			from->intValue = ic;

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		// Double constants can be implicitly converted to int
		else if( from->dataType.IsDoubleType() )
		{
			double fc = from->doubleValue;
			int ic = int(fc);

			if( double(ic) != fc )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_g_d_g, fc, ic, double(ic));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET8);

				bc->RemoveLastCode();
				bc->InstrINT(BC_SET4, ic);
			}
			
			from->dataType = asCDataType(ttInt, true, false);
			from->intValue = ic;

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsUnsignedType() )
		{
			// Verify that it is possible to convert to signed without getting negative
			if( from->intValue < 0 )
			{
				asCString str;
				str.Format(TXT_CHANGE_SIGN_u_d, from->dwordValue, from->intValue);
				if( !isExplicit && node ) Warning(str, node);
			}

			assert(!bc || bc->GetLastCode() == BC_SET4);

			from->dataType = asCDataType(ttInt, true, false);

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsBitVectorType() )
		{
			assert(!bc || bc->GetLastCode() == BC_SET4);

			from->dataType = asCDataType(ttInt, true, false);

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsIntegerType() && from->dataType.GetSizeInMemoryBytes() < to.GetSizeInMemoryBytes() )
			from->dataType = to;
		else if( from->dataType.IsIntegerType() &&
		         from->dataType.GetSizeInMemoryBytes() > to.GetSizeInMemoryBytes() )
		{
			// Verify if it is possible
			if( to.GetSizeInMemoryBytes() == 1 )
			{
				if( char(from->intValue) != from->intValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->intValue = char(from->intValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}
			else if( to.GetSizeInMemoryBytes() == 2 )
			{
				if( short(from->intValue) != from->intValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->intValue = short(from->intValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}

			from->dataType.tokenType = to.tokenType;
		}
	}
	else if( to.IsUnsignedType() )
	{
		if( from->dataType.IsFloatType() )
		{
			float fc = from->floatValue;
			unsigned int uic = (unsigned int)(fc);

			if( float(uic) != fc )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_g_u_g, fc, uic, float(uic));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrDWORD(BC_SET4, uic);
			}

			from->dataType = asCDataType(ttUInt, true, false);
			from->intValue = uic;

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType == asCDataType(ttDouble, true, false) )
		{
			double fc = from->doubleValue;
			unsigned int uic = (unsigned int)(fc);

			if( double(uic) != fc )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_g_u_g, fc, uic, double(uic));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET8);

				bc->RemoveLastCode();
				bc->InstrDWORD(BC_SET4, uic);
			}

			from->dataType = asCDataType(ttUInt, true, false);
			from->intValue = uic;

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsIntegerType() )
		{
			// Verify that it is possible to convert to unsigned without loosing negative
			if( from->intValue < 0 )
			{
				asCString str;
				str.Format(TXT_CHANGE_SIGN_d_u, from->intValue, from->dwordValue);
				if( !isExplicit && node ) Warning(str, node);
			}

			assert(!bc || bc->GetLastCode() == BC_SET4);

			from->dataType = asCDataType(ttUInt, true, false);

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsBitVectorType() )
		{
			assert(!bc || bc->GetLastCode() == BC_SET4);

			from->dataType = asCDataType(ttUInt, true, false);

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsUnsignedType() && from->dataType.GetSizeInMemoryBytes() < to.GetSizeInMemoryBytes() )
			from->dataType = to;
		else if( from->dataType.IsUnsignedType() &&
		         from->dataType.GetSizeInMemoryBytes() > to.GetSizeInMemoryBytes() )
		{
			// Verify if it is possible
			if( to.GetSizeInMemoryBytes() == 1 )
			{
				if( asBYTE(from->dwordValue) != from->dwordValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->dwordValue = asBYTE(from->dwordValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}
			else if( to.GetSizeInMemoryBytes() == 2 )
			{
				if( asWORD(from->dwordValue) != from->dwordValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->dwordValue = asWORD(from->dwordValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}

			from->dataType.tokenType = to.tokenType;
		}
	}
	else if( to.IsFloatType() )
	{
		if( from->dataType.IsDoubleType() )
		{
			double ic = from->doubleValue;
			float fc = float(ic);

			if( double(fc) != ic )
			{
				asCString str;
				str.Format(TXT_POSSIBLE_LOSS_OF_PRECISION);
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET8);

				bc->RemoveLastCode();
				bc->InstrFLOAT(BC_SET4, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->floatValue = fc;
		}
		else if( from->dataType.IsIntegerType() )
		{
			int ic = from->intValue;
			float fc = float(ic);

			if( int(fc) != ic )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_d_g_d, ic, fc, int(fc));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrFLOAT(BC_SET4, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->floatValue = fc;
		}
		else if( from->dataType.IsUnsignedType() )
		{
			unsigned int uic = from->dwordValue;
			float fc = float(uic);

			if( (unsigned int)(fc) != uic )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_u_g_u, uic, fc, (unsigned int)(fc));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrFLOAT(BC_SET4, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->floatValue = fc;
		}
	}
	else if( to.IsDoubleType() )
	{
		if( from->dataType.IsFloatType() )
		{
			float ic = from->floatValue;
			double fc = double(ic);

			// Don't check for float->double
		//	if( float(fc) != ic )
		//	{
		//		acCString str;
		//		str.Format(TXT_NOT_EXACT_g_g_g, ic, fc, float(fc));
		//		if( !isExplicit ) Warning(str, node);
		//	}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrDOUBLE(BC_SET8, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->doubleValue = fc;
		}
		else if( from->dataType.IsIntegerType() )
		{
			int ic = from->intValue;
			double fc = double(ic);

			if( int(fc) != ic )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_d_g_d, ic, fc, int(fc));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrDOUBLE(BC_SET8, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->doubleValue = fc;
		}
		else if( from->dataType.IsUnsignedType() )
		{
			unsigned int uic = from->dwordValue;
			double fc = double(uic);

			if( (unsigned int)(fc) != uic )
			{
				asCString str;
				str.Format(TXT_NOT_EXACT_u_g_u, uic, fc, (unsigned int)(fc));
				if( !isExplicit && node ) Warning(str, node);
			}

			if( bc )
			{
				assert(bc->GetLastCode() == BC_SET4);

				bc->RemoveLastCode();
				bc->InstrDOUBLE(BC_SET8, fc);
			}

			from->dataType.tokenType = to.tokenType;
			from->doubleValue = fc;
		}

		// A non-constant float cannot be implicitly converted to double
	}
	else if( to.IsBitVectorType() )
	{
		if( from->dataType.IsIntegerType() ||
			from->dataType.IsUnsignedType() )
		{
			assert(!bc || bc->GetLastCode() == BC_SET4);

			from->dataType = asCDataType(ttBits, true, false);

			// Try once more, in case of a smaller type
			ImplicitConversionConstant(bc, to, from, node, isExplicit);
		}
		else if( from->dataType.IsBitVectorType() && from->dataType.GetSizeInMemoryBytes() < to.GetSizeInMemoryBytes() )
			from->dataType = to;
		else if( from->dataType.IsBitVectorType() &&
		         from->dataType.GetSizeInMemoryBytes() > to.GetSizeInMemoryBytes() )
		{
			// Verify if it is possible
			if( to.GetSizeInMemoryBytes() == 1 )
			{
				if( asBYTE(from->dwordValue) != from->dwordValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->dwordValue = asBYTE(from->dwordValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}
			else if( to.GetSizeInMemoryBytes() == 2 )
			{
				if( asWORD(from->dwordValue) != from->dwordValue )
					if( !isExplicit && node ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

				from->dwordValue = asWORD(from->dwordValue);

				if( bc )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->RemoveLastCode();
					bc->InstrDWORD(BC_SET4, from->dwordValue);
				}
			}

			from->dataType.tokenType = to.tokenType;
		}
	}
}

void asCCompiler::DoAssignment(asSExprContext *ctx, asSExprContext *lctx, asSExprContext *rctx, asCScriptNode *lexpr, asCScriptNode *rexpr, int op, asCScriptNode *opNode)
{
	// Verify that the left hand value isn't a temporary variable
	if( lctx->type.isTemporary )
	{
		Error(TXT_REF_IS_TEMP, lexpr);
		return;
	}

	// If left expression resolves into a registered type
	// check if the assignment operator is overloaded, and check 
	// the type of the right hand expression. If none is found
	// the default action is a direct copy if it is the same type
	// and a simple assignment.
	asSTypeBehaviour *beh = 0;
	if( !lctx->type.dataType.isExplicitHandle ) 
		beh = engine->GetBehaviour(&lctx->type.dataType);
	if( beh )
	{
		// Find the matching overloaded operators
		asCArray<int> ops;
		int n;
		for( n = 0; n < beh->operators.GetLength(); n += 2 )
		{
			if( op == beh->operators[n] )
				ops.PushLast(beh->operators[n+1]);
		}

		asCArray<int> match;
		MatchArgument(ops, match, &rctx->type, 0);

		if( match.GetLength() == 1 )
		{
			// If it is an array, both sides must have the same subtype
			if( lctx->type.dataType.arrayType > 0 )
				if( !lctx->type.dataType.IsEqualExceptRefAndConst(rctx->type.dataType) )
					Error(TXT_BOTH_MUST_BE_SAME, opNode);

			// We must verify that the lvalue isn't const
			if( lctx->type.dataType.isReadOnly )
				Error(TXT_REF_IS_READ_ONLY, lexpr);

			// Prepare the rvalue
			asCScriptFunction *descr = engine->systemFunctions[-match[0] - 1];
			PrepareArgument(&descr->parameterTypes[0], rctx, rexpr, true, bool(descr->inOutFlags[0] & 1));

			if( rctx->type.isTemporary && lctx->bc.IsVarUsed(rctx->type.stackOffset) )
			{
				// Release the current temporary variable
				ReleaseTemporaryVariable(rctx->type, 0);

				asCArray<int> usedVars;
				lctx->bc.GetVarsUsed(usedVars);
				rctx->bc.GetVarsUsed(usedVars);
		
				asCDataType dt = rctx->type.dataType;
				dt.isReference = false;
				int newOffset = AllocateVariableNotIn(dt, true, usedVars);

				rctx->bc.ExchangeVar(rctx->type.stackOffset, newOffset);
				rctx->type.stackOffset = newOffset;
				rctx->type.isTemporary = true;
			}

			// Add code for arguments
			MergeExprContexts(ctx, rctx);

			// Add the code for the object
			Dereference(lctx, true);
			MergeExprContexts(ctx, lctx);

			// Get the argument reference
			if( descr->parameterTypes[0].isReference )
			{
				if( descr->parameterTypes[0].IsObject() && !descr->parameterTypes[0].isExplicitHandle )
					ctx->bc.InstrWORD(BC_GETOBJREF, 1);
				else
					ctx->bc.InstrWORD(BC_GETREF, 1);
			}
			else if( descr->parameterTypes[0].IsObject() )
			{
				ctx->bc.InstrWORD(BC_GETOBJ, 1);

				// The temporary variable must not be freed as it will no longer hold an object
				DeallocateVariable(rctx->type.stackOffset);
				rctx->type.isTemporary = false;
			}

			asCArray<asCTypeInfo> argTypes;
			argTypes.PushLast(rctx->type);
			PerformFunctionCall(match[0], ctx, false, &argTypes);

			return;
		}
		else if( match.GetLength() > 1 )
		{
			Error(TXT_MORE_THAN_ONE_MATCHING_OP, opNode);

			ctx->type.Set(lctx->type.dataType);

			return;
		}
	}


	if( op != ttAssignment )
	{
		// The expression must be computed
		// first so we can't change that

		// We can't just read from the lvalue
		// as that would remove the address
		// from the stack.

		// Store the lvalue in a temporary register
		// TODO: PSF isn't used anymore, look for VAR,GETREF instead
		bool isSimple = (lctx->bc.GetLastCode() == BC_PSF || lctx->bc.GetLastCode() == BC_PGA);
		if( !isSimple )
			lctx->bc.Instr(BC_STORE4);

		// Compile the operation
		asSExprContext l, r, o;
		MergeExprContexts(&l, lctx);
		l.type = lctx->type;
		MergeExprContexts(&r, rctx);
		r.type = rctx->type;
		CompileOperator(opNode, &l, &r, &o);
		rctx->type = o.type;

		PrepareForAssignment(&lctx->type.dataType, &rctx->type, &o.bc, rexpr);

		MergeExprContexts(ctx, &o);

		// Recall the lvalue
		if( !isSimple )
			ctx->bc.Instr(BC_RECALL4);
		else
		{
			// Compile the lvalue again
			asSExprContext e;
			CompileCondition(lexpr, &e);
			lctx->type = e.type;
			MergeExprContexts(ctx, &e);
		}
	}
	else
	{
		if( lctx->type.dataType.IsObject() )
		{
			PrepareArgument(&lctx->type.dataType, rctx, rexpr, true, true);
			if( !lctx->type.dataType.IsEqualExceptRefAndConst(rctx->type.dataType) )
			{
				asCString str;
				str.Format(TXT_CANT_IMPLICITLY_CONVERT_s_TO_s, (const char*)rctx->type.dataType.Format(), (const char *)lctx->type.dataType.Format());
				Error(str, rexpr);
			}
		}
		else
			PrepareForAssignment(&lctx->type.dataType, &rctx->type, &rctx->bc, rexpr);


		MergeExprContexts(ctx, rctx);
		MergeExprContexts(ctx, lctx);

		if( lctx->type.dataType.IsObject() ) 
			ctx->bc.InstrWORD(BC_GETOBJREF, 1);
	}

	PerformAssignment(&lctx->type, &ctx->bc, opNode);

	ReleaseTemporaryVariable(rctx->type, &ctx->bc);

	ctx->type = rctx->type;
}

void asCCompiler::CompileAssignment(asCScriptNode *expr, asSExprContext *ctx)
{
	asCScriptNode *lexpr = expr->firstChild;
	if( lexpr->next )
	{
		if( globalExpression )
		{
			Error(TXT_ASSIGN_IN_GLOBAL_EXPR, expr);

			ctx->type.Set(asCDataType(ttInt, true, false));
			ctx->type.isConstant = true;
			ctx->type.qwordValue = 0;
			return;
		}

		asCByteCode obc;
		asSExprContext lctx;
		asSExprContext rctx;

		// Compile the two expression terms
		CompileAssignment(lexpr->next->next, &rctx);
		CompileCondition(lexpr, &lctx);

		DoAssignment(ctx, &lctx, &rctx, lexpr, lexpr->next->next, lexpr->next->tokenType, lexpr->next);
	}
	else
		CompileCondition(lexpr, ctx);
}

void asCCompiler::CompileCondition(asCScriptNode *expr, asSExprContext *ctx)
{
	asCTypeInfo ctype;

	// Compile the conditional expression
	asCScriptNode *cexpr = expr->firstChild;
	if( cexpr->next )
	{
		//-------------------------------
		// Compile the expressions
		asSExprContext e;
		CompileExpression(cexpr, &e);
		PrepareOperand(&e, cexpr);
		ctype = e.type;
		if( !ctype.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
			Error(TXT_EXPR_MUST_BE_BOOL, cexpr);

		asSExprContext le;
		CompileAssignment(cexpr->next, &le);

		asSExprContext re;
		CompileAssignment(cexpr->next->next, &re);

		// Allow a 0 in the first case to be implicitly converted to the second type
		if( le.type.isConstant && le.type.intValue == 0 && le.type.dataType.IsUnsignedType() )
		{
			asCDataType to = re.type.dataType;
			to.isReference = false;
			to.isReadOnly  = true;
			ImplicitConversionConstant(&le.bc, to, &le.type, cexpr->next, false);
		}

		//---------------------------------
		// Output the byte code
		int afterLabel = nextLabel++;
		int elseLabel = nextLabel++;

		// Allocate temporary variable and copy the result to that one
		asCTypeInfo temp;
		temp = le.type;
		temp.dataType.isReference = false;
		temp.dataType.isReadOnly = false;
		// Make sure the variable isn't used in the initial expression
		asCArray<int> vars;
		e.bc.GetVarsUsed(vars);
		int offset = AllocateVariableNotIn(temp.dataType, true, vars);
		temp.isTemporary = true;
		temp.stackOffset = (short)offset;

		CompileConstructor(temp.dataType, offset, &ctx->bc);

		MergeExprContexts(ctx, &e);
		ctx->bc.InstrINT(BC_JZ, elseLabel);

		asCTypeInfo rtemp;
		rtemp = temp;
		rtemp.dataType.isReference = true;

		PrepareForAssignment(&rtemp.dataType, &le.type, &le.bc, cexpr->next);
		MergeExprContexts(ctx, &le);

		ctx->bc.InstrINT(BC_VAR, offset);
		ctx->bc.InstrWORD(BC_GETREF, 0);
		PerformAssignment(&rtemp, &ctx->bc, cexpr->next);
		ctx->bc.Pop(le.type.dataType.GetSizeOnStackDWords()); // Pop the original value

		// Release the old temporary variable
		ReleaseTemporaryVariable(le.type, &ctx->bc);

		ctx->bc.InstrINT(BC_JMP, afterLabel);
		ctx->bc.Label((short)elseLabel);

		// Copy the result to the same temporary variable
		PrepareForAssignment(&rtemp.dataType, &re.type, &re.bc, cexpr->next);
		MergeExprContexts(ctx, &re);

		ctx->bc.InstrINT(BC_VAR, offset);
		ctx->bc.InstrWORD(BC_GETREF, 0);
		rtemp.dataType.isReference = true;
		PerformAssignment(&rtemp, &ctx->bc, cexpr->next);
		ctx->bc.Pop(le.type.dataType.GetSizeOnStackDWords()); // Pop the original value

		// Release the old temporary variable
		ReleaseTemporaryVariable(re.type, &ctx->bc);

		ctx->bc.Label((short)afterLabel);

		ctx->bc.InstrINT(BC_VAR, offset);
		ctx->bc.InstrWORD(BC_GETREF, 0);

		// Make sure both expressions have the same type
		if( le.type.dataType != re.type.dataType )
			Error(TXT_BOTH_MUST_BE_SAME, expr);

		// Set the temporary variable as output
		ctx->type = rtemp;
		ctx->type.dataType.isReference = true;

		// Make sure the output isn't marked as being a literal constant
		ctx->type.isConstant = false;
	}
	else
		CompileExpression(cexpr, ctx);
}

void asCCompiler::CompileExpression(asCScriptNode *expr, asSExprContext *ctx)
{
	// Convert to polish post fix, i.e: a+b => ab+
	asCArray<asCScriptNode *> stack;
	asCArray<asCScriptNode *> stack2;
	asCArray<asCScriptNode *> postfix;

	asCScriptNode *node = expr->firstChild;
	while( node )
	{
		int precedence = GetPrecedence(node);

		while( stack.GetLength() > 0 &&
			   precedence <= GetPrecedence(stack[stack.GetLength()-1]) )
			stack2.PushLast(stack.PopLast());

		stack.PushLast(node);

		node = node->next;
	}

	while( stack.GetLength() > 0 )
		stack2.PushLast(stack.PopLast());

	// Preallocate the memory
	postfix.Allocate(stack.GetLength(), false);

	// We need to swap operands so that the left
	// operand is always computed before the right
	SwapPostFixOperands(stack2, postfix);

	// Compile the postfix formatted expression
	CompilePostFixExpression(&postfix, ctx);
}

void asCCompiler::SwapPostFixOperands(asCArray<asCScriptNode *> &postfix, asCArray<asCScriptNode *> &target)
{
	if( postfix.GetLength() == 0 ) return;

	asCScriptNode *node = postfix.PopLast();
	if( node->nodeType == snExprTerm )
	{
		target.PushLast(node);
		return;
	}

	SwapPostFixOperands(postfix, target);
	SwapPostFixOperands(postfix, target);

	target.PushLast(node);
}

void asCCompiler::CompilePostFixExpression(asCArray<asCScriptNode *> *postfix, asSExprContext *ctx)
{
	// Shouldn't send any byte code
	assert(ctx->bc.GetLastCode() == -1);

	// Pop the last node
	asCScriptNode *node = postfix->PopLast();

	// If term, compile the term
	if( node->nodeType == snExprTerm )
	{
		CompileExpressionTerm(node, ctx);
		return;
	}

	// Compile the two expression terms
	asSExprContext r, l;

	CompilePostFixExpression(postfix, &l);
	CompilePostFixExpression(postfix, &r);

	// Compile the operation
	CompileOperator(node, &l, &r, ctx);
}

void asCCompiler::CompileExpressionTerm(asCScriptNode *node, asSExprContext *ctx)
{
	// Shouldn't send any byte code
	assert(ctx->bc.GetLastCode() == -1);

	// Compile the value node
	asCScriptNode *vnode = node->firstChild;
	while( vnode->nodeType != snExprValue )
		vnode = vnode->next;

	asSExprContext v;
	CompileExpressionValue(vnode, &v);

	// Compile post fix operators
	asCScriptNode *pnode = vnode->next;
	while( pnode )
	{
		CompileExpressionPostOp(pnode, &v);
		pnode = pnode->next;
	}

	// Compile pre fix operators
	pnode = vnode->prev;
	while( pnode )
	{
		CompileExpressionPreOp(pnode, &v);
		pnode = pnode->prev;
	}

	// Return the byte code and final type description
	MergeExprContexts(ctx, &v);

    ctx->type = v.type;
}

void asCCompiler::CompileExpressionValue(asCScriptNode *node, asSExprContext *ctx)
{
	// Shouldn't receive any byte code
	assert(ctx->bc.GetLastCode() == -1);

	asCScriptNode *vnode = node->firstChild;
	if( vnode->nodeType == snIdentifier )
	{
		GETSTRING(name, &script->code[vnode->tokenPos], vnode->tokenLength);

		sVariable *v = variables->GetVariable(name);
		if( v == 0 )
		{
			// Is it a global property?
			bool isCompiled = true;
			asCProperty *prop = builder->GetGlobalProperty(name, &isCompiled);
			if( prop )
			{
				ctx->type.Set(prop->type);
				ctx->type.dataType.isReference = true;
				ctx->type.dataType.isReadOnly = prop->type.isReadOnly;
				ctx->type.dataType.isObjectHandle = prop->type.isExplicitHandle;
				ctx->type.dataType.isExplicitHandle = false;
				ctx->type.stackOffset = 0x7FFF;

				// Verify that the global property has been compiled already
				if( isCompiled )
				{
					// TODO: If the global property is a pure constant
					// we can allow the compiler to optimize it. Pure
					// constants are global constant variables that were
					// initialized by literal constants.

					// Push the address of the variable on the stack
					ctx->bc.InstrINT(BC_PGA, prop->index);
				}
				else
				{
					asCString str;
					str.Format(TXT_UNINITIALIZED_GLOBAL_VAR_s, prop->name.AddressOf());
					Error(str, vnode);
				}
			}
			else
			{
				asCString str;
				str.Format(TXT_s_NOT_DECLARED, (const char *)name);
				Error(str, vnode);

				// Give dummy value
				ctx->bc.InstrDWORD(BC_SET4, 0);
				ctx->type.Set(asCDataType(ttInt, false, true));
				ctx->type.stackOffset = 0x7FFF;

				// Declare the variable now so that it will not be reported again
				variables->DeclareVariable(name, asCDataType(ttInt, false, false), 0x7FFF);

				// Mark the variable as initialized so that the user will not be bother by it again
				sVariable *v = variables->GetVariable(name);
				assert(v);
				if( v ) v->isInitialized = true;
			}
		}
		else
		{
			// Push the address of the variable on the stack
			ctx->bc.InstrINT(BC_VAR, v->stackOffset);
			ctx->bc.InstrWORD(BC_GETREF, 0);

			ctx->type.Set(v->type);
			ctx->type.dataType.isReference = true;
			ctx->type.dataType.isObjectHandle = v->type.isExplicitHandle;
			ctx->type.dataType.isExplicitHandle = false;
			ctx->type.stackOffset = (short)v->stackOffset;
			ctx->type.isVariable = true;

			// Implicitly dereference primitive parameters sent by reference
			if( v->type.isReference && (!v->type.IsObject() || v->type.isExplicitHandle) )
				ctx->bc.Instr(BC_RD4);
		}
	}
	else if( vnode->nodeType == snConstant )
	{
		if( vnode->tokenType == ttIntConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			asDWORD val = asStringScanUInt(value, 10, 0);
			ctx->bc.InstrDWORD(BC_SET4, val);

			ctx->type.Set(asCDataType(ttUInt, true, false));
			ctx->type.isConstant = true;
			ctx->type.dwordValue = val;
		}
		else if( vnode->tokenType == ttBitsConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos+2], vnode->tokenLength-2);

			// TODO: Check for overflow
			int val = asStringScanUInt(value, 16, 0);
			ctx->bc.InstrDWORD(BC_SET4, val);

			ctx->type.Set(asCDataType(ttBits, true, false));
			ctx->type.isConstant = true;
			ctx->type.intValue = val;
		}
		else if( vnode->tokenType == ttFloatConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			float v = float(asStringScanDouble(value, 0));
			ctx->bc.InstrFLOAT(BC_SET4, v);

			ctx->type.Set(asCDataType(ttFloat, true, false));
			ctx->type.isConstant = true;
			ctx->type.floatValue = v;
		}
		else if( vnode->tokenType == ttDoubleConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			double v = asStringScanDouble(value, 0);
			ctx->bc.InstrDOUBLE(BC_SET8, v);

			ctx->type.Set(asCDataType(ttDouble, true, false));
			ctx->type.isConstant = true;
			ctx->type.doubleValue = v;
		}
		else if( vnode->tokenType == ttTrue ||
			     vnode->tokenType == ttFalse )
		{
			if( vnode->tokenType == ttTrue )
				ctx->bc.InstrDWORD(BC_SET4, VALUE_OF_BOOLEAN_TRUE);
			else
				ctx->bc.InstrDWORD(BC_SET4, 0); 

			ctx->type.Set(asCDataType(ttBool, true, false));
			ctx->type.isConstant = true;
			ctx->type.dwordValue = vnode->tokenType == ttTrue ? VALUE_OF_BOOLEAN_TRUE : 0;
		}
		else if( vnode->tokenType == ttStringConstant )
		{
			asCString str;

			asCScriptNode *snode = vnode->firstChild;
			while( snode )
			{
				GETSTRING(cat, &script->code[snode->tokenPos+1], snode->tokenLength-2);

				str += cat;

				snode = snode->next;
			}

			// Register the constant string with the engine
			int id = RegisterConstantBStr(str.AddressOf(), str.GetLength());

			// Call the string factory function to create a string object
			asCScriptFunction *descr = engine->stringFactory;
			if( descr == 0 )
			{
				// Error
				Error(TXT_STRINGS_NOT_RECOGNIZED, vnode);

				// Give dummy value
				ctx->bc.InstrDWORD(BC_SET4, 0);
				ctx->type.Set(asCDataType(ttInt, false, true));
				ctx->type.stackOffset = 0x7FFF;
			}
			else
			{
				ctx->bc.InstrWORD(BC_STR, id);
				PerformFunctionCall(descr->id, ctx);
			}			
		}
		else if( vnode->tokenType == ttNull )
		{
			ctx->bc.InstrDWORD(BC_SET4, 0);

			ctx->type.Set(asCDataType(ttInt, true, false));
			ctx->type.dataType.isExplicitHandle = true;
			ctx->type.isConstant = true;
			ctx->type.intValue = 0;
		}
		else
			assert(false);
	}
	else if( vnode->nodeType == snFunctionCall )
	{
		if( globalExpression )
		{
			Error(TXT_FUNCTION_IN_GLOBAL_EXPR, vnode);

			// Output dummy code
			ctx->bc.InstrDWORD(BC_SET4, 0);

			ctx->type.Set(asCDataType(ttInt, true, false));
			ctx->type.isConstant = true;
			ctx->type.qwordValue = 0;
		}
		else
			CompileFunctionCall(vnode, ctx, 0, false);
	}
	else if( vnode->nodeType == snAssignment )
	{
		asSExprContext e;
		CompileAssignment(vnode, &e);
		MergeExprContexts(ctx, &e);
		ctx->type = e.type;
	}
	else
		assert(false);
}

void asCCompiler::CompileConversion(asCScriptNode *node, asSExprContext *ctx)
{
	// Should be empty
	assert(ctx->bc.GetLastCode() == -1);

	// Verify that there is only one argument
	if( node->lastChild->firstChild != node->lastChild->lastChild )
	{
		Error(TXT_ONLY_ONE_ARGUMENT_IN_CAST, node->lastChild);
	}

	// Compile the expression
	asSExprContext expr;
	CompileAssignment(node->lastChild->firstChild, &expr);
	MergeExprContexts(ctx, &expr);

	ctx->type.Set(builder->CreateDataTypeFromNode(node->firstChild, script));
	ctx->type.dataType.isReadOnly = true; // Default to const
	assert(ctx->type.dataType.tokenType != ttIdentifier);

	IsVariableInitialized(&expr.type, node);

	// Try an implicit conversion first
	ImplicitConversion(&ctx->bc, ctx->type.dataType, &expr.type, node, true);

	// If no type conversion is really tried ignore it
	if( ctx->type.dataType == expr.type.dataType )
	{
		// This will keep information about constant type
		ctx->type = expr.type;
		return;
	}

	if( ctx->type.dataType.IsEqualExceptConst(expr.type.dataType) && ctx->type.dataType.IsPrimitive() )
	{
		ctx->type = expr.type;
		ctx->type.dataType.isReadOnly = true;
		return;
	}

	if( expr.type.isConstant )
		ctx->type.isConstant = true;

	bool conversionOK = false;
	if( ctx->type.dataType.IsIntegerType() )
	{
		if( expr.type.isConstant )
		{
			if( expr.type.dataType.IsFloatType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.intValue = int(expr.type.floatValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsDoubleType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET8);
				ctx->type.intValue = int(expr.type.doubleValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsBitVectorType() ||
					 expr.type.dataType.IsIntegerType() ||
					 expr.type.dataType.IsUnsignedType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.dwordValue = expr.type.dwordValue;
				conversionOK = true;
			}

			if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
				ctx->type.intValue = char(ctx->type.intValue);
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
				ctx->type.intValue = short(ctx->type.intValue);

			ctx->bc.ClearAll();
			ctx->bc.InstrINT(BC_SET4, ctx->type.intValue);
		}
		else
		{
			if( expr.type.dataType.IsFloatType() )
			{
				conversionOK = true;
				ctx->bc.Instr(BC_F2I);
			}
			else if( expr.type.dataType.IsDoubleType() )
			{
				conversionOK = true;
				ctx->bc.Instr(BC_dTOi);
			}
			else if( expr.type.dataType.IsBitVectorType() ||
					 expr.type.dataType.IsIntegerType() ||
					 expr.type.dataType.IsUnsignedType() )
				conversionOK = true;

			if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
				ctx->bc.Instr(BC_SB);
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
				ctx->bc.Instr(BC_SW);
		}
	}
	else if( ctx->type.dataType.IsUnsignedType() )
	{
		if( expr.type.isConstant )
		{
			if( expr.type.dataType.IsFloatType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.dwordValue = asDWORD(expr.type.floatValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsDoubleType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET8);
				ctx->type.dwordValue = asDWORD(expr.type.doubleValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsBitVectorType() ||
					 expr.type.dataType.IsIntegerType() ||
					 expr.type.dataType.IsUnsignedType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.dwordValue = expr.type.dwordValue;
				conversionOK = true;
			}

			if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
				ctx->type.dwordValue = asBYTE(ctx->type.dwordValue);
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
				ctx->type.dwordValue = asWORD(ctx->type.dwordValue);

			ctx->bc.ClearAll();
			ctx->bc.InstrDWORD(BC_SET4, ctx->type.dwordValue);
		}
		else
		{
			if( expr.type.dataType.IsFloatType() )
			{
				conversionOK = true;
				ctx->bc.Instr(BC_F2UI);
			}
			else if( expr.type.dataType.IsDoubleType() )
			{
				conversionOK = true;
				ctx->bc.Instr(BC_dTOui);
			}
			else if( expr.type.dataType.IsBitVectorType() ||
					 expr.type.dataType.IsIntegerType() ||
					 expr.type.dataType.IsUnsignedType() )
				conversionOK = true;

			if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
				ctx->bc.Instr(BC_UB);
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
				ctx->bc.Instr(BC_UW);
		}
	}
	else if( ctx->type.dataType.IsFloatType() )
	{
		if( expr.type.isConstant )
		{
			if( expr.type.dataType.IsDoubleType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET8);
				ctx->type.floatValue = float(expr.type.doubleValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsIntegerType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.floatValue = float(expr.type.intValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsUnsignedType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.floatValue = float(expr.type.dwordValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsBitVectorType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.dwordValue = expr.type.dwordValue;
				conversionOK = true;
			}

			ctx->bc.ClearAll();
			ctx->bc.InstrDWORD(BC_SET4, ctx->type.dwordValue);
		}
		else
		{
			if( expr.type.dataType.IsDoubleType() )
			{
				ctx->bc.Instr(BC_dTOf);
				return;
			}
			else if( expr.type.dataType.IsIntegerType() )
			{
				ctx->bc.Instr(BC_I2F);
				return;
			}
			else if( expr.type.dataType.IsUnsignedType() )
			{
				ctx->bc.Instr(BC_UI2F);
				return;
			}
			else if( expr.type.dataType.IsBitVectorType() )
				return;
		}
	}
	else if( ctx->type.dataType == asCDataType(ttDouble, true, false) )
	{
		if( expr.type.isConstant )
		{
			if( expr.type.dataType.IsFloatType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.doubleValue = double(expr.type.floatValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsIntegerType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.doubleValue = double(expr.type.intValue);
				conversionOK = true;
			}
			else if( expr.type.dataType.IsUnsignedType() )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->type.doubleValue = double(expr.type.dwordValue);
				conversionOK = true;
			}

			ctx->bc.ClearAll();
			ctx->bc.InstrQWORD(BC_SET8, ctx->type.qwordValue);
		}
		else
		{
			if( expr.type.dataType.IsFloatType() )
			{
				ctx->bc.Instr(BC_fTOd);
				return;
			}
			else if( expr.type.dataType.IsIntegerType() )
			{
				ctx->bc.Instr(BC_iTOd);
				return;
			}
			else if( expr.type.dataType.IsUnsignedType() )
			{
				ctx->bc.Instr(BC_uiTOd);
				return;
			}
		}
	}
	else if( ctx->type.dataType.IsBitVectorType() )
	{
		if( expr.type.isConstant )
		{
			if( expr.type.dataType.IsIntegerType() ||
				expr.type.dataType.IsUnsignedType() ||
				expr.type.dataType.IsFloatType() || 
				expr.type.dataType.IsBitVectorType() )
			{
				ctx->type.dwordValue = expr.type.dwordValue;

				if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
					ctx->type.dwordValue = asBYTE(ctx->type.dwordValue);
				else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
					ctx->type.dwordValue = asWORD(ctx->type.dwordValue);

				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->bc.ClearAll();
				ctx->bc.InstrDWORD(BC_SET4, ctx->type.dwordValue);

				return;
			}
		}
		else
		{
			if( expr.type.dataType.IsIntegerType() ||
				expr.type.dataType.IsUnsignedType() ||
				expr.type.dataType.IsFloatType() ||
				expr.type.dataType.IsBitVectorType() )
			{
				if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
					ctx->bc.Instr(BC_UB);
				else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
					ctx->bc.Instr(BC_UW);

				return;
			}
		}
	}

	if( conversionOK )
		return;

	// Conversion not available

	asCString strTo, strFrom;

	strTo = ctx->type.dataType.Format();
	strFrom = expr.type.dataType.Format();

	asCString msg;
	msg.Format(TXT_NO_CONVERSION_s_TO_s, (const char *)strFrom, (const char *)strTo);

	Error(msg, node);
}

void asCCompiler::AfterFunctionCall(int funcID, asCScriptNode *argListNode, asSExprContext *ctx, asCArray<asCTypeInfo> &argTypes)
{
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	// Parameters that are sent by reference should be assigned 
	// to the evaluated expression if it is an lvalue
	asCScriptNode *arg = 0;
	if( argListNode )
		arg = argListNode->lastChild;

	// Evaluate the arguments from last to first
	int n = descr->parameterTypes.GetLength() - 1;
	for( ; n >= 0; n-- )
	{
		if( descr->parameterTypes[n].isReference && (descr->inOutFlags[n] & 2) )
		{
			assert( arg );

			// Store the argument for later processing
			asSDeferredOutParam outParam;
			outParam.argNode = arg;
			outParam.argType = argTypes[n];
			outParam.argInOutFlags = descr->inOutFlags[n];

			ctx->deferredOutParams.PushLast(outParam);
		}
		else
		{
			// Release the temporary variable now
			ReleaseTemporaryVariable(argTypes[n], &ctx->bc);
		}

		// It's possible that there are extra 
		// arguments inserted by the compiler
		if( arg )
			arg = arg->prev;
	}
}

void asCCompiler::ProcessDeferredOutParams(asSExprContext *ctx)
{
	if( isProcessingDeferredOutParams ) return;

	isProcessingDeferredOutParams = true;

	for( int n = 0; n < ctx->deferredOutParams.GetLength(); n++ )
	{
		asSDeferredOutParam outParam = ctx->deferredOutParams[n];
		asSExprContext expr;
		CompileAssignment(outParam.argNode, &expr);

		// Check if the expression is complex, that is, 
		// calls any functions or writes any data to variables
		if( expr.bc.IsComplex() && (outParam.argInOutFlags == 3) )
		{
			Warning(TXT_ARG_COMPUTED_TWICE, outParam.argNode);
		}

		// Verify that the expression result in a lvalue
		if( IsLValue(expr.type) )
		{
			asSExprContext rctx;
			rctx.bc.InstrINT(BC_VAR, outParam.argType.stackOffset);
			rctx.bc.InstrWORD(BC_GETREF, 0);

			rctx.type = outParam.argType;
			rctx.type.dataType.isReference = true;

			asSExprContext o;
			DoAssignment(&o, &expr, &rctx, outParam.argNode, outParam.argNode, ttAssignment, outParam.argNode);

			o.bc.Pop(o.type.dataType.GetSizeOnStackDWords());

			MergeExprContexts(ctx, &o);
		}
		else
		{
			// We must still evaluate the expression
			MergeExprContexts(ctx, &expr);
			ctx->bc.Pop(expr.type.dataType.GetSizeOnStackDWords());

			// Give a warning
			Warning(TXT_ARG_NOT_LVALUE, outParam.argNode);

			ReleaseTemporaryVariable(outParam.argType, &ctx->bc);
		}

		ReleaseTemporaryVariable(expr.type, &ctx->bc);
	}

	ctx->deferredOutParams.SetLength(0);
	isProcessingDeferredOutParams = false;
}

void asCCompiler::CompileFunctionCall(asCScriptNode *node, asSExprContext *ctx, asCObjectType *objectType, bool objIsConst)
{
	// The first node is a datatype node, we must determine if it really is a 
	// datatype in which case we are compiling a constructor or a cast, otherwise 
	// it is a function call
	asCString name;
	bool isFunction = false;
	bool isConstructor = false;
	asCTypeInfo tempObj;
	asCArray<int> funcs;
	asCScriptNode *nm = node->firstChild->firstChild;
	if( nm->tokenType == ttIdentifier && nm->next == 0 )
	{
		name.Copy(&script->code[nm->tokenPos], nm->tokenLength);
		if( !engine->GetObjectType(name) )
		{
			if( objectType )
				builder->GetObjectMethodDescriptions(name, objectType, funcs, objIsConst);
			else
				builder->GetFunctionDescriptions(name, funcs);
			
			isFunction = true;
		}
	}

	if( !isFunction )
	{
		// It is possible that the name is really a constructor
		asCDataType dt;
		dt = builder->CreateDataTypeFromNode(node->firstChild, script);
		if( dt.objectType == 0 )
		{
			// This is a cast to a primitive type
			CompileConversion(node, ctx);
			return;
		}
		else
		{
			asSTypeBehaviour *beh = engine->GetBehaviour(&dt);
			if( beh )
				funcs = beh->constructors;

			// Make sure it is not being called as a object method
			if( objectType )
				Error(TXT_ILLEGAL_CALL, node);
			else
			{
				isConstructor = true;

				tempObj.dataType = dt;
				tempObj.stackOffset = AllocateVariable(dt, true);
				tempObj.dataType.isReference = true;
				tempObj.isTemporary = true;

				// Push the address of the object on the stack
				ctx->bc.InstrINT(BC_VAR, tempObj.stackOffset);
			}
		}
	}

	// Compile the arguments
	asCArray<asCTypeInfo> argTypes;
	asCArray<asSExprContext *> args;
	asCArray<asCTypeInfo> temporaryVariables;
	
	CompileArgumentList(node->lastChild, args, isConstructor ? &tempObj.dataType : 0);

	int n;
	for( n = 0; n < args.GetLength(); n++ )
		argTypes.PushLast(args[n]->type);

	// Special case: Allow calling func(void) with a void expression.
	if( argTypes.GetLength() == 1 && argTypes[0].dataType == asCDataType(ttVoid, false, false) )
	{
		// Evaluate the expression before the function call
		MergeExprContexts(ctx, args[0]);
		delete args[0];
		args.SetLength(0);
		argTypes.SetLength(0);
	}

	// Special case: If this is an object constructor and there are no arguments use the default constructor.
	// If none has been registered, just allocate the variable and push it on the stack.
	if( isConstructor && argTypes.GetLength() == 0 )
	{
		asSTypeBehaviour *beh = engine->GetBehaviour(&tempObj.dataType);
		if( beh && beh->construct == 0 )
		{
			// Call the default constructor
			ctx->type = tempObj;
			ctx->bc.InstrWORD(BC_GETREF, 0);
			DefaultConstructor(&ctx->bc, tempObj.dataType);

			// Push the reference on the stack
			ctx->bc.InstrINT(BC_VAR, tempObj.stackOffset);
			ctx->bc.InstrWORD(BC_GETREF, 0);
			return;
		}
	}

	MatchFunctions(funcs, argTypes, node, name, objIsConst);

	if( funcs.GetLength() != 1 )
	{
		// The error was reported by MatchFunctions()

		// Dummy value
		ctx->bc.InstrDWORD(BC_SET4, 0);
		ctx->type.Set(asCDataType(ttInt, true, false));
	}
	else
	{
		asCByteCode objBC;

		if( !isConstructor )
		{
			objBC.AddCode(&ctx->bc);
		}

		PrepareFunctionCall(funcs[0], node->lastChild, &ctx->bc, args);
		
		if( !isConstructor )
		{
			// Verify if any of the args variable offsets are used in the other code.
			// If they are exchange the offset for a new one
			for( int n = 0; n < args.GetLength(); n++ )
			{
				if( args[n]->type.isTemporary && objBC.IsVarUsed(args[n]->type.stackOffset) )
				{
					// Release the current temporary variable
					ReleaseTemporaryVariable(args[n]->type, 0);

					asCArray<int> usedVars;
					objBC.GetVarsUsed(usedVars);
					ctx->bc.GetVarsUsed(usedVars);
			
					asCDataType dt = args[n]->type.dataType;
					dt.isReference = false;
					int newOffset = AllocateVariableNotIn(dt, true, usedVars);

					ctx->bc.ExchangeVar(args[n]->type.stackOffset, newOffset);
					args[n]->type.stackOffset = newOffset;
					args[n]->type.isTemporary = true;
				}
			}

			ctx->bc.AddCode(&objBC);
		}

		MoveArgsToStack(funcs[0], &ctx->bc, args, objectType && !isConstructor);

		int offset = 0;
		for( int n = 0; n < args.GetLength(); n++ )
		{
			offset += args[n]->type.dataType.GetSizeOnStackDWords();
			argTypes[n] = args[n]->type;
		}
	
		if( isConstructor )
			ctx->bc.InstrWORD(BC_GETREF, offset);

		PerformFunctionCall(funcs[0], ctx, isConstructor, &argTypes, node->lastChild);

		if( isConstructor )
		{
			// The constructor doesn't return anything, 
			// so we have to manually inform the type of  
			// the return value
			ctx->type = tempObj;

			// Push the address of the object on the stack again
			ctx->bc.InstrINT(BC_VAR, tempObj.stackOffset);
			ctx->bc.InstrWORD(BC_GETREF, 0);
		}
	}

	// Cleanup
	for( n = 0; n < args.GetLength(); n++ )
		if( args[n] ) delete args[n];
}

void asCCompiler::CompileExpressionPreOp(asCScriptNode *node, asSExprContext *ctx)
{
	int op = node->tokenType;

	IsVariableInitialized(&ctx->type, node);

	if( op == ttHandle )
	{
		// Verify that the type allow its handle to be taken
		if( ctx->type.dataType.isExplicitHandle || !ctx->type.dataType.IsObject() || !ctx->type.dataType.objectType->beh.addref || !ctx->type.dataType.objectType->beh.release )
		{
			Error(TXT_OBJECT_HANDLE_NOT_SUPPORTED, node);
		}

		if( !ctx->type.dataType.isReference )
		{
			Error(TXT_NOT_VALID_REFERENCE, node);
		}

		// Mark the type as an object handle
		ctx->type.dataType.isObjectHandle = false;
		ctx->type.dataType.isExplicitHandle = true;
	}
	else if( op == ttMinus && ctx->type.dataType.IsObject() )
	{
		asCTypeInfo objType = ctx->type;

		Dereference(ctx, true);

		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(&ctx->type, node);

		// Now find a matching function for the object type
		asSTypeBehaviour *beh = engine->GetBehaviour(&ctx->type.dataType);
		if( beh == 0 )
		{
			asCString str;
			str.Format(TXT_OBJECT_DOESNT_SUPPORT_NEGATE_OP);
			Error(str, node);
		}
		else
		{
			// Find the negate operator
			int opNegate = 0;
			bool found = false;
			int n;
			for( n = 0; n < beh->operators.GetLength(); n+= 2 )
			{
				// Only accept the negate operator
				if( ttMinus == beh->operators[n] && 
					engine->systemFunctions[-beh->operators[n+1] - 1]->parameterTypes.GetLength() == 0 )
				{
					found = true;
					opNegate = beh->operators[n+1];
					break;
				}
			}

			// Did we find a suitable function?
			if( found )
			{
				PerformFunctionCall(opNegate, ctx);
			}
			else
			{
				asCString str;
				str.Format(TXT_OBJECT_DOESNT_SUPPORT_NEGATE_OP);
				Error(str, node);
			}
		}

		// Release the potentially temporary object
		ReleaseTemporaryVariable(objType, &ctx->bc);
	}
	else if( op == ttPlus || op == ttMinus )
	{
		asCDataType to = ctx->type.dataType;
		to.isReference = false;

		// TODO: The case -2147483648 gives an unecessary warning of changed sign for implicit conversion

		if( ctx->type.dataType.IsUnsignedType() )
		{
			if( ctx->type.dataType.GetSizeInMemoryBytes() == 1 )
				to.tokenType = ttInt8;
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 2 )
				to.tokenType = ttInt16;
			else if( ctx->type.dataType.GetSizeInMemoryBytes() == 4 )
				to.tokenType = ttInt;
			else
				Error(TXT_INVALID_TYPE, node);
		}

		ImplicitConversion(&ctx->bc, to, &ctx->type, node, false);

		if( ctx->type.dataType.IsIntegerType() )
		{
			if( op == ttMinus )
			{
				if( ctx->type.isConstant )
				{
					assert(ctx->bc.GetLastCode() == BC_SET4);
					ctx->bc.ClearAll();
					ctx->bc.InstrINT(BC_SET4, -ctx->type.intValue);
					ctx->type.intValue = -ctx->type.intValue;
					return;
				}

				ctx->bc.Instr(BC_NEGi);
			}
		}
		else if( ctx->type.dataType.IsFloatType() )
		{
			if( op == ttMinus )
			{
				if( ctx->type.isConstant )
				{
					assert(ctx->bc.GetLastCode() == BC_SET4);
					ctx->bc.ClearAll();
					ctx->bc.InstrFLOAT(BC_SET4, -ctx->type.floatValue);
					ctx->type.floatValue = -ctx->type.floatValue;
					return;
				}

				ctx->bc.Instr(BC_NEGf);
			}
		}
		else if( ctx->type.dataType.IsDoubleType() )
		{
			if( op == ttMinus )
			{
				if( ctx->type.isConstant )
				{
					assert(ctx->bc.GetLastCode() == BC_SET8);
					ctx->bc.ClearAll();
					ctx->bc.InstrDOUBLE(BC_SET8, -ctx->type.doubleValue);
					ctx->type.doubleValue = -ctx->type.doubleValue;
					return;
				}

				ctx->bc.Instr(BC_NEGd);
			}
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttNot )
	{
		PrepareOperand(ctx, node);

		if( ctx->type.dataType.IsEqualExceptConst(asCDataType(ttBool, true, false)) )
		{
			if( ctx->type.isConstant )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->bc.ClearAll();
				ctx->bc.InstrDWORD(BC_SET4, ctx->type.dwordValue == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
				ctx->type.dwordValue = (ctx->type.dwordValue == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
				return;
			}

			ctx->bc.Instr(BC_TZ);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttBitNot )
	{
		PrepareOperand(ctx, node);

		if( ctx->type.dataType.IsBitVectorType() )
		{
			if( ctx->type.isConstant )
			{
				assert(ctx->bc.GetLastCode() == BC_SET4);
				ctx->bc.ClearAll();
				ctx->bc.InstrDWORD(BC_SET4, ~ctx->type.dwordValue);
				ctx->type.dwordValue = ~ctx->type.dwordValue;
				return;
			}

			ctx->bc.Instr(BC_BNOT);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttInc || op == ttDec )
	{
		if( globalExpression )
			Error(TXT_INC_OP_IN_GLOBAL_EXPR, node);

		if( !ctx->type.dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);

		// Make sure the reference isn't a temporary variable
		if( ctx->type.isTemporary )
			Error(TXT_REF_IS_TEMP, node);
		if( ctx->type.dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( ctx->type.dataType == asCDataType(ttInt, false, true) ||
			ctx->type.dataType == asCDataType(ttUInt, false, true) )
		{
			if( op == ttInc )
				ctx->bc.Instr(BC_INCi);
			else
				ctx->bc.Instr(BC_DECi);
		}
		else if( ctx->type.dataType == asCDataType(ttInt16, false, true) ||
			     ctx->type.dataType == asCDataType(ttUInt16, false, true) )
		{
			if( op == ttInc )
				ctx->bc.Instr(BC_INCi16);
			else
				ctx->bc.Instr(BC_DECi16);
		}
		else if( ctx->type.dataType == asCDataType(ttInt8, false, true) ||
			     ctx->type.dataType == asCDataType(ttUInt8, false, true) )
		{
			if( op == ttInc )
				ctx->bc.Instr(BC_INCi8);
			else
				ctx->bc.Instr(BC_DECi8);
		}
		else if( ctx->type.dataType == asCDataType(ttFloat, false, true) )
		{
			if( op == ttInc )
				ctx->bc.Instr(BC_INCf);
			else
				ctx->bc.Instr(BC_DECf);
		}
		else if( ctx->type.dataType == asCDataType(ttDouble, false, true) )
		{
			if( op == ttInc )
				ctx->bc.Instr(BC_INCd);
			else
				ctx->bc.Instr(BC_DECd);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else
		// Unknown operator
		assert(false);
}

void asCCompiler::CompileExpressionPostOp(asCScriptNode *node, asSExprContext *ctx)
{
	int op = node->tokenType;

	if( op == ttInc || op == ttDec )
	{
		if( globalExpression )
			Error(TXT_INC_OP_IN_GLOBAL_EXPR, node);

		// Make sure the reference isn't a temporary variable
		if( !ctx->type.dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);
		if( ctx->type.isTemporary )
			Error(TXT_REF_IS_TEMP, node);
		if( ctx->type.dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( ctx->type.dataType == asCDataType(ttInt, false, true) ||
			ctx->type.dataType == asCDataType(ttUInt, false, true) )
		{
			if( op == ttInc ) ctx->bc.Instr(BC_INCi); else ctx->bc.Instr(BC_DECi);
			Dereference(ctx, true);
			ctx->bc.InstrDWORD(BC_SET4, 1); // Let optimizer change to SET1
			if( op == ttInc ) ctx->bc.Instr(BC_SUBi); else ctx->bc.Instr(BC_ADDi);

			ctx->type.dataType.isReference = false;
		}
		else if( ctx->type.dataType == asCDataType(ttInt16, false, true) ||
			     ctx->type.dataType == asCDataType(ttUInt16, false, true) )
		{
			if( op == ttInc ) ctx->bc.Instr(BC_INCi16); else ctx->bc.Instr(BC_DECi16);
			Dereference(ctx, true);
			ctx->bc.InstrDWORD(BC_SET4, 1);
			if( op == ttInc ) ctx->bc.Instr(BC_SUBi); else ctx->bc.Instr(BC_ADDi);
		}
		else if( ctx->type.dataType == asCDataType(ttInt8, false, true) ||
			     ctx->type.dataType == asCDataType(ttUInt8, false, true) )
		{
			if( op == ttInc ) ctx->bc.Instr(BC_INCi8); else ctx->bc.Instr(BC_DECi8);
			Dereference(ctx, true);
			ctx->bc.InstrDWORD(BC_SET4, 1);
			if( op == ttInc ) ctx->bc.Instr(BC_SUBi); else ctx->bc.Instr(BC_ADDi);
		}
		else if( ctx->type.dataType == asCDataType(ttFloat, false, true) )
		{
			if( op == ttInc ) ctx->bc.Instr(BC_INCf); else ctx->bc.Instr(BC_DECf);
			Dereference(ctx, true);
			ctx->bc.InstrFLOAT(BC_SET4, 1);
			if( op == ttInc ) ctx->bc.Instr(BC_SUBf); else ctx->bc.Instr(BC_ADDf);

			ctx->type.dataType.isReference = false;
		}
		else if( ctx->type.dataType == asCDataType(ttDouble, false, true) )
		{
			if( op == ttInc ) ctx->bc.Instr(BC_INCd); else ctx->bc.Instr(BC_DECd);
			Dereference(ctx, true);
			ctx->bc.InstrDOUBLE(BC_SET8, 1);
			if( op == ttInc ) ctx->bc.Instr(BC_SUBd); else ctx->bc.Instr(BC_ADDd);

			ctx->type.dataType.isReference = false;
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttDot )
	{
		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(&ctx->type, node);

		if( node->firstChild->nodeType == snIdentifier )
		{
			// Get the property name
			GETSTRING(name, &script->code[node->firstChild->tokenPos], node->firstChild->tokenLength);

			Dereference(ctx, true);

			// Find the property offset and type
			if( ctx->type.dataType.IsObject() )
			{
				bool isConst = ctx->type.dataType.isReadOnly;

				asCProperty *prop = builder->GetObjectProperty(ctx->type.dataType, name);
				if( prop )
				{
					// Put the offset on the stack
					ctx->bc.InstrINT(BC_SET4, prop->byteOffset);
					if( op == ttDot )
						ctx->bc.Instr(BC_ADDi);

					// Set the new type (keeping info about temp variable)
					ctx->type.dataType = prop->type;
					ctx->type.dataType.isReference = true;
					ctx->type.dataType.isReadOnly = isConst ? true : prop->type.isReadOnly;
					ctx->type.isVariable = false;

					if( ctx->type.dataType.IsObject() && !ctx->type.dataType.isExplicitHandle )
					{
						// Objects that are members are not references
						ctx->type.dataType.isReference = false;
					}

					if( ctx->type.dataType.isExplicitHandle )
					{
						ctx->type.dataType.isObjectHandle = true;
						ctx->type.dataType.isExplicitHandle = false;
						ctx->type.dataType.isReadOnly = prop->type.isReadOnly;

						// The handle should be const if the object is const
						ctx->type.dataType.isConstHandle = isConst;
					}
				}
				else
				{
					asCString str;
					str.Format(TXT_s_NOT_MEMBER_OF_s, (const char *)name, (const char *)ctx->type.dataType.Format());
					Error(str, node);
				}
			}
			else
			{
				asCString str;
				str.Format(TXT_s_NOT_MEMBER_OF_s, (const char *)name, (const char *)ctx->type.dataType.Format());
				Error(str, node);
			}
		}
		else
		{
			if( globalExpression )
				Error(TXT_METHOD_IN_GLOBAL_EXPR, node);

			// Make sure it is an object we are accessing
			if( !ctx->type.dataType.IsObject() )
			{
				asCString str;
				str.Format(TXT_ILLEGAL_OPERATION_ON_s, (const char *)ctx->type.dataType.Format());
				Error(str, node);
			}
			else
			{
				// We need the object pointer
				Dereference(ctx, true);

				bool isConst = ctx->type.dataType.isReadOnly;

				asCObjectType *trueObj = ctx->type.dataType.objectType;

				asCTypeInfo objType = ctx->type;

				// Compile function call
				CompileFunctionCall(node->firstChild, ctx, trueObj, isConst);

				// Release the potentially temporary object
				ReleaseTemporaryVariable(objType, &ctx->bc);
			}
		}
	}
	else if( op == ttOpenBracket )
	{
		Dereference(ctx, true);
		bool isConst = ctx->type.dataType.isReadOnly;

		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(&ctx->type, node);

		// Compile the expression
		asSExprContext expr;
		CompileAssignment(node->firstChild, &expr);

		asCTypeInfo objType = ctx->type;

		// Now find a matching function for the object type and indexing type
		asSTypeBehaviour *beh = engine->GetBehaviour(&ctx->type.dataType);
		if( beh == 0 )
		{
			asCString str;
			str.Format(TXT_OBJECT_DOESNT_SUPPORT_INDEX_OP, ctx->type.dataType.Format().AddressOf());
			Error(str, node);
		}
		else
		{
			asCArray<int> ops;
			int n;
			if( isConst )
			{
				// Only list const behaviours
				for( n = 0; n < beh->operators.GetLength(); n+= 2 )
				{
					if( ttOpenBracket == beh->operators[n] && engine->systemFunctions[-1-beh->operators[n+1]]->isReadOnly )
						ops.PushLast(beh->operators[n+1]);
				}
			}
			else
			{
				// TODO:
				// Prefer non-const over const
				for( n = 0; n < beh->operators.GetLength(); n+= 2 )
				{
					if( ttOpenBracket == beh->operators[n] )
						ops.PushLast(beh->operators[n+1]);
				}
			}
			
			asCArray<int> ops1;
			MatchArgument(ops, ops1, &expr.type, 0);

			if( !isConst ) 
				FilterConst(ops1);

			// Did we find a suitable function?
			if( ops1.GetLength() == 1 )
			{
				asCScriptFunction *descr = engine->systemFunctions[-ops1[0] - 1];

				// Store the code for the object
				asCByteCode objBC;
				objBC.AddCode(&ctx->bc);

				// Add code for arguments

				PrepareArgument(&descr->parameterTypes[0], &expr, node->firstChild, true, bool(descr->inOutFlags[0] & 1));
				MergeExprContexts(ctx, &expr);

				if( descr->parameterTypes[0].isReference )
				{
					if( descr->parameterTypes[0].IsObject() && !descr->parameterTypes[0].isExplicitHandle )
						ctx->bc.InstrWORD(BC_GETOBJREF, 0);
					else
						ctx->bc.InstrWORD(BC_GETREF, 0);
				}
				else if( descr->parameterTypes[0].IsObject() )
				{
					ctx->bc.InstrWORD(BC_GETOBJ, 0);

					// The temporary variable must not be freed as it will no longer hold an object
					DeallocateVariable(expr.type.stackOffset);
					expr.type.isTemporary = false;
				}

				// Add the code for the object again
				ctx->bc.AddCode(&objBC);

				asCArray<asCTypeInfo> argTypes;
				argTypes.PushLast(expr.type);
				PerformFunctionCall(descr->id, ctx, false, &argTypes);

				// TODO: Ugly code
				// The default array returns a reference to the subtype
				if( objType.dataType.IsDefaultArrayType(engine) )
				{
					ctx->type.dataType = objType.dataType.GetSubType(engine);
					if( ctx->type.dataType.isExplicitHandle )
					{
						ctx->type.dataType.isExplicitHandle = false;
						ctx->type.dataType.isObjectHandle = true;
					}
					if( ctx->type.dataType.IsObject() && !ctx->type.dataType.isObjectHandle )
						ctx->type.dataType.isReference = false;
					else
						ctx->type.dataType.isReference = true;
				}

				ctx->type.isVariable = false;
			}
			else if( ops.GetLength() > 1 )
			{
				Error(TXT_MORE_THAN_ONE_MATCHING_OP, node);
			}
			else
			{
				asCString str;
				str.Format(TXT_NO_MATCHING_OP_FOUND_FOR_TYPE_s, expr.type.dataType.Format().AddressOf());
				Error(str, node);
			}
		}

		// Release the potentially temporary object
		ReleaseTemporaryVariable(objType, &ctx->bc);
	}
}

int asCCompiler::GetPrecedence(asCScriptNode *op)
{
	// x*y, x/y, x%y
	// x+y, x-y
	// x<=y, x<y, x>=y, x>y
	// x==y, x!=y
	// x and y
	// (x xor y)
	// x or y

	// The following are not used in this function,
	// but should have lower precedence than the above
	// x ? y : z
	// x = y

	// The expression term have the highest precedence
	if( op->nodeType == snExprTerm )
		return 1;

	// Evaluate operators by token
	int tokenType = op->tokenType;
	if( tokenType == ttStar || tokenType == ttSlash || tokenType == ttPercent )
		return 0;

	if( tokenType == ttPlus || tokenType == ttMinus )
		return -1;

	if( tokenType == ttBitShiftLeft ||
		tokenType == ttBitShiftRight ||
		tokenType == ttBitShiftRightArith )
		return -2;

	if( tokenType == ttAmp )
		return -3;

	if( tokenType == ttBitXor )
		return -4;

	if( tokenType == ttBitOr )
		return -5;

	if( tokenType == ttLessThanOrEqual ||
		tokenType == ttLessThan ||
		tokenType == ttGreaterThanOrEqual ||
		tokenType == ttGreaterThan )
		return -6;

	if( tokenType == ttEqual || tokenType == ttNotEqual || tokenType == ttXor )
		return -7;

	if( tokenType == ttAnd )
		return -8;

	if( tokenType == ttOr )
		return -9;

	// Unknown operator
	assert(false);

	return 0;
}

int asCCompiler::MatchArgument(asCArray<int> &funcs, asCArray<int> &matches, asCTypeInfo *argType, int paramNum)
{
	bool isExactMatch = false;
	bool isMatchExceptConst = false;
	bool isMatchWithBaseType = false;
	bool isMatchExceptSign = false;

	int n;

	matches.SetLength(0);

	for( n = 0; n < funcs.GetLength(); n++ )
	{
		asCScriptFunction *desc = builder->GetFunctionDescription(funcs[n]);

		// Does the function have arguments enough?
		if( desc->parameterTypes.GetLength() <= paramNum )
			continue;

		// Can we make the match by implicit conversion?
		asCTypeInfo ti = *argType;
		ImplicitConversion(0, desc->parameterTypes[paramNum], &ti, 0, false);
		if( desc->parameterTypes[paramNum].IsEqualExceptRef(ti.dataType) )
		{
			// Is it an exact match?
			if( argType->dataType.IsEqualExceptRef(ti.dataType) )
			{
				if( !isExactMatch ) matches.SetLength(0);

				isExactMatch = true;

				matches.PushLast(funcs[n]);
				continue;
			}

			if( !isExactMatch )
			{
				// Is it a match except const?
				if( argType->dataType.IsEqualExceptRefAndConst(ti.dataType) )
				{
					if( !isMatchExceptConst ) matches.SetLength(0);

					isMatchExceptConst = true;

					matches.PushLast(funcs[n]);
					continue;
				}

				if( !isMatchExceptConst )
				{
					// Is it a size promotion, e.g. int8 -> int?
					if( argType->dataType.IsSameBaseType(ti.dataType) )
					{
						if( !isMatchWithBaseType ) matches.SetLength(0);

						isMatchWithBaseType = true;

						matches.PushLast(funcs[n]);
						continue;
					}

					if( !isMatchWithBaseType )
					{
						// Conversion between signed and unsigned integer is better than between integer and float

						// Is it a match except for sign?
						if( argType->dataType.IsIntegerType() && ti.dataType.IsUnsignedType() ||
							argType->dataType.IsUnsignedType() && ti.dataType.IsIntegerType() )
						{
							if( !isMatchExceptSign ) matches.SetLength(0);

							isMatchExceptSign = true;

							matches.PushLast(funcs[n]);
							continue;
						}

						if( !isMatchExceptSign )
							matches.PushLast(funcs[n]);
					}
				}
			}
		}
	}

	return matches.GetLength();
}

bool asCCompiler::CompileOverloadedOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	// TODO: An operator can be overloaded for an object or can be global

	// What type of operator is it?
	int token = node->tokenType;

	// What overloaded operators of this type do we have?
	asCArray<int> ops;
	int n;
	for( n = 0; n < engine->globalBehaviours.operators.GetLength(); n += 2 )
	{
		if( token == engine->globalBehaviours.operators[n] )
			ops.PushLast(engine->globalBehaviours.operators[n+1]);
	}

	// Find the best matches for each argument
	asCArray<int> ops1;
	asCArray<int> ops2;
	MatchArgument(ops, ops1, &lctx->type, 0);
	MatchArgument(ops, ops2, &rctx->type, 1);

	// Find intersection of the two sets of matching operators
	ops.SetLength(0);
	for( n = 0; n < ops1.GetLength(); n++ )
	{
		for( int m = 0; m < ops2.GetLength(); m++ )
		{
			if( ops1[n] == ops2[m] )
			{
				ops.PushLast(ops1[n]);
				break;
			}
		}
	}

	// Did we find an operator?
	if( ops.GetLength() == 1 )
	{
		asCScriptFunction *descr = engine->systemFunctions[-ops[0] - 1];

		// Add code for arguments
		PrepareArgument(&descr->parameterTypes[0], lctx, node, true, bool(descr->inOutFlags[0] & 1));
		MergeExprContexts(ctx, lctx);
		PrepareArgument(&descr->parameterTypes[1], rctx, node, true, bool(descr->inOutFlags[1] & 1));
		MergeExprContexts(ctx, rctx);
	
		// Swap the order of the arguments
		if( lctx->type.dataType.GetSizeOnStackDWords() == 2 )
			ctx->bc.Instr(BC_SWAP48);
		else if( rctx->type.dataType.GetSizeOnStackDWords() == 2 )
			ctx->bc.Instr(BC_SWAP84);
		else
			ctx->bc.Instr(BC_SWAP4);

		int offset = 0;
		if( descr->parameterTypes[0].isReference )
		{
			if( descr->parameterTypes[0].IsObject() && !descr->parameterTypes[0].isExplicitHandle )
				ctx->bc.InstrWORD(BC_GETOBJREF, 0);
			else
				ctx->bc.InstrWORD(BC_GETREF, 0);
		}
		else if( descr->parameterTypes[0].IsObject() )
		{
			ctx->bc.InstrWORD(BC_GETOBJ, 0);

			// The temporary variable must not be freed as it will no longer hold an object
			DeallocateVariable(rctx->type.stackOffset);
			rctx->type.isTemporary = false;
		}
		offset += descr->parameterTypes[0].GetSizeOnStackDWords();
		if( descr->parameterTypes[1].isReference )
		{
			if( descr->parameterTypes[1].IsObject() && !descr->parameterTypes[1].isExplicitHandle )
				ctx->bc.InstrWORD(BC_GETOBJREF, offset);
			else
				ctx->bc.InstrWORD(BC_GETREF, offset);
		}
		else if( descr->parameterTypes[1].IsObject() )
		{
			ctx->bc.InstrWORD(BC_GETOBJ, offset);

			// The temporary variable must not be freed as it will no longer hold an object
			DeallocateVariable(rctx->type.stackOffset);
			rctx->type.isTemporary = false;

			offset += descr->parameterTypes[1].GetSizeOnStackDWords();
		}

		asCArray<asCTypeInfo> argTypes;
		argTypes.PushLast(lctx->type);
		argTypes.PushLast(rctx->type);
		PerformFunctionCall(descr->id, ctx, false, &argTypes);

		// Don't continue
		return true;
	}
	else if( ops.GetLength() > 1 )
	{
		Error(TXT_MORE_THAN_ONE_MATCHING_OP, node);	

		// Don't continue
		return true;
	}

	// No suitable operator was found
	return false;
}

void asCCompiler::CompileOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	IsVariableInitialized(&lctx->type, node);
	IsVariableInitialized(&rctx->type, node);

	if( lctx->type.dataType.isExplicitHandle || rctx->type.dataType.isExplicitHandle )
	{
		CompileOperatorOnHandles(node, lctx, rctx, ctx);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(lctx->type, &ctx->bc);
		ReleaseTemporaryVariable(rctx->type, &ctx->bc);

		return;
	}
	else
	{
		// Compile an overloaded operator for the two operands
		if( CompileOverloadedOperator(node, lctx, rctx, ctx) )
			return;

		// If either of the types are objects we shouldn't continue
		if( lctx->type.dataType.IsObject() || rctx->type.dataType.IsObject() )
		{
			asCString str;
			str.Format(TXT_NO_MATCHING_OP_FOUND_FOR_TYPE_s, lctx->type.dataType.Format().AddressOf());
			Error(str, node);
			return;
		}

		// Math operators
		// + - * / % += -= *= /= %=
		int op = node->tokenType;
		if( op == ttPlus    || op == ttAddAssign ||
			op == ttMinus   || op == ttSubAssign ||
			op == ttStar    || op == ttMulAssign ||
			op == ttSlash   || op == ttDivAssign ||
			op == ttPercent || op == ttModAssign )
		{
			CompileMathOperator(node, lctx, rctx, ctx);

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(lctx->type, &ctx->bc);
			ReleaseTemporaryVariable(rctx->type, &ctx->bc);

			return;
		}

		// Bitwise operators
		// << >> >>> & | ^ <<= >>= >>>= &= |= ^=
		if( op == ttAmp                || op == ttAndAssign         ||
			op == ttBitOr              || op == ttOrAssign          ||
			op == ttBitXor             || op == ttXorAssign         ||
			op == ttBitShiftLeft       || op == ttShiftLeftAssign   ||
			op == ttBitShiftRight      || op == ttShiftRightLAssign ||
			op == ttBitShiftRightArith || op == ttShiftRightAAssign )
		{
			CompileBitwiseOperator(node, lctx, rctx, ctx);

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(lctx->type, &ctx->bc);
			ReleaseTemporaryVariable(rctx->type, &ctx->bc);

			return;
		}

		// Comparison operators
		// == != < > <= >=
		if( op == ttEqual       || op == ttNotEqual           ||
			op == ttLessThan    || op == ttLessThanOrEqual    ||
			op == ttGreaterThan || op == ttGreaterThanOrEqual )
		{
			CompileComparisonOperator(node, lctx, rctx, ctx);

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(lctx->type, &ctx->bc);
			ReleaseTemporaryVariable(lctx->type, &ctx->bc);

			return;
		}

		// Boolean operators
		// && || ^^
		if( op == ttAnd || op == ttOr || op == ttXor )
		{
			CompileBooleanOperator(node, lctx, rctx, ctx);

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(lctx->type, &ctx->bc);
			ReleaseTemporaryVariable(rctx->type, &ctx->bc);

			return;
		}
	}

	assert(false);
}



void asCCompiler::CompileMathOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext  *rctx, asSExprContext *ctx)
{
	// Implicitly convert the operands to a number type
	asCDataType to;
	if( lctx->type.dataType.IsDoubleType() || rctx->type.dataType.IsDoubleType() )
		to.tokenType = ttDouble;
	else if( lctx->type.dataType.IsFloatType() || rctx->type.dataType.IsFloatType() )
		to.tokenType = ttFloat;
	else if( lctx->type.dataType.IsIntegerType() || rctx->type.dataType.IsIntegerType() )
		to.tokenType = ttInt;
	else if( lctx->type.dataType.IsUnsignedType() || rctx->type.dataType.IsUnsignedType() )
		to.tokenType = ttUInt;
	else if( lctx->type.dataType.IsBitVectorType() || rctx->type.dataType.IsBitVectorType() )
		to.tokenType = ttUInt;

	// Do the actual conversion
	ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);
	ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

	// Verify that the conversion was successful
	if( !lctx->type.dataType.IsIntegerType() &&
		!lctx->type.dataType.IsUnsignedType() &&
		!lctx->type.dataType.IsFloatType() &&
		!lctx->type.dataType.IsDoubleType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_MATH_TYPE, lctx->type.dataType.Format().AddressOf());
		Error(str, node);
	}

	if( !rctx->type.dataType.IsIntegerType() &&
		!rctx->type.dataType.IsUnsignedType() &&
		!rctx->type.dataType.IsFloatType() &&
		!rctx->type.dataType.IsDoubleType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_MATH_TYPE, rctx->type.dataType.Format().AddressOf());
		Error(str, node);
	}

	bool isConstant = lctx->type.isConstant && rctx->type.isConstant;

	int op = node->tokenType;
	if( !isConstant )
	{
		if( op == ttAddAssign || op == ttSubAssign ||
			op == ttMulAssign || op == ttDivAssign ||
			op == ttModAssign )
		{
			// Compound assignments execute the right hand value first
			MergeExprContexts(ctx, rctx);
			MergeExprContexts(ctx, lctx);

			// Swap the values where necessary
			if( op == ttSubAssign || op == ttDivAssign || op == ttModAssign )
			{
				if( lctx->type.dataType.GetSizeOnStackDWords() == 1 )
					ctx->bc.Instr(BC_SWAP4);
				else
					ctx->bc.Instr(BC_SWAP8);
			}
		}
		else
		{
			MergeExprContexts(ctx, lctx);
			MergeExprContexts(ctx, rctx);
		}
	}

	ctx->type.Set(lctx->type.dataType);

	if( lctx->type.dataType.IsIntegerType() ||
		lctx->type.dataType.IsUnsignedType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				ctx->bc.Instr(BC_ADDi);
			else if( op == ttMinus || op == ttSubAssign )
				ctx->bc.Instr(BC_SUBi);
			else if( op == ttStar || op == ttMulAssign )
				ctx->bc.Instr(BC_MULi);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rctx->type.isConstant && rctx->type.intValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_DIVi);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rctx->type.isConstant && rctx->type.intValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_MODi);
			}
		}
		else
		{
			int v = 0;
			if( op == ttPlus )
				v = lctx->type.intValue + rctx->type.intValue;
			else if( op == ttMinus )
				v = lctx->type.intValue - rctx->type.intValue;
			else if( op == ttStar )
				v = lctx->type.intValue * rctx->type.intValue;
			else if( op == ttSlash )
			{
				if( rctx->type.intValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = lctx->type.intValue / rctx->type.intValue;
			}
			else if( op == ttPercent )
			{
				if( rctx->type.intValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = lctx->type.intValue % rctx->type.intValue;
			}

			ctx->bc.InstrINT(BC_SET4, v);

			ctx->type.isConstant = true;
			ctx->type.intValue = v;
		}
	}
	else if( lctx->type.dataType.IsFloatType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				ctx->bc.Instr(BC_ADDf);
			else if( op == ttMinus || op == ttSubAssign )
				ctx->bc.Instr(BC_SUBf);
			else if( op == ttStar || op == ttMulAssign )
				ctx->bc.Instr(BC_MULf);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rctx->type.isConstant && rctx->type.floatValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_DIVf);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rctx->type.isConstant && rctx->type.floatValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_MODf);
			}
		}
		else
		{
			float v = 0.0f;
			if( op == ttPlus )
				v = lctx->type.floatValue + rctx->type.floatValue;
			else if( op == ttMinus )
				v = lctx->type.floatValue - rctx->type.floatValue;
			else if( op == ttStar )
				v = lctx->type.floatValue * rctx->type.floatValue;
			else if( op == ttSlash )
			{
				if( rctx->type.floatValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = lctx->type.floatValue / rctx->type.floatValue;
			}
			else if( op == ttPercent )
			{
				if( rctx->type.floatValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = fmodf(lctx->type.floatValue, rctx->type.floatValue);
			}

			ctx->bc.InstrFLOAT(BC_SET4, v);

			ctx->type.isConstant = true;
			ctx->type.floatValue = v;
		}
	}
	else if( lctx->type.dataType.IsDoubleType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				ctx->bc.Instr(BC_ADDd);
			else if( op == ttMinus || op == ttSubAssign )
				ctx->bc.Instr(BC_SUBd);
			else if( op == ttStar || op == ttMulAssign )
				ctx->bc.Instr(BC_MULd);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rctx->type.isConstant && rctx->type.doubleValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_DIVd);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rctx->type.isConstant && rctx->type.doubleValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				ctx->bc.Instr(BC_MODd);
			}
		}
		else
		{
			double v = 0.0;
			if( op == ttPlus )
				v = lctx->type.doubleValue + rctx->type.doubleValue;
			else if( op == ttMinus )
				v = lctx->type.doubleValue - rctx->type.doubleValue;
			else if( op == ttStar )
				v = lctx->type.doubleValue * rctx->type.doubleValue;
			else if( op == ttSlash )
			{
				if( rctx->type.doubleValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = lctx->type.doubleValue / rctx->type.doubleValue;
			}
			else if( op == ttPercent )
			{
				if( rctx->type.doubleValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = fmod(lctx->type.doubleValue, rctx->type.doubleValue);
			}

			ctx->bc.InstrDOUBLE(BC_SET8, v);

			ctx->type.isConstant = true;
			ctx->type.doubleValue = v;
		}
	}
	else
	{
		// Shouldn't be possible
		assert(false);
	}
}

void asCCompiler::CompileBitwiseOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	int op = node->tokenType;
	if( op == ttAmp    || op == ttAndAssign ||
		op == ttBitOr  || op == ttOrAssign  ||
		op == ttBitXor || op == ttXorAssign )
	{
		// Both operands must be bitvectors of same length
		asCDataType to;
		to.tokenType = ttBits;

		// Do the actual conversion
		ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);
		ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

		// Verify that the conversion was successful
		if( !lctx->type.dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, lctx->type.dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		if( !rctx->type.dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, rctx->type.dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		bool isConstant = lctx->type.isConstant && rctx->type.isConstant;

		if( !isConstant )
		{
			if( op == ttAndAssign || op == ttOrAssign || op == ttXorAssign )
			{
				// Compound assignments execute the right hand value first
				MergeExprContexts(ctx, rctx);
				MergeExprContexts(ctx, lctx);
			}
			else
			{
				MergeExprContexts(ctx, lctx);
				MergeExprContexts(ctx, rctx);
			}
		}

		// Remember the result
		ctx->type.Set(asCDataType(lctx->type.dataType.tokenType, true, false));

		if( !isConstant )
		{
			if( op == ttAmp || op == ttAndAssign )
				ctx->bc.Instr(BC_BAND);
			else if( op == ttBitOr || op == ttOrAssign )
				ctx->bc.Instr(BC_BOR);
			else if( op == ttBitXor || op == ttXorAssign )
				ctx->bc.Instr(BC_BXOR);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttAmp )
				v = lctx->type.dwordValue & rctx->type.dwordValue;
			else if( op == ttBitOr )
				v = lctx->type.dwordValue | rctx->type.dwordValue;
			else if( op == ttBitXor )
				v = lctx->type.dwordValue ^ rctx->type.dwordValue;

			ctx->bc.InstrDWORD(BC_SET4, v);

			// Remember the result
			ctx->type.isConstant = true;
			ctx->type.dwordValue = v;
		}
	}
	else if( op == ttBitShiftLeft       || op == ttShiftLeftAssign   ||
		     op == ttBitShiftRight      || op == ttShiftRightLAssign ||
			 op == ttBitShiftRightArith || op == ttShiftRightAAssign )
	{
		// Left operand must be bitvector
		asCDataType to;
		to.tokenType = ttBits;
		ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);

		// Right operand must be uint
		to.tokenType = ttUInt;
		ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

		// Verify that the conversion was successful
		if( !lctx->type.dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, lctx->type.dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		if( !rctx->type.dataType.IsUnsignedType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, rctx->type.dataType.Format().AddressOf(), "uint");
			Error(str, node);
		}

		bool isConstant = lctx->type.isConstant && rctx->type.isConstant;

		if( !isConstant )
		{
			if( op == ttShiftLeftAssign || op == ttShiftRightLAssign || op == ttShiftRightAAssign )
			{
				// Compound assignments execute the right hand value first
				MergeExprContexts(ctx, rctx);
				MergeExprContexts(ctx, lctx);

				// Swap the values
				ctx->bc.Instr(BC_SWAP4);
			}
			else
			{
				MergeExprContexts(ctx, lctx);
				MergeExprContexts(ctx, rctx);
			}
		}

		// Remember the result
		ctx->type.Set(asCDataType(lctx->type.dataType.tokenType, true, false));

		if( !isConstant )
		{
			if( op == ttBitShiftLeft || op == ttShiftLeftAssign )
				ctx->bc.Instr(BC_BSLL);
			else if( op == ttBitShiftRight || op == ttShiftRightLAssign )
				ctx->bc.Instr(BC_BSRL);
			else if( op == ttBitShiftRightArith || op == ttShiftRightAAssign )
				ctx->bc.Instr(BC_BSRA);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttBitShiftLeft )
				v = lctx->type.dwordValue << rctx->type.dwordValue;
			else if( op == ttBitShiftRight )
				v = lctx->type.dwordValue >> rctx->type.dwordValue;
			else if( op == ttBitShiftRightArith )
				v = lctx->type.intValue >> rctx->type.dwordValue;

			ctx->bc.InstrDWORD(BC_SET4, v);

			// Remember the result
			ctx->type.isConstant = true;
			ctx->type.dwordValue = v;
		}
	}
}

void asCCompiler::CompileComparisonOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	// Both operands must be of the same type

	// Implicitly convert the operands to a number type
	asCDataType to;
	if( lctx->type.dataType.IsDoubleType() || rctx->type.dataType.IsDoubleType() )
		to.tokenType = ttDouble;
	else if( lctx->type.dataType.IsFloatType() || rctx->type.dataType.IsFloatType() )
		to.tokenType = ttFloat;
	else if( lctx->type.dataType.IsIntegerType() || rctx->type.dataType.IsIntegerType() )
		to.tokenType = ttInt;
	else if( lctx->type.dataType.IsUnsignedType() || rctx->type.dataType.IsUnsignedType() )
		to.tokenType = ttUInt;
	else if( lctx->type.dataType.IsBitVectorType() || rctx->type.dataType.IsBitVectorType() )
		to.tokenType = ttUInt;
	else if( lctx->type.dataType.IsBooleanType() || rctx->type.dataType.IsBooleanType() )
		to.tokenType = ttBool;

	// Is it an operation on signed values?
	bool signMismatch = false;
	if( !lctx->type.dataType.IsUnsignedType() || !rctx->type.dataType.IsUnsignedType() )
	{
		if( lctx->type.dataType.tokenType == ttUInt )
		{
			if( !lctx->type.isConstant )
				signMismatch = true;
			else if( lctx->type.dwordValue & (1<<31) )
				signMismatch = true;
		}
		if( rctx->type.dataType.tokenType == ttUInt )
		{
			if( !rctx->type.isConstant )
				signMismatch = true;
			else if( rctx->type.dwordValue & (1<<31) )
				signMismatch = true;
		}
	}

	// Check for signed/unsigned mismatch
	if( signMismatch )
		Warning(TXT_SIGNED_UNSIGNED_MISMATCH, node);

	// Do the actual conversion
	ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);
	ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

	// Verify that the conversion was successful
	if( !lctx->type.dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, lctx->type.dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	if( !rctx->type.dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, rctx->type.dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	bool isConstant = lctx->type.isConstant && rctx->type.isConstant;
	int op = node->tokenType;
	
	ctx->type.Set(asCDataType(ttBool, true, false));

	if( to.IsBooleanType() )
	{
		int op = node->tokenType;
		if( op == ttEqual || op == ttNotEqual )
		{
			if( !isConstant )
			{
				// Make sure they are equal if not false
				lctx->bc.Instr(BC_TNZ);
				rctx->bc.Instr(BC_TNZ);

				MergeExprContexts(ctx, lctx);
				MergeExprContexts(ctx, rctx);

				if( op == ttEqual )
				{
					ctx->bc.Instr(BC_CMPi);
					ctx->bc.Instr(BC_TZ);
				}
				else if( op == ttNotEqual )
					ctx->bc.Instr(BC_CMPi);
			}
			else
			{
				// Make sure they are equal if not false
				if( lctx->type.dwordValue != 0 ) lctx->type.dwordValue = VALUE_OF_BOOLEAN_TRUE;
				if( rctx->type.dwordValue != 0 ) rctx->type.dwordValue = VALUE_OF_BOOLEAN_TRUE;

				asDWORD v = 0;
				if( op == ttEqual )
				{
					v = lctx->type.intValue - rctx->type.intValue;
					if( v == 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;
				}
				else if( op == ttNotEqual )
				{
					v = lctx->type.intValue - rctx->type.intValue;
					if( v != 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;
				}

				ctx->bc.InstrDWORD(BC_SET4, v);

				ctx->type.isConstant = true;
				ctx->type.intValue = v;
			}
		}
		else
		{
			// TODO: Use TXT_ILLEGAL_OPERATION_ON
			Error(TXT_ILLEGAL_OPERATION, node);
		}
	}
	else
	{
		if( !isConstant )
		{
			MergeExprContexts(ctx, lctx);
			MergeExprContexts(ctx, rctx);

			if( lctx->type.dataType.IsIntegerType() )
				ctx->bc.Instr(BC_CMPi);
			else if( lctx->type.dataType.IsUnsignedType() )
				ctx->bc.Instr(BC_CMPui);
			else if( lctx->type.dataType.IsFloatType() )
				ctx->bc.Instr(BC_CMPf);
			else if( lctx->type.dataType.IsDoubleType() )
				ctx->bc.Instr(BC_CMPd);
			else
				assert(false);

			if( op == ttEqual )
				ctx->bc.Instr(BC_TZ);
			else if( op == ttNotEqual )
				ctx->bc.Instr(BC_TNZ);
			else if( op == ttLessThan )
				ctx->bc.Instr(BC_TS);
			else if( op == ttLessThanOrEqual )
				ctx->bc.Instr(BC_TNP);
			else if( op == ttGreaterThan )
				ctx->bc.Instr(BC_TP);
			else if( op == ttGreaterThanOrEqual )
				ctx->bc.Instr(BC_TNS);
		}
		else
		{
			int i = 0;
			if( lctx->type.dataType.IsIntegerType() )
			{
				int v = lctx->type.intValue - rctx->type.intValue;
				if( v < 0 ) i = -1;
				if( v > 0 ) i = 1;
			}
			else if( lctx->type.dataType.IsUnsignedType() )
			{
				asDWORD v1 = lctx->type.dwordValue;
				asDWORD v2 = rctx->type.dwordValue;
				if( v1 < v2 ) i = -1;
				if( v1 > v2 ) i = 1;
			}
			else if( lctx->type.dataType.IsFloatType() )
			{
				float v = lctx->type.floatValue - rctx->type.floatValue;
				if( v < 0 ) i = -1;
				if( v > 0 ) i = 1;
			}
			else if( lctx->type.dataType.IsDoubleType() )
			{
				double v = lctx->type.doubleValue - rctx->type.doubleValue;
				if( v < 0 ) i = -1;
				if( v > 0 ) i = 1;
			}


			if( op == ttEqual )
				i = (i == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttNotEqual )
				i = (i != 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttLessThan )
				i = (i < 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttLessThanOrEqual )
				i = (i <= 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttGreaterThan )
				i = (i > 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttGreaterThanOrEqual )
				i = (i >= 0 ? VALUE_OF_BOOLEAN_TRUE : 0);

			ctx->bc.InstrINT(BC_SET4, i);

			ctx->type.isConstant = true;
			ctx->type.intValue = i;
		}
	}
}

void asCCompiler::CompileBooleanOperator(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	// Both operands must be booleans
	asCDataType to;
	to.tokenType = ttBool;

	// Do the actual conversion
	ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);
	ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

	// Verify that the conversion was successful
	if( !lctx->type.dataType.IsBooleanType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, lctx->type.dataType.Format().AddressOf(), "bool");
		Error(str, node);
	}

	if( !rctx->type.dataType.IsBooleanType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, rctx->type.dataType.Format().AddressOf(), "bool");
		Error(str, node);
	}

	bool isConstant = lctx->type.isConstant && rctx->type.isConstant;

	ctx->type.Set(asCDataType(ttBool, true, false));

	// What kind of operator is it?
	int op = node->tokenType;
	if( op == ttXor )
	{
		if( !isConstant )
		{
			// Make sure they are equal if not false
			lctx->bc.Instr(BC_TNZ);
			rctx->bc.Instr(BC_TNZ);

			MergeExprContexts(ctx, lctx);
			MergeExprContexts(ctx, rctx);

			ctx->bc.Instr(BC_CMPi);
		}
		else
		{
			// Make sure they are equal if not false
			if( lctx->type.dwordValue != 0 ) lctx->type.dwordValue = VALUE_OF_BOOLEAN_TRUE;
			if( rctx->type.dwordValue != 0 ) rctx->type.dwordValue = VALUE_OF_BOOLEAN_TRUE;

			asDWORD v = 0;
			v = lctx->type.intValue - rctx->type.intValue;
			if( v != 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;

			ctx->bc.InstrDWORD(BC_SET4, v);

			ctx->type.isConstant = true;
			ctx->type.intValue = v;
		}
	}
	else if( op == ttAnd ||
			 op == ttOr )
	{
		if( !isConstant )
		{
			// If or-operator and first value is 1 the second value shouldn't be calculated
			// if and-operator and first value is 0 the second value shouldn't be calculated
			MergeExprContexts(ctx, lctx);
			int label1 = nextLabel++;
			int label2 = nextLabel++;
			if( op == ttAnd )
			{
				ctx->bc.InstrINT(BC_JNZ, label1);
				ctx->bc.InstrDWORD(BC_SET4, 0); // Let optimizer change to SET1
				ctx->bc.InstrINT(BC_JMP, label2);
			}
			else if( op == ttOr )
			{
				ctx->bc.InstrINT(BC_JZ, label1);
				ctx->bc.InstrDWORD(BC_SET4, 1); // Let optimizer change to SET1
				ctx->bc.InstrINT(BC_JMP, label2);
			}

			ctx->bc.Label((short)label1);
			MergeExprContexts(ctx, rctx);
			ctx->bc.Label((short)label2);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttAnd )
				v = lctx->type.dwordValue && rctx->type.dwordValue;
			else if( op == ttOr )
				v = lctx->type.dwordValue || rctx->type.dwordValue;

			ctx->bc.InstrDWORD(BC_SET4, v);

			// Remember the result
			ctx->type.isConstant = true;
			ctx->type.dwordValue = v;
		}
	}
}

void asCCompiler::CompileOperatorOnHandles(asCScriptNode *node, asSExprContext *lctx, asSExprContext *rctx, asSExprContext *ctx)
{
	// Dereference the operands
	Dereference(lctx, true);
	Dereference(rctx, true);

	// Implicitly convert null to the other type
	asCDataType to;
	if( lctx->type.IsNullConstant() )
		to = rctx->type.dataType;
	else if( rctx->type.IsNullConstant() )
		to = lctx->type.dataType;
	else
	{
		// TODO: Use the common base type
		to = lctx->type.dataType;
	}

	// Do the conversion
	ImplicitConversion(&lctx->bc, to, &lctx->type, node, false);
	ImplicitConversion(&rctx->bc, to, &rctx->type, node, false);

	// Both operands must be of the same type

	// Verify that the conversion was successful
	if( !lctx->type.dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, lctx->type.dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	if( !rctx->type.dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, rctx->type.dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	ctx->type.Set(asCDataType(ttBool, true, false));

	int op = node->tokenType;
	if( op == ttEqual || op == ttNotEqual )
	{
		MergeExprContexts(ctx, lctx);
		MergeExprContexts(ctx, rctx);

		if( op == ttEqual )
		{
			ctx->bc.Instr(BC_CMPi);
			ctx->bc.Instr(BC_TZ);
		}
		else if( op == ttNotEqual )
			ctx->bc.Instr(BC_CMPi);
	}
	else
	{
		// TODO: Use TXT_ILLEGAL_OPERATION_ON
		Error(TXT_ILLEGAL_OPERATION, node);
	}
}


void asCCompiler::PerformFunctionCall(int funcID, asSExprContext *ctx, bool isConstructor, asCArray<asCTypeInfo> *argTypes, asCScriptNode *argListNode)
{
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	int argSize = descr->GetSpaceNeededForArguments();

	ctx->type.Set(descr->returnType);

	// Script functions should be referenced locally (i.e module id == 0)
	if( descr->id < 0 )
	{
		if( !isConstructor )
			ctx->bc.Call(BC_CALLSYS, descr->id, argSize + (descr->objectType ? 1 : 0));
		else
			ctx->bc.Alloc(BC_ALLOC, engine->GetObjectTypeIndex(descr->objectType), descr->id, argSize+1);
	}
	else
	{
		if( descr->id & FUNC_IMPORTED )
			ctx->bc.Call(BC_CALLBND, descr->id, argSize + (descr->objectType ? 1 : 0));
		else
			ctx->bc.Call(BC_CALL, descr->id, argSize + (descr->objectType ? 1 : 0));
	}

	if( ctx->type.dataType.IsObject() && !descr->returnType.isReference )
	{
		// Allocate a temporary variable for the returned object
		int returnOffset = AllocateVariable(descr->returnType, true);
		ctx->type.isTemporary = true;
		ctx->type.stackOffset = (short)returnOffset;
		ctx->type.dataType.isReference = true;
		ctx->type.dataType.isObjectHandle = ctx->type.dataType.isExplicitHandle;
		ctx->type.dataType.isExplicitHandle = false;

		// Move the pointer from the object register to the temporary variable
		ctx->bc.InstrSHORT(BC_STOREOBJ, returnOffset);

		// Clean up arguments
		if( argTypes )
			AfterFunctionCall(funcID, argListNode, ctx, *argTypes);

		ProcessDeferredOutParams(ctx);

		// Push the temporary variable reference on the stack
		ctx->bc.InstrINT(BC_VAR, returnOffset);
		ctx->bc.InstrWORD(BC_GETREF, 0);
	}
	else if( descr->returnType.isReference )
	{
		// Clean up arguments
		if( argTypes )
			AfterFunctionCall(funcID, argListNode, ctx, *argTypes);

		// Do not process the output parameters yet, because it 
		// might invalidate the returned reference

		ctx->bc.Instr(BC_RRET4);
		if( descr->returnType.IsObject() )
		{
			// We are getting the pointer to the object 
			// not a pointer to a object variable
			ctx->type.dataType.isReference = false;
		}
	}
	else 
	{
		if( descr->returnType.GetSizeOnStackDWords() == 1 )
		{
			ctx->bc.Instr(BC_RRET4);
			if( !descr->returnType.isReference )
			{
				if( descr->returnType.GetSizeInMemoryBytes() == 1 )
				{
					if( descr->returnType.IsIntegerType() )
						ctx->bc.Instr(BC_SB);
					else
						ctx->bc.Instr(BC_UB);
				}
				else if( descr->returnType.GetSizeInMemoryBytes() == 2 )
				{
					if( descr->returnType.IsIntegerType() )
						ctx->bc.Instr(BC_SW);
					else
						ctx->bc.Instr(BC_UW);
				}
			}
		}
		else if( descr->returnType.GetSizeOnStackDWords() == 2 )
			ctx->bc.Instr(BC_RRET8);

		// Clean up arguments
		if( argTypes )
			AfterFunctionCall(funcID, argListNode, ctx, *argTypes);

		ProcessDeferredOutParams(ctx);
	}
}


void asCCompiler::MergeExprContexts(asSExprContext *before, asSExprContext *after)
{
	before->bc.AddCode(&after->bc);

	for( int n = 0; n < after->deferredOutParams.GetLength(); n++ )
		before->deferredOutParams.PushLast(after->deferredOutParams[n]);

	after->deferredOutParams.SetLength(0);
}

void asCCompiler::FilterConst(asCArray<int> &funcs)
{
	if( funcs.GetLength() == 0 ) return;

	// This is only done for object methods
	asCScriptFunction *desc = builder->GetFunctionDescription(funcs[0]);
	if( desc->objectType == 0 ) return;

	// Check if there are any non-const matches
	int n;
	bool foundNonConst = false;
	for( n = 0; n < funcs.GetLength(); n++ )
	{
		desc = builder->GetFunctionDescription(funcs[n]);
		if( !desc->isReadOnly )
		{
			foundNonConst = true;
			break;
		}
	}

	if( foundNonConst )
	{
		// Remove all const methods
		for( n = 0; n < funcs.GetLength(); n++ )
		{
			desc = builder->GetFunctionDescription(funcs[n]);
			if( desc->isReadOnly )
			{
				if( n == funcs.GetLength() - 1 )
					funcs.PopLast();
				else
					funcs[n] = funcs.PopLast();

				n--;
			}
		}
	}
}


