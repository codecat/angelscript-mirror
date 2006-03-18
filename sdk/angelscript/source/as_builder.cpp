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
// as_builder.cpp
//
// This is the class that manages the compilation of the scripts
//


#include "as_config.h"
#include "as_builder.h"
#include "as_parser.h"
#include "as_compiler.h"
#include "as_tokendef.h"
#include "as_string_util.h"
#include "as_outputbuffer.h"
#include "as_texts.h"

asCBuilder::asCBuilder(asCScriptEngine *engine, asCModule *module)
{
	this->engine = engine;
	this->module = module;
	out = 0;
}

asCBuilder::~asCBuilder()
{
	int n;

	// Free all functions
	for( n = 0; n < functions.GetLength(); n++ )
	{
		if( functions[n] )
		{
			if( functions[n]->node ) 
				delete functions[n]->node;

			delete functions[n];
		}

		functions[n] = 0;
	}

	// Free all global variables
	for( n = 0; n < globVariables.GetLength(); n++ )
	{
		if( globVariables[n] )
		{
			if( globVariables[n]->node )
				delete globVariables[n]->node;

			delete globVariables[n];
			globVariables[n] = 0;
		}
	}

	// Free all the loaded files
	for( n = 0; n < scripts.GetLength(); n++ )
	{
		if( scripts[n] )
			delete scripts[n];

		scripts[n] = 0;
	}
}

void asCBuilder::SetOutputStream(asIOutputStream *out)
{
	this->out = out;
}

int asCBuilder::AddCode(const char *name, const char *code, int codeLength, int lineOffset, int sectionIdx, bool makeCopy)
{
	asCScriptCode *script = new asCScriptCode;
	script->SetCode(name, code, codeLength, makeCopy);
	script->lineOffset = lineOffset;
	script->idx = sectionIdx;
	scripts.PushLast(script);

	return 0;
}

int asCBuilder::Build()
{
	numErrors = 0;
	numWarnings = 0;

	ParseScripts();
	CompileGlobalVariables();
	CompileFunctions();

	if( numErrors > 0 )
		return asERROR;

	return asSUCCESS;
}

int asCBuilder::BuildString(const char *string, asCContext *ctx)
{
	numErrors = 0;
	numWarnings = 0;

	// Add the string to the script code
	asCScriptCode *script = new asCScriptCode;
	script->SetCode(TXT_EXECUTESTRING, string, true);
	script->lineOffset = -1; // Compensate for "void ExecuteString() {\n"
	scripts.PushLast(script);

	// Parse the string
	asCParser parser(this);
	if( parser.ParseScript(scripts[0]) >= 0 )
	{
		// Find the function
		asCScriptNode *node = parser.GetScriptNode();
		node = node->firstChild;
		if( node->nodeType == snFunction )
		{
			node->DisconnectParent();

			sFunctionDescription *func = new sFunctionDescription;
			functions.PushLast(func);

			func->script = scripts[0];
			func->node = node;
			func->name = "";
		}
		else
		{
			// TODO: An error occurred
			assert(false);
		}
	}

	if( numErrors == 0 )
	{
		// Compile the function
		asCCompiler compiler;
		if( compiler.CompileFunction(this, functions[0]->script, functions[0]->node) >= 0 )
		{
			asCScriptFunction *execfunc = new asCScriptFunction;
			execfunc->id = asFUNC_STRING | module->moduleID;

			// Copy byte code to the registered function
			execfunc->byteCode.SetLength(compiler.byteCode.GetSize());
			compiler.byteCode.Output(execfunc->byteCode.AddressOf());
			execfunc->stackNeeded = compiler.byteCode.largestStackUsed;
			execfunc->lineNumbers = compiler.byteCode.lineNumbers;

			execfunc->objVariablePos = compiler.objVariablePos;
			execfunc->objVariableTypes = compiler.objVariableTypes;

			ctx->SetExecuteStringFunction(execfunc);

#ifdef AS_DEBUG
			// DEBUG: output byte code
			compiler.byteCode.DebugOutput("__ExecuteString.txt", module, engine);
#endif
		}
	}

	if( numErrors > 0 )
		return asERROR;

	return asSUCCESS;
}

