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
// as_scriptengine.cpp
//
// The implementation of the script engine interface
//


#include <malloc.h>

#include "as_config.h"
#include "as_scriptengine.h"
#include "as_builder.h"
#include "as_context.h"
#include "as_bstr_util.h"
#include "as_string_util.h"
#include "as_tokenizer.h"
#include "as_texts.h"
#include "as_module.h"
#include "as_callfunc.h"
#include "as_arrayobject.h"
#include "as_generic.h"

AS_API const char * asGetLibraryVersion()
{
#ifdef _DEBUG
	return ANGELSCRIPT_VERSION_STRING " DEBUG";
#else
	return ANGELSCRIPT_VERSION_STRING;
#endif
}

AS_API asIScriptEngine * asCreateScriptEngine(asDWORD version)
{
	if( (version/10000) != ANGELSCRIPT_VERSION_MAJOR )
		return 0;

	if( (version/100)%100 != ANGELSCRIPT_VERSION_MINOR )
		return 0;

	if( (version%100) > ANGELSCRIPT_VERSION_BUILD )
		return 0;

	return new asCScriptEngine();
}

asCScriptEngine::asCScriptEngine()
{
	refCount = 1;
	
	stringFactory = 0;

	configFailed = false;

	isPrepared = false;

	lastModule = 0;

	initialContextStackSize = 1024;      // 1 KB
	maximumContextStackSize = 0;         // no limit

#ifdef USE_ASM_VM
	asCContext::CreateRelocTable();
#endif

	RegisterArrayObject(this);
}

asCScriptEngine::~asCScriptEngine()
{
	assert(refCount == 0);

	Reset();

	int n;
	for( n = 0; n < scriptModules.GetLength(); n++ )
		if( scriptModules[n] ) delete scriptModules[n];
	scriptModules.SetLength(0);

	for( n = 0; n < globalProps.GetLength(); n++ )
	{
		if( globalProps[n] )
			delete globalProps[n];
	}
	globalProps.SetLength(0);
	globalPropAddresses.SetLength(0);

	for( n = 0; n < arrayTypes.GetLength(); n++ )
	{
		if( arrayTypes[n] )
			delete arrayTypes[n];
	}
	arrayTypes.SetLength(0);
	
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] )
			delete objectTypes[n];
	}
	objectTypes.SetLength(0);

	for( n = 0; n < systemFunctions.GetLength(); n++ )
		delete systemFunctions[n];
	systemFunctions.SetLength(0);

	for( n = 0; n < systemFunctionInterfaces.GetLength(); n++ )
		delete systemFunctionInterfaces[n];
	systemFunctionInterfaces.SetLength(0);
}

int asCScriptEngine::AddRef()
{
	ENTERCRITICALSECTION(engineCritical);
	int r = ++refCount;
	LEAVECRITICALSECTION(engineCritical);
	return r;
}

int asCScriptEngine::Release()
{	
	ENTERCRITICALSECTION(engineCritical);
	int r = --refCount;

	if( refCount == 0 )
	{
		// Must leave the critical section before deleting the object
		LEAVECRITICALSECTION(engineCritical);

		delete this;
		return 0;
	}

	LEAVECRITICALSECTION(engineCritical);

	return r;
}

void asCScriptEngine::Reset()
{
	int n;
	for( n = 0; n < scriptModules.GetLength(); ++n )
	{
		if( scriptModules[n] )
			scriptModules[n]->Discard();
	}
}

int asCScriptEngine::AddScriptSection(const char *module, const char *name, const char *code, int codeLength, int lineOffset, bool makeCopy)
{
	asCModule *mod = GetModule(module, true);
	if( mod == 0 ) return asNO_MODULE;

	// Discard the module if it is in use
	if( mod->IsUsed() )
	{
		mod->Discard();
		
		// Get another module
		mod = GetModule(module, true);
	}

	return mod->AddScriptSection(name, code, codeLength, lineOffset, makeCopy);
}

int asCScriptEngine::Build(const char *module, asIOutputStream *out)
{
	if( configFailed )
	{
		if( out )
			out->Write(TXT_INVALID_CONFIGURATION);
		return asINVALID_CONFIGURATION;
	}

	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->Build(out);
}

int asCScriptEngine::Discard(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	mod->Discard();

	// TODO: Must protect this for multiple accesses
	// Verify if there are any modules that can be deleted
	for( int n = 0; n < scriptModules.GetLength(); n++ )
	{
		if( scriptModules[n] && scriptModules[n]->CanDelete() )
		{
			delete scriptModules[n];
			scriptModules[n] = 0;
		}
	}
	
	return 0;
}

int asCScriptEngine::ResetModule(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->ResetGlobalVars();
}

int asCScriptEngine::GetFunctionCount(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetFunctionCount();
}

int asCScriptEngine::GetFunctionIDByName(const char *module, const char *name)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetFunctionIDByName(name);
}

int asCScriptEngine::GetFunctionIDByDecl(const char *module, const char *decl)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetFunctionIDByDecl(decl);
}

