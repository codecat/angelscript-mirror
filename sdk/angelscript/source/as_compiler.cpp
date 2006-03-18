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
	cleanCode.ClearAll();

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
	returnType = builder->ModifyDataTypeFromNode(returnType, func->firstChild->next);

	// If the return type is larger than a dword the address
	// to the memory where to store the value is sent as first 
	// argument. And the function should return the same address

	// If the return type is primitive it should always be on the stack (e.g double)

	// If the return type has behaviours it should be treated just as a large object
	if( returnType.IsComplex(engine) && !returnType.isReference )
	{
		returnType.isReference = true;

		variables->DeclareVariable("return address", returnType, stackPos);

		stackPos--;
	}

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
		node = node->next;

		type = builder->ModifyDataTypeFromNode(type, node);
		node = node->next;

		// Is the data type allowed?
		if( type.GetSizeOnStackDWords() == 0 )
		{
			asCString str;
			str.Format(TXT_PARAMETER_CANT_BE_s, (const char *)type.Format());
			Error(str, node);
		}

		// If the parameter has a name then declare it as variable
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

		// Add destructors to exception handler
		if( !vs.variables[n]->type.isReference )
		{
			asSTypeBehaviour *beh = builder->GetBehaviour(&vs.variables[n]->type);
			if( beh && beh->destruct )
				byteCode.Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, vs.variables[n]->stackOffset);
		}
	}

	variables->DeclareVariable("return", returnType, stackPos);

	//--------------------------------------------
	// Compile the statement block
	bool hasReturn;
	asCByteCode bc;
	CompileStatementBlock(func->lastChild, false, &hasReturn, &bc);

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

	// Generate the exception handler routine from byte code
	byteCode.GenerateExceptionHandler(cleanCode);

	// Finalize the bytecode
	byteCode.Finalize();

	if( hasCompileErrors ) return -1;

	return 0;
}

void asCCompiler::DefaultConstructor(asCByteCode *bc, asCDataType &type)
{
	asSTypeBehaviour *beh = engine->GetBehaviour(&type);
	if( type.IsDefaultArrayType(engine) )
	{
		int defaultArrays = 0;
		asCDataType sub = type;
		while( sub.IsDefaultArrayType(engine) )
		{
			defaultArrays++;
			sub = sub.GetSubType(engine);
		}
		
		// The default array object needs some extra data
		bc->InstrDWORD(BC_SET4, engine->GetBehaviourIndex(&sub)); // Behaviour index
		bc->InstrDWORD(BC_SET4, sub.GetSizeInMemoryBytes() | (defaultArrays << 24));  // Element size and arraydimensions
		bc->Call(BC_CALLSYS, (asDWORD)beh->construct, 3);
	}
	else
	{
		bc->Call(BC_CALLSYS, (asDWORD)beh->construct, 1);
	}
}

void asCCompiler::CompileConstructor(asCDataType &type, int offset, asCByteCode *bc)
{
	// Call constructor for the data type
	asSTypeBehaviour *beh = builder->GetBehaviour(&type);
	if( beh && beh->construct )
	{
		bc->InstrSHORT(BC_PSF, (short)offset);
		DefaultConstructor(bc, type);
	}
	else
	{
		// Pointers have a default constructor that set them to 0
		if( type.pointerLevel > 0 )
		{
			bc->InstrDWORD(BC_SET4, 0); 
			bc->InstrSHORT(BC_PSF, (short)offset);
			bc->Instr(BC_MOV4);
		}
	}

	// Add a destructor for the exception handler
	if( beh && beh->destruct )
		bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, offset);
}

void asCCompiler::CompileDestructor(asCDataType &type, int offset, asCByteCode *bc)
{
	if( !type.isReference )
	{
		// Call destructor for the data type
		asSTypeBehaviour *beh = builder->GetBehaviour(&type);
		if( beh && beh->destruct )
		{
			bc->InstrSHORT(BC_PSF, (short)offset);
			bc->Call(BC_CALLSYS, (asDWORD)beh->destruct, 1);

			// Remove destructor call in the exception cleanup code
			bc->Destructor(BC_REMDESTRSF, (asDWORD)beh->destruct, offset);
		}
	}
}

void asCCompiler::LineInstr(asCByteCode *bc, int pos)
{
	int r;
	script->ConvertPosToRowCol(pos, &r, 0);
	bc->Line(r);
}

void asCCompiler::CompileStatementBlock(asCScriptNode *block, bool ownVariableScope, bool *hasReturn, asCByteCode *bc)
{
	*hasReturn = false;
	bool isFinished = false;
	bool hasWarned = false;

	if( ownVariableScope )
		AddVariableScope();

	// Add a line instruction at the start of the block
	LineInstr(bc, block->tokenPos);

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

		if( node->nodeType == snDeclaration )
			CompileDeclaration(node, bc);
		else
			CompileStatement(node, hasReturn, bc);

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

	asCByteCode bc;

	// Compile the expression
	if( node && node->nodeType == snArgList )
	{
		// Make sure that it is a registered type, and that it isn't a pointer
		if( gvar->datatype.objectType == 0 )
		{
			Error(TXT_MUST_BE_OBJECT, node);
		}
		else
		{
			// Compile the arguments
			asCArray<asCTypeInfo> argTypes;
			asCArray<asCByteCode *> argByteCodes;
			asCArray<asCTypeInfo> temporaryVariables;
			int argCount;
			
			CompileArgumentList(node, argTypes, argByteCodes, &gvar->datatype);
			argCount = argTypes.GetLength();

			// Take a copy of the argument types so that 
			// we can free the temporary variables later on
			temporaryVariables = argTypes;

			// Find all constructors
			asCArray<int> funcs;
			asSTypeBehaviour *beh = engine->GetBehaviour(&gvar->datatype);
			if( beh )
				funcs = beh->constructors;

			asCString str = gvar->datatype.Format();
			MatchFunctions(funcs, argTypes, node, str);
			
			if( funcs.GetLength() == 1 )
			{
				bc.InstrINT(BC_PGA, gvar->index);

				PrepareFunctionCall(funcs[0], node, &bc, argTypes, argByteCodes);

				asCTypeInfo type;
				PerformFunctionCall(funcs[0], &type, &bc);

				// Release all temporary variables used by arguments
				for( int n = 0; n < temporaryVariables.GetLength(); n++ )
					ReleaseTemporaryVariable(temporaryVariables[n], &bc);
			}

			// Cleanup
			for( int n = 0; n < argCount; n++ )
				if( argByteCodes[n] ) delete argByteCodes[n];
		}
	}
	else
	{
		// Call constructor for all data types
		asSTypeBehaviour *beh = engine->GetBehaviour(&gvar->datatype);
		if( beh && beh->construct )
		{
			bc.InstrINT(BC_PGA, gvar->index);
			DefaultConstructor(&bc, gvar->datatype);
		}
		else
		{
			// Default constructor for pointers
			if( gvar->datatype.pointerLevel > 0 )
			{
				bc.InstrDWORD(BC_SET4, 0); // Let optimizer change to SET1 
				bc.InstrINT(BC_PGA, gvar->index);
				bc.Instr(BC_MOV4);
			}
		}

		if( node )
		{
			asCByteCode exprBC;
			asCTypeInfo exprType;
			CompileAssignment(node, &exprBC, &exprType);

			PrepareForAssignment(&gvar->datatype, &exprType, &exprBC, node);

			// Add expression code to bytecode
			bc.AddCode(&exprBC);

			// Add byte code for storing value of expression in variable
			bc.InstrINT(BC_PGA, gvar->index);

			asCTypeInfo ltype;
			ltype.Set(gvar->datatype);
			ltype.dataType.isReference = true;
			ltype.stackOffset = -1;
			ltype.dataType.isReadOnly = false;

			PerformAssignment(&ltype, &bc, node);

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(exprType, &bc);

			bc.Pop(exprType.dataType.GetSizeOnStackDWords());
		}
	}

	// Concatenate the bytecode
	int varSize = GetVariableOffset(variableAllocations.GetLength()) - 1;
	byteCode.Push(varSize);
	byteCode.AddCode(&bc);
	
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

void asCCompiler::PrepareFunctionCall(int funcID, asCScriptNode *argListNode, asCByteCode *bc, asCArray<asCTypeInfo> &argTypes, asCArray<asCByteCode *> &argByteCodes)
{
	// When a match has been found, compile the final byte code using correct parameter types
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	// Add code for arguments
	int argSize = 0, n;
	asCScriptNode *arg = argListNode->lastChild;
	for( n = argTypes.GetLength()-1; n >= 0; n-- )
	{
		int size = ReserveSpaceForArgument(descr->parameterTypes[n], bc, false);

		bc->AddCode(argByteCodes[n]);

		// Implictly convert the value to the parameter type
		ImplicitConversion(bc, descr->parameterTypes[n], &argTypes[n], arg, false);

		MoveArgumentToReservedSpace(descr->parameterTypes[n], &argTypes[n], arg, bc, size > 0, 0);
		
		argSize += descr->parameterTypes[n].GetSizeOnStackDWords();

		if( arg )
			arg = arg->prev;
	}

	// Remove destructors for the arguments just before the call
	int offset = 0;
	for( n = 0; n < descr->parameterTypes.GetLength(); n++ )
	{
		if( !descr->parameterTypes[n].isReference )
		{
			asSTypeBehaviour *beh = builder->GetBehaviour(&descr->parameterTypes[n]);
			if( beh && beh->destruct )
				bc->Destructor(BC_REMDESTRSP, (asDWORD)beh->destruct, offset);
		}

		offset -= descr->parameterTypes[n].GetSizeOnStackDWords();
	}

}

void asCCompiler::CompileArgumentList(asCScriptNode *node, asCArray<asCTypeInfo> &argTypes, asCArray<asCByteCode *> &argByteCodes, asCDataType *type)
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
	argTypes.SetLength(argCount);
	argByteCodes.SetLength(argCount);
	int n;
	for( n = 0; n < argCount; n++ )
		argByteCodes[n] = 0;

	n = argCount-1;
	if( type && type->IsDefaultArrayType(engine) )
	{
		int defaultArrays = 0;
		asCDataType sub = *type;
		while( sub.IsDefaultArrayType(engine) )
		{
			defaultArrays++;
			sub = sub.GetSubType(engine);
		}

		// Pass base types behaviour index and size, also pass the array dimension
		argByteCodes[n] = new asCByteCode;
		argByteCodes[n]->InstrDWORD(BC_SET4, engine->GetBehaviourIndex(&sub)); // Behaviour index
		argTypes[n].Set(asCDataType(ttInt, false, false));
		n--;
		argByteCodes[n] = new asCByteCode;
		argByteCodes[n]->InstrDWORD(BC_SET4, sub.GetSizeInMemoryBytes() | (defaultArrays << 24)); // Element size
		argTypes[n].Set(asCDataType(ttInt, false, false));
		n--;
	}

	// Compile the arguments in reverse order (as they will be pushed on the stack)
	arg = node->lastChild;
	while( arg )
	{
		argByteCodes[n] = new asCByteCode;
		CompileAssignment(arg, argByteCodes[n], &argTypes[n]);

		IsVariableInitialized(&argTypes[n], arg);

		n--;
		arg = arg->prev;
	}
}

void asCCompiler::MatchFunctions(asCArray<int> &funcs, asCArray<asCTypeInfo> &argTypes, asCScriptNode *node, const char *name)
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

		if( funcs.GetLength() == 0 )
			str.Format(TXT_NO_MATCHING_SIGNATURES_TO_s, (const char *)str);
		else
			str.Format(TXT_MULTIPLE_MATCHING_SIGNATURES_TO_s, (const char *)str);

		Error(str, node);
	}
}