void asCBuilder::ParseScripts()
{
	// Parse all the files into one
	asCParser parser(this);
	int n = 0;
	while( n < scripts.GetLength() )
	{
		// Parse the script file
		if( parser.ParseScript(scripts[n]) >= 0 )
		{
			// Find global nodes
			asCScriptNode *node = parser.GetScriptNode();
			node = node->firstChild;
			while( node )
			{
				asCScriptNode *next = node->next;
				node->DisconnectParent();

				if( node->nodeType == snFunction )
				{
					RegisterScriptFunction(module->GetNextFunctionId(), node, scripts[n]);
				}
				else if( node->nodeType == snGlobalVar )
				{
					RegisterGlobalVar(node, scripts[n]);
				}
				else if( node->nodeType == snImport )
				{
					RegisterImportedFunction(module->GetNextImportedFunctionId(), node, scripts[n]);
				}
				else
				{
					// Unused script node
					int r, c;
					scripts[n]->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteWarning(scripts[n]->name, TXT_UNUSED_SCRIPT_NODE, r, c);

					delete node;
				}

				node = next;
			}
		}

		n++;
	}
}

void asCBuilder::CompileFunctions()
{
	// Compile each function
	for( int n = 0; n < functions.GetLength(); n++ )
	{
		asCCompiler compiler;

		int r, c;
		functions[n]->script->ConvertPosToRowCol(functions[n]->node->tokenPos, &r, &c);
		asCString str = module->scriptFunctions[n]->GetDeclaration(engine);
		str.Format(TXT_COMPILING_s, str.AddressOf());
		WriteInfo(functions[n]->script->name, str, r, c, true);

		if( compiler.CompileFunction(this, functions[n]->script, functions[n]->node) >= 0 )
		{
			// Copy byte code to the registered function
			module->scriptFunctions[n]->byteCode.SetLength(compiler.byteCode.GetSize());
			compiler.byteCode.Output(module->scriptFunctions[n]->byteCode.AddressOf());
			module->scriptFunctions[n]->stackNeeded = compiler.byteCode.largestStackUsed;
			module->scriptFunctions[n]->lineNumbers = compiler.byteCode.lineNumbers;

			module->scriptFunctions[n]->objVariablePos = compiler.objVariablePos;
			module->scriptFunctions[n]->objVariableTypes = compiler.objVariableTypes;

#ifdef AS_DEBUG
			// DEBUG: output byte code
			compiler.byteCode.DebugOutput("__" + functions[n]->name + ".txt", module, engine);
#endif
		}

		preMessage = "";
	}
}

int asCBuilder::ParseDataType(const char *datatype, asCDataType *result)
{
	numErrors = 0;
	numWarnings = 0;

	asCScriptCode source;
	source.SetCode("", datatype, true);

	asCParser parser(this);
	int r = parser.ParseDataType(&source);
	if( r < 0 )
		return asINVALID_TYPE;

	// Get data type and property name
	asCScriptNode *dataType = parser.GetScriptNode()->firstChild;

	*result = CreateDataTypeFromNode(dataType, &source);

	if( numErrors > 0 )
		return asINVALID_TYPE;

	return asSUCCESS;
}