const char *asCScriptEngine::GetFunctionDeclaration(int funcID, int *length)
{
	asCString *tempString = &threadManager.GetLocalData()->string;
	if( (funcID & 0xFFFF) == asFUNC_STRING )
	{
		*tempString = "void @ExecuteString()";
	}
	else
	{
		asCScriptFunction *func = GetScriptFunction(funcID);
		if( func == 0 ) return 0;

		*tempString = func->GetDeclaration(this);
	}

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetFunctionSection(int funcID, int *length)
{
	asCString *tempString = &threadManager.GetLocalData()->string;
	if( (funcID & 0xFFFF) == asFUNC_STRING )
	{
		*tempString = "@ExecuteString";
	}
	else
	{
		asCScriptFunction *func = GetScriptFunction(funcID);
		if( func == 0 ) return 0;

		asCModule *module = GetModule(funcID);
		if( module == 0 ) return 0;

		*tempString = *module->scriptSections[func->scriptSectionIdx];
	}

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetFunctionName(int funcID, int *length)
{
	asCString *tempString = &threadManager.GetLocalData()->string;
	if( (funcID & 0xFFFF) == asFUNC_STRING )
	{
		*tempString = "@ExecuteString";
	}
	else
	{
		asCScriptFunction *func = GetScriptFunction(funcID);
		if( func == 0 ) return 0;

		*tempString = func->name;
	}

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

int asCScriptEngine::GetGlobalVarCount(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetGlobalVarCount();
}

int asCScriptEngine::GetGlobalVarIDByIndex(const char *module, int index)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->moduleID | index;
}

int asCScriptEngine::GetGlobalVarIDByName(const char *module, const char *name)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetGlobalVarIDByName(name);
}

int asCScriptEngine::GetGlobalVarIDByDecl(const char *module, const char *decl)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetGlobalVarIDByDecl(decl);
}