void asCCompiler::CompileDeclaration(asCScriptNode *decl, asCByteCode *bc)
{
	// Add a line instruction
	LineInstr(bc, decl->tokenPos);

	// Get the data type
	asCDataType dataType = builder->CreateDataTypeFromNode(decl->firstChild, script);

	// Declare all variables in this declaration
	asCScriptNode *node = decl->firstChild->next;
	while( node )
	{
		asCDataType type = builder->ModifyDataTypeFromNode(dataType, node);

		node = node->next;

		// Is the type allowed?
		if( type.GetSizeOnStackDWords() == 0 || type.isReference )
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
			if( type.objectType == 0 )
			{
				Error(TXT_MUST_BE_OBJECT, node);
			}
			else
			{
				// Compile the arguments
				asCArray<asCTypeInfo> argTypes;
				asCArray<asCByteCode *> argByteCodes;
				asCArray<asCTypeInfo> temporaryVariables;
				int argCount;
				
				CompileArgumentList(node, argTypes, argByteCodes, &type);
				argCount = argTypes.GetLength();

				// Take a copy of the argument types so that 
				// we can free the temporary variables later on
				temporaryVariables = argTypes;

				// Find all constructors
				asCArray<int> funcs;
				asSTypeBehaviour *beh = engine->GetBehaviour(&type);
				if( beh )
					funcs = beh->constructors;

				asCString str = type.Format();
				MatchFunctions(funcs, argTypes, node, str);

				if( funcs.GetLength() == 1 )
				{
					sVariable *v = variables->GetVariable(name);
					bc->InstrSHORT(BC_PSF, (short)v->stackOffset);

					PrepareFunctionCall(funcs[0], node, bc, argTypes, argByteCodes);

					asCTypeInfo type;
					PerformFunctionCall(funcs[0], &type, bc);

					// Add a destructor for the exception handler
					if( beh && beh->destruct )
						bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, v->stackOffset);

					// Release all temporary variables used by arguments
					for( int n = 0; n < temporaryVariables.GetLength(); n++ )
						ReleaseTemporaryVariable(temporaryVariables[n], bc);
				}

				// Cleanup
				for( int n = 0; n < argCount; n++ )
					if( argByteCodes[n] ) delete argByteCodes[n];
			}

			node = node->next;
		}
		else
		{
			// Call the default constructor here
			CompileConstructor(type, offset, bc);
			
			// Is the variable initialized?
			if( node && node->nodeType == snAssignment )
			{
				// TODO: We can use a copy constructor here

				// Compile the expression
				asCByteCode exprBC;
				asCTypeInfo exprType;
				CompileAssignment(node, &exprBC, &exprType);

				PrepareForAssignment(&type, &exprType, &exprBC, node);

				// Add expression code to bytecode
				bc->AddCode(&exprBC);

				// Add byte code for storing value of expression in variable
				sVariable *v = variables->GetVariable(name);
				bc->InstrSHORT(BC_PSF, (short)v->stackOffset);

				asCTypeInfo ltype;
				ltype.Set(v->type);
				ltype.dataType.isReference = true;
				ltype.stackOffset = (short)v->stackOffset;
				// Allow initialization of constant variables
				ltype.dataType.isReadOnly = false;

				PerformAssignment(&ltype, bc, node->prev);

				// Release temporary variables used by expression
				ReleaseTemporaryVariable(exprType, bc);

				bc->Pop(exprType.dataType.GetSizeOnStackDWords());

				node = node->next;
			}
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

	// Add a line instruction
	asCByteCode exprBC;
	LineInstr(&exprBC, snode->tokenPos);

	// Compile the switch expression
	asCTypeInfo exprType;
	CompileAssignment(snode->firstChild, &exprBC, &exprType);
	PrepareOperand(&exprType, &exprBC, snode->firstChild);

	// Verify that the expression is a primitive type
	if( !exprType.dataType.IsIntegerType() && !exprType.dataType.IsUnsignedType() )
	{
		Error(TXT_SWITCH_MUST_BE_INTEGRAL, snode->firstChild);
		return;
	}

	// Store the value in a temporary variable so that it can be used several times
	asCTypeInfo type;
	type.Set(exprType.dataType);
	// Make sure the data type is 32 bit
	if( type.dataType.IsIntegerType() ) type.dataType.tokenType = ttInt;
	if( type.dataType.IsUnsignedType() ) type.dataType.tokenType = ttUInt;
	type.dataType.isReference = false;
	type.dataType.isReadOnly = false;
	int offset = AllocateVariable(type.dataType, true);
	type.isTemporary = true;
	type.stackOffset = offset;
	
	PrepareForAssignment(&type.dataType, &exprType, &exprBC, snode->firstChild);

	exprBC.InstrSHORT(BC_PSF, offset);
	type.dataType.isReference = true;

	PerformAssignment(&type, &exprBC, snode->firstChild);

	// Release temporary variable used by expression
	ReleaseTemporaryVariable(exprType, &exprBC);

	exprBC.Pop(exprType.dataType.GetSizeOnStackDWords());

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
			asCTypeInfo ctype;
			asCByteCode cbc;
			CompileExpression(cnode->firstChild, &cbc, &ctype);

			// Verify that the result is a constant
			if( !ctype.isConstant )
				Error(TXT_SWITCH_CASE_MUST_BE_CONSTANT, cnode->firstChild);

			// Verify that the result is an integral number
			if( !ctype.dataType.IsIntegerType() && !ctype.dataType.IsUnsignedType() )
				Error(TXT_SWITCH_MUST_BE_INTEGRAL, cnode->firstChild);

			// Store constant for later use
			caseValues.PushLast(ctype.intValue);

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
	exprBC.InstrSHORT(BC_PSF, offset);
	exprBC.Instr(BC_RD4);
	exprBC.InstrINT(BC_SET4, caseValues[caseValues.GetLength()-1]);
	exprBC.Instr(BC_CMPi);
	exprBC.Instr(BC_TP);
	exprBC.InstrINT(BC_JNZ, defaultLabel);

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
			exprBC.InstrSHORT(BC_PSF, offset);
			exprBC.Instr(BC_RD4);
			exprBC.InstrINT(BC_SET4, caseValues[ranges[range]]);
			exprBC.Instr(BC_CMPi);
			exprBC.Instr(BC_TS);
			exprBC.InstrINT(BC_JNZ, defaultLabel);

			int nextRangeLabel = nextLabel++;
			// If this is the last range we don't have to make this test
			if( range < ranges.GetLength() - 1 )
			{
				// If the value is larger than the largest case value in the range, jump to the next range
				exprBC.InstrSHORT(BC_PSF, offset);
				exprBC.Instr(BC_RD4);
				exprBC.InstrINT(BC_SET4, maxRange);
				exprBC.Instr(BC_CMPi);
				exprBC.Instr(BC_TP);
				exprBC.InstrINT(BC_JNZ, nextRangeLabel);
			}

			// Jump forward according to the value
			exprBC.InstrSHORT(BC_PSF, offset);
			exprBC.Instr(BC_RD4);
			exprBC.InstrINT(BC_SET4, caseValues[ranges[range]]);
			exprBC.Instr(BC_SUBi);
			exprBC.JmpP(maxRange - caseValues[ranges[range]]);

			// Add the list of jumps to the correct labels (any holes, jump to default)
			index = ranges[range];
			for( n = caseValues[index]; n <= maxRange; n++ )
			{
				if( caseValues[index] == n )
					exprBC.InstrINT(BC_JMP, caseLabels[index++]);
				else
					exprBC.InstrINT(BC_JMP, defaultLabel);
			}

			exprBC.Label(nextRangeLabel);
		}
		else
		{
			// Simply make a comparison with each value
			int n;
			for( n = ranges[range]; n < index; ++n )
			{
				exprBC.InstrSHORT(BC_PSF, offset);
				exprBC.Instr(BC_RD4);
				exprBC.InstrINT(BC_SET4, caseValues[n]);
				exprBC.Instr(BC_CMPi);
				exprBC.Instr(BC_TZ);
				exprBC.InstrINT(BC_JNZ, caseLabels[n]);
			}
		}
	}

	// Catch any value that falls trough
	exprBC.InstrINT(BC_JMP, defaultLabel);

	// Release the temporary variable previously stored
	ReleaseTemporaryVariable(type, &exprBC);

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
			exprBC.Label(firstCaseLabel++);

			CompileCase(cnode->firstChild->next, &exprBC);
		}
		else
		{
			exprBC.Label(defaultLabel);

			// Is default the last case?
			if( cnode->next )
			{
				// We've already reported this error
				break;
			}

			CompileCase(cnode->firstChild, &exprBC);
		}

		cnode = cnode->next;
	}	

	//--------------------------------

	bc->AddCode(&exprBC);

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

		CompileStatement(node, &hasReturn, bc);

		if( !hasCompileErrors )
			assert( tempVariables.GetLength() == 0 );
		
		node = node->next;
	}
}

void asCCompiler::CompileIfStatement(asCScriptNode *inode, bool *hasReturn, asCByteCode *bc)
{
	// Add a line instruction
	LineInstr(bc, inode->tokenPos);

	// We will use one label for the if statement
	// and possibly another for the else statement
	int afterLabel = nextLabel++;

	// Compile the expression
	asCByteCode exprBC;
	asCTypeInfo exprType;
	CompileAssignment(inode->firstChild, &exprBC, &exprType);
	PrepareOperand(&exprType, &exprBC, inode->firstChild);
	if( exprType.dataType != asCDataType(ttBool, true, false) )
		Error(TXT_EXPR_MUST_BE_BOOL, inode->firstChild);

	if( !exprType.isConstant )
	{
		// Add byte code from the expression
		bc->AddCode(&exprBC);

		// Add a test
		bc->InstrINT(BC_JZ, afterLabel);
	}
	else if( exprType.dwordValue == 0 )
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

	// Add a line instruction
	LineInstr(bc, fnode->tokenPos);
	
	//---------------------------------------
	// Compile the initialization statement
	asCByteCode initBC;
	asCTypeInfo exprType;

	if( fnode->firstChild->nodeType == snDeclaration )
		CompileDeclaration(fnode->firstChild, &initBC);
	else
		CompileExpressionStatement(fnode->firstChild, &initBC);

	//-----------------------------------
	// Compile the condition statement
	asCByteCode exprBC;
	asCScriptNode *second = fnode->firstChild->next;
	if( second->firstChild )
	{
		CompileAssignment(second->firstChild, &exprBC, &exprType);
		PrepareOperand(&exprType, &exprBC, second);
		if( exprType.dataType != asCDataType(ttBool, true, false) )
			Error(TXT_EXPR_MUST_BE_BOOL, second);

		// If expression is false exit the loop
		exprBC.InstrINT(BC_JZ, afterLabel);
	}

	//---------------------------
	// Compile the increment statement
	asCByteCode nextBC;
	asCScriptNode *third = second->next;
	if( third->nodeType == snAssignment )
	{
		CompileAssignment(third, &nextBC, &exprType);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(exprType, &nextBC);

		// Pop the value from the stack
		nextBC.Pop(exprType.dataType.GetSizeOnStackDWords()); 
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

	bc->AddCode(&exprBC);
	bc->AddCode(&forBC);
	bc->Label((short)continueLabel);
	bc->AddCode(&nextBC);
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

	// Add a line instruction
	LineInstr(bc, wnode->tokenPos);

	// Compile expression
	asCByteCode exprBC;
	asCTypeInfo exprType;
	CompileAssignment(wnode->firstChild, &exprBC, &exprType);
	PrepareOperand(&exprType, &exprBC, wnode->firstChild);
	if( exprType.dataType != asCDataType(ttBool, true, false) )
		Error(TXT_EXPR_MUST_BE_BOOL, wnode->firstChild);

	// Add byte code for the expression
	bc->AddCode(&exprBC);

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
	bc->AddCode(&whileBC);

	// Add label before the expression
	bc->Label((short)beforeTest);

	// Add a suspend bytecode inside the loop to guarantee  
	// that the application can suspend the execution
	bc->Instr(BC_SUSPEND);

	// Add a line instruction
	LineInstr(bc, wnode->lastChild->tokenPos);

	// Compile expression
	asCByteCode exprBC;
	asCTypeInfo exprType;
	CompileAssignment(wnode->lastChild, &exprBC, &exprType);
	PrepareOperand(&exprType, &exprBC, wnode->lastChild);
	if( exprType.dataType != asCDataType(ttBool, true, false) )
		Error(TXT_EXPR_MUST_BE_BOOL, wnode->firstChild);

	// Add byte code for the expression
	bc->AddCode(&exprBC);

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
	// Add a line instruction
	LineInstr(bc, node->tokenPos);

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

	// Add the destructors again so that the exception handler can scan the code linearly
	vs = variables;
	while( !vs->isBreakScope ) 
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
		{
			// Add a destructor for the exception handler
			asSTypeBehaviour *beh = builder->GetBehaviour(&vs->variables[n]->type);
			if( beh && beh->destruct )
				bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, vs->variables[n]->stackOffset);
		}

		vs = vs->parent;
	} 
}

void asCCompiler::CompileContinueStatement(asCScriptNode *node, asCByteCode *bc)
{
	// Add a line instruction
	LineInstr(bc, node->tokenPos);

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

	// Add the destructors again so that the exception handler can scan the code linearly
	vs = variables;
	while( !vs->isContinueScope )
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
		{
			// Add a destructor for the exception handler
			asSTypeBehaviour *beh = builder->GetBehaviour(&vs->variables[n]->type);
			if( beh && beh->destruct )
				bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, vs->variables[n]->stackOffset);
		}

		vs = vs->parent;
	}	
}

void asCCompiler::CompileExpressionStatement(asCScriptNode *enode, asCByteCode *bc)
{
	// Add a line instruction
	LineInstr(bc, enode->tokenPos);

	asCTypeInfo exprType;
	if( enode->firstChild )
	{
		// Compile the expression 
		CompileAssignment(enode->firstChild, bc, &exprType);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(exprType, bc);

		// Pop the value from the stack
		bc->Pop(exprType.dataType.GetSizeOnStackDWords()); 
	}
}

