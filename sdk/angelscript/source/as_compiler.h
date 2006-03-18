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
// as_compiler.h
//
// The class that does the actual compilation of the functions
//



#ifndef AS_COMPILER_H
#define AS_COMPILER_H

#include "as_builder.h"
#include "as_scriptfunction.h"
#include "as_variablescope.h"
#include "as_bytecode.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_types.h"
#include "as_typeinfo.h"

class asCCompiler
{
public:
	asCCompiler();
	~asCCompiler();

	int CompileFunction(asCBuilder *builder, asCScriptCode *script, asCScriptNode *func);
	int CompileGlobalVariable(asCBuilder *builder, asCScriptCode *script, asCScriptNode *expr, sGlobalVariableDescription *gvar);

	asCByteCode byteCode;
	asCByteCode cleanCode;

protected:
	friend class asCBuilder;

	void Reset(asCBuilder *builder, asCScriptCode *script);

	// Statements
	void CompileStatementBlock(asCScriptNode *block, bool ownVariableScope, bool *hasReturn, asCByteCode *bc);
	void CompileDeclaration(asCScriptNode *decl, asCByteCode *bc);
	void CompileStatement(asCScriptNode *statement, bool *hasReturn, asCByteCode *bc);
	void CompileIfStatement(asCScriptNode *node, bool *hasReturn, asCByteCode *bc);
	void CompileSwitchStatement(asCScriptNode *node, bool *hasReturn, asCByteCode *bc);
	void CompileCase(asCScriptNode *node, asCByteCode *bc);
	void CompileForStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileWhileStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileDoWhileStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileBreakStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileContinueStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileReturnStatement(asCScriptNode *node, asCByteCode *bc);
	void CompileExpressionStatement(asCScriptNode *node, asCByteCode *bc);

	// Expressions
	void CompileAssignment(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type);
	void CompileCondition(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type);
	void CompileExpression(asCScriptNode *expr, asCByteCode *bc, asCTypeInfo *type);
	void SwapPostFixOperands(asCArray<asCScriptNode *> &postfix, asCArray<asCScriptNode *> &target);
	void CompilePostFixExpression(asCArray<asCScriptNode *> *postfix, asCByteCode *bc, asCTypeInfo *type);
	void CompileExpressionTerm(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type);
	void CompileExpressionPreOp(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type);
	void CompileExpressionPostOp(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type);
	void CompileExpressionValue(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type);
	void CompileFunctionCall(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type, asCObjectType *objectType);
	void CompileConversion(asCScriptNode *node, asCByteCode *bc, asCTypeInfo *type);
	void CompileOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	void CompileOperatorOnPointers(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	void CompileMathOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	void CompileBitwiseOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	void CompileComparisonOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	void CompileBooleanOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *lterm, asCTypeInfo *rterm, asCTypeInfo *res);
	bool CompileOverloadedOperator(asCScriptNode *node, asCByteCode *lbc, asCByteCode *rbc, asCByteCode *bc, asCTypeInfo *ltype, asCTypeInfo *rtype, asCTypeInfo *type);

	void DefaultConstructor(asCByteCode *bc, asCDataType &dt);
	void CompileConstructor(asCDataType &type, int offset, asCByteCode *bc);
	void CompileDestructor(asCDataType &type, int offset, asCByteCode *bc);
	void CompileArgumentList(asCScriptNode *node, asCArray<asCTypeInfo> &argTypes, asCArray<asCByteCode *> &argByteCodes, asCDataType *type = 0);
	void MatchFunctions(asCArray<int> &funcs, asCArray<asCTypeInfo> &argTypes, asCScriptNode *node, const char *name);

	// Helper functions
	void PrepareOperand(asCTypeInfo *type, asCByteCode *bc, asCScriptNode *node);
	void PrepareForAssignment(asCDataType *lvalue, asCTypeInfo *rvalue, asCByteCode *bc, asCScriptNode *node);
	void PerformAssignment(asCTypeInfo *lvalue, asCByteCode *bc, asCScriptNode *node);
	bool IsVariableInitialized(asCTypeInfo *type, asCScriptNode *node);
	void Dereference(asCByteCode *bc, asCTypeInfo *type);
	void ImplicitConversion(asCByteCode *bc, const asCDataType &to, asCTypeInfo *from, asCScriptNode *node, bool isExplicit);
	void ImplicitConversionConstant(asCByteCode *bc, const asCDataType &to, asCTypeInfo *from, asCScriptNode *node, bool isExplicit);
	int  MatchArgument(asCArray<int> &funcs, asCArray<int> &matches, asCTypeInfo *argType, int paramNum);
    void MoveArgumentToReservedSpace(asCDataType &paramType, asCTypeInfo *argType, asCScriptNode *node, asCByteCode *bc, bool haveReservedSpace, int offset);
	int  ReserveSpaceForArgument(asCDataType &dt, asCByteCode *bc, bool alwaysReserve);
    void PerformFunctionCall(int funcID, asCTypeInfo *type, asCByteCode *bc);
	void PrepareFunctionCall(int funcID, asCScriptNode *argListNode, asCByteCode *bc, asCArray<asCTypeInfo> &argTypes, asCArray<asCByteCode *> &argByteCodes);
	 
	void LineInstr(asCByteCode *bc, int pos);

	int  RegisterConstantBStr(const char *str, int len);
	int  GetPrecedence(asCScriptNode *op);

	void Error(const char *msg, asCScriptNode *node);
	void Warning(const char *msg, asCScriptNode *node);

	void AddVariableScope(bool isBreakScope = false, bool isContinueScope = false);
	void RemoveVariableScope();

	bool hasCompileErrors;

	int nextLabel;

	asCVariableScope *variables;
	asCBuilder *builder;
	asCScriptEngine *engine;
	asCScriptCode *script;

	asCArray<int> breakLabels;
	asCArray<int> continueLabels;

	int AllocateVariable(const asCDataType &type, bool isTemporary);
	int GetVariableOffset(int varIndex);
	int GetVariableSlot(int varOffset);
	void DeallocateVariable(int pos);
	void ReleaseTemporaryVariable(asCTypeInfo &t, asCByteCode *bc);

	asCArray<asCDataType> variableAllocations;
	asCArray<int> freeVariables;
	asCArray<int> tempVariables;

	bool globalExpression;
};

#endif