int asCBuilder::VerifyProperty(asCDataType *dt, const char *decl, asCString &name, asCDataType &type)
{
	numErrors = 0;
	numWarnings = 0;

	if( dt )
	{
		// Verify that the object type exist
		if( dt->objectType == 0 )
			return asINVALID_OBJECT;
	}

	// Check property declaration and type
	asCScriptCode source;
	source.SetCode(TXT_PROPERTY, decl, true);

	asCParser parser(this);
	int r = parser.ParsePropertyDeclaration(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// Get data type and property name
	asCScriptNode *dataType = parser.GetScriptNode()->firstChild;

	asCScriptNode *nameNode = dataType->next;

	type = CreateDataTypeFromNode(dataType, &source);
	name.Copy(&decl[nameNode->tokenPos], nameNode->tokenLength);

	// Verify property name
	if( dt )
		if( CheckNameConflictMember(*dt, name, nameNode, &source) < 0 )
			return asINVALID_NAME;
	else
		if( CheckNameConflict(name, nameNode, &source) < 0 )
			return asINVALID_NAME;

	if( numErrors > 0 )
		return asINVALID_DECLARATION;

	return asSUCCESS;
}

asCProperty *asCBuilder::GetObjectProperty(asCDataType &obj, const char *prop)
{
	assert(obj.objectType >= 0);

	// TODO: Improve linear search
	asCArray<asCProperty *> &props = obj.objectType->properties;
	for( int n = 0; n < props.GetLength(); n++ )
		if( props[n]->name == prop )
			return props[n];

	return 0;
}

asCProperty *asCBuilder::GetGlobalProperty(const char *prop, bool *isCompiled)
{
	int n;

	if( isCompiled ) *isCompiled = true;

	// TODO: Improve linear search
	// Check application registered properties
	asCArray<asCProperty *> *props = &(engine->globalProps);
	for( n = 0; n < props->GetLength(); ++n )
		if( (*props)[n]->name == prop )
			return (*props)[n];

	// TODO: Improve linear search
	// Check properties being compiled now
	asCArray<sGlobalVariableDescription *> *gvars = &globVariables;
	for( n = 0; n < gvars->GetLength(); ++n )
	{
		if( (*gvars)[n]->name == prop )
		{
			if( isCompiled ) *isCompiled = (*gvars)[n]->isCompiled;

			return (*gvars)[n]->property;
		}
	}

	// TODO: Improve linear search
	// Check previously compiled global variables
	if( module )
	{
		props = &module->scriptGlobals;
		for( n = 0; n < props->GetLength(); ++n )
			if( (*props)[n]->name == prop )
				return (*props)[n];
	}

	return 0;
}

int asCBuilder::ParseFunctionDeclaration(const char *decl, asCScriptFunction *func)
{
	numErrors = 0;
	numWarnings = 0;

	asCScriptCode source;
	source.SetCode(TXT_SYSTEM_FUNCTION, decl, true);

	asCParser parser(this);

	int r = parser.ParseFunctionDefinition(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	asCScriptNode *node = parser.GetScriptNode();

	// Find name
	asCScriptNode *n = node->firstChild->next->next;
	func->name.Copy(&source.code[n->tokenPos], n->tokenLength);

	// Initialize a script function object for registration
	func->returnType = CreateDataTypeFromNode(node->firstChild, &source);
	func->returnType = ModifyDataTypeFromNode(func->returnType, node->firstChild->next, 0);

	n = n->next->firstChild;
	while( n )
	{
		int inOutFlags;
		asCDataType type = CreateDataTypeFromNode(n, &source);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlags);
		
		// Store the parameter type
		func->parameterTypes.PushLast(type);
		func->inOutFlags.PushLast(inOutFlags);

		// Move to next parameter
		n = n->next->next;
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Set the read-only flag if const is declared after parameter list
	if( node->lastChild->nodeType == snUndefined && node->lastChild->tokenType == ttConst )
		func->isReadOnly = true;
	else 
		func->isReadOnly = false;

	if( numErrors > 0 || numWarnings > 0 )
		return asINVALID_DECLARATION;

	return 0;
}

int asCBuilder::ParseVariableDeclaration(const char *decl, asCProperty *var)
{
	numErrors = 0;
	numWarnings = 0;

	asCScriptCode source;
	source.SetCode(TXT_VARIABLE_DECL, decl, true);

	asCParser parser(this);

	int r = parser.ParsePropertyDeclaration(&source);
	if( r < 0 )
		return asINVALID_DECLARATION;

	asCScriptNode *node = parser.GetScriptNode();

	// Find name
	asCScriptNode *n = node->firstChild->next;
	var->name.Copy(&source.code[n->tokenPos], n->tokenLength);

	// Initialize a script variable object for registration
	var->type = CreateDataTypeFromNode(node->firstChild, &source);

	if( numErrors > 0 || numWarnings > 0 )
		return asINVALID_DECLARATION;

	return 0;
}

int asCBuilder::CheckNameConflictMember(asCDataType &dt, const char *name, asCScriptNode *node, asCScriptCode *code)
{
	// Check against object types
	if( engine->GetObjectType(name) != 0 )
	{
		if( code )
		{
			int r, c;
			code->ConvertPosToRowCol(node->tokenPos, &r, &c);

			asCString str;
			str.Format(TXT_NAME_CONFLICT_s_EXTENDED_TYPE, name);
			WriteError(code->name, str, r, c);
		}

		return -1;
	}

	// Check against other members
	asCObjectType *t = dt.objectType;

	// TODO: Improve linear search
	asCArray<asCProperty *> &props = t->properties;
	for( int n = 0; n < props.GetLength(); n++ )
	{
		if( props[n]->name == name )
		{
			if( code )
			{
				int r, c;
				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				asCString str;
				str.Format(TXT_NAME_CONFLICT_s_OBJ_PROPERTY, name);
				WriteError(code->name, str, r, c);
			}

			return -1;
		}
	}

	// TODO: Property names must be checked against method names
/*
	if( !isSystemFunction )
	{
		acCArray<int> funcs;
		GetObjectMethodDescriptions(name, t, funcs);
		if( funcs.GetLength() > 0 )
		{
			if( code )
			{
				int r, c;
				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				acCString str;
				str.Format(TXT_NAME_CONFLICT_s_OBJ_METHOD, name);
				WriteError(str, r, c);
			}

			return -1;
		}
	}
*/
	return 0;
}

int asCBuilder::CheckNameConflict(const char *name, asCScriptNode *node, asCScriptCode *code)
{
	// Check against object types
	if( engine->GetObjectType(name) != 0 )
	{
		if( code )
		{
			int r, c;
			code->ConvertPosToRowCol(node->tokenPos, &r, &c);

			asCString str;
			str.Format(TXT_NAME_CONFLICT_s_EXTENDED_TYPE, name);
			WriteError(code->name, str, r, c);
		}

		return -1;
	}

	// Check against global properties
	asCProperty *prop = GetGlobalProperty(name, 0);
	if( prop )
	{
		if( code )
		{
			int r, c;
			code->ConvertPosToRowCol(node->tokenPos, &r, &c);

			asCString str;
			str.Format(TXT_NAME_CONFLICT_s_GLOBAL_PROPERTY, name);

			WriteError(code->name, str, r, c);
		}

		return -1;
	}

	// TODO: Property names must be checked against function names
/*
	if( !isSystemFunction )
	{
		// Check against functions
		acCArray<int> funcs;
		GetFunctionDescriptions(name, funcs);
		if( funcs.GetLength() > 0 )
		{
			if( code )
			{
				int r, c;

				code->ConvertPosToRowCol(node->tokenPos, &r, &c);

				acCString str;
				str.Format(TXT_NAME_CONFLICT_s_FUNCTION, name);

				WriteError(str, r, c);
			}

			return -1;
		}
	}
*/

	return 0;
}


int asCBuilder::RegisterGlobalVar(asCScriptNode *node, asCScriptCode *file)
{
	// What data type is it?
	asCDataType type = CreateDataTypeFromNode(node->firstChild, file);

	if( type.GetSizeOnStackDWords() == 0 || 
		(type.IsObject() && !type.isExplicitHandle && type.GetSizeInMemoryBytes() == 0) )
	{
		asCString str;
		// TODO: Change to "'type' cannot be declared as variable"
		str.Format(TXT_DATA_TYPE_CANT_BE_s, (const char *)type.Format());
		
		int r, c;
		file->ConvertPosToRowCol(node->tokenPos, &r, &c);
		
		WriteError(file->name, str, r, c);
	}

	asCScriptNode *n = node->firstChild->next;

	while( n )
	{
		// Verify that the name isn't taken
		GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
		CheckNameConflict(name, n, file);

		// Register the global variable
		sGlobalVariableDescription *gvar = new sGlobalVariableDescription;
		globVariables.PushLast(gvar);

		gvar->script     = file;
		gvar->name       = name;
		gvar->isCompiled = false;
		gvar->datatype   = type;

		// TODO: Give error message if wrong
		assert(!gvar->datatype.isReference);

		// Allocate space on the global memory stack
		gvar->index      = module->AllocGlobalMemory(gvar->datatype.GetSizeOnStackDWords());
		gvar->node       = 0;
		if( n->next && n->next->nodeType == snAssignment )
		{
			gvar->node       = n->next;
			n->next->DisconnectParent();
		}
		else if( n->next && n->next->nodeType == snArgList) 
		{
			gvar->node = n->next;			
			n->next->DisconnectParent();				
		}

		// Add script variable to engine
		asCProperty *prop = new asCProperty;
		prop->index      = gvar->index;
		prop->name       = name;
		prop->type       = gvar->datatype;
		module->scriptGlobals.PushLast(prop);

		gvar->property = prop;

		n = n->next;
	}

	delete node;

	return 0;
}

void asCBuilder::CompileGlobalVariables()
{
	bool compileSucceeded = true;

	asCByteCode finalInit;
	asCByteCode finalExit;

	// Store state of compilation (errors, warning, output)
	int currNumErrors = numErrors;
	int currNumWarnings = numWarnings;
	asIOutputStream *stream = out;
	asCOutputBuffer outBuffer;
	out = &outBuffer;

	asCString finalOutput;

	while( compileSucceeded )
	{
		compileSucceeded = false;

		int accumErrors = 0;
		int accumWarnings = 0;

		// Restore state of compilation
		finalOutput = "";

		for( int n = 0; n < globVariables.GetLength(); n++ )
		{
			numWarnings = 0;
			numErrors = 0;
			outBuffer.output = "";
			asCByteCode init;
			asCByteCode exit;

			sGlobalVariableDescription *gvar = globVariables[n];
			if( gvar->isCompiled == false )
			{
				if( gvar->node )
				{
					int r, c;
					gvar->script->ConvertPosToRowCol(gvar->node->tokenPos, &r, &c);
					asCString str = gvar->datatype.Format();
					str += " " + gvar->name;
					str.Format(TXT_COMPILING_s, str.AddressOf());
					WriteInfo(gvar->script->name, str, r, c, true);
				}

				asCCompiler comp;
				int r = comp.CompileGlobalVariable(this, gvar->script, gvar->node, gvar);
				if( r >= 0 )
				{
					// Compilation succeeded
					gvar->isCompiled = true;
					compileSucceeded = true;

					init.AddCode(&comp.byteCode);
				}

				// Call destructor for all data types
				if( gvar->datatype.IsObject() && !gvar->datatype.isReference )
				{
					int objTypeIdx = engine->GetObjectTypeIndex(gvar->datatype.objectType);

					exit.InstrINT(BC_PGA, gvar->index);
					exit.InstrINT(BC_FREE, objTypeIdx);
				}

				if( gvar->isCompiled )
				{
					// Add warnings for this constant to the total build
					if( numWarnings )
					{
						currNumWarnings += numWarnings;
						stream->Write(outBuffer.output);
					}

					// Add compiled byte code to the final init and exit functions
					finalInit.AddCode(&init);
					finalExit.AddCode(&exit);
				}
				else
				{
					// Add output to final output
					finalOutput += outBuffer.output;
					accumErrors += numErrors;
					accumWarnings += numWarnings;
				}

				preMessage = "";
			}
		}

		if( !compileSucceeded )
		{
			// Add errors and warnings to total build
			currNumWarnings += accumWarnings;
			currNumErrors += accumErrors;

			if( stream ) stream->Write(finalOutput);
		}
	}

	// Restore states
	out = stream;
	numWarnings = currNumWarnings;
	numErrors = currNumErrors;

	// Register init code and clean up code
	finalInit.Ret(0);
	finalExit.Ret(0);

	finalInit.Finalize();
	finalExit.Finalize();

	asCByteCode cleanInit;
	asCByteCode cleanExit;

	module->initFunction.byteCode.SetLength(finalInit.GetSize());
	finalInit.Output(module->initFunction.byteCode.AddressOf());
	module->initFunction.stackNeeded = finalInit.largestStackUsed;

	module->exitFunction.byteCode.SetLength(finalExit.GetSize());
	finalExit.Output(module->exitFunction.byteCode.AddressOf());
	module->exitFunction.stackNeeded = finalExit.largestStackUsed;

#ifdef AS_DEBUG
	// DEBUG: output byte code
	finalInit.DebugOutput("__@init.txt", module, engine);
	finalExit.DebugOutput("__@exit.txt", module, engine);
#endif

}

int asCBuilder::RegisterScriptFunction(int funcID, asCScriptNode *node, asCScriptCode *file)
{
	// Find name 
	asCScriptNode *n = node->firstChild->next->next;

	// Check for name conflicts
	GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
	CheckNameConflict(name, n, file);

	sFunctionDescription *func = new sFunctionDescription;
	functions.PushLast(func);

	func->script = file;
	func->node   = node;
	func->name   = name;

	// Initialize a script function object for registration
	asCDataType returnType;
	returnType = CreateDataTypeFromNode(node->firstChild, file);
	returnType = ModifyDataTypeFromNode(returnType, node->firstChild->next, 0);
		
	asCArray<asCDataType> parameterTypes;
	asCArray<int> inOutFlags;
	n = n->next->firstChild;
	while( n )
	{
		int inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlag);

		// Store the parameter type
		parameterTypes.PushLast(type);
		inOutFlags.PushLast(inOutFlag);

		// Move to next parameter
		n = n->next->next;
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Check that the same function hasn't been registered already
	asCArray<int> funcs;
	GetFunctionDescriptions(name, funcs);
	if( funcs.GetLength() )
	{
		for( int n = 0; n < funcs.GetLength(); ++n )
		{
			asCScriptFunction *func = GetFunctionDescription(funcs[n]);

			if( parameterTypes.GetLength() == func->parameterTypes.GetLength() )
			{
				bool match = true;
				for( int p = 0; p < parameterTypes.GetLength(); ++p )
				{
					if( parameterTypes[p] != func->parameterTypes[p] )
					{
						match = false;
						break;
					}
				}

				if( match )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteError(file->name, TXT_FUNCTION_ALREADY_EXIST, r, c);
					break;
				}
			}
		}
	}

	// Register the function
	module->AddScriptFunction(file->idx, funcID, func->name, returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), parameterTypes.GetLength());

	return 0;
}