void asCCompiler::CompileReturnStatement(asCScriptNode *rnode, asCByteCode *bc)
{
	int returnDestruct = 0;

	// Add a line instruction
	LineInstr(bc, rnode->tokenPos);

	// Get return type and location
	sVariable *v = variables->GetVariable("return");
	if( v->type.GetSizeOnStackDWords() > 0 )
	{
		// Is there an expression?
		if( rnode->firstChild )
		{
			// Compile the expression
			asCTypeInfo exprType;
			CompileAssignment(rnode->firstChild, bc, &exprType);

			// Prepare the value for assignment
			IsVariableInitialized(&exprType, rnode->firstChild);
			PrepareForAssignment(&v->type, &exprType, bc, rnode->firstChild);

			// Is the return a large type
			sVariable *a = variables->GetVariable("return address");
			if( a )
			{
				// Call the constructor 
				asSTypeBehaviour *beh = builder->GetBehaviour(&v->type);
				if( beh && beh->construct )
				{
					bc->InstrSHORT(BC_PSF, 0);
					bc->Instr(BC_RD4);
					DefaultConstructor(bc, v->type);
				}

				// TODO: This is not necessary with a copy constructor
				// This destructor is added to the exception handler in case the copy 
				// throws an exception, it must be removed again before returning as 
				// the caller will become the owner of the object

				if(	beh && beh->destruct )
				{
					// The address to the object is absolute, not on the stack
					bc->Destructor(BC_ADDDESTRASF, (asDWORD)beh->destruct, 0);
					returnDestruct = (asDWORD)beh->destruct;
				}

				// Copy the value to the address given
				bc->InstrSHORT(BC_PSF, (short)a->stackOffset);
				bc->Instr(BC_RD4);

				// Use the copy behaviour, if available
				if( beh && beh->copy )
				{
					// The object reference should be the last parameter
					bc->Instr(BC_SWAP4);
					bc->Call(BC_CALLSYS, (asDWORD)beh->copy, 2);
				}
				else
				{
					bc->InstrWORD(BC_COPY, (short)a->type.GetSizeInMemoryDWords());
					bc->Pop(1); // Pop the reference to original value
				}

				// Push the address to the space where the value was stored
				bc->InstrSHORT(BC_PSF, (short)a->stackOffset);
				bc->Instr(BC_RD4);
			}

			// Release temporary variables used by expression
			ReleaseTemporaryVariable(exprType, bc);

			// double should also be returned on the stack (because it is a primitive)

			// Move the return value to the returnVal register
			if( v->type.GetSizeOnStackDWords() == 1 )
				bc->Instr(BC_SRET4);
			else
				bc->Instr(BC_SRET8);
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

	// TODO: This is not necessary when copy constructors are supported
	// Remove the destructor for the return value
	if( returnDestruct )
	{
		// The location of the object is absolute, not relative to the stack frame
		bc->Destructor(BC_REMDESTRASF, returnDestruct, 0);
	}

	// Jump to the end of the function
	bc->InstrINT(BC_JMP, 0);

	// Add the destructors again so that the exception handler can scan the code linearly
	vs = variables;
	while( vs )
	{
		for( int n = vs->variables.GetLength() - 1; n >= 0; n-- )
		{
			if( vs->variables[n]->stackOffset > 0 )
			{
				// Add a destructor for the exception handler
				asSTypeBehaviour *beh = builder->GetBehaviour(&vs->variables[n]->type);
				if( beh && beh->destruct )
					bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, vs->variables[n]->stackOffset);
			}
		}

		vs = vs->parent;
	}	
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
		// If the variable is a reference, then  
		// we should release the real variable
		asCDataType dt = t.dataType;
		dt.isReference = false;

		// Call destructor
		CompileDestructor(dt, t.stackOffset, bc);

		DeallocateVariable(t.stackOffset);
		t.isTemporary = false;
	}
}

