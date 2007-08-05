/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jonsson

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
// as_scriptengine.cpp
//
// The implementation of the script engine interface
//


#include <stdlib.h>

#include "as_config.h"
#include "as_scriptengine.h"
#include "as_builder.h"
#include "as_context.h"
#include "as_string_util.h"
#include "as_tokenizer.h"
#include "as_texts.h"
#include "as_module.h"
#include "as_callfunc.h"
#include "as_arrayobject.h"
#include "as_anyobject.h"
#include "as_generic.h"
#include "as_scriptstruct.h"
#include "as_gcobject.h"

BEGIN_AS_NAMESPACE

AS_API const char * asGetLibraryVersion()
{
#ifdef _DEBUG
	return ANGELSCRIPT_VERSION_STRING " DEBUG";
#else
	return ANGELSCRIPT_VERSION_STRING;
#endif
}

AS_API const char * asGetLibraryOptions()
{
	const char *string = " "
#ifdef AS_MAX_PORTABILITY
		"AS_MAX_PORTABILITY "
#endif
#ifdef BUILD_WITHOUT_LINE_CUES
		"BUILD_WITHOUT_LINE_CUES "
#endif
#ifdef AS_DEBUG
		"AS_DEBUG "
#endif
#ifdef AS_NO_CLASS_METHODS
		"AS_NO_CLASS_METHODS "
#endif
#ifdef AS_USE_DOUBLE_AS_FLOAT
		"AS_USE_DOUBLE_AS_FLOAT "
#endif
#ifdef AS_64BIT_PTR
		"AS_64BIT_PTR "
#endif
#ifdef AS_NO_USER_ALLOC
		"AS_NO_USER_ALLOC "
#endif
#ifdef AS_WIN
		"AS_WIN "
#endif
#ifdef AS_LINUX
		"AS_LINUX "
#endif
#ifdef AS_MAC
		"AS_MAC "
#endif
#ifdef AS_XBOX
		"AS_XBOX "
#endif
#ifdef AS_XBOX360
		"AS_XBOX360 "
#endif
#ifdef AS_PSP
		"AS_PSP "
#endif
#ifdef AS_PS2
		"AS_PS2 "
#endif
#ifdef AS_PS3
		"AS_PS3 "
#endif
#ifdef AS_DC
		"AS_DC "
#endif
#ifdef AS_PPC
		"AS_PPC "
#endif
#ifdef AS_PPC_64
		"AS_PPC_64 "
#endif
#ifdef AS_X86
		"AS_X86 "
#endif
#ifdef AS_MIPS
		"AS_MIPS "
#endif
#ifdef AS_SH4
		"AS_SH4 "
#endif
	;

	return string;
}

AS_API asIScriptEngine *asCreateScriptEngine(asDWORD version)
{
	// Verify the version that the application expects
	if( (version/10000) != ANGELSCRIPT_VERSION_MAJOR )
		return 0;

	if( (version/100)%100 != ANGELSCRIPT_VERSION_MINOR )
		return 0;

	if( (version%100) > ANGELSCRIPT_VERSION_BUILD )
		return 0;

	// Verify the size of the types
	assert( sizeof(asBYTE)  == 1 );
	assert( sizeof(asWORD)  == 2 );
	assert( sizeof(asDWORD) == 4 );
	assert( sizeof(asQWORD) == 8 );
	assert( sizeof(asPWORD) == sizeof(void*) );

	// Verify the boolean type
	assert( sizeof(bool) == AS_SIZEOF_BOOL );
	assert( true == VALUE_OF_BOOLEAN_TRUE );

	// Verify endianess
#ifdef AS_BIG_ENDIAN
	assert( *(asDWORD*)"\x00\x01\x02\x03" == 0x00010203 );
	assert( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0001020304050607) );
#else
	assert( *(asDWORD*)"\x00\x01\x02\x03" == 0x03020100 );
	assert( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0706050403020100) );
#endif

	return NEW(asCScriptEngine)();
}

int asCScriptEngine::SetEngineProperty(asDWORD property, asPWORD value)
{
	if( property == asEP_ALLOW_UNSAFE_REFERENCES )
		allowUnsafeReferences = value ? true : false;
	else if( property == asEP_OPTIMIZE_BYTECODE )
		optimizeByteCode = value ? true : false;
	else if( property == asEP_COPY_SCRIPT_SECTIONS )
		copyScriptSections = value ? true : false;
	else
		return asINVALID_ARG;

	return asSUCCESS;
}

asPWORD asCScriptEngine::GetEngineProperty(asDWORD property)
{
	if( property == asEP_ALLOW_UNSAFE_REFERENCES )
		return allowUnsafeReferences;
	else if( property == asEP_OPTIMIZE_BYTECODE )
		return optimizeByteCode;
	else if( property == asEP_COPY_SCRIPT_SECTIONS )
		return copyScriptSections;

	return 0;
}

asCScriptEngine::asCScriptEngine()
{
	// Engine properties
	allowUnsafeReferences = false;
	optimizeByteCode      = true;
	copyScriptSections    = true;



	scriptTypeBehaviours.engine = this;

	refCount = 1;
	
	stringFactory = 0;

	configFailed = false;

	isPrepared = false;

	lastModule = 0;

	// Reset the GC state
	gcState = 0;

	initialContextStackSize = 1024;      // 1 KB
	maximumContextStackSize = 0;         // no limit

	typeIdSeqNbr = 0;
	currentGroup = &defaultGroup;

	msgCallback = 0;

	// Reserve function id 0 for no function
	scriptFunctions.PushLast(0);

	// Make sure typeId for void is 0
	GetTypeIdFromDataType(asCDataType::CreatePrimitive(ttVoid, false));

	RegisterArrayObject(this);
	RegisterAnyObject(this);
	RegisterScriptStruct(this);
}

asCScriptEngine::~asCScriptEngine()
{
	assert(refCount == 0);

	Reset();

	// The modules must be deleted first, as they may use 
	// object types from the config groups
	asUINT n;
	for( n = 0; n < scriptModules.GetLength(); n++ )
	{
		if( scriptModules[n] ) 
		{
			if( scriptModules[n]->CanDelete() )
			{
				DELETE(scriptModules[n],asCModule);
			}
			else
				assert(false);
		}
	}
	scriptModules.SetLength(0);

	// Do one more garbage collect to free gc objects that were global variables
	GarbageCollect(true);

	ClearUnusedTypes();

	mapTypeIdToDataType.MoveFirst();
	while( mapTypeIdToDataType.IsValidCursor() )
	{
		DELETE(mapTypeIdToDataType.GetValue(),asCDataType);
		mapTypeIdToDataType.Erase(true);
	}

	while( configGroups.GetLength() )
	{
		// Delete config groups in the right order
		asCConfigGroup *grp = configGroups.PopLast();
		if( grp ) 
		{
			DELETE(grp,asCConfigGroup);
		}
	}

	for( n = 0; n < globalProps.GetLength(); n++ )
	{
		if( globalProps[n] )
		{
			DELETE(globalProps[n],asCProperty);
		}
	}
	globalProps.SetLength(0);
	globalPropAddresses.SetLength(0);

	for( n = 0; n < arrayTypes.GetLength(); n++ )
	{
		if( arrayTypes[n] )
		{
			arrayTypes[n]->subType = 0;
			DELETE(arrayTypes[n],asCObjectType);
		}
	}
	arrayTypes.SetLength(0);
	
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] )
		{
			objectTypes[n]->subType = 0;
			DELETE(objectTypes[n],asCObjectType);
		}
	}
	objectTypes.SetLength(0);

	for( n = 0; n < scriptFunctions.GetLength(); n++ )
		if( scriptFunctions[n] )
		{
			DELETE(scriptFunctions[n],asCScriptFunction);
		}
	scriptFunctions.SetLength(0);
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

		DELETE(this,asCScriptEngine);
		return 0;
	}

	LEAVECRITICALSECTION(engineCritical);

	return r;
}

void asCScriptEngine::Reset()
{
	GarbageCollect(true);

	asUINT n;
	for( n = 0; n < scriptModules.GetLength(); ++n )
	{
		if( scriptModules[n] )
			scriptModules[n]->Discard();
	}
}