int asCBuilder::RegisterImportedFunction(int importID, asCScriptNode *node, asCScriptCode *file)
{
	// Find name 
	asCScriptNode *f = node->firstChild;
	asCScriptNode *n = f->firstChild->next->next;

	// Check for name conflicts
	GETSTRING(name, &file->code[n->tokenPos], n->tokenLength);
	CheckNameConflict(name, n, file);

	// Initialize a script function object for registration
	asCDataType returnType;
	returnType = CreateDataTypeFromNode(f->firstChild, file);
	returnType = ModifyDataTypeFromNode(returnType, f->firstChild->next, 0);
		
	asCArray<asCDataType> parameterTypes;
	asCArray<int> inOutFlags;
	n = n->next->firstChild;
	while( n )
	{
		int inOutFlag;
		asCDataType type = CreateDataTypeFromNode(n, file);
		type = ModifyDataTypeFromNode(type, n->next, &inOutFlag);

		// Store the parameter type
		n = n->next->next;
		parameterTypes.PushLast(type);
		inOutFlags.PushLast(inOutFlag);

		// Move to next parameter
		if( n && n->nodeType == snIdentifier )
			n = n->next;
	}

	// Check that the same function hasn't been registered already
	asCArray<int> funcs;
	GetFunctionDescriptions(name, funcs);
	if( funcs.GetLength() )
	{
		for( int n = 0; n < funcs.GetLength(); ++n )
		{
			asCScriptFunction *func = GetFunctionDescription(funcs[n]);

			// TODO: Isn't the name guaranteed to be equal, because of GetFunctionDescriptions()?
			if( name == func->name && 
				parameterTypes.GetLength() == func->parameterTypes.GetLength() )
			{
				bool match = true;
				for( int p = 0; p < parameterTypes.GetLength(); ++p )
				{
					if( parameterTypes[p] != func->parameterTypes[p] )
					{
						match = false;
						break;
					}
				}

				if( match )
				{
					int r, c;
					file->ConvertPosToRowCol(node->tokenPos, &r, &c);

					WriteError(file->name, TXT_FUNCTION_ALREADY_EXIST, r, c);
					break;
				}
			}
		}
		
	}

	// Read the module name as well
	n = node->firstChild->next;
	int moduleNameString = RegisterConstantBStr(&file->code[n->tokenPos+1], n->tokenLength-2);

	delete node;

	// Register the function
	module->AddImportedFunction(importID, name, returnType, parameterTypes.AddressOf(), inOutFlags.AddressOf(), parameterTypes.GetLength(), moduleNameString);

	return 0;
}