const char *asCScriptEngine::GetGlobalVarDeclaration(int gvarID, int *length)
{
	asCModule *mod = GetModule(gvarID);
	if( mod == 0 ) return 0;

	int id = gvarID & 0xFFFF;
	if( id > mod->scriptGlobals.GetLength() )
		return 0;

	asCProperty *prop = mod->scriptGlobals[id];

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = prop->type.Format();
	*tempString += " " + prop->name;

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetGlobalVarName(int gvarID, int *length)
{
	asCModule *mod = GetModule(gvarID);
	if( mod == 0 ) return 0;

	int id = gvarID & 0xFFFF;
	if( id > mod->scriptGlobals.GetLength() )
		return 0;

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = mod->scriptGlobals[id]->name;

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

int asCScriptEngine::GetGlobalVarPointer(int gvarID, void **pointer)
{
	asCModule *mod = GetModule(gvarID);
	if( mod == 0 ) return asNO_MODULE;

	int id = gvarID & 0xFFFF;
	if( id > mod->scriptGlobals.GetLength() )
		return asNO_GLOBAL_VAR;

	if( mod->scriptGlobals[id]->type.IsObject() )
		*pointer = *(void**)(mod->globalMem.AddressOf() + mod->scriptGlobals[id]->index);
	else
		*pointer = (void*)(mod->globalMem.AddressOf() + mod->scriptGlobals[id]->index);

	return 0;
}


// Internal
asCString asCScriptEngine::GetFunctionDeclaration(int funcID)
{
	asCString str;
	if( funcID < 0 && (-funcID - 1) < systemFunctions.GetLength() )
	{
		str = systemFunctions[-funcID - 1]->GetDeclaration(this);
	}
	else
	{
		asCScriptFunction *func = GetScriptFunction(funcID);
		if( func )
			str = func->GetDeclaration(this);
	}

	return str;
}

int asCScriptEngine::CreateContext(asIScriptContext **context)
{
	return CreateContext(context, false);
}

int asCScriptEngine::CreateContext(asIScriptContext **context, bool isInternal)
{
	*context = new asCContext(this, !isInternal);

	return 0;
}

int asCScriptEngine::RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset)
{
	asCDataType dt, type;
	asCString name;

	int r;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(obj, &dt);
	if( r < 0 )
		return ConfigError(r);

	if( (r = bld.VerifyProperty(&dt, declaration, name, type)) < 0 )
		return ConfigError(r);

	// Store the property info	
	if( dt.objectType == 0 ) 
		return ConfigError(asINVALID_OBJECT);

	asCProperty *prop = new asCProperty;
	prop->name            = name;
	prop->type            = type;
	prop->byteOffset      = byteOffset;

	dt.objectType->properties.PushLast(prop);

	return asSUCCESS;
}

int asCScriptEngine::RegisterSpecialObjectType(const char *name, int byteSize, asDWORD flags)
{
	// Put the data type in the list
	asCObjectType *type = new asCObjectType;
	if( strcmp(name, asDEFAULT_ARRAY) == 0 )
		defaultArrayObjectType = type;

	type->tokenType = ttIdentifier;
	type->name = name;
	type->pointerLevel = 0;
	type->arrayType = 0;
	type->size = byteSize;
	type->flags = flags;

	// Store it in the object types
	objectTypes.PushLast(type);
	type->idx = allObjectTypes.GetLength();
	allObjectTypes.PushLast(type);

	return asSUCCESS;
}

int asCScriptEngine::RegisterObjectType(const char *name, int byteSize, asDWORD flags)
{
	// Verify flags
	if( flags > 17 )
		return ConfigError(asINVALID_ARG);

	// Verify type name
	if( name == 0 )
		return ConfigError(asINVALID_NAME);

	// Verify object size (valid sizes 0, 1, 2, or multiple of 4)
	if( byteSize < 0 )
		return ConfigError(asINVALID_ARG);
	
	if( byteSize < 4 && byteSize == 3 )
		return ConfigError(asINVALID_ARG);

	if( byteSize > 4 && (byteSize & 0x3) )
		return ConfigError(asINVALID_ARG);

	// Verify if the name has been registered as a type already
	for( int n = 0; n < allObjectTypes.GetLength(); n++ )
	{
		if( allObjectTypes[n]->name == name )
			return asALREADY_REGISTERED;
	}

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	int r = bld.ParseDataType(name, &dt);

	// If the builder fails, then the type name 
	// is new and it should be registered 
	if( r < 0 )
	{
		// Make sure the name is not a reserved keyword
		asCTokenizer t;
		int tokenLen;
		int token = t.GetToken(name, strlen(name), &tokenLen);
		if( token != ttIdentifier || strlen(name) != (unsigned)tokenLen )
			return ConfigError(asINVALID_NAME);

		int r = bld.CheckNameConflict(name, 0, 0);
		if( r < 0 ) 
			return ConfigError(asNAME_TAKEN);

		// Check against all members of the object types
		int n;
		for( n = 0; n < objectTypes.GetLength(); n++ )
		{
			int c;
			asCArray<asCProperty *> &props = objectTypes[n]->properties;
			for( c = 0; c < props.GetLength(); c++ )
				if( props[c]->name == name )
					return ConfigError(asNAME_TAKEN);

			asCObjectType *obj = objectTypes[n];
			for( c = 0; c < obj->methods.GetLength(); c++ )
				if( systemFunctions[obj->methods[c]]->name == name )
					return ConfigError(asNAME_TAKEN);
		}

		// Check against all members of the array types
		for( n = 0; n < arrayTypes.GetLength(); n++ )
		{
			int c;
			asCArray<asCProperty *> &props = arrayTypes[n]->properties;
			for( c = 0; c < props.GetLength(); c++ )
				if( props[c]->name == name )
					return ConfigError(asNAME_TAKEN);

			asCObjectType *obj = arrayTypes[n];
			for( c = 0; c < obj->methods.GetLength(); c++ )
				if( systemFunctions[obj->methods[c]]->name == name )
					return ConfigError(asNAME_TAKEN);
		}

		// Put the data type in the list
		asCObjectType *type = new asCObjectType;
		type->name = name;
		type->tokenType = ttIdentifier;
		type->pointerLevel = 0;
		type->arrayType = 0;
		type->size = byteSize;
		type->flags = flags;

		objectTypes.PushLast(type);
		type->idx = allObjectTypes.GetLength();
		allObjectTypes.PushLast(type);
	}
	else
	{
		// int[][] must not be allowed to be registered
		// if int[] hasn't been registered first
		if( dt.GetSubType(this).IsDefaultArrayType(this) )
			return ConfigError(asLOWER_ARRAY_DIMENSION_NOT_REGISTERED);

		if( dt.isReadOnly ||
			dt.isReference )
			return ConfigError(asINVALID_TYPE);
		
		// Put the data type in the list
		asCObjectType *type = new asCObjectType;
		if( dt.extendedType )
			type->name = dt.extendedType->name;
		type->tokenType = dt.tokenType;
		type->arrayType = dt.arrayType;
		type->size = byteSize;
		type->flags = flags;

		arrayTypes.PushLast(type);
		type->idx = allObjectTypes.GetLength();
		allObjectTypes.PushLast(type);
	}

	return asSUCCESS;
}

const int behave_dual_token[] =
{
	ttPlus,               // asBEHAVE_ADD
	ttMinus,              // asBEHAVE_SUBTRACT
	ttStar,               // asBEHAVE_MULTIPLY
	ttSlash,              // asBEHAVE_DIVIDE
	ttPercent,            // ssBEHAVE_MODULO
	ttEqual,              // asBEHAVE_EQUAL
	ttNotEqual,           // asBEHAVE_NOTEQUAL
	ttLessThan,           // asBEHAVE_LESSTHAN
	ttGreaterThan,        // asBEHAVE_GREATERTHAN
	ttLessThanOrEqual,    // asBEHAVE_LEQUAL
	ttGreaterThanOrEqual, // asBEHAVE_GEQUAL
	ttOr,                 // asBEHAVE_LOGIC_OR
	ttAnd,                // asBEHAVE_LOGIC_AND
	ttBitOr,              // asBEHAVE_BIT_OR
	ttAmp,                // asBEHAVE_BIT_AND
	ttBitXor,             // asBEHAVE_BIT_XOR
	ttBitShiftLeft,       // asBEHAVE_BIT_SLL
	ttBitShiftRight,      // asBEHAVE_BIT_SRL
	ttBitShiftRightArith  // asBEHAVE_BIT_SRA
};

const int behave_assign_token[] =
{
	ttAssignment,			// asBEHAVE_ASSIGNMENT
	ttAddAssign,			// asBEHAVE_ADD_ASSIGN
	ttSubAssign,			// asBEHAVE_SUB_ASSIGN
	ttMulAssign,			// asBEHAVE_MUL_ASSIGN
	ttDivAssign,			// asBEHAVE_DIV_ASSIGN
	ttModAssign,			// asBEHAVE_MOD_ASSIGN
	ttOrAssign,				// asBEHAVE_OR_ASSIGN 
	ttAndAssign,			// asBEHAVE_AND_ASSIGN
	ttXorAssign,			// asBEHAVE_XOR_ASSIGN
	ttShiftLeftAssign,		// asBEHAVE_SLL_ASSIGN
	ttShiftRightLAssign,	// asBEHAVE_SRL_ASSIGN
	ttShiftRightAAssign		// asBEHAVE_SRA_ASSIGN
};

int asCScriptEngine::RegisterSpecialObjectBehaviour(const char *datatype, asDWORD behaviour, const char *decl, asUPtr funcPointer, int callConv)
{
	assert( datatype );

	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(datatype, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;
	
	asCBuilder bld(this, 0);

	asSTypeBehaviour *beh;
	asCDataType type;

	bool isDefaultArray = strcmp(datatype, asDEFAULT_ARRAY) == 0;

	if( isDefaultArray )
		type.SetAsDefaultArray(this);

	beh = GetBehaviour(&type);

	// The object is sent by reference to the function
	type.isReference = true;

	// Verify function declaration
	asCScriptFunction func;

	r = bld.ParseFunctionDeclaration(decl, &func);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	if( isDefaultArray )
		func.objectType = defaultArrayObjectType;

	if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( isDefaultArray )
		{
			if( func.parameterTypes.GetLength() == 2 )
			{
				beh->construct = AddBehaviourFunction(func, internal);
				beh->constructors.PushLast(beh->construct);
			}
			else
				beh->constructors.PushLast(AddBehaviourFunction(func, internal));
		}
	}
	else if( behaviour == asBEHAVE_DESTRUCT )
	{
		if( beh->destruct )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->destruct = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_ADDREF )
	{
		if( beh->addref )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->addref = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_RELEASE)
	{
		if( beh->release )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->release = AddBehaviourFunction(func, internal);
	}
	else if( behaviour >= asBEHAVE_FIRST_ASSIGN && behaviour <= asBEHAVE_LAST_ASSIGN )
	{
		// Verify that there is exactly one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		if( isDefaultArray )
		{
			if( beh->copy )
				return ConfigError(asALREADY_REGISTERED);

			beh->copy = AddBehaviourFunction(func, internal);

			beh->operators.PushLast(ttAssignment);
			beh->operators.PushLast(beh->copy);
		}
	}
	else if( behaviour == asBEHAVE_INDEX )
	{
		// Verify that there is only one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.tokenType == ttVoid )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered already

		// Map behaviour to token
		beh->operators.PushLast(ttOpenBracket);
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
	}
	else
	{
		assert(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterObjectBehaviour(const char *datatype, asDWORD behaviour, const char *decl, asUPtr funcPointer, asDWORD callConv)
{
	if( datatype == 0 ) return ConfigError(asINVALID_ARG);

	asSSystemFunctionInterface internal;
	if( behaviour == asBEHAVE_ALLOC || behaviour == asBEHAVE_FREE )
	{
		if( callConv != asCALL_CDECL ) return ConfigError(asNOT_SUPPORTED);

		int r = DetectCallingConvention(0, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}
	else
	{
		if( callConv != asCALL_THISCALL &&
			callConv != asCALL_CDECL_OBJLAST &&
			callConv != asCALL_CDECL_OBJFIRST &&
			callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);

		int r = DetectCallingConvention(datatype, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}

	isPrepared = false;
	
	asCBuilder bld(this, 0);

	asSTypeBehaviour *beh;
	asCDataType type;

	int r = bld.ParseDataType(datatype, &type);
	if( r < 0 ) 
		return ConfigError(r);

	if( type.isReadOnly || type.isReference )
		return ConfigError(asINVALID_TYPE);

	// Verify that the type is allowed
	if( type.objectType == 0 )
		return ConfigError(asINVALID_TYPE);

	beh = GetBehaviour(&type, true);

	// The object is sent by reference to the function
	type.isReference = true;

	// Verify function declaration
	asCScriptFunction func;

	r = bld.ParseFunctionDeclaration(decl, &func);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	// Make sure none of the parameters (or return type) are default arrays
	for( int n = 0; n < func.parameterTypes.GetLength(); n++ )
		if( func.parameterTypes[n].IsDefaultArrayType(this) )
			return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
	if( func.returnType.IsDefaultArrayType(this) )
		return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);

	func.objectType = type.objectType;

	if( behaviour == asBEHAVE_ALLOC )
	{
		// The declaration must be "type &f(uint)"

		if( func.returnType != type )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes[0] != asCDataType(ttUInt, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		beh->alloc = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_FREE )
	{
		// The declaration must be "void f(type &in)"

		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes[0] != type )
			return ConfigError(asINVALID_DECLARATION);

		beh->free = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the same constructor hasn't been registered already

		// Store all constructors in a list
		if( func.parameterTypes.GetLength() == 0 )
		{
			beh->construct = AddBehaviourFunction(func, internal);
			beh->constructors.PushLast(beh->construct);
		}
		else
			beh->constructors.PushLast(AddBehaviourFunction(func, internal));
	}
	else if( behaviour == asBEHAVE_DESTRUCT )
	{
		if( beh->destruct )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->destruct = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_ADDREF )
	{
		if( beh->addref )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->addref = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_RELEASE)
	{
		if( beh->release )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType(ttVoid, false, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->release = AddBehaviourFunction(func, internal);
	}
	else if( behaviour >= asBEHAVE_FIRST_ASSIGN && behaviour <= asBEHAVE_LAST_ASSIGN )
	{
		// Verify that there is exactly one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is a reference to the object type
		if( func.returnType != type )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the rvalue is marked as in if a reference
		if( func.parameterTypes[0].isReference && func.inOutFlags[0] != 1 )
			return ConfigError(asINVALID_DECLARATION);

		if( behaviour == asBEHAVE_ASSIGNMENT && func.parameterTypes[0].IsEqualExceptConst(type) )
		{
			if( beh->copy )
				return ConfigError(asALREADY_REGISTERED);

			beh->copy = AddBehaviourFunction(func, internal);

			beh->operators.PushLast(ttAssignment);
			beh->operators.PushLast(beh->copy);
		}
		else
		{
			// TODO: Verify that the operator hasn't been registered with the same parameter already

			// Map behaviour to token
			beh->operators.PushLast(behave_assign_token[behaviour - asBEHAVE_FIRST_ASSIGN]); 
			beh->operators.PushLast(AddBehaviourFunction(func, internal));
		}
	}
	else if( behaviour == asBEHAVE_INDEX )
	{
		// Verify that there is only one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.tokenType == ttVoid )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered already

		// Map behaviour to token
		beh->operators.PushLast(ttOpenBracket);
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
	}
	else if( behaviour == asBEHAVE_NEGATE )
	{
		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is a the same as the type
		type.isReference = false;
		if( func.returnType != type  )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered already

		// Map behaviour to token
		beh->operators.PushLast(ttMinus);
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
	}
	else
	{
		assert(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterGlobalBehaviour(asDWORD behaviour, const char *decl, asUPtr funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(0, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;
	
	asCBuilder bld(this, 0);

	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);

	// We need a global behaviour structure
	asSTypeBehaviour *beh = &globalBehaviours;

	// Verify function declaration
	asCScriptFunction func;

	r = bld.ParseFunctionDeclaration(decl, &func);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	// Make sure none of the parameters (or return type) are default arrays
	for( int n = 0; n < func.parameterTypes.GetLength(); n++ )
		if( func.parameterTypes[n].IsDefaultArrayType(this) )
			return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
	if( func.returnType.IsDefaultArrayType(this) )
		return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);

	if( behaviour >= asBEHAVE_FIRST_DUAL && behaviour <= asBEHAVE_LAST_DUAL )
	{
		// Verify that there are exactly two parameters
		if( func.parameterTypes.GetLength() != 2 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.tokenType == ttVoid )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that at least one of the parameters is a registered type
		if( !(func.parameterTypes[0].tokenType == ttIdentifier) &&
			!(func.parameterTypes[1].tokenType == ttIdentifier) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that parameters by reference are marked as in
		if( (func.parameterTypes[0].isReference && func.inOutFlags[0] != 1) ||
			(func.parameterTypes[1].isReference && func.inOutFlags[1] != 1) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered with the same parameters already

		// Map behaviour to token
		beh->operators.PushLast(behave_dual_token[behaviour - asBEHAVE_FIRST_DUAL]); 
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
	}
	else
	{
		assert(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal)
{
	int id = -systemFunctions.GetLength() - 1;

	asSSystemFunctionInterface *newInterface = new asSSystemFunctionInterface;
	memcpy(newInterface, &internal, sizeof(internal));

	systemFunctionInterfaces.PushLast(newInterface);

	asCScriptFunction *f = new asCScriptFunction;
	f->returnType = func.returnType;
	f->objectType = func.objectType;
	f->id         = id;
	f->isReadOnly = func.isReadOnly;
	for( int n = 0; n < func.parameterTypes.GetLength(); n++ )
	{
		f->parameterTypes.PushLast(func.parameterTypes[n]);
		f->inOutFlags.PushLast(func.inOutFlags[n]);
	}

	systemFunctions.PushLast(f);

	return id;
}

int asCScriptEngine::GetObjectTypeIndex(const asCObjectType *type)
{
	if( type ) return type->idx;
	
	return -1;
}

asSTypeBehaviour *asCScriptEngine::GetBehaviour(const asCDataType *type, bool notDefault)
{
	if( type->objectType == 0 ) return 0;
	
	return &type->objectType->beh;
}

int asCScriptEngine::RegisterGlobalProperty(const char *declaration, void *pointer)
{
	asCDataType type;
	asCString name;

	int r;
	asCBuilder bld(this, 0);
	if( (r = bld.VerifyProperty(0, declaration, name, type)) < 0 )
		return ConfigError(r);

	// Don't allow registering references as global properties
	if( type.isReference )
		return ConfigError(asINVALID_TYPE);

	// Store the property info
	asCProperty *prop = new asCProperty;
	prop->name       = name;
	prop->type       = type;
	prop->index      = -1 - globalPropAddresses.GetLength();

	globalProps.PushLast(prop);
	globalPropAddresses.PushLast(pointer);

	if( type.IsObject() && !type.isReference )
	{
		// Create a pointer to a pointer
		prop->index = -1 - globalPropAddresses.GetLength();

		void **pp = &globalPropAddresses[globalPropAddresses.GetLength()-1];
		globalPropAddresses.PushLast(pp);
	}

	// Update all pointers to global objects,
	// because they change when the array is resized
	for( int n = 0; n < globalProps.GetLength(); n++ )
	{
		if( globalProps[n]->type.IsObject() && !globalProps[n]->type.isReference )
		{
			int idx = -globalProps[n]->index - 1;
			void **pp = &globalPropAddresses[idx-1];
			globalPropAddresses[idx] = (void*)pp;
		}
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterSpecialObjectMethod(const char *obj, const char *declaration, asUPtr funcPointer, int callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(obj, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	// We only support these calling conventions for object methods
	if( (unsigned)callConv != asCALL_THISCALL &&
		(unsigned)callConv != asCALL_CDECL_OBJLAST &&
		(unsigned)callConv != asCALL_CDECL_OBJFIRST )
		return ConfigError(asNOT_SUPPORTED);

	asCObjectType *objType = GetObjectType(obj);
	if( objType == 0 ) 
		return ConfigError(asINVALID_OBJECT);

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = new asSSystemFunctionInterface;
	memcpy(newInterface, &internal, sizeof(internal));
	systemFunctionInterfaces.PushLast(newInterface);

	asCScriptFunction *func = new asCScriptFunction();
	func->objectType = objType;

	objType->methods.PushLast(systemFunctions.GetLength());

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(declaration, func);
	if( r < 0 ) 
	{
		delete func;
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	asCDataType dt;
	dt.SetAsDefaultArray(this);
	r = bld.CheckNameConflictMember(dt, func->name, 0, 0);
	if( r < 0 )
	{
		delete func;
		return ConfigError(asNAME_TAKEN);
	}

	func->id = -1 - systemFunctions.GetLength();
	systemFunctions.PushLast(func);

	return 0;
}


int asCScriptEngine::RegisterObjectMethod(const char *obj, const char *declaration, asUPtr funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(obj, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	// We only support these calling conventions for object methods
	if( callConv != asCALL_THISCALL &&
		callConv != asCALL_CDECL_OBJLAST &&
		callConv != asCALL_CDECL_OBJFIRST &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);

	asCDataType dt;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(obj, &dt);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = new asSSystemFunctionInterface;
	memcpy(newInterface, &internal, sizeof(internal));
	systemFunctionInterfaces.PushLast(newInterface);

	asCScriptFunction *func = new asCScriptFunction();
	func->objectType = dt.objectType;

	func->objectType->methods.PushLast(systemFunctions.GetLength());

	r = bld.ParseFunctionDeclaration(declaration, func);
	if( r < 0 ) 
	{
		delete func;
		return ConfigError(asINVALID_DECLARATION);
	}

	// Make sure none of the parameters (or return type) are default arrays
	for( int n = 0; n < func->parameterTypes.GetLength(); n++ )
		if( func->parameterTypes[n].IsDefaultArrayType(this) )
		{
			delete func;
			return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
		}
	if( func->returnType.IsDefaultArrayType(this) )
	{
		delete func;
		return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
	}


	// Check name conflicts
	r = bld.CheckNameConflictMember(dt, func->name, 0, 0);
	if( r < 0 )
	{
		delete func;
		return ConfigError(asNAME_TAKEN);
	}

	func->id = -1 - systemFunctions.GetLength();
	systemFunctions.PushLast(func);

	return 0;
}

int asCScriptEngine::RegisterGlobalFunction(const char *declaration, asUPtr funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(0, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL && 
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = new asSSystemFunctionInterface;
	memcpy(newInterface, &internal, sizeof(internal));
	systemFunctionInterfaces.PushLast(newInterface);

	asCScriptFunction *func = new asCScriptFunction();

	asCBuilder bld(this, 0);

	r = bld.ParseFunctionDeclaration(declaration, func);
	if( r < 0 ) 
	{
		delete func;
		return ConfigError(asINVALID_DECLARATION);
	}

	// Make sure none of the parameters (or return type) are default arrays
	for( int n = 0; n < func->parameterTypes.GetLength(); n++ )
		if( func->parameterTypes[n].IsDefaultArrayType(this) )
		{
			delete func;
			return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
		}
	if( func->returnType.IsDefaultArrayType(this) )
	{
		delete func;
		return ConfigError(asAPP_CANT_INTERFACE_DEFAULT_ARRAY);
	}

	// Check name conflicts
	r = bld.CheckNameConflict(func->name, 0, 0);
	if( r < 0 )
	{
		delete func;
		return ConfigError(asNAME_TAKEN);
	}

	func->id = -1 - systemFunctions.GetLength();
	systemFunctions.PushLast(func);

	return 0;
}




asCScriptFunction *asCScriptEngine::GetScriptFunction(int funcID)
{
	asCModule *module = GetModule(funcID);
	if( module )
	{
		int f = funcID & 0xFFFF;
		if( f >= module->scriptFunctions.GetLength() )
			return 0;

		return module->GetScriptFunction(f);
	}

	return 0;
}



asCObjectType *asCScriptEngine::GetObjectType(const char *type, int pointerLevel, int arrayType)
{
	// TODO: Improve linear search
	for( int n = 0; n < objectTypes.GetLength(); n++ )
		if( objectTypes[n]->name == type &&
			objectTypes[n]->pointerLevel == pointerLevel && 
			objectTypes[n]->arrayType == arrayType )
			return objectTypes[n];

	return 0;
}



void asCScriptEngine::PrepareEngine()
{
	if( isPrepared ) return;

	for( int n = 0; n < systemFunctions.GetLength(); n++ )
	{
		// Determine the host application interface
		PrepareSystemFunction(systemFunctions[n], systemFunctionInterfaces[n], this);
	}

	isPrepared = true;
}

int asCScriptEngine::ConfigError(int err)
{ 
	configFailed = true; 
	return err; 
}


int asCScriptEngine::RegisterStringFactory(const char *datatype, asUPtr funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(0, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = new asSSystemFunctionInterface;
	memcpy(newInterface, &internal, sizeof(internal));
	systemFunctionInterfaces.PushLast(newInterface);

	asCScriptFunction *func = new asCScriptFunction();

	asCBuilder bld(this, 0);

	asCDataType dt;
	r = bld.ParseDataType(datatype, &dt);
	if( r < 0 ) 
	{
		delete func;
		return ConfigError(asINVALID_TYPE);
	}

	func->returnType = dt;
	func->parameterTypes.PushLast(asCDataType(ttInt, false, false));
	func->parameterTypes.PushLast(asCDataType(ttUInt8, true, true));
	func->id = -1 - systemFunctions.GetLength();
	systemFunctions.PushLast(func);

	stringFactory = func;

	return 0;
}

asCModule *asCScriptEngine::GetModule(const char *_name, bool create)
{
	// Accept null as well as zero-length string
	const char *name = "";
	if( _name != 0 ) name = _name;

	if( lastModule && lastModule->name == name )
	{
		if( !lastModule->isDiscarded )
			return lastModule;

		lastModule = 0;
	}

	// TODO: Improve linear search
	for( int n = 0; n < scriptModules.GetLength(); ++n )
		if( scriptModules[n] && scriptModules[n]->name == name )
		{
			if( !scriptModules[n]->isDiscarded )
			{
				lastModule = scriptModules[n];
				return lastModule;
			}
		}

	if( create )
	{
		// TODO: Store a list of free indices
		// Should find a free spot, not just the last one
		int idx;
		for( idx = 0; idx < scriptModules.GetLength(); ++idx )
			if( scriptModules[idx] == 0 )
				break;

		int moduleID = idx << 16;
		assert(moduleID <= 0x3FF0000);

		asCModule *module = new asCModule(name, moduleID, this);

		if( idx == scriptModules.GetLength() ) 
			scriptModules.PushLast(module);
		else
			scriptModules[idx] = module;

		lastModule = module;

		return lastModule;
	}

	return 0;
}

asCModule *asCScriptEngine::GetModule(int id)
{
	id = asMODULEIDX(id);
	if( id >= scriptModules.GetLength() ) return 0;
	return scriptModules[id];
}


int asCScriptEngine::SaveByteCode(const char *_module, asIBinaryStream *stream) 
{
	if( stream ) 
	{
		asCModule* module = GetModule(_module, false);

		// TODO: Shouldn't allow saving if the build wasn't successful

		if( module ) 
		{
			asCRestore rest(module, stream, this);
			return rest.Save();
		}

		return asNO_MODULE;
	}

	return asERROR;
}


int asCScriptEngine::LoadByteCode(const char *_module, asIBinaryStream *stream) 
{
	if( stream ) 
	{
		asCModule* module = GetModule(_module, true);
		if( module == 0 ) return asNO_MODULE;

		if( module->IsUsed() )
		{
			module->Discard();

			// Get another module
			module = GetModule(_module, true);
		}

		if( module ) 
		{
			asCRestore rest(module, stream, this);
			return rest.Restore();
		}

		return asNO_MODULE;
	}

	return asERROR;
}

int asCScriptEngine::SetDefaultContextStackSize(asUINT initial, asUINT maximum)
{
	// Sizes are given in bytes, but we store them in dwords
	initialContextStackSize = initial/4;
	maximumContextStackSize = maximum/4;

	return asSUCCESS;
}

int asCScriptEngine::GetImportedFunctionCount(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetImportedFunctionCount();
}

int asCScriptEngine::GetImportedFunctionIndexByDecl(const char *module, const char *decl)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->GetImportedFunctionIndexByDecl(decl);
}

const char *asCScriptEngine::GetImportedFunctionDeclaration(const char *module, int index, int *length)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return 0;

	asCScriptFunction *func = mod->GetImportedFunction(index);
	if( func == 0 ) return 0;

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = func->GetDeclaration(this);

	if( length ) *length = tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetImportedFunctionSourceModule(const char *module, int index, int *length)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return 0;

	const char *str = mod->GetImportedFunctionSourceModule(index);
	if( length && str )
		*length = strlen(str);

	return str;
}

int asCScriptEngine::BindImportedFunction(const char *module, int index, int funcID)
{
	asCModule *dstModule = GetModule(module, false);
	if( dstModule == 0 ) return asNO_MODULE;

	return dstModule->BindImportedFunction(index, funcID);
}

int asCScriptEngine::UnbindImportedFunction(const char *module, int index)
{
	asCModule *dstModule = GetModule(module, false);
	if( dstModule == 0 ) return asNO_MODULE;

	return dstModule->BindImportedFunction(index, -1);
}

const char *asCScriptEngine::GetModuleNameFromIndex(int index, int *length)
{
	asCModule *module = GetModule(index << 16);
	if( module == 0 ) return 0;

	const char *str = module->name.AddressOf();

	if( length && str )
		*length = strlen(str);

	return str;
}

int asCScriptEngine::GetFunctionIDByIndex(const char *module, int index)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->moduleID | index;
}

int asCScriptEngine::GetModuleIndex(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->moduleID >> 16;
}

int asCScriptEngine::BindAllImportedFunctions(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	bool notAllFunctionsWereBound = false;

	// Bind imported functions
	int c = mod->GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
	{
		asCScriptFunction *func = mod->GetImportedFunction(n);
		if( func == 0 ) return asERROR;

		asCString str = func->GetDeclaration(this);

		// Get module name from where the function should be imported
		const char *moduleName = mod->GetImportedFunctionSourceModule(n);
		if( moduleName == 0 ) return asERROR;

		int funcID = GetFunctionIDByDecl(moduleName, str);
		if( funcID < 0 )
			notAllFunctionsWereBound = true;
		else
		{
			if( mod->BindImportedFunction(n, funcID) < 0 )
				notAllFunctionsWereBound = true;
		}
	}

	if( notAllFunctionsWereBound )
		return asCANT_BIND_ALL_FUNCTIONS;

	return asSUCCESS;
}

int asCScriptEngine::UnbindAllImportedFunctions(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	int c = mod->GetImportedFunctionCount();
	for( int n = 0; n < c; ++n )
		mod->BindImportedFunction(n, -1);

	return asSUCCESS;
}

int asCScriptEngine::ExecuteString(const char *module, const char *script, asIOutputStream *out, asIScriptContext **ctx, asDWORD flags)
{
	// Make sure the config worked
	if( configFailed )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
			*ctx = 0;
		if( out )
			out->Write(TXT_INVALID_CONFIGURATION);
		return asINVALID_CONFIGURATION;
	}

	PrepareEngine();

	asIScriptContext *exec = 0;
	if( !(flags & asEXECSTRING_USE_MY_CONTEXT) )
	{
		int r = CreateContext(&exec, false);
		if( r < 0 )
		{
			if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
				*ctx = 0;
			return r;
		}
		if( ctx )
		{
			*ctx = exec;
			exec->AddRef();
		}
	}
	else
	{
		if( *ctx == 0 )
			return asINVALID_ARG;
		exec = *ctx;
		exec->AddRef();
	}

	// Get the module to compile the string in
	asCModule *mod = GetModule(module, true);

	// Compile string function
	asCBuilder builder(this, mod);
	builder.SetOutputStream(out);

	asCString str = script;
	str = "void ExecuteString(){\n" + str + ";}";

	int r = builder.BuildString(str, (asCContext*)exec);
	if( r < 0 )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
		{
			(*ctx)->Release();
			*ctx = 0;
		}
		exec->Release();
		return asERROR;
	}

	// Prepare and execute the context
	r = ((asCContext*)exec)->PrepareSpecial(mod->moduleID | asFUNC_STRING);
	if( r < 0 )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
		{
			(*ctx)->Release();
			*ctx = 0;
		}
		exec->Release();
		return r;
	}

	if( flags & asEXECSTRING_ONLY_PREPARE )
		r = asEXECUTION_PREPARED;
	else
		r = exec->Execute();

	exec->Release();

	return r;
}

asCObjectType *asCScriptEngine::GetArrayType(asCDataType &type)
{
	if( type.tokenType == ttIdentifier )
	{
		// TODO: Improve linear search
		for( int n = 0; n < arrayTypes.GetLength(); n++ )
		{
			if( arrayTypes[n]->tokenType == ttIdentifier && 
				arrayTypes[n]->name == type.extendedType->name &&
				arrayTypes[n]->arrayType == type.arrayType )
				return arrayTypes[n];
		}
	}
	else
	{
		// TODO: Improve linear search
		for( int n = 0; n < arrayTypes.GetLength(); n++ )
		{
			if( arrayTypes[n]->tokenType == type.tokenType &&
				arrayTypes[n]->arrayType == type.arrayType )
				return arrayTypes[n];
		}
	}

	return defaultArrayObjectType;
}

void asCScriptEngine::CallObjectMethod(void *obj, int func)
{
	asSSystemFunctionInterface *i = systemFunctionInterfaces[-func-1];
	asCScriptFunction *s = systemFunctions[-func-1];
	CallObjectMethod(obj, i, s);
}

void asCScriptEngine::CallObjectMethod(void *obj, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		void (*f)(void *) = (void (*)(void *))(i->func);
		f(obj);
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		void (asCSimpleDummy::*f)() = p.mthd;
		(((asCSimpleDummy*)obj)->*f)();
	}
	else 
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		void (*f)(void *) = (void (*)(void *))(i->func);
		f(obj);
	}
#endif
}


void asCScriptEngine::CallObjectMethod(void *obj, void *param, int func)
{
	asSSystemFunctionInterface *i = systemFunctionInterfaces[-func-1];
	asCScriptFunction *s = systemFunctions[-func-1];
	CallObjectMethod(obj, param, i, s);
}

void asCScriptEngine::CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
#ifdef __GNUC__
	if( i->callConv == ICC_CDECL_OBJLAST )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param, obj);			
	}
	else if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, (asDWORD*)&param);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJFIRST || i->callConv == ICC_THISCALL )*/
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(obj, param);			
	}
#else
#ifndef AS_NO_CLASS_METHODS
	if( i->callConv == ICC_THISCALL )
	{
		union
		{
			asSIMPLEMETHOD_t mthd;
			asFUNCTION_t func;
		} p;
		p.func = (void (*)())(i->func);
		void (asCSimpleDummy::*f)(void *) = (void (asCSimpleDummy::*)(void *))(p.mthd);
			(((asCSimpleDummy*)obj)->*f)(param);
	}
	else 
#endif		
	if( i->callConv == ICC_CDECL_OBJLAST )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param, obj);			
	}
	else if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, (asDWORD*)&param);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
	else /*if( i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(obj, param);			
	}
#endif
}

void asCScriptEngine::CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		void (*f)(void *, void *) = (void (*)(void *, void *))(i->func);
		f(param1, param2);			
	}
	else if( i->callConv == ICC_STDCALL )
	{
		void (STDCALL *f)(void *, void *) = (void (STDCALL *)(void *, void *))(i->func);
		f(param1, param2);			
	}
	else
	{
		asCGeneric gen(this, s, 0, (asDWORD*)&param1);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
	}
}

void *asCScriptEngine::CallAlloc(int objTypeIdx)
{
	asCObjectType *type = allObjectTypes[objTypeIdx];
	void *(*custom_alloc)(asUINT);
	if( type->beh.alloc )
	{
		asSSystemFunctionInterface *intf = systemFunctionInterfaces[-type->beh.alloc - 1];
		custom_alloc = (void *(*)(asUINT))intf->func;
	}
	else
		custom_alloc = malloc;

	return custom_alloc(type->size);
}

void asCScriptEngine::CallFree(int objTypeIdx, void *obj)
{
	asCObjectType *type = allObjectTypes[objTypeIdx];
	void (*custom_free)(void *);
	if( type->beh.free )
	{
		asSSystemFunctionInterface *intf = systemFunctionInterfaces[-type->beh.free - 1];
		custom_free = (void (*)(void *))intf->func;
	}
	else
		custom_free = free;

	custom_free(obj);
}