int asCScriptEngine::SetMessageCallback(const asUPtr &callback, void *obj, asDWORD callConv)
{
	msgCallback = true;
	msgCallbackObj = obj;
	bool isObj = false;
	if( (unsigned)callConv == asCALL_GENERIC )
	{
		msgCallback = false;
		return asNOT_SUPPORTED;
	}
	if( (unsigned)callConv >= asCALL_THISCALL )
	{
		isObj = true;
		if( obj == 0 )
		{
			msgCallback = false;
			return asINVALID_ARG;
		}
	}
	int r = DetectCallingConvention(isObj, callback, callConv, &msgCallbackFunc);
	if( r < 0 ) msgCallback = false;
	return r;
}

int asCScriptEngine::ClearMessageCallback()
{
	msgCallback = false;
	return 0;
}

void asCScriptEngine::CallMessageCallback(const char *section, int row, int col, int type, const char *message)
{
	if( !msgCallback ) return;

	asSMessageInfo msg;
	msg.section = section;
	msg.row     = row;
	msg.col     = col;
	msg.type    = type;
	msg.message = message;

	if( msgCallbackFunc.callConv < ICC_THISCALL )
		CallGlobalFunction(&msg, msgCallbackObj, &msgCallbackFunc, 0);
	else
		CallObjectMethod(msgCallbackObj, &msg, &msgCallbackFunc, 0);
}

int asCScriptEngine::AddScriptSection(const char *module, const char *name, const char *code, size_t codeLength, int lineOffset)
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

	return mod->AddScriptSection(name, code, (int)codeLength, lineOffset, copyScriptSections);
}

int asCScriptEngine::Build(const char *module)
{
	if( configFailed )
	{
		CallMessageCallback("", 0, 0, 0, TXT_INVALID_CONFIGURATION);
		return asINVALID_CONFIGURATION;
	}

	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	int r = mod->Build();

	memoryMgr.FreeUnusedMemory();

	return r;
}

int asCScriptEngine::Discard(const char *module)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	mod->Discard();

	// TODO: Must protect this for multiple accesses
	// Verify if there are any modules that can be deleted
	bool hasDeletedModules = false;
	for( asUINT n = 0; n < scriptModules.GetLength(); n++ )
	{
		if( scriptModules[n] && scriptModules[n]->CanDelete() )
		{
			hasDeletedModules = true;
			DELETE(scriptModules[n],asCModule);
			scriptModules[n] = 0;
		}
	}

	if( hasDeletedModules )
		ClearUnusedTypes();

	return 0;
}



void asCScriptEngine::ClearUnusedTypes()
{
	for(;;)
	{
		asUINT n;
		for( n = 0; n < classTypes.GetLength(); n++ )
		{
			if( classTypes[n]->refCount == 0 )
			{
				RemoveFromTypeIdMap(classTypes[n]);
				DELETE(classTypes[n],asCObjectType);

				if( n == classTypes.GetLength() - 1 )
					classTypes.PopLast();
				else
					classTypes[n] = classTypes.PopLast();

				n--;
			}
		}

		bool didClearArrayType = false;
		for( n = 0; n < scriptArrayTypes.GetLength(); n++ )
		{
			if( scriptArrayTypes[n] && 
				scriptArrayTypes[n]->refCount == 0 &&
				!IsTypeUsedInParams(scriptArrayTypes[n]) )
			{
				RemoveArrayType(scriptArrayTypes[n]);
				didClearArrayType = true;
				n--;
			}
		}

		if( didClearArrayType == false )
			break;
	}
}

bool asCScriptEngine::IsTypeUsedInParams(asCObjectType *ot)
{
	for( asUINT n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		asCScriptFunction *func = scriptFunctions[n];
		if( func )
		{
			if( func->returnType.GetObjectType() == ot ) 
				return true;

			for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
			{
				if( func->parameterTypes[p].GetObjectType() == ot ) 
					return true;
			}
		}
	}

	return false;
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

int asCScriptEngine::GetFunctionIDByIndex(const char *module, int index)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return asNO_MODULE;

	return mod->scriptFunctions[index]->id;
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

//-----------------

int asCScriptEngine::GetMethodCount(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return asINVALID_ARG;
	
	asCObjectType *ot = dt->GetObjectType();
	if( ot == 0 ) return asINVALID_TYPE;

	return (int)ot->methods.GetLength();
}

int asCScriptEngine::GetMethodIDByIndex(int typeId, int index)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return asINVALID_ARG;
	
	asCObjectType *ot = dt->GetObjectType();
	if( ot == 0 ) return asINVALID_TYPE;

	if( index < 0 || (unsigned)index >= ot->methods.GetLength() ) return asINVALID_ARG;

	return ot->methods[index];
}

int asCScriptEngine::GetMethodIDByName(int typeId, const char *name)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return asINVALID_ARG;
	
	asCObjectType *ot = dt->GetObjectType();
	if( ot == 0 ) return asINVALID_TYPE;

	int id = -1;
	for( size_t n = 0; n < ot->methods.GetLength(); n++ )
	{
		if( scriptFunctions[ot->methods[n]]->name == name )
		{
			if( id == -1 )
				id = ot->methods[n];
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;

}

int asCScriptEngine::GetMethodIDByDecl(int typeId, const char *decl)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return asINVALID_ARG;
	
	asCObjectType *ot = dt->GetObjectType();
	if( ot == 0 ) return asINVALID_TYPE;

	// Get the module from one of the methods
	if( ot->methods.GetLength() == 0 )
		return asNO_FUNCTION;

	asCModule *mod = scriptFunctions[ot->methods[0]]->module;
	if( mod == 0 ) 
	{	
		if( scriptFunctions[ot->methods[0]]->funcType == asFUNC_INTERFACE )
			return GetMethodIDByDecl(ot, decl, 0);

		return asNO_MODULE;
	}

	return mod->GetMethodIDByDecl(ot, decl);
}