asCScriptFunction *asCBuilder::GetFunctionDescription(int id)
{
	// The top 16 bits holds the moduleID

	// Get the description from the engine
	if( id < 0 )
		return engine->systemFunctions[-id - 1];
	else if( (id & 0xFFFF0000) == 0 )
		return module->scriptFunctions[id];
	else 
		return module->importedFunctions[id & 0xFFFF];
}

void asCBuilder::GetFunctionDescriptions(const char *name, asCArray<int> &funcs)
{
	int n;
	// TODO: Improve linear search
	for( n = 0; n < module->scriptFunctions.GetLength(); n++ )
	{
		if( module->scriptFunctions[n]->name == name )
			funcs.PushLast(module->scriptFunctions[n]->id);
	}

	// TODO: Improve linear search
	for( n = 0; n < module->importedFunctions.GetLength(); n++ )
	{
		if( module->importedFunctions[n]->name == name )
			funcs.PushLast(module->importedFunctions[n]->id);
	}

	// TODO: Improve linear search
	for( n = 0; n < engine->systemFunctions.GetLength(); n++ )
	{
		if( engine->systemFunctions[n]->objectType == 0 && engine->systemFunctions[n]->name == name )
			funcs.PushLast(engine->systemFunctions[n]->id);
	}
}

void asCBuilder::GetObjectMethodDescriptions(const char *name, asCObjectType *objectType, asCArray<int> &methods, bool objIsConst)
{
	// TODO: Improve linear search
	if( objIsConst )
	{
		// Only add const methods to the list
		for( int n = 0; n < engine->systemFunctions.GetLength(); n++ )
		{
			if( engine->systemFunctions[n]->objectType == objectType &&
				engine->systemFunctions[n]->name == name &&
				engine->systemFunctions[n]->isReadOnly )
				methods.PushLast(engine->systemFunctions[n]->id);
		}
	}
	else
	{
		// TODO: Prefer non-const over const
		for( int n = 0; n < engine->systemFunctions.GetLength(); n++ )
		{
			if( engine->systemFunctions[n]->objectType == objectType &&
				engine->systemFunctions[n]->name == name )
				methods.PushLast(engine->systemFunctions[n]->id);
		}
	}
}