void asCCompiler::Dereference(asCByteCode *bc, asCTypeInfo *type)
{
	if( type->dataType.isReference )
	{
		if( type->dataType.GetSizeInMemoryDWords() == 1 )
		{
			type->dataType.isReference = false;
			type->dataType.isReadOnly = true;

			if( bc )
			{
				bc->Instr(BC_RD4);

				if( type->dataType.IsIntegerType() || type->dataType.IsEqualExceptRefAndConst(asCDataType(ttBool, false, false)) )
				{
					if( type->dataType.GetSizeInMemoryBytes() == 1 )
						bc->Instr(BC_SB);
					else if( type->dataType.GetSizeInMemoryBytes() == 2 )
						bc->Instr(BC_SW);
				}
				else if( type->dataType.IsUnsignedType() || type->dataType.IsBitVectorType() )
				{
					if( type->dataType.GetSizeInMemoryBytes() == 1 )
						bc->Instr(BC_UB);
					else if( type->dataType.GetSizeInMemoryBytes() == 2 )
						bc->Instr(BC_UW);
				}
			}
		}
		else if( type->dataType.GetSizeInMemoryDWords() == 2 )
		{
			type->dataType.isReference = false;
			type->dataType.isReadOnly = true;

			if( bc ) bc->Instr(BC_RD8);
		}
		else
		{
			assert(false);
		}

		// If the variable dereferenced was a temporary variable we need to release it
		if( bc ) 
			ReleaseTemporaryVariable(*type, bc);
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
	if( !v->type.IsPrimitive() && v->type.pointerLevel == 0 || v->type.arrayDimensions > 0 ) return true;

	// Mark as initialized so that the user will not be bothered again
	v->isInitialized = true;

	// Write warning
	asCString str;
	str.Format(TXT_s_NOT_INITIALIZED, (const char *)v->name);
	Warning(str, node);

	return false;
}

void asCCompiler::PrepareOperand(asCTypeInfo *type, asCByteCode *bc, asCScriptNode *node)
{
	// Check if the variable is initialized (if it indeed is a variable)
	IsVariableInitialized(type, node);

	asCDataType to = type->dataType;
	to.isReference = false;

	ImplicitConversion(bc, to, type, node, false);
}

void asCCompiler::PrepareForAssignment(asCDataType *lvalue, asCTypeInfo *type, asCByteCode *bc, asCScriptNode *node)
{
	asCByteCode prep;

	if( lvalue->IsComplex(engine) )
	{
		// A complex type requires a reference
		asCDataType to = *lvalue;
		to.isReference = true;
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

	// If the assignment will be made with the copy behaviour then the rvalue must be a reference
	if( lvalue->IsComplex(engine) )
		assert(type->dataType.isReference);

	// Make sure the rvalue is initialized if it is a variable
	IsVariableInitialized(type, node);

	if( prep.GetLastCode() != -1 )
		bc->AddCode(&prep);
}

void asCCompiler::PerformAssignment(asCTypeInfo *lvalue, asCByteCode *bc, asCScriptNode *node)
{
	if( lvalue->dataType.isReadOnly )
		Error(TXT_REF_IS_READ_ONLY, node);

	if( !lvalue->dataType.isReference )
	{
		Error(TXT_NOT_VALID_REFERENCE, node);
		return;
	}
	
	if( !lvalue->dataType.IsComplex(engine) )
	{
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
	else
	{
		asSTypeBehaviour *beh = builder->GetBehaviour(&lvalue->dataType);
		if( beh && beh->copy )
		{
			// Call the copy operator
			bc->Instr(BC_SWAP4);
			bc->Call(BC_CALLSYS, (asDWORD)beh->copy, 2);
			bc->Instr(BC_RRET4);
		}
		else
		{
			// Default copy operator

			// Copy larger data types from a reference
			bc->InstrWORD(BC_COPY, (asWORD)lvalue->dataType.GetSizeInMemoryDWords()); 
		}
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
	// Start by implicitly converting constant values
	if( from->isConstant ) 
		ImplicitConversionConstant(bc, to, from, node, isExplicit);

	// After the constant value has been converted we have the following possibilities
	
	if( !from->dataType.isReference )
	{
		// The value is on the stack

		if( from->dataType.pointerLevel == 0 && to.pointerLevel == 0 &&
			from->dataType.arrayDimensions == 0 && to.arrayDimensions == 0 )
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


			// Primitive non-references are always const
			from->dataType.isReadOnly = true;
		}
	}

	if( !to.isReference )
	{
		if( from->dataType.isReference )
		{
			// A reference to a primitive type (const or not) can be converted to a non-reference by dereferencing
			if( from->dataType.IsPrimitive() )
			{
				Dereference(bc, from);

				// Try once more, since the base type may be different
				ImplicitConversion(bc, to, from, node, isExplicit);
			}
			
			// A reference to a pointer (that doesn't need behaviour) can also be converted to a non-reference by dereferencing
			if( from->dataType.pointerLevel > 0 && !from->dataType.IsComplex(engine) )
				Dereference(bc, from);

			// TODO: Maybe this should be made in the CompileFunctionCall()
			// A reference to an object with behaviour is converted to a non-reference by using a copy construct
		}
	}
	else
	{
		if( from->dataType.isReference )
		{
			// A reference to a non-const can be converted to a reference to a const
			if( to.isReadOnly ) 
				from->dataType.isReadOnly = true;
		}
		else
		{
			// Only primitives and pointers can be non-references (or null pointer)
			assert(from->isConstant || from->dataType.IsPrimitive() || from->dataType.pointerLevel > 0);

			// A non-reference can be converted to a const reference, by putting the value in a temporary variable
			from->dataType.isReference = true;
			from->dataType.isReadOnly = true;

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
					bc->InstrSHORT(BC_PSF, (short)offset);
					bc->Instr(BC_MOV4);
				}
				else if( from->dataType.GetSizeInMemoryDWords() == 2 )
				{
					bc->InstrSHORT(BC_PSF, (short)offset);
					bc->Instr(BC_WRT8);
					bc->Pop(2);
				}
				else
				{
					// Shouldn't be possible
					assert(false);
				}

				// Put the address to the variable on the stack
				bc->InstrSHORT(BC_PSF, (short)offset);
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
	if( to.arrayDimensions > 0 ) return;

	// Only 0 can be implicitly converted to a pointer
	if( to.pointerLevel > 0 )
	{
		asCDataType temp = asCDataType(ttInt, true, false);
		ImplicitConversion(bc, temp, from, node, isExplicit);
		if( from->dataType.IsIntegerType() && from->dwordValue == 0 )
		{
			// Copy all except isReference
			from->dataType = to;
			from->dataType.isReference = false;
			from->dataType.isReadOnly = true;
			return;
		}
	}

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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
				if( !isExplicit ) Warning(str, node);
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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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
					if( !isExplicit ) Warning(TXT_VALUE_TOO_LARGE_FOR_TYPE, node);

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

void asCCompiler::CompileAssignment(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type)
{
	asCScriptNode *lexpr = expr->firstChild;
	if( lexpr->next )
	{
		if( globalExpression )
		{
			Error(TXT_ASSIGN_IN_GLOBAL_EXPR, expr);

			type->Set(asCDataType(ttInt, true, false));
			type->isConstant = true;
			type->qwordValue = 0;
			return;
		}

		asCByteCode rbc, lbc, obc;
		asCTypeInfo rtype, ltype;

		// Compile the two expression terms
		CompileAssignment(lexpr->next->next, &rbc, &rtype);
		CompileCondition(lexpr, &lbc, &ltype);

		// If left expression resolves into a registered type
		// check if the assignment operator is overloaded, and check 
		// the type of the right hand expression. If none is found
		// the default action is a direct copy if it is the same type
		// and a simple assignment.
		asSTypeBehaviour *beh = builder->GetBehaviour(&ltype.dataType);
		if( beh )
		{
			// If it is a default array, both sides must have the same subtype
			if( ltype.dataType.arrayDimensions > 0 )
				if( ltype.dataType != rtype.dataType )
					Error(TXT_BOTH_MUST_BE_SAME, lexpr->next);

			// We must verify that the lvalue isn't const
			if( ltype.dataType.isReadOnly )
				Error(TXT_REF_IS_READ_ONLY, lexpr->next);

			// Find the matching overloaded operators
			int op = lexpr->next->tokenType;
			asCArray<int> ops;
			int n;
			for( n = 0; n < beh->operators.GetLength(); n += 2 )
			{
				if( op == beh->operators[n] )
					ops.PushLast(beh->operators[n+1]);
			}

			asCArray<int> match;
			MatchArgument(ops, match, &rtype, 0);

			if( match.GetLength() == 1 )
			{
				// Add the code for the object
				bc->AddCode(&lbc);

				asCScriptFunction *descr = engine->systemFunctions[-match[0] - 1];

				int argSize = descr->parameterTypes[0].GetSizeOnStackDWords();

				// Add code for arguments
				int size = -ReserveSpaceForArgument(descr->parameterTypes[0], bc, false);
				bc->AddCode(&rbc);
				// Implictly convert the value to the parameter type
				ImplicitConversion(bc, descr->parameterTypes[0], &rtype, lexpr->next->next, false);

				MoveArgumentToReservedSpace(descr->parameterTypes[0], &rtype, lexpr->next->next, bc, size > 0, 0);

				// Remove destructors for the arguments just before the call
				beh = builder->GetBehaviour(&descr->parameterTypes[0]);
				if( !descr->parameterTypes[0].isReference && beh && beh->destruct )
					bc->Destructor(BC_REMDESTRSP, (asDWORD)beh->destruct, 0);

				PerformFunctionCall(match[0], type, bc);

				// Release all temporary variables used by arguments
				ReleaseTemporaryVariable(rtype, bc);

				return;
			}
			else if( ops.GetLength() > 1 )
			{
				Error(TXT_MORE_THAN_ONE_MATCHING_OP, lexpr->next);

				type->Set(ltype.dataType);

				return;
			}
		}


		int op = lexpr->next->tokenType;
		if( op != ttAssignment )
		{
			// The expression must be computed
			// first so we can't change that

			// We can't just read from the lvalue
			// as that would remove the address
			// from the stack.

			// Store the lvalue in a temporary register
			bool isSimple = (lbc.GetLastCode() == BC_PSF || lbc.GetLastCode() == BC_PGA);
			if( !isSimple )
				lbc.Instr(BC_STORE4);

			// Compile the operation
			asCTypeInfo lltype = ltype, rrtype = rtype;

			CompileOperator(lexpr->next, &lbc, &rbc, &obc, &lltype, &rrtype, &rtype);

			PrepareForAssignment(&ltype.dataType, &rtype, &obc, lexpr->next->next);

			bc->AddCode(&obc);

			// Recall the lvalue
			if( !isSimple )
				bc->Instr(BC_RECALL4);
			else
			{
				// Compile the lvalue again
				CompileCondition(lexpr, &lbc, &ltype);
				bc->AddCode(&lbc);
			}
		}
		else
		{
			PrepareForAssignment(&ltype.dataType, &rtype, &rbc, lexpr->next->next);

			bc->AddCode(&rbc);
			bc->AddCode(&lbc);
		}

		PerformAssignment(&ltype, bc, lexpr->next);

		*type = rtype;
	}
	else
		CompileCondition(lexpr, bc, type);
}

void asCCompiler::CompileCondition(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type)
{
	asCTypeInfo ctype, rtype, ltype;

	// Compile the conditional expression
	asCScriptNode *cexpr = expr->firstChild;
	if( cexpr->next )
	{
		//-------------------------------
		// Compile the expressions
		asCByteCode ebc;
		CompileExpression(cexpr, &ebc, &ctype);
		PrepareOperand(&ctype, &ebc, cexpr);
		if( ctype.dataType != asCDataType(ttBool, true, false) )
			Error(TXT_EXPR_MUST_BE_BOOL, cexpr);

		asCByteCode lebc;
		CompileAssignment(cexpr->next, &lebc, &ltype);

		asCByteCode rebc;
		CompileAssignment(cexpr->next->next, &rebc, &rtype);

		// Allow a 0 in the first case to be implicitly converted to the second type
		if( ltype.isConstant && ltype.intValue == 0 && ltype.dataType.IsUnsignedType() )
		{
			asCDataType to = rtype.dataType;
			to.isReference = false;
			to.isReadOnly  = true;
			ImplicitConversionConstant(&lebc, to, &ltype, cexpr->next, false);
		}

		//---------------------------------
		// Output the byte code
		int afterLabel = nextLabel++;
		int elseLabel = nextLabel++;

		// Allocate temporary variable and copy the result to that one
		asCTypeInfo temp;
		temp = ltype;
		temp.dataType.isReference = false;
		temp.dataType.isReadOnly = false;
		int offset = AllocateVariable(temp.dataType, true);
		temp.isTemporary = true;
		temp.stackOffset = (short)offset;

		CompileConstructor(temp.dataType, offset, bc);

		bc->AddCode(&ebc);
		bc->InstrINT(BC_JZ, elseLabel);

		asCTypeInfo rtemp;
		rtemp = temp;
		rtemp.dataType.isReference = true;

		PrepareForAssignment(&rtemp.dataType, &ltype, &lebc, cexpr->next);
		bc->AddCode(&lebc);

		bc->InstrWORD(BC_PSF, offset);
		PerformAssignment(&rtemp, bc, cexpr->next);
		bc->Pop(ltype.dataType.GetSizeOnStackDWords()); // Pop the original value

		// Release the old temporary variable
		ReleaseTemporaryVariable(ltype, bc);

		bc->InstrINT(BC_JMP, afterLabel);
		bc->Label((short)elseLabel);

		// Copy the result to the same temporary variable
		PrepareForAssignment(&rtemp.dataType, &rtype, &rebc, cexpr->next);
		bc->AddCode(&rebc);

		bc->InstrWORD(BC_PSF, offset);
		PerformAssignment(&rtemp, bc, cexpr->next);
		bc->Pop(ltype.dataType.GetSizeOnStackDWords()); // Pop the original value

		// Release the old temporary variable
		ReleaseTemporaryVariable(rtype, bc);

		bc->Label((short)afterLabel);

		bc->InstrWORD(BC_PSF, offset);

		// Make sure both expressions have the same type
		if( ltype.dataType != rtype.dataType )
			Error(TXT_BOTH_MUST_BE_SAME, expr);

		// Set the temporary variable as output
		*type = rtemp;

		// Make sure the output isn't marked as being a literal constant
		type->isConstant = false;
	}
	else
		CompileExpression(cexpr, bc, type);
}

void asCCompiler::CompileExpression(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type)
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
	asCByteCode exprBC;
	CompilePostFixExpression(&postfix, &exprBC, type);
	bc->AddCode(&exprBC);
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

void asCCompiler::CompilePostFixExpression(asCArray<asCScriptNode *> *postfix, asCByteCode *bc, asCTypeInfo *type)
{
	// Shouldn't send any byte code
	assert(bc->GetLastCode() == -1);

	// Pop the last node
	asCScriptNode *node = postfix->PopLast();

	// If term, compile the term
	if( node->nodeType == snExprTerm )
	{
		CompileExpressionTerm(node, bc, type);
		return;
	}

	// Compile the two expression terms
	asCByteCode rbc, lbc;
	asCTypeInfo rtype, ltype;

	CompilePostFixExpression(postfix, &lbc, &ltype);
	CompilePostFixExpression(postfix, &rbc, &rtype);

	// Compile the operation
	CompileOperator(node, &lbc, &rbc, bc, &ltype, &rtype, type);
}

void asCCompiler::CompileExpressionTerm(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type)
{
	// Shouldn't send any byte code
	assert(bc->GetLastCode() == -1);

	// Compile the value node
	asCScriptNode *vnode = node->firstChild;
	while( vnode->nodeType != snExprValue )
		vnode = vnode->next;

	asCByteCode vbc;
	asCTypeInfo vtype;
	CompileExpressionValue(vnode, &vbc, &vtype);

	// Compile post fix operators
	asCScriptNode *pnode = vnode->next;
	while( pnode )
	{
		CompileExpressionPostOp(pnode, &vbc, &vtype);
		pnode = pnode->next;
	}

	// Compile pre fix operators
	pnode = vnode->prev;
	while( pnode )
	{
		CompileExpressionPreOp(pnode, &vbc, &vtype);
		pnode = pnode->prev;
	}

	// Return the byte code and final type description
	bc->AddCode(&vbc);

    *type = vtype;
}

void asCCompiler::CompileExpressionValue(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type)
{
	// Shouldn't receive any byte code
	assert(bc->GetLastCode() == -1);

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
				type->Set(prop->type);
				type->dataType.isReference = true;
				type->dataType.isReadOnly = prop->type.isReadOnly;
				type->stackOffset = 0x7FFF;

				// Verify that the global property has been compiled already
				if( isCompiled )
				{
					// TODO: If the global property is a pure constant
					// we can allow the compiler to optimize it. Pure
					// constants are global constant variables that were
					// initialized by literal constants.

					// Push the address of the variable on the stack
					bc->InstrINT(BC_PGA, prop->index);
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
				bc->InstrDWORD(BC_SET4, 0);
				type->Set(asCDataType(ttInt, false, true));
				type->stackOffset = 0x7FFF;

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
			bc->InstrSHORT(BC_PSF, (short)v->stackOffset);

			type->Set(v->type);
			type->dataType.isReference = true;
			type->stackOffset = (short)v->stackOffset;
			type->isVariable = true;

			// Implicitly dereference parameters sent by reference
			if( v->type.isReference )
				bc->Instr(BC_RD4);
		}
	}
	else if( vnode->nodeType == snConstant )
	{
		if( vnode->tokenType == ttIntConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			asDWORD val = asStringScanUInt(value, 10, 0);
			bc->InstrDWORD(BC_SET4, val);

			type->Set(asCDataType(ttUInt, true, false));
			type->isConstant = true;
			type->dwordValue = val;
		}
		else if( vnode->tokenType == ttBitsConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos+2], vnode->tokenLength-2);

			// TODO: Check for overflow
			int val = asStringScanUInt(value, 16, 0);
			bc->InstrDWORD(BC_SET4, val);

			type->Set(asCDataType(ttBits, true, false));
			type->isConstant = true;
			type->intValue = val;
		}
		else if( vnode->tokenType == ttFloatConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			float v = float(asStringScanDouble(value, 0));
			bc->InstrFLOAT(BC_SET4, v);

			type->Set(asCDataType(ttFloat, true, false));
			type->isConstant = true;
			type->floatValue = v;
		}
		else if( vnode->tokenType == ttDoubleConstant )
		{
			GETSTRING(value, &script->code[vnode->tokenPos], vnode->tokenLength);

			// TODO: Check for overflow
			double v = asStringScanDouble(value, 0);
			bc->InstrDOUBLE(BC_SET8, v);

			type->Set(asCDataType(ttDouble, true, false));
			type->isConstant = true;
			type->doubleValue = v;
		}
		else if( vnode->tokenType == ttTrue ||
			     vnode->tokenType == ttFalse )
		{
			if( vnode->tokenType == ttTrue )
				bc->InstrDWORD(BC_SET4, VALUE_OF_BOOLEAN_TRUE);
			else
				bc->InstrDWORD(BC_SET4, 0); 

			type->Set(asCDataType(ttBool, true, false));
			type->isConstant = true;
			type->dwordValue = vnode->tokenType == ttTrue ? VALUE_OF_BOOLEAN_TRUE : 0;
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
				bc->InstrDWORD(BC_SET4, 0);
				type->Set(asCDataType(ttInt, false, true));
				type->stackOffset = 0x7FFF;
			}
			else
			{
				bc->InstrWORD(BC_STR, id);
				PerformFunctionCall(descr->id, type, bc);
			}			
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
			bc->InstrDWORD(BC_SET4, 0);

			type->Set(asCDataType(ttInt, true, false));
			type->isConstant = true;
			type->qwordValue = 0;
		}
		else
			CompileFunctionCall(vnode, bc, type, 0);
	}
	else if( vnode->nodeType == snAssignment )
		CompileAssignment(vnode, bc, type);
	else if( vnode->nodeType == snConversion )
		CompileConversion(vnode, bc, type);
	else
		assert(false);
}

void asCCompiler::CompileConversion(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type)
{
	// Should be empty
	assert(bc->GetLastCode() == -1);

	// Compile the expression
	asCTypeInfo exprType;
	CompileAssignment(node->lastChild, bc, &exprType);

	type->Set(builder->CreateDataTypeFromNode(node->firstChild, script));
	type->dataType.isReadOnly = true; // Default to const
	assert(type->dataType.tokenType != ttIdentifier);

	IsVariableInitialized(&exprType, node);

	// Try an implicit conversion first
	ImplicitConversion(bc, type->dataType, &exprType, node, true);

	// If no type conversion is really tried ignore it
	if( type->dataType == exprType.dataType )
	{
		// This will keep information about constant type
		*type = exprType;
		return;
	}

	if( type->dataType.IsEqualExceptConst(exprType.dataType) && type->dataType.IsPrimitive() )
	{
		*type = exprType;
		type->dataType.isReadOnly = true;
		return;
	}

	if( exprType.isConstant )
		type->isConstant = true;

	bool conversionOK = false;
	if( type->dataType.IsIntegerType() )
	{
		if( exprType.isConstant )
		{
			if( exprType.dataType.IsFloatType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->intValue = int(exprType.floatValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsDoubleType() )
			{
				assert(bc->GetLastCode() == BC_SET8);
				type->intValue = int(exprType.doubleValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsBitVectorType() ||
					 exprType.dataType.IsIntegerType() ||
					 exprType.dataType.IsUnsignedType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->dwordValue = exprType.dwordValue;
				conversionOK = true;
			}

			if( type->dataType.GetSizeInMemoryBytes() == 1 )
				type->intValue = char(type->intValue);
			else if( type->dataType.GetSizeInMemoryBytes() == 2 )
				type->intValue = short(type->intValue);

			bc->ClearAll();
			bc->InstrINT(BC_SET4, type->intValue);
		}
		else
		{
			if( exprType.dataType.IsFloatType() )
			{
				conversionOK = true;
				bc->Instr(BC_F2I);
			}
			else if( exprType.dataType.IsDoubleType() )
			{
				conversionOK = true;
				bc->Instr(BC_dTOi);
			}
			else if( exprType.dataType.IsBitVectorType() ||
					 exprType.dataType.IsIntegerType() ||
					 exprType.dataType.IsUnsignedType() )
				conversionOK = true;

			if( type->dataType.GetSizeInMemoryBytes() == 1 )
				bc->Instr(BC_SB);
			else if( type->dataType.GetSizeInMemoryBytes() == 2 )
				bc->Instr(BC_SW);
		}
	}
	else if( type->dataType.IsUnsignedType() )
	{
		if( exprType.isConstant )
		{
			if( exprType.dataType.IsFloatType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->dwordValue = asDWORD(exprType.floatValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsDoubleType() )
			{
				assert(bc->GetLastCode() == BC_SET8);
				type->dwordValue = asDWORD(exprType.doubleValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsBitVectorType() ||
					 exprType.dataType.IsIntegerType() ||
					 exprType.dataType.IsUnsignedType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->dwordValue = exprType.dwordValue;
				conversionOK = true;
			}

			if( type->dataType.GetSizeInMemoryBytes() == 1 )
				type->dwordValue = asBYTE(type->dwordValue);
			else if( type->dataType.GetSizeInMemoryBytes() == 2 )
				type->dwordValue = asWORD(type->dwordValue);

			bc->ClearAll();
			bc->InstrDWORD(BC_SET4, type->dwordValue);
		}
		else
		{
			if( exprType.dataType.IsFloatType() )
			{
				conversionOK = true;
				bc->Instr(BC_F2UI);
			}
			else if( exprType.dataType.IsDoubleType() )
			{
				conversionOK = true;
				bc->Instr(BC_dTOui);
			}
			else if( exprType.dataType.IsBitVectorType() ||
					 exprType.dataType.IsIntegerType() ||
					 exprType.dataType.IsUnsignedType() )
				conversionOK = true;

			if( type->dataType.GetSizeInMemoryBytes() == 1 )
				bc->Instr(BC_UB);
			else if( type->dataType.GetSizeInMemoryBytes() == 2 )
				bc->Instr(BC_UW);
		}
	}
	else if( type->dataType.IsFloatType() )
	{
		if( exprType.isConstant )
		{
			if( exprType.dataType.IsDoubleType() )
			{
				assert(bc->GetLastCode() == BC_SET8);
				type->floatValue = float(exprType.doubleValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsIntegerType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->floatValue = float(exprType.intValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsUnsignedType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->floatValue = float(exprType.dwordValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsBitVectorType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->dwordValue = exprType.dwordValue;
				conversionOK = true;
			}

			bc->ClearAll();
			bc->InstrDWORD(BC_SET4, type->dwordValue);
		}
		else
		{
			if( exprType.dataType.IsDoubleType() )
			{
				bc->Instr(BC_dTOf);
				return;
			}
			else if( exprType.dataType.IsIntegerType() )
			{
				bc->Instr(BC_I2F);
				return;
			}
			else if( exprType.dataType.IsUnsignedType() )
			{
				bc->Instr(BC_UI2F);
				return;
			}
			else if( exprType.dataType.IsBitVectorType() )
				return;
		}
	}
	else if( type->dataType == asCDataType(ttDouble, true, false) )
	{
		if( exprType.isConstant )
		{
			if( exprType.dataType.IsFloatType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->doubleValue = double(exprType.floatValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsIntegerType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->doubleValue = double(exprType.intValue);
				conversionOK = true;
			}
			else if( exprType.dataType.IsUnsignedType() )
			{
				assert(bc->GetLastCode() == BC_SET4);
				type->doubleValue = double(exprType.dwordValue);
				conversionOK = true;
			}

			bc->ClearAll();
			bc->InstrQWORD(BC_SET8, type->qwordValue);
		}
		else
		{
			if( exprType.dataType.IsFloatType() )
			{
				bc->Instr(BC_fTOd);
				return;
			}
			else if( exprType.dataType.IsIntegerType() )
			{
				bc->Instr(BC_iTOd);
				return;
			}
			else if( exprType.dataType.IsUnsignedType() )
			{
				bc->Instr(BC_uiTOd);
				return;
			}
		}
	}
	else if( type->dataType.IsBitVectorType() )
	{
		if( exprType.isConstant )
		{
			if( exprType.dataType.IsIntegerType() ||
				exprType.dataType.IsUnsignedType() ||
				exprType.dataType.IsFloatType() || 
				exprType.dataType.IsBitVectorType() )
			{
				type->dwordValue = exprType.dwordValue;

				if( type->dataType.GetSizeInMemoryBytes() == 1 )
					type->dwordValue = asBYTE(type->dwordValue);
				else if( type->dataType.GetSizeInMemoryBytes() == 2 )
					type->dwordValue = asWORD(type->dwordValue);

				assert(bc->GetLastCode() == BC_SET4);
				bc->ClearAll();
				bc->InstrDWORD(BC_SET4, type->dwordValue);

				return;
			}
		}
		else
		{
			if( exprType.dataType.IsIntegerType() ||
				exprType.dataType.IsUnsignedType() ||
				exprType.dataType.IsFloatType() ||
				exprType.dataType.IsBitVectorType() )
			{
				if( type->dataType.GetSizeInMemoryBytes() == 1 )
					bc->Instr(BC_UB);
				else if( type->dataType.GetSizeInMemoryBytes() == 2 )
					bc->Instr(BC_UW);

				return;
			}
		}
	}

	if( conversionOK )
		return;

	// Conversion not available

	asCString strTo, strFrom;

	strTo = type->dataType.Format();
	strFrom = exprType.dataType.Format();

	asCString msg;
	msg.Format(TXT_NO_CONVERSION_s_TO_s, (const char *)strFrom, (const char *)strTo);

	Error(msg, node);
}

void asCCompiler::CompileFunctionCall(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type, asCObjectType *objectType)
{
	// Compile the arguments
	asCArray<asCTypeInfo> argTypes;
	asCArray<asCByteCode *> argByteCodes;
	asCArray<asCTypeInfo> temporaryVariables;
	int argCount;
	
	CompileArgumentList(node->lastChild, argTypes, argByteCodes);
	argCount = argTypes.GetLength();

	// Find all functions with the correct name
	GETSTRING(name, &script->code[node->firstChild->tokenPos], node->firstChild->tokenLength);

	asCArray<int> funcs;
	if( objectType )
		builder->GetObjectMethodDescriptions(name, objectType, funcs);
	else
		builder->GetFunctionDescriptions(name, funcs);

	bool isConstructor = false;
	asCTypeInfo tempObj;
	if( funcs.GetLength() == 0 )
	{
		// It is possible that the name is really a constructor
		asCDataType dt;
		dt.tokenType = ttIdentifier;
		dt.extendedType = engine->GetObjectType(name);
		if( dt.extendedType )
		{
			dt.objectType = dt.extendedType;
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
				bc->InstrWORD(BC_PSF, tempObj.stackOffset);
			}
		}
	}

	MatchFunctions(funcs, argTypes, node, name);

	if( funcs.GetLength() != 1 )
	{
		// The error was reported by MatchFunctions()

		// Dummy value
		bc->InstrDWORD(BC_SET4, 0);
		type->Set(asCDataType(ttInt, true, false));
	}
	else
	{
		PrepareFunctionCall(funcs[0], node->lastChild, bc, argTypes, argByteCodes);
		
		PerformFunctionCall(funcs[0], type, bc);

		// Release all temporary variables used by arguments
		for( int n = 0; n < argTypes.GetLength(); n++ )
			ReleaseTemporaryVariable(argTypes[n], bc);

		if( isConstructor )
		{
			// The constructor doesn't return anything, 
			// so we have to manually inform the type of  
			// the return value
			*type = tempObj;

			// Push the address of the object on the stack again
			bc->InstrWORD(BC_PSF, tempObj.stackOffset);

			// Insert a destructor for the object
			asSTypeBehaviour *beh = builder->GetBehaviour(&tempObj.dataType);
			if( beh && beh->destruct )
				bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, tempObj.stackOffset);
		}
	}

	// Cleanup
	for( int n = 0; n < argCount; n++ )
		if( argByteCodes[n] ) delete argByteCodes[n];
}

void asCCompiler::CompileExpressionPreOp(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type)
{
	int op = node->tokenType;

	IsVariableInitialized(type, node);

	if( op == ttMinus && type->dataType.objectType != 0 )
	{
		asCTypeInfo objType = *type;

		// Must be a reference to an object
		if( !type->dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);

		// TODO: We can convert non-reference to reference by putting it in a temporary variable

		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(type, node);

		// Now find a matching function for the object type
		asSTypeBehaviour *beh = engine->GetBehaviour(&type->dataType);
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
				asCScriptFunction *descr = engine->systemFunctions[-opNegate - 1];

				PerformFunctionCall(opNegate, type, bc);
			}
			else
			{
				asCString str;
				str.Format(TXT_OBJECT_DOESNT_SUPPORT_NEGATE_OP);
				Error(str, node);
			}
		}

		// Release the potentially temporary object
		ReleaseTemporaryVariable(objType, bc);
	}
	else if( op == ttPlus || op == ttMinus )
	{
		asCDataType to = type->dataType;
		to.isReference = false;

		// TODO: The case -2147483648 gives an unecessary warning of changed sign for implicit conversion

		if( type->dataType.IsUnsignedType() )
		{
			if( type->dataType.GetSizeInMemoryBytes() == 1 )
				to.tokenType = ttInt8;
			else if( type->dataType.GetSizeInMemoryBytes() == 2 )
				to.tokenType = ttInt16;
			else if( type->dataType.GetSizeInMemoryBytes() == 4 )
				to.tokenType = ttInt;
			else
				Error(TXT_INVALID_TYPE, node);
		}

		ImplicitConversion(bc, to, type, node, false);

		if( type->dataType.IsIntegerType() )
		{
			if( op == ttMinus )
			{
				if( type->isConstant )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->ClearAll();
					bc->InstrINT(BC_SET4, -type->intValue);
					type->intValue = -type->intValue;
					return;
				}

				bc->Instr(BC_NEGi);
			}
		}
		else if( type->dataType.IsFloatType() )
		{
			if( op == ttMinus )
			{
				if( type->isConstant )
				{
					assert(bc->GetLastCode() == BC_SET4);
					bc->ClearAll();
					bc->InstrFLOAT(BC_SET4, -type->floatValue);
					type->floatValue = -type->floatValue;
					return;
				}

				bc->Instr(BC_NEGf);
			}
		}
		else if( type->dataType.IsDoubleType() )
		{
			if( op == ttMinus )
			{
				if( type->isConstant )
				{
					assert(bc->GetLastCode() == BC_SET8);
					bc->ClearAll();
					bc->InstrDOUBLE(BC_SET8, -type->doubleValue);
					type->doubleValue = -type->doubleValue;
					return;
				}

				bc->Instr(BC_NEGd);
			}
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttNot )
	{
		PrepareOperand(type, bc, node);

		if( type->dataType == asCDataType(ttBool, true, false) )
		{
			if( type->isConstant )
			{
				assert(bc->GetLastCode() == BC_SET4);
				bc->ClearAll();
				bc->InstrDWORD(BC_SET4, type->dwordValue == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
				type->dwordValue = (type->dwordValue == 0 ? VALUE_OF_BOOLEAN_TRUE : 0);
				return;
			}

			bc->Instr(BC_TZ);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttBitNot )
	{
		PrepareOperand(type, bc, node);

		if( type->dataType.IsBitVectorType() )
		{
			if( type->isConstant )
			{
				assert(bc->GetLastCode() == BC_SET4);
				bc->ClearAll();
				bc->InstrDWORD(BC_SET4, ~type->dwordValue);
				type->dwordValue = ~type->dwordValue;
				return;
			}

			bc->Instr(BC_BNOT);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttInc || op == ttDec )
	{
		if( globalExpression )
			Error(TXT_INC_OP_IN_GLOBAL_EXPR, node);

		if( !type->dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);

		// Make sure the reference isn't a temporary variable
		if( type->isTemporary )
			Error(TXT_REF_IS_TEMP, node);
		if( type->dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( type->dataType == asCDataType(ttInt, false, true) ||
			type->dataType == asCDataType(ttUInt, false, true) )
		{
			if( op == ttInc )
				bc->Instr(BC_INCi);
			else
				bc->Instr(BC_DECi);
		}
		else if( type->dataType == asCDataType(ttInt16, false, true) ||
			     type->dataType == asCDataType(ttUInt16, false, true) )
		{
			if( op == ttInc )
				bc->Instr(BC_INCi16);
			else
				bc->Instr(BC_DECi16);
		}
		else if( type->dataType == asCDataType(ttInt8, false, true) ||
			     type->dataType == asCDataType(ttUInt8, false, true) )
		{
			if( op == ttInc )
				bc->Instr(BC_INCi8);
			else
				bc->Instr(BC_DECi8);
		}
		else if( type->dataType == asCDataType(ttFloat, false, true) )
		{
			if( op == ttInc )
				bc->Instr(BC_INCf);
			else
				bc->Instr(BC_DECf);
		}
		else if( type->dataType == asCDataType(ttDouble, false, true) )
		{
			if( op == ttInc )
				bc->Instr(BC_INCd);
			else
				bc->Instr(BC_DECd);
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else
		// Unknown operator
		assert(false);
}

void asCCompiler::CompileExpressionPostOp(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type)
{
	int op = node->tokenType;

	if( op == ttInc || op == ttDec )
	{
		if( globalExpression )
			Error(TXT_INC_OP_IN_GLOBAL_EXPR, node);

		// Make sure the reference isn't a temporary variable
		if( !type->dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);
		if( type->isTemporary )
			Error(TXT_REF_IS_TEMP, node);
		if( type->dataType.isReadOnly )
			Error(TXT_REF_IS_READ_ONLY, node);

		if( type->dataType == asCDataType(ttInt, false, true) ||
			type->dataType == asCDataType(ttUInt, false, true) )
		{
			if( op == ttInc ) bc->Instr(BC_INCi); else bc->Instr(BC_DECi);
			Dereference(bc, type);
			bc->InstrDWORD(BC_SET4, 1); // Let optimizer change to SET1
			if( op == ttInc ) bc->Instr(BC_SUBi); else bc->Instr(BC_ADDi);

			type->dataType.isReference = false;
		}
		else if( type->dataType == asCDataType(ttInt16, false, true) ||
			     type->dataType == asCDataType(ttUInt16, false, true) )
		{
			if( op == ttInc ) bc->Instr(BC_INCi16); else bc->Instr(BC_DECi16);
			Dereference(bc, type);
			bc->InstrDWORD(BC_SET4, 1);
			if( op == ttInc ) bc->Instr(BC_SUBi); else bc->Instr(BC_ADDi);
		}
		else if( type->dataType == asCDataType(ttInt8, false, true) ||
			     type->dataType == asCDataType(ttUInt8, false, true) )
		{
			if( op == ttInc ) bc->Instr(BC_INCi8); else bc->Instr(BC_DECi8);
			Dereference(bc, type);
			bc->InstrDWORD(BC_SET4, 1);
			if( op == ttInc ) bc->Instr(BC_SUBi); else bc->Instr(BC_ADDi);
		}
		else if( type->dataType == asCDataType(ttFloat, false, true) )
		{
			if( op == ttInc ) bc->Instr(BC_INCf); else bc->Instr(BC_DECf);
			Dereference(bc, type);
			bc->InstrFLOAT(BC_SET4, 1);
			if( op == ttInc ) bc->Instr(BC_SUBf); else bc->Instr(BC_ADDf);

			type->dataType.isReference = false;
		}
		else if( type->dataType == asCDataType(ttDouble, false, true) )
		{
			if( op == ttInc ) bc->Instr(BC_INCd); else bc->Instr(BC_DECd);
			Dereference(bc, type);
			bc->InstrDOUBLE(BC_SET8, 1);
			if( op == ttInc ) bc->Instr(BC_SUBd); else bc->Instr(BC_ADDd);

			type->dataType.isReference = false;
		}
		else
			Error(TXT_ILLEGAL_OPERATION, node);
	}
	else if( op == ttDot || op == ttArrow )
	{
		// Must be a reference for the dot operator
		if( op == ttDot && !type->dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);

		// Check pointer level
		if( op == ttDot && type->dataType.pointerLevel > 0 )
			Error(TXT_ILLEGAL_OPERATION, node);
		else if( op == ttArrow && type->dataType.pointerLevel != 1 )
			Error(TXT_ILLEGAL_OPERATION, node);

		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(type, node);

		if( node->firstChild->nodeType == snIdentifier )
		{
			// Get the property name
			GETSTRING(name, &script->code[node->firstChild->tokenPos], node->firstChild->tokenLength);

			asCDataType objType = type->dataType;
			objType.isReference = false;

			// Find the property offset and type
			if( objType.tokenType == ttIdentifier )
			{
				if( objType.pointerLevel )
				{
					// Convert to the type that is being pointed to
					objType.pointerLevel = 0;
					objType.objectType = objType.extendedType;
				}

				asCProperty *prop = builder->GetObjectProperty(objType, name);
				if( prop )
				{
					// Put the offset on the stack
					bc->InstrINT(BC_SET4, prop->byteOffset);
					if( op == ttDot )
						bc->Instr(BC_ADDi);
					else if( type->dataType.isReference )
						bc->Instr(BC_ADDOFF);
					else
						bc->Instr(BC_ADDi);

					// Set the new type (keeping info about temp variable)
					type->dataType = prop->type;
					type->dataType.isReference = true;
					type->dataType.isReadOnly = prop->type.isReadOnly;
				}
				else
				{
					asCString str;
					str.Format(TXT_s_NOT_MEMBER_OF_s, (const char *)name, (const char *)objType.Format());
					Error(str, node);
				}
			}
			else
			{
				asCString str;
				str.Format(TXT_s_NOT_MEMBER_OF_s, (const char *)name, (const char *)objType.Format());
				Error(str, node);
			}
		}
		else
		{
			if( globalExpression )
				Error(TXT_METHOD_IN_GLOBAL_EXPR, node);

			// Make sure it is an object we are accessing
			if( type->dataType.tokenType != ttIdentifier && type->dataType.arrayDimensions == 0 )
			{
				asCString str;
				str.Format(TXT_ILLEGAL_OPERATION_ON_s, (const char *)type->dataType.Format());
				Error(str, node);

				// Set a dummy type
				type->Set(asCDataType(ttInt, false, false));
			}
			else
			{
				// We need the pointer to the object
				if( op == ttArrow && type->dataType.isReference )
					bc->Instr(BC_RD4);

				asCObjectType *trueObj = type->dataType.objectType;
				if( op == ttArrow )
					trueObj = type->dataType.extendedType;

				asCTypeInfo objType = *type;

				// Compile function call
				CompileFunctionCall(node->firstChild, bc, type, trueObj);

				// Release the potentially temporary object
				ReleaseTemporaryVariable(objType, bc);
			}
		}
	}
	else if( op == ttOpenBracket )
	{
		// Must be a reference to an object
		if( !type->dataType.isReference )
			Error(TXT_NOT_VALID_REFERENCE, node);

		// TODO: We can convert non-reference to reference by putting it in a temporary variable

		// Check if the variable is initialized (if it indeed is a variable)
		IsVariableInitialized(type, node);

		// Compile the expression
		asCByteCode exprBC;
		asCTypeInfo exprType;
		CompileAssignment(node->firstChild, &exprBC, &exprType);

		asCTypeInfo objType = *type;

		// Now find a matching function for the object type and indexing type
		asSTypeBehaviour *beh = engine->GetBehaviour(&type->dataType);
		if( beh == 0 )
		{
			asCString str;
			str.Format(TXT_OBJECT_DOESNT_SUPPORT_INDEX_OP, type->dataType.Format().AddressOf());
			Error(str, node);
		}
		else
		{
			asCArray<int> ops;
			int n;
			for( n = 0; n < beh->operators.GetLength(); n+= 2 )
			{
				if( ttOpenBracket == beh->operators[n] )
					ops.PushLast(beh->operators[n+1]);
			}

			asCArray<int> ops1;
			MatchArgument(ops, ops1, &exprType, 0);

			// Did we find a suitable function?
			if( ops1.GetLength() == 1 )
			{
				asCScriptFunction *descr = engine->systemFunctions[-ops1[0] - 1];

				int argSize = descr->parameterTypes[0].GetSizeOnStackDWords();

				// Add code for arguments
				int size = ReserveSpaceForArgument(descr->parameterTypes[0], bc, false);
				
				bc->AddCode(&exprBC);

				// Implictly convert the value to the parameter type
				ImplicitConversion(bc, descr->parameterTypes[0], &exprType, node->firstChild, false);

				MoveArgumentToReservedSpace(descr->parameterTypes[0], &exprType, node->firstChild, bc, size > 0, 0);

				// Remove destructors for the arguments just before the call
				if( !descr->parameterTypes[0].isReference )
				{
					asSTypeBehaviour *beh = builder->GetBehaviour(&descr->parameterTypes[0]);
					if( beh && beh->destruct )
						bc->Destructor(BC_REMDESTRSP, (asDWORD)beh->destruct, 0);
				}

				PerformFunctionCall(descr->id, type, bc);

				// Release temporary variables
				ReleaseTemporaryVariable(exprType, bc);

				// TODO: Ugly code
				// The default array returns a reference to the subtype
				if( objType.dataType.IsDefaultArrayType(engine) )
				{
					type->dataType = objType.dataType.GetSubType(engine);
					type->dataType.isReference = true;
				}
			}
			else if( ops.GetLength() > 1 )
			{
				Error(TXT_MORE_THAN_ONE_MATCHING_OP, node);
			}
			else
			{
				asCString str;
				str.Format(TXT_NO_MATCHING_OP_FOUND_FOR_TYPE_s, exprType.dataType.Format().AddressOf());
				Error(str, node);
			}
		}

		// Release the potentially temporary object
		ReleaseTemporaryVariable(objType, bc);
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
		ImplicitConversion(0, desc->parameterTypes[paramNum], &ti, 0, true);
		if( desc->parameterTypes[paramNum].IsEqualExceptRefAndConst(ti.dataType) )
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

void asCCompiler::MoveArgumentToReservedSpace(asCDataType &paramType, asCTypeInfo *argType, asCScriptNode *node, asCByteCode *bc, bool haveReservedSpace, int offset)
{
	if( paramType.isReference )
	{
		ImplicitConversion(bc, paramType, argType, node, false);

		if( haveReservedSpace )
		{
			// Copy the reference to the reserved space
			bc->InstrSHORT(BC_PSP, (short)(offset - argType->dataType.GetSizeOnStackDWords()));
			bc->Instr(BC_MOV4);
		}
	}
	else
	{
		bool isComplex = paramType.IsComplex(engine) && !paramType.isReference;
		if( !isComplex )
		{
			ImplicitConversion(bc, paramType, argType, node, false);

			if( haveReservedSpace )
			{
				// Copy the value to the reserved space
				bc->InstrSHORT(BC_PSP, (short)(offset - argType->dataType.GetSizeOnStackDWords()));
				if( argType->dataType.GetSizeOnStackDWords() == 1 )
					bc->Instr(BC_MOV4);
				else
				{
					bc->Instr(BC_WRT8);
					bc->Pop(2);
				}
			}
		}
		else
		{
			assert(haveReservedSpace);
			
			// Copy the value to the space allocated before
			bc->InstrSHORT(BC_PSP, (short)(offset - argType->dataType.GetSizeOnStackDWords()));

			asCTypeInfo ti;
			ti.dataType = paramType;
			ti.dataType.isReference = true;

			PerformAssignment(&ti, bc, node);

			// Pop original value
			bc->Pop(argType->dataType.GetSizeOnStackDWords());
		}
	}
}

int asCCompiler::ReserveSpaceForArgument(asCDataType &dt, asCByteCode *bc, bool alwaysReserve)
{
	int sizeReserved = 0;
	bool isComplex = dt.IsComplex(engine) && !dt.isReference;
	if( isComplex || alwaysReserve )
	{
		sizeReserved = dt.GetSizeOnStackDWords();
		bc->Push(sizeReserved);

		if( !dt.isReference )
		{
			asSTypeBehaviour *beh = builder->GetBehaviour(&dt);
			if( beh && beh->construct )
			{
				bc->InstrSHORT(BC_PSP, 0);
				DefaultConstructor(bc, dt);

				// Add destructor for exception handling
				if( beh && beh->destruct )
					bc->Destructor(BC_ADDDESTRSP, (asDWORD)beh->destruct, 0);
			}
		}
	}

	return sizeReserved;
}

bool asCCompiler::CompileOverloadedOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
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
	MatchArgument(ops, ops1, ltype, 0);
	MatchArgument(ops, ops2, rtype, 1);

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

		int argSize = descr->parameterTypes[0].GetSizeOnStackDWords() + descr->parameterTypes[1].GetSizeOnStackDWords();

		// Add code for arguments
		ReserveSpaceForArgument(descr->parameterTypes[1], bc, true);
		int size = -ReserveSpaceForArgument(descr->parameterTypes[0], bc, false);
		bc->AddCode(lbc);
		ImplicitConversion(bc, descr->parameterTypes[0], ltype, node, false);
		MoveArgumentToReservedSpace(descr->parameterTypes[0], ltype, node, bc, size > 0, 0);
		bc->AddCode(rbc);
		ImplicitConversion(bc, descr->parameterTypes[1], rtype, node, false);
		MoveArgumentToReservedSpace(descr->parameterTypes[1], rtype, node, bc, true, -descr->parameterTypes[0].GetSizeOnStackDWords());

		// Remove destructors for the arguments just before the call
		int offset = 0;
		for( n = 0; n < 2; n++ )
		{
			asSTypeBehaviour *beh = builder->GetBehaviour(&descr->parameterTypes[n]);
			if( !descr->parameterTypes[n].isReference && beh && beh->destruct )
				bc->Destructor(BC_REMDESTRSP, (asDWORD)beh->destruct, offset);
			offset -= descr->parameterTypes[n].GetSizeOnStackDWords();
		}

		PerformFunctionCall(descr->id, type, bc);

		// Release all temporary variables used by arguments
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

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

void asCCompiler::CompileOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
{
	IsVariableInitialized(ltype, node);
	IsVariableInitialized(rtype, node);

	// Compile an overloaded operator for the two operands
	if( CompileOverloadedOperator(node, lbc, rbc, bc, ltype, rtype, type) )
		return;

	// If no overloaded operator is found, continue compilation here
	if( ltype->dataType.pointerLevel > 0 ||
		rtype->dataType.pointerLevel > 0 )
	{
		CompileOperatorOnPointers(node, lbc, rbc, bc, ltype, rtype, type);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

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
		CompileMathOperator(node, lbc, rbc, bc, ltype, rtype, type);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

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
		CompileBitwiseOperator(node, lbc, rbc, bc, ltype, rtype, type);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

		return;
	}

	// Comparison operators
	// == != < > <= >=
	if( op == ttEqual       || op == ttNotEqual           ||
		op == ttLessThan    || op == ttLessThanOrEqual    ||
		op == ttGreaterThan || op == ttGreaterThanOrEqual )
	{
		CompileComparisonOperator(node, lbc, rbc, bc, ltype, rtype, type);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

		return;
	}

	// Boolean operators
	// && || ^^
	if( op == ttAnd || op == ttOr || op == ttXor )
	{
		CompileBooleanOperator(node, lbc, rbc, bc, ltype, rtype, type);

		// Release temporary variables used by expression
		ReleaseTemporaryVariable(*ltype, bc);
		ReleaseTemporaryVariable(*rtype, bc);

		return;
	}

	assert(false);
}

void asCCompiler::CompileOperatorOnPointers(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
{
	asCDataType to;

	if( ltype->isConstant )
		to = rtype->dataType;
	else
		to = ltype->dataType;

	// Implictly convert the same type, this will allow 0 to be converted to a pointer
	to.isReference = false;
	ImplicitConversion(lbc, to, ltype, node, false);
	ImplicitConversion(rbc, to, rtype, node, false);

	// ImplicitConversion() doesn't dereference pointers with 
	// behaviours, but in this case it is safe to do so
	if( ltype->dataType.isReference )
		Dereference(lbc, ltype);
	if( rtype->dataType.isReference )
		Dereference(rbc, rtype);

	bc->AddCode(lbc);
	bc->AddCode(rbc);

	bool isConstant = ltype->isConstant && rtype->isConstant;

	// What kind of operator is it?
	int op = node->tokenType;
	if( op == ttEqual || op == ttNotEqual )
	{
		if( !ltype->dataType.IsEqualExceptConst(rtype->dataType) )
			// TODO: Use TXT_NO_CONVERSION
			Error(TXT_BOTH_MUST_BE_SAME, node);

		type->Set(asCDataType(ttBool, true, false));

		if( !isConstant )
		{
			bc->Instr(BC_CMPi);
			if( op == ttEqual )
				bc->Instr(BC_TZ);
			else if( op == ttNotEqual )
				bc->Instr(BC_TNZ);
		}
		else
		{
			int i = 0;
			if( op == ttEqual )
				i = (ltype->dwordValue == rtype->dwordValue ? VALUE_OF_BOOLEAN_TRUE : 0);
			else if( op == ttNotEqual )
				i = (ltype->dwordValue != rtype->dwordValue ? VALUE_OF_BOOLEAN_TRUE : 0);

			bc->InstrINT(BC_SET4, i);

			type->isConstant = true;
			type->intValue = i;
		}
	}
	else
	{
		// TODO: Show data type of left hand value, show operator
		Error(TXT_ILLEGAL_OPERATION, node);
	}
}


void asCCompiler::CompileMathOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
{
	// Implicitly convert the operands to a number type
	asCDataType to;
	if( ltype->dataType.IsDoubleType() || rtype->dataType.IsDoubleType() )
		to.tokenType = ttDouble;
	else if( ltype->dataType.IsFloatType() || rtype->dataType.IsFloatType() )
		to.tokenType = ttFloat;
	else if( ltype->dataType.IsIntegerType() || rtype->dataType.IsIntegerType() )
		to.tokenType = ttInt;
	else if( ltype->dataType.IsUnsignedType() || rtype->dataType.IsUnsignedType() )
		to.tokenType = ttUInt;
	else if( ltype->dataType.IsBitVectorType() || rtype->dataType.IsBitVectorType() )
		to.tokenType = ttUInt;

	// Do the actual conversion
	ImplicitConversion(lbc, to, ltype, node, false);
	ImplicitConversion(rbc, to, rtype, node, false);

	// Verify that the conversion was successful
	if( !ltype->dataType.IsIntegerType() &&
		!ltype->dataType.IsUnsignedType() &&
		!ltype->dataType.IsFloatType() &&
		!ltype->dataType.IsDoubleType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_MATH_TYPE, ltype->dataType.Format().AddressOf());
		Error(str, node);
	}

	if( !rtype->dataType.IsIntegerType() &&
		!rtype->dataType.IsUnsignedType() &&
		!rtype->dataType.IsFloatType() &&
		!rtype->dataType.IsDoubleType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_MATH_TYPE, rtype->dataType.Format().AddressOf());
		Error(str, node);
	}

	bool isConstant = ltype->isConstant && rtype->isConstant;

	int op = node->tokenType;
	if( !isConstant )
	{
		if( op == ttAddAssign || op == ttSubAssign ||
			op == ttMulAssign || op == ttDivAssign ||
			op == ttModAssign )
		{
			// Compound assignments execute the right hand value first
			bc->AddCode(rbc);
			bc->AddCode(lbc);

			// Swap the values where necessary
			if( op == ttSubAssign || op == ttDivAssign || op == ttModAssign )
			{
				if( ltype->dataType.GetSizeOnStackDWords() == 1 )
					bc->Instr(BC_SWAP4);
				else
					bc->Instr(BC_SWAP8);
			}
		}
		else
		{
			bc->AddCode(lbc);
			bc->AddCode(rbc);
		}
	}

	type->Set(ltype->dataType);

	if( ltype->dataType.IsIntegerType() ||
		ltype->dataType.IsUnsignedType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				bc->Instr(BC_ADDi);
			else if( op == ttMinus || op == ttSubAssign )
				bc->Instr(BC_SUBi);
			else if( op == ttStar || op == ttMulAssign )
				bc->Instr(BC_MULi);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rtype->isConstant && rtype->intValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_DIVi);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rtype->isConstant && rtype->intValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_MODi);
			}
		}
		else
		{
			int v = 0;
			if( op == ttPlus )
				v = ltype->intValue + rtype->intValue;
			else if( op == ttMinus )
				v = ltype->intValue - rtype->intValue;
			else if( op == ttStar )
				v = ltype->intValue * rtype->intValue;
			else if( op == ttSlash )
			{
				if( rtype->intValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = ltype->intValue / rtype->intValue;
			}
			else if( op == ttPercent )
			{
				if( rtype->intValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = ltype->intValue % rtype->intValue;
			}

			bc->InstrINT(BC_SET4, v);

			type->isConstant = true;
			type->intValue = v;
		}
	}
	else if( ltype->dataType.IsFloatType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				bc->Instr(BC_ADDf);
			else if( op == ttMinus || op == ttSubAssign )
				bc->Instr(BC_SUBf);
			else if( op == ttStar || op == ttMulAssign )
				bc->Instr(BC_MULf);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rtype->isConstant && rtype->floatValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_DIVf);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rtype->isConstant && rtype->floatValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_MODf);
			}
		}
		else
		{
			float v = 0.0f;
			if( op == ttPlus )
				v = ltype->floatValue + rtype->floatValue;
			else if( op == ttMinus )
				v = ltype->floatValue - rtype->floatValue;
			else if( op == ttStar )
				v = ltype->floatValue * rtype->floatValue;
			else if( op == ttSlash )
			{
				if( rtype->floatValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = ltype->floatValue / rtype->floatValue;
			}
			else if( op == ttPercent )
			{
				if( rtype->floatValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = fmodf(ltype->floatValue, rtype->floatValue);
			}

			bc->InstrFLOAT(BC_SET4, v);

			type->isConstant = true;
			type->floatValue = v;
		}
	}
	else if( ltype->dataType.IsDoubleType() )
	{
		if( !isConstant )
		{
			if( op == ttPlus || op == ttAddAssign )
				bc->Instr(BC_ADDd);
			else if( op == ttMinus || op == ttSubAssign )
				bc->Instr(BC_SUBd);
			else if( op == ttStar || op == ttMulAssign )
				bc->Instr(BC_MULd);
			else if( op == ttSlash || op == ttDivAssign )
			{
				if( rtype->isConstant && rtype->doubleValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_DIVd);
			}
			else if( op == ttPercent || op == ttModAssign )
			{
				if( rtype->isConstant && rtype->doubleValue == 0 )
					Error(TXT_DIVIDE_BY_ZERO, node);
				bc->Instr(BC_MODd);
			}
		}
		else
		{
			double v = 0.0;
			if( op == ttPlus )
				v = ltype->doubleValue + rtype->doubleValue;
			else if( op == ttMinus )
				v = ltype->doubleValue - rtype->doubleValue;
			else if( op == ttStar )
				v = ltype->doubleValue * rtype->doubleValue;
			else if( op == ttSlash )
			{
				if( rtype->doubleValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = ltype->doubleValue / rtype->doubleValue;
			}
			else if( op == ttPercent )
			{
				if( rtype->doubleValue == 0 )
				{
					Error(TXT_DIVIDE_BY_ZERO, node);
					v = 0;
				}
				else
					v = fmod(ltype->doubleValue, rtype->doubleValue);
			}

			bc->InstrDOUBLE(BC_SET8, v);

			type->isConstant = true;
			type->doubleValue = v;
		}
	}
}

void asCCompiler::CompileBitwiseOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
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
		ImplicitConversion(lbc, to, ltype, node, false);
		ImplicitConversion(rbc, to, rtype, node, false);

		// Verify that the conversion was successful
		if( !ltype->dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, ltype->dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		if( !rtype->dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, rtype->dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		bool isConstant = ltype->isConstant && rtype->isConstant;

		if( !isConstant )
		{
			if( op == ttAndAssign || op == ttOrAssign || op == ttXorAssign )
			{
				// Compound assignments execute the right hand value first
				bc->AddCode(rbc);
				bc->AddCode(lbc);
			}
			else
			{
				bc->AddCode(lbc);
				bc->AddCode(rbc);
			}
		}

		// Remember the result
		type->Set(asCDataType(ltype->dataType.tokenType, true, false));

		if( !isConstant )
		{
			if( op == ttAmp || op == ttAndAssign )
				bc->Instr(BC_BAND);
			else if( op == ttBitOr || op == ttOrAssign )
				bc->Instr(BC_BOR);
			else if( op == ttBitXor || op == ttXorAssign )
				bc->Instr(BC_BXOR);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttAmp )
				v = ltype->dwordValue & rtype->dwordValue;
			else if( op == ttBitOr )
				v = ltype->dwordValue | rtype->dwordValue;
			else if( op == ttBitXor )
				v = ltype->dwordValue ^ rtype->dwordValue;

			bc->InstrDWORD(BC_SET4, v);

			// Remember the result
			type->isConstant = true;
			type->dwordValue = v;
		}
	}
	else if( op == ttBitShiftLeft       || op == ttShiftLeftAssign   ||
		     op == ttBitShiftRight      || op == ttShiftRightLAssign ||
			 op == ttBitShiftRightArith || op == ttShiftRightAAssign )
	{
		// Left operand must be bitvector
		asCDataType to;
		to.tokenType = ttBits;
		ImplicitConversion(lbc, to, ltype, node, false);

		// Right operand must be uint
		to.tokenType = ttUInt;
		ImplicitConversion(rbc, to, rtype, node, false);

		// Verify that the conversion was successful
		if( !ltype->dataType.IsBitVectorType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, ltype->dataType.Format().AddressOf(), "bits");
			Error(str, node);
		}

		if( !rtype->dataType.IsUnsignedType() )
		{
			asCString str;
			str.Format(TXT_NO_CONVERSION_s_TO_s, rtype->dataType.Format().AddressOf(), "uint");
			Error(str, node);
		}

		bool isConstant = ltype->isConstant && rtype->isConstant;

		if( !isConstant )
		{
			if( op == ttShiftLeftAssign || op == ttShiftRightLAssign || op == ttShiftRightAAssign )
			{
				// Compound assignments execute the right hand value first
				bc->AddCode(rbc);
				bc->AddCode(lbc);

				// Swap the values
				bc->Instr(BC_SWAP4);
			}
			else
			{
				bc->AddCode(lbc);
				bc->AddCode(rbc);
			}
		}

		// Remember the result
		type->Set(asCDataType(ltype->dataType.tokenType, true, false));

		if( !isConstant )
		{
			if( op == ttBitShiftLeft || op == ttShiftLeftAssign )
				bc->Instr(BC_BSLL);
			else if( op == ttBitShiftRight || op == ttShiftRightLAssign )
				bc->Instr(BC_BSRL);
			else if( op == ttBitShiftRightArith || op == ttShiftRightAAssign )
				bc->Instr(BC_BSRA);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttBitShiftLeft )
				v = ltype->dwordValue << rtype->dwordValue;
			else if( op == ttBitShiftRight )
				v = ltype->dwordValue >> rtype->dwordValue;
			else if( op == ttBitShiftRightArith )
				v = ltype->intValue >> rtype->dwordValue;

			bc->InstrDWORD(BC_SET4, v);

			// Remember the result
			type->isConstant = true;
			type->dwordValue = v;
		}
	}
}

void asCCompiler::CompileComparisonOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
{
	// Both operands must be of the same type

	// Implicitly convert the operands to a number type
	asCDataType to;
	if( ltype->dataType.IsDoubleType() || rtype->dataType.IsDoubleType() )
		to.tokenType = ttDouble;
	else if( ltype->dataType.IsFloatType() || rtype->dataType.IsFloatType() )
		to.tokenType = ttFloat;
	else if( ltype->dataType.IsIntegerType() || rtype->dataType.IsIntegerType() )
		to.tokenType = ttInt;
	else if( ltype->dataType.IsUnsignedType() || rtype->dataType.IsUnsignedType() )
		to.tokenType = ttUInt;
	else if( ltype->dataType.IsBitVectorType() || rtype->dataType.IsBitVectorType() )
		to.tokenType = ttUInt;
	else if( ltype->dataType.IsBooleanType() || rtype->dataType.IsBooleanType() )
		to.tokenType = ttBool;

	// Is it an operation on signed values?
	bool signMismatch = false;
	if( !ltype->dataType.IsUnsignedType() || !rtype->dataType.IsUnsignedType() )
	{
		if( ltype->dataType.tokenType == ttUInt )
		{
			if( !ltype->isConstant )
				signMismatch = true;
			else if( ltype->dwordValue & (1<<31) )
				signMismatch = true;
		}
		if( rtype->dataType.tokenType == ttUInt )
		{
			if( !rtype->isConstant )
				signMismatch = true;
			else if( rtype->dwordValue & (1<<31) )
				signMismatch = true;
		}
	}

	// Check for signed/unsigned mismatch
	if( signMismatch )
		Warning(TXT_SIGNED_UNSIGNED_MISMATCH, node);

	// Do the actual conversion
	ImplicitConversion(lbc, to, ltype, node, false);
	ImplicitConversion(rbc, to, rtype, node, false);

	// Verify that the conversion was successful
	if( !ltype->dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, ltype->dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	if( !rtype->dataType.IsEqualExceptConst(to) )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, rtype->dataType.Format().AddressOf(), to.Format().AddressOf());
		Error(str, node);
	}

	bool isConstant = ltype->isConstant && rtype->isConstant;
	int op = node->tokenType;
	
	type->Set(asCDataType(ttBool, true, false));

	if( to.IsBooleanType() )
	{
		int op = node->tokenType;
		if( op == ttEqual || op == ttNotEqual )
		{
			if( !isConstant )
			{
				// Make sure they are equal if not false
				lbc->Instr(BC_TNZ);
				rbc->Instr(BC_TNZ);

				bc->AddCode(lbc);
				bc->AddCode(rbc);

				if( op == ttEqual )
				{
					bc->Instr(BC_CMPi);
					bc->Instr(BC_TZ);
				}
				else if( op == ttNotEqual )
					bc->Instr(BC_CMPi);
			}
			else
			{
				// Make sure they are equal if not false
				if( ltype->dwordValue != 0 ) ltype->dwordValue = VALUE_OF_BOOLEAN_TRUE;
				if( rtype->dwordValue != 0 ) rtype->dwordValue = VALUE_OF_BOOLEAN_TRUE;

				asDWORD v = 0;
				if( op == ttEqual )
				{
					v = ltype->intValue - rtype->intValue;
					if( v == 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;
				}
				else if( op == ttNotEqual )
				{
					v = ltype->intValue - rtype->intValue;
					if( v != 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;
				}

				bc->InstrDWORD(BC_SET4, v);

				type->isConstant = true;
				type->intValue = v;
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
			bc->AddCode(lbc);
			bc->AddCode(rbc);

			if( ltype->dataType.IsIntegerType() )
				bc->Instr(BC_CMPi);
			else if( ltype->dataType.IsUnsignedType() )
				bc->Instr(BC_CMPui);
			else if( ltype->dataType.IsFloatType() )
				bc->Instr(BC_CMPf);
			else if( ltype->dataType.IsDoubleType() )
				bc->Instr(BC_CMPd);
			else
				assert(false);

			if( op == ttEqual )
				bc->Instr(BC_TZ);
			else if( op == ttNotEqual )
				bc->Instr(BC_TNZ);
			else if( op == ttLessThan )
				bc->Instr(BC_TS);
			else if( op == ttLessThanOrEqual )
				bc->Instr(BC_TNP);
			else if( op == ttGreaterThan )
				bc->Instr(BC_TP);
			else if( op == ttGreaterThanOrEqual )
				bc->Instr(BC_TNS);
		}
		else
		{
			int i = 0;
			if( ltype->dataType.IsIntegerType() )
			{
				int v = ltype->intValue - rtype->intValue;
				if( v < 0 ) i = -1;
				if( v > 0 ) i = 1;
			}
			else if( ltype->dataType.IsUnsignedType() )
			{
				asDWORD v1 = ltype->dwordValue;
				asDWORD v2 = rtype->dwordValue;
				if( v1 < v2 ) i = -1;
				if( v1 > v2 ) i = 1;
			}
			else if( ltype->dataType.IsFloatType() )
			{
				float v = ltype->floatValue - rtype->floatValue;
				if( v < 0 ) i = -1;
				if( v > 0 ) i = 1;
			}
			else if( ltype->dataType.IsDoubleType() )
			{
				double v = ltype->doubleValue - rtype->doubleValue;
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

			bc->InstrINT(BC_SET4, i);

			type->isConstant = true;
			type->intValue = i;
		}
	}
}

void asCCompiler::CompileBooleanOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type)
{
	// Both operands must be booleans
	asCDataType to;
	to.tokenType = ttBool;

	// Do the actual conversion
	ImplicitConversion(lbc, to, ltype, node, false);
	ImplicitConversion(rbc, to, rtype, node, false);

	// Verify that the conversion was successful
	if( !ltype->dataType.IsBooleanType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, ltype->dataType.Format().AddressOf(), "bool");
		Error(str, node);
	}

	if( !rtype->dataType.IsBooleanType() )
	{
		asCString str;
		str.Format(TXT_NO_CONVERSION_s_TO_s, rtype->dataType.Format().AddressOf(), "bool");
		Error(str, node);
	}

	bool isConstant = ltype->isConstant && rtype->isConstant;

	type->Set(asCDataType(ttBool, true, false));

	// What kind of operator is it?
	int op = node->tokenType;
	if( op == ttXor )
	{
		if( !isConstant )
		{
			// Make sure they are equal if not false
			lbc->Instr(BC_TNZ);
			rbc->Instr(BC_TNZ);

			bc->AddCode(lbc);
			bc->AddCode(rbc);

			bc->Instr(BC_CMPi);
		}
		else
		{
			// Make sure they are equal if not false
			if( ltype->dwordValue != 0 ) ltype->dwordValue = VALUE_OF_BOOLEAN_TRUE;
			if( rtype->dwordValue != 0 ) rtype->dwordValue = VALUE_OF_BOOLEAN_TRUE;

			asDWORD v = 0;
			v = ltype->intValue - rtype->intValue;
			if( v != 0 ) v = VALUE_OF_BOOLEAN_TRUE; else v = 0;

			bc->InstrDWORD(BC_SET4, v);

			type->isConstant = true;
			type->intValue = v;
		}
	}
	else if( op == ttAnd ||
			 op == ttOr )
	{
		if( !isConstant )
		{
			// If or-operator and first value is 1 the second value shouldn't be calculated
			// if and-operator and first value is 0 the second value shouldn't be calculated
			bc->AddCode(lbc);
			int label1 = nextLabel++;
			int label2 = nextLabel++;
			if( op == ttAnd )
			{
				bc->InstrINT(BC_JNZ, label1);
				bc->InstrDWORD(BC_SET4, 0); // Let optimizer change to SET1
				bc->InstrINT(BC_JMP, label2);
			}
			else if( op == ttOr )
			{
				bc->InstrINT(BC_JZ, label1);
				bc->InstrDWORD(BC_SET4, 1); // Let optimizer change to SET1
				bc->InstrINT(BC_JMP, label2);
			}

			bc->Label((short)label1);
			bc->AddCode(rbc);
			bc->Label((short)label2);
		}
		else
		{
			asDWORD v = 0;
			if( op == ttAnd )
				v = ltype->dwordValue && rtype->dwordValue;
			else if( op == ttOr )
				v = ltype->dwordValue || rtype->dwordValue;

			bc->InstrDWORD(BC_SET4, v);

			// Remember the result
			type->isConstant = true;
			type->dwordValue = v;
		}
	}
}


void asCCompiler::PerformFunctionCall(int funcID, asCTypeInfo *type, asCByteCode *bc)
{
	asCScriptFunction *descr = builder->GetFunctionDescription(funcID);

	int argSize = descr->GetSpaceNeededForArguments();

	// Is the return value returned by reference?
	bool returnByRef = false;
	int returnOffset = 0;
	type->Set(descr->returnType);
	if( descr->returnType.IsComplex(engine) && !descr->returnType.isReference )
	{
		returnByRef = true;

		// Allocate temporary variable
		// The constructor will be called by the function
		returnOffset = AllocateVariable(descr->returnType, true);
		type->isTemporary = true;
		type->stackOffset = (short)returnOffset;

		// We receive a reference to the temporary variable
		type->dataType.isReference = true;

		// Push address to the temporary variable as first argument
		bc->InstrWORD(BC_PSF, (asWORD)returnOffset);
		argSize++;
	}

	// Script functions should be referenced locally (i.e module id == 0)
	if( descr->id < 0 )
	{
		bc->Call(BC_CALLSYS, descr->id, argSize + (descr->objectType ? 1 : 0));
	}
	else
	{
		if( descr->id & FUNC_IMPORTED )
			bc->Call(BC_CALLBND, descr->id, argSize + (descr->objectType ? 1 : 0));
		else
			bc->Call(BC_CALL, descr->id, argSize + (descr->objectType ? 1 : 0));
	}

	// Add destructor for returned value to exception handler
	if( !descr->returnType.isReference )
	{
		asSTypeBehaviour *beh = builder->GetBehaviour(&descr->returnType);
		if( beh && beh->destruct )
			bc->Destructor(BC_ADDDESTRSF, (asDWORD)beh->destruct, returnOffset);
	}

	// Push returned value on stack
	if( returnByRef )
		bc->Instr(BC_RRET4);
	else if( descr->returnType.GetSizeOnStackDWords() == 1 )
	{
		bc->Instr(BC_RRET4);
		if( !descr->returnType.isReference )
		{
			if( descr->returnType.GetSizeInMemoryBytes() == 1 )
			{
				if( descr->returnType.IsIntegerType() )
					bc->Instr(BC_SB);
				else
					bc->Instr(BC_UB);
			}
			else if( descr->returnType.GetSizeInMemoryBytes() == 2 )
			{
				if( descr->returnType.IsIntegerType() )
					bc->Instr(BC_SW);
				else
					bc->Instr(BC_UW);
			}
		}
	}
	else if( descr->returnType.GetSizeOnStackDWords() == 2 )
		bc->Instr(BC_RRET8);
}