int asCScriptEngine::GetMethodIDByDecl(asCObjectType *ot, const char *decl, asCModule *mod)
{
	asCBuilder bld(this, mod);

	asCScriptFunction func(mod);
	int r = bld.ParseFunctionDeclaration(decl, &func);
	if( r < 0 )
		return asINVALID_DECLARATION;

	// TODO: Improve linear search
	// Search script functions for matching interface
	int id = -1;
	for( size_t n = 0; n < ot->methods.GetLength(); ++n )
	{
		if( func.name == scriptFunctions[ot->methods[n]]->name && 
			func.returnType == scriptFunctions[ot->methods[n]]->returnType &&
			func.parameterTypes.GetLength() == scriptFunctions[ot->methods[n]]->parameterTypes.GetLength() )
		{
			bool match = true;
			for( size_t p = 0; p < func.parameterTypes.GetLength(); ++p )
			{
				if( func.parameterTypes[p] != scriptFunctions[ot->methods[n]]->parameterTypes[p] )
				{
					match = false;
					break;
				}
			}

			if( match )
			{
				if( id == -1 )
					id = ot->methods[n];
				else
					return asMULTIPLE_FUNCTIONS;
			}
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}


//----------------------

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

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetFunctionModule(int funcId, int *length)
{
	asCModule *mod = GetModuleFromFuncId(funcId);
	if( !mod ) return 0;

	if( length ) *length = (int)mod->name.GetLength();

	return mod->name.AddressOf();
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

		asCModule *module = GetModuleFromFuncId(funcID);
		if( module == 0 ) return 0;

		*tempString = *module->scriptSections[func->scriptSectionIdx];
	}

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

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

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

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
	if( id > (int)mod->scriptGlobals.GetLength() )
		return 0;

	asCProperty *prop = mod->scriptGlobals[id];

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = prop->type.Format();
	*tempString += " " + prop->name;

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetGlobalVarName(int gvarID, int *length)
{
	asCModule *mod = GetModule(gvarID);
	if( mod == 0 ) return 0;

	int id = gvarID & 0xFFFF;
	if( id > (int)mod->scriptGlobals.GetLength() )
		return 0;

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = mod->scriptGlobals[id]->name;

	// TODO: return size_t
	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

// For primitives, object handles and references the address of the value is returned
// For objects the address of the reference that holds the object is returned
void *asCScriptEngine::GetGlobalVarPointer(int gvarID)
{
	asCModule *mod = GetModule(gvarID);
	if( mod == 0 ) return 0;

	int id = gvarID & 0xFFFF;
	if( id > (int)mod->scriptGlobals.GetLength() )
		return 0;

	return (void*)(mod->globalMem.AddressOf() + mod->scriptGlobals[id]->index);

	UNREACHABLE_RETURN;
}




// Internal
asCString asCScriptEngine::GetFunctionDeclaration(int funcID)
{
	asCString str;
	asCScriptFunction *func = GetScriptFunction(funcID);
	if( func )
		str = func->GetDeclaration(this);

	return str;
}

asCScriptFunction *asCScriptEngine::GetScriptFunction(int funcID)
{
	int f = funcID & 0xFFFF;
	if( f >= (int)scriptFunctions.GetLength() )
		return 0;

	return scriptFunctions[f];
}



asIScriptContext *asCScriptEngine::CreateContext()
{
	asIScriptContext *ctx = 0;
	CreateContext(&ctx, false);
	return ctx;
}

int asCScriptEngine::CreateContext(asIScriptContext **context, bool isInternal)
{
	*context = NEW(asCContext)(this, !isInternal);

	return 0;
}


int asCScriptEngine::RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset)
{
	// Verify that the correct config group is used
	if( currentGroup->FindType(obj) == 0 )
		return asWRONG_CONFIG_GROUP;

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
	if( dt.GetObjectType() == 0 ) 
		return ConfigError(asINVALID_OBJECT);

	asCProperty *prop = NEW(asCProperty);
	prop->name            = name;
	prop->type            = type;
	prop->byteOffset      = byteOffset;

	dt.GetObjectType()->properties.PushLast(prop);

	currentGroup->RefConfigGroup(FindConfigGroupForObjectType(type.GetObjectType()));	

	return asSUCCESS;
}

int asCScriptEngine::RegisterSpecialObjectType(const char *name, int byteSize, asDWORD flags)
{
	// Put the data type in the list
	asCObjectType *type;
	if( strcmp(name, asDEFAULT_ARRAY) == 0 )
	{
		type = NEW(asCObjectType)(this);
		defaultArrayObjectType = type;
		type->refCount++;
	}
	else if( strcmp(name, ANY_TOKEN) == 0 )
	{
		type = NEW(asCObjectType)(this);
		anyObjectType = type;
		type->refCount++;
	}
	else
		return asERROR;

	type->tokenType = ttIdentifier;
	type->name = name;
	type->arrayType = 0;
	type->size = byteSize;
	type->flags = flags;

	// Store it in the object types
	objectTypes.PushLast(type);

	// Add these types to the default config group
	defaultGroup.objTypes.PushLast(type);

	return asSUCCESS;
}

int asCScriptEngine::RegisterInterface(const char *name)
{
	if( name == 0 ) return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == name )
			return asALREADY_REGISTERED;
	}

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	bool oldMsgCallback = msgCallback; msgCallback = false;
	int r = bld.ParseDataType(name, &dt);
	msgCallback = oldMsgCallback;
	if( r >= 0 ) return ConfigError(asERROR);

	// Make sure the name is not a reserved keyword
	asCTokenizer t;
	size_t tokenLen;
	int token = t.GetToken(name, strlen(name), &tokenLen);
	if( token != ttIdentifier || strlen(name) != tokenLen )
		return ConfigError(asINVALID_NAME);

	r = bld.CheckNameConflict(name, 0, 0);
	if( r < 0 ) 
		return ConfigError(asNAME_TAKEN);

	// Don't have to check against members of object  
	// types as they are allowed to use the names

	// Register the object type for the interface
	asCObjectType *st = NEW(asCObjectType)(this);
	st->arrayType = 0;
	st->flags = asOBJ_CLASS_CDA | asOBJ_SCRIPT_STRUCT;
	st->size = 0; // Cannot be instanciated
	st->name = name;
	st->tokenType = ttIdentifier;

	// Use the default script struct behaviours
	st->beh.construct = 0;
	st->beh.addref = scriptTypeBehaviours.beh.addref;
	st->beh.release = scriptTypeBehaviours.beh.release;
	st->beh.copy = 0; 

	objectTypes.PushLast(st);

	currentGroup->objTypes.PushLast(st);

	return asSUCCESS;
}