void asCBuilder::WriteInfo(const char *scriptname, const char *message, int r, int c, bool pre)
{
	if( out )
	{
		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_INFO, message);

		if( pre )
			preMessage = str;
		else
			out->Write(str);
	}
}

void asCBuilder::WriteError(const char *scriptname, const char *message, int r, int c)
{
	numErrors++;

	if( out )
	{
		if( preMessage != "" )
		{
			out->Write(preMessage);
			preMessage = "";
		}

		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_ERROR, message);

		out->Write(str);
	}
}

void asCBuilder::WriteWarning(const char *scriptname, const char *message, int r, int c)
{
	numWarnings++;

	if( out )
	{
		if( preMessage != "" )
		{
			out->Write(preMessage);
			preMessage = "";
		}

		asCString str;
		str.Format("%s (%d, %d) : %s : %s\n", scriptname, r, c, TXT_WARNING, message);

		out->Write(str);
	}
}


asCDataType asCBuilder::CreateDataTypeFromNode(asCScriptNode *node, asCScriptCode *file)
{
	assert(node->nodeType == snDataType);

	asCDataType dt;

	asCScriptNode *n = node->firstChild;

	if( n->tokenType == ttConst )
	{
		dt.isReadOnly = true;
		n = n->next;
	}

	asCScriptNode *typeNode = n;

	dt.tokenType = n->tokenType;
	if( dt.tokenType == ttIdentifier )
	{
		asCString str;
		str.Copy(&file->code[n->tokenPos], n->tokenLength);

		dt.extendedType = engine->GetObjectType(str);
		dt.objectType = dt.extendedType;
		if( dt.extendedType == 0 )
		{
			str.Format(TXT_IDENTIFIER_s_NOT_DATA_TYPE, (const char *)str);

			int r, c;
			file->ConvertPosToRowCol(n->tokenPos, &r, &c);

			WriteError(file->name, str, r, c);

			dt.tokenType = ttInt;
		}
	}
	else
	{
		dt.extendedType = 0;
		dt.objectType = 0;
	}

	// Determine array dimensions and object handles
	n = n->next;
	int arrayType = 0;
	int arrayDimensions = 0;
	bool isHandle = false;
	while( n && (n->tokenType == ttOpenBracket || n->tokenType == ttHandle) )
	{
		if( n->tokenType == ttOpenBracket )
		{
			if( arrayDimensions == 4 )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);
				WriteError(file->name, TXT_TOO_MANY_ARRAY_DIMENSIONS, r, c);
				break;
			}
			// Push another level into the description
			arrayType = (arrayType << 2) | 2 | (isHandle ? 1 : 0);
			arrayDimensions++;
			isHandle = false;
		}
		else
		{
			if( isHandle )
			{
				int r, c;
				file->ConvertPosToRowCol(n->tokenPos, &r, &c);
				WriteError(file->name, TXT_OBJECT_HANDLE_NOT_SUPPORTED, r, c);
				break;
			}

			isHandle = true;
		}
		n = n->next;
	}

	dt.arrayType = arrayType;
	if( dt.arrayType )
		dt.objectType = engine->GetArrayType(dt);

	if( isHandle )
	{
		dt.isExplicitHandle = true;
		if( !dt.objectType || dt.objectType->beh.addref == 0 || dt.objectType->beh.release == 0 )
		{
			int r, c;
			file->ConvertPosToRowCol(typeNode->tokenPos, &r, &c);

			WriteError(file->name, TXT_OBJECT_HANDLE_NOT_SUPPORTED, r, c);

			dt.isExplicitHandle = false;
		}
	}

	// TODO: Must verify that each of the levels support object handles

	return dt;
}

asCDataType asCBuilder::ModifyDataTypeFromNode(const asCDataType &type, asCScriptNode *node, int *inOutFlags)
{
	asCDataType dt = type;

	// Is the argument sent by reference?
	asCScriptNode *n = node->firstChild;
	if( n && n->tokenType == ttAmp )
	{
		dt.isReference = true;
		n = n->next;
	}

	if( inOutFlags ) *inOutFlags = 0;

	if( n && inOutFlags )
	{
		if( n->tokenType == ttIn ) 
			*inOutFlags = 1;
		else if( n->tokenType == ttOut )
			*inOutFlags = 2;
		else if( n->tokenType == ttInOut )
			*inOutFlags = 3;
		else
			assert(false);
	}

	return dt;
}

int asCBuilder::RegisterConstantBStr(const char *bstr, int len)
{
	asCArray<char> str;
	str.Allocate(len, false);

	for( int n = 0; n < len; n++ )
	{
		if( bstr[n] == '\\' )
		{
			++n;
			if( n == len ) return -1;

			if( bstr[n] == '"' )
				str.PushLast('"');
			else if( bstr[n] == 'n' )
				str.PushLast('\n');
			else if( bstr[n] == 'r' )
				str.PushLast('\r');
			else if( bstr[n] == '0' )
				str.PushLast('\0');
			else if( bstr[n] == '\\' )
				str.PushLast('\\');
			else if( bstr[n] == 'x' || bstr[n] == 'X' )
			{
				++n;
				if( n == len ) break;

				int val = 0;
				if( bstr[n] >= '0' && bstr[n] <= '9' )
					val = bstr[n] - '0';
				else if( bstr[n] >= 'a' && bstr[n] <= 'f' )
					val = bstr[n] - 'a' + 10;
				else if( bstr[n] >= 'A' && bstr[n] <= 'F' )
					val = bstr[n] - 'A' + 10;
				else
					continue;

				++n;
				if( n == len )
				{
					str.PushLast((char)val);
					break;
				}

				if( bstr[n] >= '0' && bstr[n] <= '9' )
					val = val*16 + bstr[n] - '0';
				else if( bstr[n] >= 'a' && bstr[n] <= 'f' )
					val = val*16 + bstr[n] - 'a' + 10;
				else if( bstr[n] >= 'A' && bstr[n] <= 'F' )
					val = val*16 + bstr[n] - 'A' + 10;
				else
				{
					str.PushLast((char)val);
					continue;
				}

				str.PushLast((char)val);
			}
			else
				continue;
		}
		else
			str.PushLast(bstr[n]);
	}

	return module->AddConstantBStr(str.AddressOf(), str.GetLength());
}

asBSTR *asCBuilder::GetConstantBStr(int bstrID)
{
	return module->GetConstantBStr(bstrID);
}