int asCScriptEngine::RegisterInterfaceMethod(const char *intf, const char *declaration)
{
	// Verify that the correct config group is set. 
	if( currentGroup->FindType(intf) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	asCDataType dt;
	asCBuilder bld(this, 0);
	int r = bld.ParseDataType(intf, &dt);
	if( r < 0 )
		return ConfigError(r);
	
	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->objectType = dt.GetObjectType();

	func->objectType->methods.PushLast((int)scriptFunctions.GetLength());

	r = bld.ParseFunctionDeclaration(declaration, func);
	if( r < 0 ) 
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	r = bld.CheckNameConflictMember(dt, func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	func->funcType = asFUNC_INTERFACE;
	SetScriptFunction(func);

	func->ComputeSignatureId(this);

	// If parameter type from other groups are used, add references
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroup(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

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

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == name )
			return asALREADY_REGISTERED;
	}

	for( n = 0; n < arrayTypes.GetLength(); n++ )
	{
		if( arrayTypes[n] && arrayTypes[n]->name == name )
			return asALREADY_REGISTERED;
	}		

	// Verify the most recently created script array type
	asCObjectType *mostRecentScriptArrayType = 0;
	if( scriptArrayTypes.GetLength() )
		mostRecentScriptArrayType = scriptArrayTypes[scriptArrayTypes.GetLength()-1];

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	bool oldMsgCallback = msgCallback; msgCallback = false;
	int r = bld.ParseDataType(name, &dt);
	msgCallback = oldMsgCallback;

	// If the builder fails, then the type name 
	// is new and it should be registered 
	if( r < 0 )
	{
		// Make sure the name is not a reserved keyword
		asCTokenizer t;
		size_t tokenLen;
		int token = t.GetToken(name, strlen(name), &tokenLen);
		if( token != ttIdentifier || strlen(name) != tokenLen )
			return ConfigError(asINVALID_NAME);

		int r = bld.CheckNameConflict(name, 0, 0);
		if( r < 0 ) 
			return ConfigError(asNAME_TAKEN);

		// Don't have to check against members of object  
		// types as they are allowed to use the names

		// Put the data type in the list
		asCObjectType *type = NEW(asCObjectType)(this);
		type->name = name;
		type->tokenType = ttIdentifier;
		type->arrayType = 0;
		type->size = byteSize;
		type->flags = flags;

		objectTypes.PushLast(type);

		currentGroup->objTypes.PushLast(type);
	}
	else
	{
		// int[][] must not be allowed to be registered
		// if int[] hasn't been registered first
		if( dt.GetSubType().IsScriptArray() )
			return ConfigError(asLOWER_ARRAY_DIMENSION_NOT_REGISTERED);

		if( dt.IsReadOnly() ||
			dt.IsReference() )
			return ConfigError(asINVALID_TYPE);

		// Was the script array type created before?
		if( scriptArrayTypes[scriptArrayTypes.GetLength()-1] == mostRecentScriptArrayType ||
			mostRecentScriptArrayType == dt.GetObjectType() )
			return asNOT_SUPPORTED;

		// Is the script array type already being used?
		if( dt.GetObjectType()->refCount > 1 )
			return asNOT_SUPPORTED;

		// Put the data type in the list
		asCObjectType *type = NEW(asCObjectType)(this);
		type->name = name;
		type->subType = dt.GetSubType().GetObjectType();
		if( type->subType ) type->subType->refCount++;
		type->tokenType = dt.GetSubType().GetTokenType();
		type->arrayType = dt.GetArrayType();
		type->size = byteSize;
		type->flags = flags;

		arrayTypes.PushLast(type);

		currentGroup->objTypes.PushLast(type);

		// Remove the built-in array type, which will no longer be used.
		RemoveArrayType(dt.GetObjectType());
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

int asCScriptEngine::RegisterSpecialObjectBehaviour(asCObjectType *objType, asDWORD behaviour, const char *decl, const asUPtr &funcPointer, int callConv)
{
	assert( objType );

	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(true, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;
	
	asCBuilder bld(this, 0);

	asSTypeBehaviour *beh;
	asCDataType type;

	bool isDefaultArray = (objType->flags & asOBJ_SCRIPT_ARRAY) ? true : false;

	if( isDefaultArray )
		type = asCDataType::CreateDefaultArray(this);
	else if( objType->flags & (asOBJ_SCRIPT_STRUCT|asOBJ_SCRIPT_ANY) )
	{
		type.MakeHandle(false);
		type.MakeReadOnly(false);
		type.MakeReference(false);
		type.SetObjectType(objType);
		type.SetTokenType(ttIdentifier);
	}

	beh = type.GetBehaviour();

	// The object is sent by reference to the function
	type.MakeReference(true);

	// Verify function declaration
	asCScriptFunction func(0);

	r = bld.ParseFunctionDeclaration(decl, &func, &internal.paramAutoHandles, &internal.returnAutoHandle);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	if( isDefaultArray )
		func.objectType = defaultArrayObjectType;
	else
		func.objectType = objType;

	if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( objType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY | asOBJ_SCRIPT_ANY) )
		{
			if( func.parameterTypes.GetLength() == 1 )
			{
				beh->construct = AddBehaviourFunction(func, internal);
				beh->constructors.PushLast(beh->construct);
			}
			else
				beh->constructors.PushLast(AddBehaviourFunction(func, internal));
		}
	}
	else if( behaviour == asBEHAVE_ADDREF )
	{
		if( beh->addref )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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

		if( objType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY | asOBJ_SCRIPT_ANY) )
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
		if( func.returnType.GetTokenType() == ttVoid )
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

int asCScriptEngine::RegisterObjectBehaviour(const char *datatype, asDWORD behaviour, const char *decl, const asUPtr &funcPointer, asDWORD callConv)
{
	// Verify that the correct config group is used
	if( currentGroup->FindType(datatype) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	if( datatype == 0 ) return ConfigError(asINVALID_ARG);

	asSSystemFunctionInterface internal;
	if( behaviour == asBEHAVE_ALLOC || behaviour == asBEHAVE_FREE )
	{
		if( callConv != asCALL_CDECL ) return ConfigError(asNOT_SUPPORTED);

		memset(&internal, 0, sizeof(asSSystemFunctionInterface));
		internal.func = (size_t)funcPointer.f.func;
		internal.callConv = ICC_CDECL;
	}
	else
	{
#ifdef AS_MAX_PORTABILITY
		if( callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);
#else
		if( callConv != asCALL_THISCALL &&
			callConv != asCALL_CDECL_OBJLAST &&
			callConv != asCALL_CDECL_OBJFIRST &&
			callConv != asCALL_GENERIC )
			return ConfigError(asNOT_SUPPORTED);
#endif

		int r = DetectCallingConvention(datatype != 0, funcPointer, callConv, &internal);
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

	if( type.IsReadOnly() || type.IsReference() )
		return ConfigError(asINVALID_TYPE);

	// Verify that the type is allowed
	if( type.GetObjectType() == 0 )
		return ConfigError(asINVALID_TYPE);

	beh = type.GetBehaviour();

	// The object is sent by reference to the function
	type.MakeReference(true);

	// Verify function declaration
	asCScriptFunction func(0);

	r = bld.ParseFunctionDeclaration(decl, &func, &internal.paramAutoHandles, &internal.returnAutoHandle);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	func.objectType = type.GetObjectType();

	if( behaviour == asBEHAVE_ALLOC )
	{
		// The declaration must be "type &f(uint)"

		if( func.returnType != type )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		if( func.parameterTypes[0] != asCDataType::CreatePrimitive(ttUInt, false) )
			return ConfigError(asINVALID_DECLARATION);

		beh->alloc = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_FREE )
	{
		// The declaration must be "void f(type &in)"

		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
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
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		beh->release = AddBehaviourFunction(func, internal);
	}
	else if( behaviour >= asBEHAVE_FIRST_ASSIGN && behaviour <= asBEHAVE_LAST_ASSIGN )
	{
		// Verify that the var type is not used
		if( VerifyVarTypeNotInFunction(&func) < 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there is exactly one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is a reference to the object type
		if( func.returnType != type )
			return ConfigError(asINVALID_DECLARATION);

		if( !allowUnsafeReferences )
		{
			// Verify that the rvalue is marked as in if a reference
			if( func.parameterTypes[0].IsReference() && func.inOutFlags[0] != 1 )
				return ConfigError(asINVALID_DECLARATION);
		}

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
		// Verify that the var type is not used
		if( VerifyVarTypeNotInFunction(&func) < 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there is only one parameter
		if( func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.GetTokenType() == ttVoid )
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
		type.MakeReference(false);
		if( func.returnType != type  )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered already

		// Map behaviour to token
		beh->operators.PushLast(ttMinus);
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
	}
	else
	{
		if( behaviour >= asBEHAVE_FIRST_DUAL &&
			behaviour <= asBEHAVE_LAST_DUAL )
		{
			CallMessageCallback("", 0, 0, 0, TXT_MUST_BE_GLOBAL_BEHAVIOUR);
		}
		else
			assert(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterGlobalBehaviour(asDWORD behaviour, const char *decl, const asUPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;
	
	asCBuilder bld(this, 0);

#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	// We need a global behaviour structure
	asSTypeBehaviour *beh = &globalBehaviours;

	// Verify function declaration
	asCScriptFunction func(0);

	r = bld.ParseFunctionDeclaration(decl, &func, &internal.paramAutoHandles, &internal.returnAutoHandle);
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	if( behaviour >= asBEHAVE_FIRST_DUAL && behaviour <= asBEHAVE_LAST_DUAL )
	{
		// Verify that the var type is not used
		if( VerifyVarTypeNotInFunction(&func) < 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are exactly two parameters
		if( func.parameterTypes.GetLength() != 2 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that the return type is not void
		if( func.returnType.GetTokenType() == ttVoid )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that at least one of the parameters is a registered type
		if( !(func.parameterTypes[0].GetTokenType() == ttIdentifier) &&
			!(func.parameterTypes[1].GetTokenType() == ttIdentifier) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the operator hasn't been registered with the same parameters already

		// Map behaviour to token
		beh->operators.PushLast(behave_dual_token[behaviour - asBEHAVE_FIRST_DUAL]); 
		beh->operators.PushLast(AddBehaviourFunction(func, internal));
		currentGroup->globalBehaviours.PushLast((int)beh->operators.GetLength()-2);
	}
	else
	{
		assert(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::VerifyVarTypeNotInFunction(asCScriptFunction *func)
{
	// Don't allow var type in this function
	if( func->returnType.GetTokenType() == ttQuestion )
		return asINVALID_DECLARATION;

	for( unsigned int n = 0; n < func->parameterTypes.GetLength(); n++ )
		if( func->parameterTypes[n].GetTokenType() == ttQuestion )
			return asINVALID_DECLARATION;

	return 0;
}

int asCScriptEngine::AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal)
{
	asUINT n;

	int id = GetNextScriptFunctionId();

	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface);
	newInterface->func               = internal.func;
	newInterface->baseOffset         = internal.baseOffset;
	newInterface->callConv           = internal.callConv;
	newInterface->scriptReturnSize   = internal.scriptReturnSize;
	newInterface->hostReturnInMemory = internal.hostReturnInMemory;
	newInterface->hostReturnFloat    = internal.hostReturnFloat;
	newInterface->hostReturnSize     = internal.hostReturnSize;
	newInterface->paramSize          = internal.paramSize;
	newInterface->takesObjByVal      = internal.takesObjByVal;
	newInterface->paramAutoHandles   = internal.paramAutoHandles;
	newInterface->returnAutoHandle   = internal.returnAutoHandle;
	newInterface->hasAutoHandles     = internal.hasAutoHandles;

	asCScriptFunction *f = NEW(asCScriptFunction)(0);
	f->funcType    = asFUNC_SYSTEM;
	f->sysFuncIntf = newInterface;
	f->returnType  = func.returnType;
	f->objectType  = func.objectType;
	f->id          = id;
	f->isReadOnly  = func.isReadOnly;
	for( n = 0; n < func.parameterTypes.GetLength(); n++ )
	{
		f->parameterTypes.PushLast(func.parameterTypes[n]);
		f->inOutFlags.PushLast(func.inOutFlags[n]);
	}

	SetScriptFunction(f);

	// If parameter type from other groups are used, add references
	if( f->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(f->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( n = 0; n < f->parameterTypes.GetLength(); n++ )
	{
		if( f->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroup(f->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	return id;
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
	if( type.IsReference() )
		return ConfigError(asINVALID_TYPE);

	// Store the property info
	asCProperty *prop = NEW(asCProperty);
	prop->name       = name;
	prop->type       = type;
	prop->index      = -1 - (int)globalPropAddresses.GetLength();

	// TODO: Reuse slots emptied when removing configuration groups
	globalProps.PushLast(prop);
	globalPropAddresses.PushLast(pointer);

	currentGroup->globalProps.PushLast(prop);
	// If from another group add reference
	if( type.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(type.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}

	if( type.IsObject() && !type.IsReference() && !type.IsObjectHandle() )
	{
		// Create a pointer to a pointer
		prop->index = -1 - (int)globalPropAddresses.GetLength();

		void **pp = &globalPropAddresses[globalPropAddresses.GetLength()-1];
		globalPropAddresses.PushLast(pp);
	}

	// Update all pointers to global objects,
	// because they change when the array is resized
	for( asUINT n = 0; n < globalProps.GetLength(); n++ )
	{
		if( globalProps[n] && 
			globalProps[n]->type.IsObject() && 
			!globalProps[n]->type.IsReference() && 
			!globalProps[n]->type.IsObjectHandle() )
		{
			int idx = -globalProps[n]->index - 1;
			void **pp = &globalPropAddresses[idx-1];

			// Update the chached pointers in the modules
			for( asUINT m = 0; m < scriptModules.GetLength(); m++ )
			{
				if( scriptModules[m] )
					scriptModules[m]->UpdateGlobalVarPointer(globalPropAddresses[idx], (void*)pp);
			}

			globalPropAddresses[idx] = (void*)pp;
		}
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterSpecialObjectMethod(const char *obj, const char *declaration, const asUPtr &funcPointer, int callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(obj != 0, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	// We only support these calling conventions for object methods
	if( (unsigned)callConv != asCALL_THISCALL &&
		(unsigned)callConv != asCALL_CDECL_OBJLAST &&
		(unsigned)callConv != asCALL_CDECL_OBJFIRST &&
		(unsigned)callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);

	asCObjectType *objType = GetObjectType(obj);
	if( objType == 0 ) 
		return ConfigError(asINVALID_OBJECT);

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface);
	memcpy(newInterface, &internal, sizeof(internal));

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;
	func->objectType  = objType;

	objType->methods.PushLast((int)scriptFunctions.GetLength());

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(declaration, func, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
	if( r < 0 ) 
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	asCDataType dt;
	dt = asCDataType::CreateDefaultArray(this);
	r = bld.CheckNameConflictMember(dt, func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	return 0;
}


int asCScriptEngine::RegisterObjectMethod(const char *obj, const char *declaration, const asUPtr &funcPointer, asDWORD callConv)
{
	// Verify that the correct config group is set. 
	if( currentGroup->FindType(obj) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(obj != 0, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

	// We only support these calling conventions for object methods
#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_THISCALL &&
		callConv != asCALL_CDECL_OBJLAST &&
		callConv != asCALL_CDECL_OBJFIRST &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	asCDataType dt;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(obj, &dt);
	if( r < 0 )
		return ConfigError(r);

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface);
	memcpy(newInterface, &internal, sizeof(internal));

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;
	func->objectType  = dt.GetObjectType();

	func->objectType->methods.PushLast((int)scriptFunctions.GetLength());

	r = bld.ParseFunctionDeclaration(declaration, func, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
	if( r < 0 ) 
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	r = bld.CheckNameConflictMember(dt, func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	// If parameter type from other groups are used, add references
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroup(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	return 0;
}

int asCScriptEngine::RegisterGlobalFunction(const char *declaration, const asUPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL && 
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	isPrepared = false;

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface);
	memcpy(newInterface, &internal, sizeof(internal));

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(declaration, func, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
	if( r < 0 ) 
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_DECLARATION);
	}

	// Check name conflicts
	r = bld.CheckNameConflict(func->name.AddressOf(), 0, 0);
	if( r < 0 )
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asNAME_TAKEN);
	}

	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	currentGroup->scriptFunctions.PushLast(func);

	// If parameter type from other groups are used, add references
	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(func->returnType.GetObjectType());
		currentGroup->RefConfigGroup(group);
	}
	for( asUINT n = 0; n < func->parameterTypes.GetLength(); n++ )
	{
		if( func->parameterTypes[n].GetObjectType() )
		{
			asCConfigGroup *group = FindConfigGroup(func->parameterTypes[n].GetObjectType());
			currentGroup->RefConfigGroup(group);
		}
	}

	return 0;
}






asCObjectType *asCScriptEngine::GetObjectType(const char *type)
{
	// TODO: Improve linear search
	for( asUINT n = 0; n < objectTypes.GetLength(); n++ )
		if( objectTypes[n] &&
			objectTypes[n]->name == type &&
			objectTypes[n]->arrayType == 0 )
			return objectTypes[n];

	return 0;
}


// This method will only return registered array types, since only they have the name stored
asCObjectType *asCScriptEngine::GetArrayType(const char *type)
{
	if( type[0] == 0 )
		return 0;

	// TODO: Improve linear search
	for( asUINT n = 0; n < arrayTypes.GetLength(); n++ )
		if( arrayTypes[n] &&
			arrayTypes[n]->name == type )
			return arrayTypes[n];

	return 0;
}



void asCScriptEngine::PrepareEngine()
{
	if( isPrepared ) return;

	for( asUINT n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		// Determine the host application interface
		if( scriptFunctions[n] && scriptFunctions[n]->funcType == asFUNC_SYSTEM )
			PrepareSystemFunction(scriptFunctions[n], scriptFunctions[n]->sysFuncIntf, this);
	}

	isPrepared = true;
}

int asCScriptEngine::ConfigError(int err)
{ 
	configFailed = true; 
	return err; 
}


int asCScriptEngine::RegisterStringFactory(const char *datatype, const asUPtr &funcPointer, asDWORD callConv)
{
	asSSystemFunctionInterface internal;
	int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
	if( r < 0 )
		return ConfigError(r);

#ifdef AS_MAX_PORTABILITY
	if( callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#else
	if( callConv != asCALL_CDECL && 
		callConv != asCALL_STDCALL &&
		callConv != asCALL_GENERIC )
		return ConfigError(asNOT_SUPPORTED);
#endif

	// Put the system function in the list of system functions
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface);
	memcpy(newInterface, &internal, sizeof(internal));

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;

	asCBuilder bld(this, 0);

	asCDataType dt;
	r = bld.ParseDataType(datatype, &dt);
	if( r < 0 ) 
	{
		DELETE(func,asCScriptFunction);
		return ConfigError(asINVALID_TYPE);
	}

	func->returnType = dt;
	func->parameterTypes.PushLast(asCDataType::CreatePrimitive(ttInt, true));
	asCDataType parm1 = asCDataType::CreatePrimitive(ttUInt8, true);
	parm1.MakeReference(true);
	func->parameterTypes.PushLast(parm1);
	func->id = GetNextScriptFunctionId();
	SetScriptFunction(func);

	stringFactory = func;

	if( func->returnType.GetObjectType() )
	{
		asCConfigGroup *group = FindConfigGroup(func->returnType.GetObjectType());
		if( group == 0 ) group = &defaultGroup;
		group->scriptFunctions.PushLast(func);
	}

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
	for( asUINT n = 0; n < scriptModules.GetLength(); ++n )
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
		asUINT idx;
		for( idx = 0; idx < scriptModules.GetLength(); ++idx )
			if( scriptModules[idx] == 0 )
				break;

		int moduleID = idx << 16;
		assert(moduleID <= 0x3FF0000);

		asCModule *module = NEW(asCModule)(name, moduleID, this);

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
	// TODO: This may not work any longer
	id = (id >> 16) & 0x3FF;
	if( id >= (int)scriptModules.GetLength() ) return 0;
	return scriptModules[id];
}

asCModule *asCScriptEngine::GetModuleFromFuncId(int id)
{
	if( id < 0 ) return 0;
	id &= 0xFFFF;
	if( id >= (int)scriptFunctions.GetLength() ) return 0;
	asCScriptFunction *func = scriptFunctions[id];
	if( func == 0 ) return 0;
	return func->module;
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

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

const char *asCScriptEngine::GetImportedFunctionSourceModule(const char *module, int index, int *length)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return 0;

	const char *str = mod->GetImportedFunctionSourceModule(index);
	if( length && str )
		*length = (int)strlen(str);

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

		int funcID = GetFunctionIDByDecl(moduleName, str.AddressOf());
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


int asCScriptEngine::ExecuteString(const char *module, const char *script, asIScriptContext **ctx, asDWORD flags)
{
	// Make sure the config worked
	if( configFailed )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
			*ctx = 0;

		CallMessageCallback("",0,0,0,TXT_INVALID_CONFIGURATION);
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
	asCString str = script;
	str = "void ExecuteString(){\n" + str + "\n;}";

	int r = builder.BuildString(str.AddressOf(), (asCContext*)exec);
	memoryMgr.FreeUnusedMemory();
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
	r = ((asCContext*)exec)->PrepareSpecial(asFUNC_STRING, mod);
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

void asCScriptEngine::RemoveArrayType(asCObjectType *t)
{
	// Start searching from the end of the list, as most of 
	// the time it will be the last two types

	int n;
	for( n = (int)arrayTypes.GetLength()-1; n >= 0; n-- )
	{
		if( arrayTypes[n] == t ) 
		{
			if( n == (signed)arrayTypes.GetLength()-1 )
				arrayTypes.PopLast();
			else
				arrayTypes[n] = arrayTypes.PopLast();
		}
	}

	for( n = (int)scriptArrayTypes.GetLength()-1; n >= 0; n-- )
	{
		if( scriptArrayTypes[n] == t )
		{
			if( n == (signed)scriptArrayTypes.GetLength()-1 )
				scriptArrayTypes.PopLast();
			else
				scriptArrayTypes[n] = scriptArrayTypes.PopLast();
		}
	}

	DELETE(t,asCObjectType);
}

asCObjectType *asCScriptEngine::GetArrayTypeFromSubType(asCDataType &type)
{
	int arrayType = type.GetArrayType();
	if( type.IsObjectHandle() )
		arrayType = (arrayType<<2) | 3;
	else
		arrayType = (arrayType<<2) | 2;

	// Is there any array type already with this subtype?
	if( type.IsObject() )
	{
		// TODO: Improve linear search
		for( asUINT n = 0; n < arrayTypes.GetLength(); n++ )
		{
			if( arrayTypes[n] &&
				arrayTypes[n]->tokenType == ttIdentifier &&
				arrayTypes[n]->subType == type.GetObjectType() &&
				arrayTypes[n]->arrayType == arrayType )
				return arrayTypes[n];
		}
	}
	else
	{
		// TODO: Improve linear search
		for( asUINT n = 0; n < arrayTypes.GetLength(); n++ )
		{
			if( arrayTypes[n] && 
				arrayTypes[n]->tokenType == type.GetTokenType() &&
				arrayTypes[n]->arrayType == arrayType )
				return arrayTypes[n];
		}
	}

	// No previous array type has been registered

	// Create a new array type based on the defaultArrayObjectType
	asCObjectType *ot = NEW(asCObjectType)(this);
	ot->arrayType = arrayType;
	ot->flags = asOBJ_CLASS_CDA | asOBJ_SCRIPT_ARRAY;
	ot->size = sizeof(asCArrayObject);
	ot->name = ""; // Built-in script arrays are registered without name
	ot->tokenType = type.GetTokenType();
	ot->methods = defaultArrayObjectType->methods;
	ot->beh.construct = defaultArrayObjectType->beh.construct;
	ot->beh.constructors = defaultArrayObjectType->beh.constructors;
	ot->beh.addref = defaultArrayObjectType->beh.addref;
	ot->beh.release = defaultArrayObjectType->beh.release;
	ot->beh.copy = defaultArrayObjectType->beh.copy;
	ot->beh.operators = defaultArrayObjectType->beh.operators;

	// The object type needs to store the sub type as well
	ot->subType = type.GetObjectType();
	if( ot->subType ) ot->subType->refCount++;

	// TODO: The indexing behaviour and assignment  
	// behaviour should use the correct datatype

	// Verify if the subtype contains an any object, in which case this array is a potential circular reference
	if( ot->subType && (ot->subType->flags & asOBJ_CONTAINS_ANY) )
		ot->flags |= asOBJ_POTENTIAL_CIRCLE | asOBJ_CONTAINS_ANY;

	arrayTypes.PushLast(ot);

	// We need to store the object type somewhere for clean-up later
	scriptArrayTypes.PushLast(ot);

	return ot;
}

void asCScriptEngine::CallObjectMethod(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	CallObjectMethod(obj, s->sysFuncIntf, s);
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
	asCScriptFunction *s = scriptFunctions[func];
	CallObjectMethod(obj, param, s->sysFuncIntf, s);
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

void *asCScriptEngine::CallAlloc(asCObjectType *type)
{
	asALLOCFUNC_t custom_alloc;
	if( type->beh.alloc )
	{
		asSSystemFunctionInterface *intf = scriptFunctions[type->beh.alloc]->sysFuncIntf;
		custom_alloc = (asALLOCFUNC_t)intf->func;
	}
	else
		custom_alloc = userAlloc;

#if defined(AS_DEBUG) && !defined(AS_NO_USER_ALLOC)
	return ((asALLOCFUNCDEBUG_t)(custom_alloc))(type->size, __FILE__, __LINE__);
#else
	return custom_alloc(type->size);
#endif
}

void asCScriptEngine::CallFree(asCObjectType *type, void *obj)
{
	asFREEFUNC_t custom_free;
	if( type->beh.free )
	{
		asSSystemFunctionInterface *intf = scriptFunctions[type->beh.free]->sysFuncIntf;
		custom_free = (asFREEFUNC_t)intf->func;
	}
	else
		custom_free = userFree;

	custom_free(obj);
}

void asCScriptEngine::AddScriptObjectToGC(asCGCObject *obj)
{
	obj->AddRef();
	gcObjects.PushLast(obj);
}

int asCScriptEngine::GarbageCollect(bool doFullCycle)
{
	if( doFullCycle )
	{
		// Reset GC
		gcState = 0;
		unmarked.SetLength(0);

		int r;
		while( (r = GCInternal()) == 1 );

		// Take the opportunity to clear unused types as well
		ClearUnusedTypes();

		return r;
	}

	// Run another step
	return GCInternal();
}

int asCScriptEngine::GetObjectsInGarbageCollectorCount()
{
	return (int)gcObjects.GetLength();
}

int asCScriptEngine::GCInternal()
{
	enum egcState
	{
		destroyGarbage_init = 0,
		destroyGarbage_loop,
		destroyGarbage_haveMore,
		clearCounters,
		countReferences_init,
		countReferences_loop, 
		detectGarbage_init,
		detectGarbage_loop1,
		detectGarbage_loop2,
		verifyUnmarked,
		breakCircles_init,
		breakCircles_loop,
		breakCircles_haveGarbage
	};

	for(;;)
	{
		switch( gcState )
		{
		case destroyGarbage_init:
		{
			// If there are no objects to be freed then don't start
			if( gcObjects.GetLength() == 0 )
				return 0;

			gcIdx = (asUINT)-1;
			gcState = destroyGarbage_loop;
		}
		break;

		case destroyGarbage_loop:
		case destroyGarbage_haveMore:
		{
			// If the refCount has reached 1, then only the GC still holds a 
			// reference to the object, thus we don't need to worry about the 
			// application touching the objects during collection

			// Destroy obvious garbage, i.e. objects with refCount == 1.
			// If an object is released, then repeat since 
			// it may have created another
			while( ++gcIdx < gcObjects.GetLength() )
			{
				if( gcObjects[gcIdx]->gc.refCount == 1 )
				{
					// Release the object immediately
					gcObjects[gcIdx]->Release();
					if( gcIdx == gcObjects.GetLength() - 1 )
						gcObjects.PopLast();
					else
						gcObjects[gcIdx] = gcObjects.PopLast();
					gcIdx--;

					gcState = destroyGarbage_haveMore;

					// Allow the application to work a little
					return 1;
				}
			}

			// Only move to the next step if no garbage was detected in this step
			if( gcState == destroyGarbage_haveMore ) 
				gcState = destroyGarbage_init;
			else
				gcState = clearCounters;
		}
		break;

		case clearCounters:
		{
			// Clear the GC counter for the objects that are still alive.
			// This counter will be used to count the references between 
			// objects on the list.
			for( asUINT n = 0; n < gcObjects.GetLength(); n++ )
			{
				gcObjects[n]->gc.gcCount = gcObjects[n]->gc.refCount - 1;

				// Mark the object so that we can  
				// see if it has changed since read
				gcObjects[n]->gc.refCount |= 0x80000000;
			}

			gcState = countReferences_init;
		}
		break;

		case countReferences_init:
		{
			gcIdx = (asUINT)-1;
			gcState = countReferences_loop;
		}
		break;

		case countReferences_loop:
		{
			// It is possible that the application alters the reference count 
			// after we have cleared the gcCounter, so we need to detect this.
			// What happens if the application creates a new object?

			// Count references between objects on the list
			while( ++gcIdx < gcObjects.GetLength() )
			{
				if( !(gcObjects[gcIdx]->gc.refCount & 0x8000000) )
				{
					gcObjects[gcIdx]->CountReferences();

					// Allow the application to work a little
					return 1;
				}
			}

			gcState = detectGarbage_init;
		}
		break;

		case detectGarbage_init:
		{
			gcIdx = (asUINT)-1;
			gcState = detectGarbage_loop1;
		}
		break;

		case detectGarbage_loop1:
		{
			// If an object's refCount decreased during the counting it will still be marked as live, since the gcCount will not reach 0
			// If an object's refCount increased during the counting it will still be marked as live, since the flag will be cleared
			
			// It is possible that the application has altered the reference count
			// after it was checked. We need to mark those objects as live as well.
			// What happens if an object is created?

			// Any objects that have gcCount > 0 is referenced
			// from outside the list. Mark them and all the 
			// objects they reference as live objects
			while( ++gcIdx < gcObjects.GetLength() )
			{
				// Objects that have been tampered with should also be marked as live
				if( (gcObjects[gcIdx]->gc.gcCount != 0 || !(gcObjects[gcIdx]->gc.refCount & 0x80000000)) && gcObjects[gcIdx]->gc.gcCount != -1 )
				{
					unmarked.PushLast(gcObjects[gcIdx]);

					// Allow the application to work a little
					return 1;
				}
			}

			gcState = detectGarbage_loop2;
		}
		break;

		case detectGarbage_loop2:
		{
			// It is possible that the application has altered the reference count
			// after it was checked. We need to mark those objects as live as well.
			// What happens if an object is created?
	
			while( unmarked.GetLength() )
			{
				asCGCObject *gcObj = unmarked.PopLast();
				
				// Mark the object as alive
				gcObj->gc.gcCount = -1;

				// Add unmarked references to the list
				gcObj->AddUnmarkedReferences(unmarked);

				// Allow the application to work a little
				return 1;
			}

			gcState = verifyUnmarked;
		}
		break;

		case verifyUnmarked:
		{
			// In this step we must make sure that none of the still unmarked objects
			// has been touched by the application. If they have then we must run the
			// detectGarbage loop once more.
			for( asUINT n = 0; n < gcObjects.GetLength(); n++ )
			{
				if( gcObjects[n]->gc.gcCount != -1 && !(gcObjects[n]->gc.refCount & 0x80000000) )
				{
					// The unmarked object was touched, rerun the detectGarbage loop
					gcState = detectGarbage_init;
					return 1;
				}
			}

			// No unmarked object was touched, we can now be sure 
			// that objects that have gcCount == 0 really is garbage
			gcState = breakCircles_init;
		}
		break;

		case breakCircles_init:
		{
			gcIdx = (asUINT)-1;
			gcState = breakCircles_loop;
		}
		break;

		case breakCircles_loop:
		case breakCircles_haveGarbage:
		{
			// Any objects still with gcCount == 0 is garbage involved 
			// in circular references. Break the circular references, by 
			// having the objects release all their member handles. 
			while( ++gcIdx < gcObjects.GetLength() )
			{
				if( gcObjects[gcIdx]->gc.gcCount == 0 )
				{
					// Release all its members to break the circle
					gcObjects[gcIdx]->ReleaseAllHandles();
					gcState = breakCircles_haveGarbage;

					// Allow the application to work a little
					return 1;
				}
			}

			// If no handles garbage was detected we can finish now
			if( gcState != breakCircles_haveGarbage )
			{
				// Restart the GC
				gcState = destroyGarbage_init;
				return 0;
			}
			else
			{
				// Restart the GC
				gcState = destroyGarbage_init;
				return 1;
			}
		}
		break;
		} // switch
	}

	// Shouldn't reach this point
	UNREACHABLE_RETURN;
}

int asCScriptEngine::GetTypeIdFromDataType(const asCDataType &dt)
{
	// Find the existing type id
	mapTypeIdToDataType.MoveFirst();
	while( mapTypeIdToDataType.IsValidCursor() )
	{
		if( mapTypeIdToDataType.GetValue()->IsEqualExceptRefAndConst(dt) )
			return mapTypeIdToDataType.GetKey();

		mapTypeIdToDataType.MoveNext();
	}

	// The type id doesn't exist, create it

	// Setup the basic type id
	int typeId = typeIdSeqNbr++;
	if( dt.GetObjectType() )
	{
		if( dt.GetObjectType()->flags & asOBJ_SCRIPT_STRUCT ) typeId |= asTYPEID_SCRIPTSTRUCT;
		else if( dt.GetObjectType()->flags & asOBJ_SCRIPT_ANY ) typeId |= asTYPEID_SCRIPTANY;
		else if( dt.GetObjectType()->flags & asOBJ_SCRIPT_ARRAY ) typeId |= asTYPEID_SCRIPTARRAY;
		else typeId |= asTYPEID_APPOBJECT;
	}

	// Insert the basic object type
	asCDataType *newDt = NEW(asCDataType)(dt);
	newDt->MakeReference(false);
	newDt->MakeReadOnly(false);
	newDt->MakeHandle(false);

	mapTypeIdToDataType.Insert(typeId, newDt);

	// If the object type supports object handles then register those types as well
	if( dt.IsObject() && dt.GetObjectType()->beh.addref && dt.GetObjectType()->beh.release )
	{
		newDt = NEW(asCDataType)(dt);
		newDt->MakeReference(false);
		newDt->MakeReadOnly(false);
		newDt->MakeHandle(true);
		newDt->MakeHandleToConst(false);

		mapTypeIdToDataType.Insert(typeId | asTYPEID_OBJHANDLE, newDt);

		newDt = NEW(asCDataType)(dt);
		newDt->MakeReference(false);
		newDt->MakeReadOnly(false);
		newDt->MakeHandle(true);
		newDt->MakeHandleToConst(true);

		mapTypeIdToDataType.Insert(typeId | asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST, newDt);
	}

	return GetTypeIdFromDataType(dt);
}

const asCDataType *asCScriptEngine::GetDataTypeFromTypeId(int typeId)
{
	if( mapTypeIdToDataType.MoveTo(typeId) )
		return mapTypeIdToDataType.GetValue();

	return 0;
}

const asCObjectType *asCScriptEngine::GetObjectTypeFromTypeId(int typeId)
{
	if( mapTypeIdToDataType.MoveTo(typeId) )
		return mapTypeIdToDataType.GetValue()->GetObjectType();

	return 0;
}

void asCScriptEngine::RemoveFromTypeIdMap(asCObjectType *type)
{
	mapTypeIdToDataType.MoveFirst();
	while( mapTypeIdToDataType.IsValidCursor() )
	{
		asCDataType *dt = mapTypeIdToDataType.GetValue();
		if( dt->GetObjectType() == type )
		{
			DELETE(dt,asCDataType);
			mapTypeIdToDataType.Erase(true);
		}
		else
			mapTypeIdToDataType.MoveNext();
	}
}

int asCScriptEngine::GetTypeIdByDecl(const char *module, const char *decl)
{
	asCModule *mod = GetModule(module, false);

	asCDataType dt;
	asCBuilder bld(this, mod);
	int r = bld.ParseDataType(decl, &dt);
	if( r < 0 )
		return asINVALID_TYPE;

	return GetTypeIdFromDataType(dt);
}

const char *asCScriptEngine::GetTypeDeclaration(int typeId, int *length)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return 0;

	asCString *tempString = &threadManager.GetLocalData()->string;
	*tempString = dt->Format();

	// TODO: Return size_t
	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

void *asCScriptEngine::CreateScriptObject(int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return 0;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return 0;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type Id valid?
	if( !dt ) return 0;

	// Allocate the memory
	asCObjectType *objType = dt->GetObjectType();
	void *ptr = CallAlloc(objType);
	if( ptr == 0 ) return 0;

	// Construct the object
	if( objType->flags & asOBJ_SCRIPT_STRUCT )
	{
		int r = ConstructScriptStruct(ptr, objType, this);
		if( r < 0 )	return 0;
	}
	else if( objType->flags & asOBJ_SCRIPT_ARRAY )
		ArrayObjectConstructor(objType, (asCArrayObject*)ptr);
	else if( objType->flags & asOBJ_SCRIPT_ANY )
		AnyObjectConstructor(objType, (asCAnyObject*)ptr);
	else
	{
		int funcIndex = objType->beh.construct;
		if( funcIndex )
			CallObjectMethod(ptr, funcIndex);
	}

	return ptr;
}

int asCScriptEngine::BeginConfigGroup(const char *groupName)
{
	// Make sure the group name doesn't already exist
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
			return asNAME_TAKEN;			
	}

	if( currentGroup != &defaultGroup )
		return asNOT_SUPPORTED;

	asCConfigGroup *group = NEW(asCConfigGroup)();
	group->groupName = groupName;

	configGroups.PushLast(group);
	currentGroup = group;

	return 0;
}

int asCScriptEngine::EndConfigGroup()
{
	// Raise error if trying to end the default config
	if( currentGroup == &defaultGroup )
		return asNOT_SUPPORTED;

	currentGroup = &defaultGroup;

	return 0;
}

int asCScriptEngine::RemoveConfigGroup(const char *groupName)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
		{
			asCConfigGroup *group = configGroups[n];
			if( group->refCount > 0 )
				return asCONFIG_GROUP_IS_IN_USE;

			// Verify if any objects registered in this group is still alive
			if( group->HasLiveObjects(this) )
				return asCONFIG_GROUP_IS_IN_USE;

			// Remove the group from the list
			if( n == configGroups.GetLength() - 1 )
				configGroups.PopLast();
			else
				configGroups[n] = configGroups.PopLast();

			// Remove the configurations registered with this group
			group->RemoveConfiguration(this);

			DELETE(group,asCConfigGroup);
		}
	}

	return 0;
}

asCConfigGroup *asCScriptEngine::FindConfigGroup(asCObjectType *ot)
{
	// Find the config group where this object type is registered
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		for( asUINT m = 0; m < configGroups[n]->objTypes.GetLength(); m++ )
		{
			if( configGroups[n]->objTypes[m] == ot )
				return configGroups[n];
		}
	}

	return 0;
}

asCConfigGroup *asCScriptEngine::FindConfigGroupForFunction(int funcId)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		// Check global functions
		asUINT m;
		for( m = 0; m < configGroups[n]->scriptFunctions.GetLength(); m++ )
		{
			if( configGroups[n]->scriptFunctions[m]->id == funcId )
				return configGroups[n];
		}

		// Check global behaviours
		for( m = 0; m < configGroups[n]->globalBehaviours.GetLength(); m++ )
		{
			int id = configGroups[n]->globalBehaviours[m]+1;
			if( funcId == globalBehaviours.operators[id] )
				return configGroups[n];
		}
	}

	return 0;
}


asCConfigGroup *asCScriptEngine::FindConfigGroupForGlobalVar(int gvarId)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		for( asUINT m = 0; m < configGroups[n]->globalProps.GetLength(); m++ )
		{
			if( configGroups[n]->globalProps[m]->index == gvarId )
				return configGroups[n];
		}
	}

	return 0;
}

asCConfigGroup *asCScriptEngine::FindConfigGroupForObjectType(asCObjectType *objType)
{
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		for( asUINT m = 0; m < configGroups[n]->objTypes.GetLength(); m++ )
		{
			if( configGroups[n]->objTypes[m] == objType )
				return configGroups[n];
		}
	}

	return 0;
}

int asCScriptEngine::SetConfigGroupModuleAccess(const char *groupName, const char *module, bool hasAccess)
{
	asCConfigGroup *group = 0;
	
	// Make sure the group name doesn't already exist
	for( asUINT n = 0; n < configGroups.GetLength(); n++ )
	{
		if( configGroups[n]->groupName == groupName )
		{
			group = configGroups[n];			
			break;
		}
	}
	
	if( group == 0 )
		return asWRONG_CONFIG_GROUP;

	return group->SetModuleAccess(module, hasAccess);
}

int asCScriptEngine::GetNextScriptFunctionId()
{
	if( freeScriptFunctionIds.GetLength() )
		return freeScriptFunctionIds.PopLast();

	int id = (int)scriptFunctions.GetLength();
	scriptFunctions.PushLast(0);
	return id;
}

void asCScriptEngine::SetScriptFunction(asCScriptFunction *func)
{
	scriptFunctions[func->id] = func;
}

void asCScriptEngine::DeleteScriptFunction(int id)
{
	if( id < 0 ) return;
	id &= 0xFFFF;
	if( id >= (int)scriptFunctions.GetLength() ) return;

	if( scriptFunctions[id] )
	{
		asCScriptFunction *func = scriptFunctions[id];

		// Remove the function from the list of script functions
		if( id == (int)scriptFunctions.GetLength() - 1 )
		{
			scriptFunctions.PopLast();
		}
		else
		{
			scriptFunctions[id] = 0;
			freeScriptFunctionIds.PushLast(id);
		}

		// Is the function used as signature id?
		if( func->signatureId == id )
		{
			// Remove the signature id
			asUINT n;
			for( n = 0; n < signatureIds.GetLength(); n++ )
			{
				if( signatureIds[n] == func )
				{
					signatureIds[n] = signatureIds[signatureIds.GetLength()-1];
					signatureIds.PopLast();
				}
			}

			// Update all functions using the signature id
			int newSigId = 0;
			for( n = 0; n < scriptFunctions.GetLength(); n++ )
			{
				if( scriptFunctions[n] && scriptFunctions[n]->signatureId == id )
				{
					if( newSigId == 0 )
					{
						newSigId = scriptFunctions[n]->id;
						signatureIds.PushLast(scriptFunctions[n]);
					}

					scriptFunctions[n]->signatureId = newSigId;
				}
			}
		}

		// Delete the script function
		DELETE(func,asCScriptFunction);
	}
}


END_AS_NAMESPACE

