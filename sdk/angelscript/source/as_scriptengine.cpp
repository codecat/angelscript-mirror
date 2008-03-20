/*
   AngelCode Scripting Library
   Copyright (c) 2003-2008 Andreas Jonsson

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
#include "as_generic.h"
#include "as_scriptstruct.h"

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
#ifdef AS_XENON
		"AS_XENON "
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
	asASSERT( sizeof(asBYTE)  == 1 );
	asASSERT( sizeof(asWORD)  == 2 );
	asASSERT( sizeof(asDWORD) == 4 );
	asASSERT( sizeof(asQWORD) == 8 );
	asASSERT( sizeof(asPWORD) == sizeof(void*) );

	// Verify the boolean type
	asASSERT( sizeof(bool) == AS_SIZEOF_BOOL );
	asASSERT( true == VALUE_OF_BOOLEAN_TRUE );

	// Verify endianess
#ifdef AS_BIG_ENDIAN
	asASSERT( *(asDWORD*)"\x00\x01\x02\x03" == 0x00010203 );
	asASSERT( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0001020304050607) );
#else
	asASSERT( *(asDWORD*)"\x00\x01\x02\x03" == 0x03020100 );
	asASSERT( *(asQWORD*)"\x00\x01\x02\x03\x04\x05\x06\x07" == I64(0x0706050403020100) );
#endif

	return NEW(asCScriptEngine)();
}

int asCScriptEngine::SetEngineProperty(asEEngineProp property, asPWORD value)
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

asPWORD asCScriptEngine::GetEngineProperty(asEEngineProp property)
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
	RegisterScriptStruct(this);
}

asCScriptEngine::~asCScriptEngine()
{
	asASSERT(refCount == 0);

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
				asASSERT(false);
		}
	}
	scriptModules.SetLength(0);

	// Do one more garbage collect to free gc objects that were global variables
	GarbageCollect(true);

	ClearUnusedTypes();

	asSMapNode<int,asCDataType*> *cursor = 0;
	while( mapTypeIdToDataType.MoveFirst(&cursor) )
	{
		DELETE(mapTypeIdToDataType.GetValue(cursor),asCDataType);
		mapTypeIdToDataType.Erase(cursor);
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

int asCScriptEngine::SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv)
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

void asCScriptEngine::CallMessageCallback(const char *section, int row, int col, asEMsgType type, const char *message)
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
	PrepareEngine();

	if( configFailed )
	{
		CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_INVALID_CONFIGURATION);
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
	// Build a list of all types to check for
	asCArray<asCObjectType*> types;
	types = classTypes;
	types.Concatenate(scriptArrayTypes);

	// Go through all modules
	asUINT n;
	for( n = 0; n < scriptModules.GetLength() && types.GetLength(); n++ )
	{
		asCModule *mod = scriptModules[n];
		if( mod )
		{
			// Go through all globals
			asUINT m;
			for( m = 0; m < mod->scriptGlobals.GetLength() && types.GetLength(); m++ )
			{
				if( mod->scriptGlobals[m]->type.GetObjectType() )
					RemoveTypeAndRelatedFromList(types, mod->scriptGlobals[m]->type.GetObjectType());
			}

			// Go through all script class declarations
			for( m = 0; m < mod->classTypes.GetLength() && types.GetLength(); m++ )
				RemoveTypeAndRelatedFromList(types, mod->classTypes[m]);
		}
	}

	// Go through all function parameters and remove used types
	for( n = 0; n < scriptFunctions.GetLength() && types.GetLength(); n++ )
	{
		asCScriptFunction *func = scriptFunctions[n];
		if( func )
		{
			asCObjectType *ot;
			if( (ot = func->returnType.GetObjectType()) != 0 )
				RemoveTypeAndRelatedFromList(types, ot);

			for( asUINT p = 0; p < func->parameterTypes.GetLength(); p++ )
			{
				if( (ot = func->parameterTypes[p].GetObjectType()) != 0 )
					RemoveTypeAndRelatedFromList(types, ot);
			}
		}
	}

	// Go through all global properties
	for( n = 0; n < globalProps.GetLength() && types.GetLength(); n++ )
	{
		if( globalProps[n] && globalProps[n]->type.GetObjectType() )
			RemoveTypeAndRelatedFromList(types, globalProps[n]->type.GetObjectType());
	}

	// All that remains in the list after this can be discarded, since they are no longer used
	for(;;)
	{
		bool didClearArrayType = false;

		for( n = 0; n < types.GetLength(); n++ )
		{
			if( types[n]->refCount == 0 )
			{
				if( types[n]->arrayType )
				{
					didClearArrayType = true;
					RemoveArrayType(types[n]);
				}
				else
				{
					RemoveFromTypeIdMap(types[n]);
					DELETE(types[n],asCObjectType);

					int i = classTypes.IndexOf(types[n]);
					if( i == (signed)classTypes.GetLength() - 1 )
						classTypes.PopLast();
					else
						classTypes[i] = classTypes.PopLast();
				}

				// Remove the type from the array
				if( n < types.GetLength() - 1 )
					types[n] = types.PopLast();
				else
					types.PopLast();
				n--;
			}
		}

		if( didClearArrayType == false )
			break;
	}
}

void asCScriptEngine::RemoveTypeAndRelatedFromList(asCArray<asCObjectType*> &types, asCObjectType *ot)
{
	// Remove the type from the list
	int i = types.IndexOf(ot);
	if( i == -1 ) return;

	if( i == (signed)types.GetLength() - 1 )
		types.PopLast();
	else
		types[i] = types.PopLast();

	// If the type is an array, then remove all sub types as well
	if( ot->subType )
	{
		while( ot->subType )
		{
			ot = ot->subType;
			RemoveTypeAndRelatedFromList(types, ot);
		}
		return;
	}

	// If the type is a class, then remove all properties types as well
	if( ot->properties.GetLength() )
	{
		for( asUINT n = 0; n < ot->properties.GetLength(); n++ )
			RemoveTypeAndRelatedFromList(types, ot->properties[n]->type.GetObjectType());
	}
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
	int r = bld.ParseFunctionDeclaration(decl, &func, false);
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
	st->flags = asOBJ_REF | asOBJ_SCRIPT_STRUCT;
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

	r = bld.ParseFunctionDeclaration(declaration, func, false);
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

	// Return function id as success
	return func->id;
}

int asCScriptEngine::RegisterObjectType(const char *name, int byteSize, asDWORD flags)
{
	isPrepared = false;

	// Verify flags
	//   Must have either asOBJ_REF or asOBJ_VALUE
	if( flags & asOBJ_REF )
	{
		// Can optionally have the asOBJ_GC, asOBJ_NOHANDLE, or asOBJ_SCOPED flag set, but nothing else
		if( flags & ~(asOBJ_REF | asOBJ_GC | asOBJ_NOHANDLE | asOBJ_SCOPED) )
			return ConfigError(asINVALID_ARG);

		// flags are exclusive
		if( (flags & asOBJ_GC) && (flags & (asOBJ_NOHANDLE|asOBJ_SCOPED)) )
			return ConfigError(asINVALID_ARG);
		if( (flags & asOBJ_NOHANDLE) && (flags & (asOBJ_GC|asOBJ_SCOPED)) )
			return ConfigError(asINVALID_ARG);
		if( (flags & asOBJ_SCOPED) && (flags & (asOBJ_GC|asOBJ_NOHANDLE)) )
			return ConfigError(asINVALID_ARG);
	}
	else if( flags & asOBJ_VALUE )
	{
		// Cannot use reference flags
		if( flags & (asOBJ_REF | asOBJ_GC | asOBJ_SCOPED) )
			return ConfigError(asINVALID_ARG);

		// Must have either asOBJ_APP_CLASS, asOBJ_APP_PRIMITIVE, or asOBJ_APP_FLOAT
		if( flags & asOBJ_APP_CLASS )
		{
			// Must not set the primitive or float flag
			if( flags & (asOBJ_APP_PRIMITIVE |
				         asOBJ_APP_FLOAT) )
				return ConfigError(asINVALID_ARG);
		}
		else if( flags & asOBJ_APP_PRIMITIVE )
		{
			// Must not set the class flags nor the float flag
			if( flags & (asOBJ_APP_CLASS             |
				         asOBJ_APP_CLASS_CONSTRUCTOR |
						 asOBJ_APP_CLASS_DESTRUCTOR  |
						 asOBJ_APP_CLASS_ASSIGNMENT  |
						 asOBJ_APP_FLOAT) )
				return ConfigError(asINVALID_ARG);
		}
		else if( flags & asOBJ_APP_FLOAT )
		{
			// Must not set the class flags nor the primitive flag
			if( flags & (asOBJ_APP_CLASS             |
				         asOBJ_APP_CLASS_CONSTRUCTOR |
						 asOBJ_APP_CLASS_DESTRUCTOR  |
						 asOBJ_APP_CLASS_ASSIGNMENT  |
						 asOBJ_APP_PRIMITIVE) )
				return ConfigError(asINVALID_ARG);
		}
	}
	else
		return ConfigError(asINVALID_ARG);

	// Don't allow anything else than the defined flags
	if( flags - (flags & asOBJ_MASK_VALID_FLAGS) )
		return ConfigError(asINVALID_ARG);
	
	// Value types must have a defined size 
	if( (flags & asOBJ_VALUE) && byteSize == 0 )
	{
		CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_VALUE_TYPE_MUST_HAVE_SIZE);
		return ConfigError(asINVALID_ARG);
	}

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
			return ConfigError(asNOT_SUPPORTED);

		// Is the script array type already being used?
		if( dt.GetObjectType()->refCount > 1 )
			return ConfigError(asNOT_SUPPORTED);

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

int asCScriptEngine::RegisterSpecialObjectBehaviour(asCObjectType *objType, asDWORD behaviour, const char *decl, const asSFuncPtr &funcPointer, int callConv)
{
	asASSERT( objType );

	asSSystemFunctionInterface internal;
	if( behaviour == asBEHAVE_FACTORY )
	{
		int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}
	else
	{
		int r = DetectCallingConvention(true, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
	}

	isPrepared = false;

	asCBuilder bld(this, 0);

	asSTypeBehaviour *beh;
	asCDataType type;

	bool isDefaultArray = (objType->flags & asOBJ_SCRIPT_ARRAY) ? true : false;

	if( isDefaultArray )
		type = asCDataType::CreateDefaultArray(this);
	else if( objType->flags & asOBJ_SCRIPT_STRUCT )
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

	// The default array object is actually being registered 
	// with incorrect declarations, but that's a concious decision
	bld.ParseFunctionDeclaration(decl, &func, true, &internal.paramAutoHandles, &internal.returnAutoHandle);

	if( isDefaultArray )
		func.objectType = defaultArrayObjectType;
	else
		func.objectType = objType;

	if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( objType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY) )
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
	else if( behaviour == asBEHAVE_FACTORY )
	{
		if( objType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY) )
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

		if( objType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY) )
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
	else if( behaviour >= asBEHAVE_FIRST_GC &&
		     behaviour <= asBEHAVE_LAST_GC )
	{
		// Register the gc behaviours

		// Verify parameter count
		if( (behaviour == asBEHAVE_GETREFCOUNT ||
			 behaviour == asBEHAVE_SETGCFLAG   ||
			 behaviour == asBEHAVE_GETGCFLAG) &&
			func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		if( (behaviour == asBEHAVE_ENUMREFS ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( behaviour == asBEHAVE_GETREFCOUNT &&
			func.returnType != asCDataType::CreatePrimitive(ttInt, false) )
			return ConfigError(asINVALID_DECLARATION);
		
		if( behaviour == asBEHAVE_GETGCFLAG &&
			func.returnType != asCDataType::CreatePrimitive(ttBool, false) )
			return ConfigError(asINVALID_DECLARATION);
		
		if( (behaviour == asBEHAVE_SETGCFLAG ||
			 behaviour == asBEHAVE_ENUMREFS  ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( behaviour == asBEHAVE_GETREFCOUNT )
			beh->gcGetRefCount = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_SETGCFLAG )
			beh->gcSetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_GETGCFLAG )
			beh->gcGetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_ENUMREFS )
			beh->gcEnumReferences = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_RELEASEREFS )
			beh->gcReleaseAllReferences = AddBehaviourFunction(func, internal);
	}
	else
	{
		asASSERT(false);

		return ConfigError(asINVALID_ARG);
	}

	return asSUCCESS;
}

int asCScriptEngine::RegisterObjectBehaviour(const char *datatype, asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv)
{
	// Verify that the correct config group is used
	if( currentGroup->FindType(datatype) == 0 )
		return ConfigError(asWRONG_CONFIG_GROUP);

	if( datatype == 0 ) return ConfigError(asINVALID_ARG);

	asSSystemFunctionInterface internal;
	if( behaviour == asBEHAVE_FACTORY )
	{
		int r = DetectCallingConvention(false, funcPointer, callConv, &internal);
		if( r < 0 )
			return ConfigError(r);
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

	r = bld.ParseFunctionDeclaration(decl, &func, true, &internal.paramAutoHandles, &internal.returnAutoHandle, (behaviour == asBEHAVE_FACTORY) && (type.GetObjectType()->flags & asOBJ_SCOPED) );
	if( r < 0 )
		return ConfigError(asINVALID_DECLARATION);

	func.objectType = type.GetObjectType();

	if( behaviour == asBEHAVE_CONSTRUCT )
	{
		// Verify that it is a value type
		if( !(func.objectType->flags & asOBJ_VALUE) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the same constructor hasn't been registered already

		// Store all constructors in a list
		if( func.parameterTypes.GetLength() == 0 )
		{
			func.id = beh->construct = AddBehaviourFunction(func, internal);
			beh->constructors.PushLast(beh->construct);
		}
		else
		{
			func.id = AddBehaviourFunction(func, internal);
			beh->constructors.PushLast(func.id);
		}
	}
	else if( behaviour == asBEHAVE_DESTRUCT )
	{
		// Must be a value type
		if( !(func.objectType->flags & asOBJ_VALUE) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->destruct )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->destruct = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_FACTORY )
	{
		// Must be a ref type and must not have asOBJ_NOHANDLE
		if( !(func.objectType->flags & asOBJ_REF) || (func.objectType->flags & asOBJ_NOHANDLE) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		// Verify that the return type is a handle to the type
		if( func.returnType != asCDataType::CreateObjectHandle(func.objectType, false) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: Verify that the same factory function hasn't been registered already

		// Store all factory functions in a list
		if( func.parameterTypes.GetLength() == 0 )
		{
			// Share variable with constructor
			func.id = beh->construct = AddBehaviourFunction(func, internal);
			beh->constructors.PushLast(beh->construct);
		}
		else
		{
			// Share variable with constructor
			func.id = AddBehaviourFunction(func, internal);
			beh->constructors.PushLast(func.id);
		}
	}
	else if( behaviour == asBEHAVE_ADDREF )
	{
		// Must be a ref type and must not have asOBJ_NOHANDLE, nor asOBJ_SCOPED
		if( !(func.objectType->flags & asOBJ_REF) || 
			(func.objectType->flags & asOBJ_NOHANDLE) || 
			(func.objectType->flags & asOBJ_SCOPED) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->addref )
			return ConfigError(asALREADY_REGISTERED);
											 
		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->addref = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_RELEASE)
	{
		// Must be a ref type and must not have asOBJ_NOHANDLE
		if( !(func.objectType->flags & asOBJ_REF) || (func.objectType->flags & asOBJ_NOHANDLE) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		if( beh->release )
			return ConfigError(asALREADY_REGISTERED);

		// Verify that the return type is void
		if( func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		// Verify that there are no parameters
		if( func.parameterTypes.GetLength() > 0 )
			return ConfigError(asINVALID_DECLARATION);

		func.id = beh->release = AddBehaviourFunction(func, internal);
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

			func.id = beh->copy = AddBehaviourFunction(func, internal);

			beh->operators.PushLast(ttAssignment);
			beh->operators.PushLast(beh->copy);
		}
		else
		{
			// TODO: Verify that the operator hasn't been registered with the same parameter already

			// Map behaviour to token
			beh->operators.PushLast(behave_assign_token[behaviour - asBEHAVE_FIRST_ASSIGN]);
			func.id = AddBehaviourFunction(func, internal);
			beh->operators.PushLast(func.id);
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
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
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
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
	}
	else if( behaviour >= asBEHAVE_FIRST_GC &&
		     behaviour <= asBEHAVE_LAST_GC )
	{
		// Only allow GC behaviours for types registered to be garbage collected
		if( !(func.objectType->flags & asOBJ_GC) )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_ILLEGAL_BEHAVIOUR_FOR_TYPE);
			return ConfigError(asILLEGAL_BEHAVIOUR_FOR_TYPE);
		}

		// Verify parameter count
		if( (behaviour == asBEHAVE_GETREFCOUNT ||
			 behaviour == asBEHAVE_SETGCFLAG   ||
			 behaviour == asBEHAVE_GETGCFLAG) &&
			func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		if( (behaviour == asBEHAVE_ENUMREFS ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.parameterTypes.GetLength() != 1 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( behaviour == asBEHAVE_GETREFCOUNT &&
			func.returnType != asCDataType::CreatePrimitive(ttInt, false) )
			return ConfigError(asINVALID_DECLARATION);
		
		if( behaviour == asBEHAVE_GETGCFLAG &&
			func.returnType != asCDataType::CreatePrimitive(ttBool, false) )
			return ConfigError(asINVALID_DECLARATION);
		
		if( (behaviour == asBEHAVE_SETGCFLAG ||
			 behaviour == asBEHAVE_ENUMREFS  ||
			 behaviour == asBEHAVE_RELEASEREFS) &&
			func.returnType != asCDataType::CreatePrimitive(ttVoid, false) )
			return ConfigError(asINVALID_DECLARATION);

		if( behaviour == asBEHAVE_GETREFCOUNT )
			func.id = beh->gcGetRefCount = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_SETGCFLAG )
			func.id = beh->gcSetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_GETGCFLAG )
			func.id = beh->gcGetFlag = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_ENUMREFS )
			func.id = beh->gcEnumReferences = AddBehaviourFunction(func, internal);
		else if( behaviour == asBEHAVE_RELEASEREFS )
			func.id = beh->gcReleaseAllReferences = AddBehaviourFunction(func, internal);
	}
	else if( behaviour == asBEHAVE_VALUE_CAST )
	{
		// Verify parameter count
		if( func.parameterTypes.GetLength() != 0 )
			return ConfigError(asINVALID_DECLARATION);

		// Verify return type
		if( func.returnType.IsEqualExceptRefAndConst(asCDataType::CreatePrimitive(ttBool, false)) )
			return ConfigError(asNOT_SUPPORTED);

		if( func.returnType.IsEqualExceptRefAndConst(asCDataType::CreatePrimitive(ttVoid, false)) )
			return ConfigError(asINVALID_DECLARATION);

		// TODO: verify that the same cast is not registered already (const or non-const is treated the same for the return type)

		// Map behaviour to token
		beh->operators.PushLast(ttCast);
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
	}
	else
	{
		if( behaviour >= asBEHAVE_FIRST_DUAL &&
			behaviour <= asBEHAVE_LAST_DUAL )
		{
			CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, TXT_MUST_BE_GLOBAL_BEHAVIOUR);
		}
		else
			asASSERT(false);

		return ConfigError(asINVALID_ARG);
	}

	// Return function id as success
	return func.id;
}

int asCScriptEngine::RegisterGlobalBehaviour(asDWORD behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv)
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

	r = bld.ParseFunctionDeclaration(decl, &func, true, &internal.paramAutoHandles, &internal.returnAutoHandle);
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
		func.id = AddBehaviourFunction(func, internal);
		beh->operators.PushLast(func.id);
		currentGroup->globalBehaviours.PushLast((int)beh->operators.GetLength()-2);
	}
	else
	{
		return ConfigError(asINVALID_ARG);
	}

	// Return the function id as success
	return func.id;
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

int asCScriptEngine::RegisterSpecialObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, int callConv)
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
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;
	func->objectType  = objType;

	objType->methods.PushLast((int)scriptFunctions.GetLength());

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(declaration, func, true, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
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


int asCScriptEngine::RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv)
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
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;
	func->objectType  = dt.GetObjectType();

	r = bld.ParseFunctionDeclaration(declaration, func, true, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
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
	func->objectType->methods.PushLast(func->id);
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

	// Return the function id as success
	return func->id;
}

int asCScriptEngine::RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv)
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
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface)(internal);

	asCScriptFunction *func = NEW(asCScriptFunction)(0);
	func->funcType    = asFUNC_SYSTEM;
	func->sysFuncIntf = newInterface;

	asCBuilder bld(this, 0);
	r = bld.ParseFunctionDeclaration(declaration, func, true, &newInterface->paramAutoHandles, &newInterface->returnAutoHandle);
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

	// Return the function id as success
	return func->id;
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
	if( configFailed ) return;

	asUINT n;
	for( n = 0; n < scriptFunctions.GetLength(); n++ )
	{
		// Determine the host application interface
		if( scriptFunctions[n] && scriptFunctions[n]->funcType == asFUNC_SYSTEM )
			PrepareSystemFunction(scriptFunctions[n], scriptFunctions[n]->sysFuncIntf, this);
	}

	// Validate object type registrations
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && !(objectTypes[n]->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY)) )
		{
			bool missingBehaviour = false;

			// Verify that GC types have all behaviours
			if( objectTypes[n]->flags & asOBJ_GC )
			{
				if( objectTypes[n]->beh.addref                 == 0 ||
					objectTypes[n]->beh.release                == 0 ||
					objectTypes[n]->beh.gcGetRefCount          == 0 ||
					objectTypes[n]->beh.gcSetFlag              == 0 ||
					objectTypes[n]->beh.gcGetFlag              == 0 ||
					objectTypes[n]->beh.gcEnumReferences       == 0 ||
					objectTypes[n]->beh.gcReleaseAllReferences == 0 )
				{
					missingBehaviour = true;
				}
			}
			// Verify that scoped ref types have the release behaviour
			else if( objectTypes[n]->flags & asOBJ_SCOPED )
			{
				if( objectTypes[n]->beh.release == 0 )
				{
					missingBehaviour = true;
				}
			}
			// Verify that ref types have add ref and release behaviours
			else if( (objectTypes[n]->flags & asOBJ_REF) && 
				     !(objectTypes[n]->flags & asOBJ_NOHANDLE) )
			{
				if( objectTypes[n]->beh.addref  == 0 ||
					objectTypes[n]->beh.release == 0 )
				{
					missingBehaviour = true;
				}
			}
			// Verify that non-pod value types have the constructor and destructor registered
			else if( (objectTypes[n]->flags & asOBJ_VALUE) &&
				     !(objectTypes[n]->flags & asOBJ_POD) )
			{
				if( objectTypes[n]->beh.construct == 0 ||
					objectTypes[n]->beh.destruct  == 0 )
				{
					missingBehaviour = true;
				}
			}

			if( missingBehaviour )
			{
				asCString str;
				str.Format(TXT_TYPE_s_IS_MISSING_BEHAVIOURS, objectTypes[n]->name.AddressOf());
				CallMessageCallback("", 0, 0, asMSGTYPE_ERROR, str.AddressOf());
				ConfigError(asINVALID_CONFIGURATION);
			}
		}
	}

	isPrepared = true;
}

int asCScriptEngine::ConfigError(int err)
{
	configFailed = true;
	return err;
}


int asCScriptEngine::RegisterStringFactory(const char *datatype, const asSFuncPtr &funcPointer, asDWORD callConv)
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
	asSSystemFunctionInterface *newInterface = NEW(asSSystemFunctionInterface)(internal);

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

	// Register function id as success
	return func->id;
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
		asASSERT(moduleID <= 0x3FF0000);

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

	return asINVALID_ARG;
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

	return asINVALID_ARG;
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
	PrepareEngine();

	// Make sure the config worked
	if( configFailed )
	{
		if( ctx && !(flags & asEXECSTRING_USE_MY_CONTEXT) )
			*ctx = 0;

		CallMessageCallback("",0,0,asMSGTYPE_ERROR,TXT_INVALID_CONFIGURATION);
		return asINVALID_CONFIGURATION;
	}

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
	ot->flags = asOBJ_REF | asOBJ_SCRIPT_ARRAY;
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
	ot->beh.gcGetRefCount = defaultArrayObjectType->beh.gcGetRefCount;
	ot->beh.gcSetFlag = defaultArrayObjectType->beh.gcSetFlag;
	ot->beh.gcGetFlag = defaultArrayObjectType->beh.gcGetFlag;
	ot->beh.gcEnumReferences = defaultArrayObjectType->beh.gcEnumReferences;
	ot->beh.gcReleaseAllReferences = defaultArrayObjectType->beh.gcReleaseAllReferences;

	// The object type needs to store the sub type as well
	ot->subType = type.GetObjectType();
	if( ot->subType ) ot->subType->refCount++;

	// TODO: The indexing behaviour and assignment
	// behaviour should use the correct datatype

	// Verify if the subtype contains an any object, in which case this array is a potential circular reference
	// TODO: We may be a bit smarter here. If we can guarantee that the array type cannot be part of the 
	// potential circular reference then we don't need to set the flag 
	if( ot->subType && (ot->subType->flags & asOBJ_GC) )
		ot->flags |= asOBJ_GC;

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
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asDWORD baseOffset;
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = i->baseOffset;
		void (asCSimpleDummy::*f)() = p.mthd;
		(((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
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

bool asCScriptEngine::CallObjectMethodRetBool(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	asSSystemFunctionInterface *i = s->sysFuncIntf;

#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asDWORD baseOffset;
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = i->baseOffset;
		bool (asCSimpleDummy::*f)() = (bool (asCSimpleDummy::*)())(p.mthd);
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
		bool (*f)(void *) = (bool (*)(void *))(i->func);
		return f(obj);
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
		bool (asCSimpleDummy::*f)() = (bool (asCSimpleDummy::*)())p.mthd;
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		bool (*f)(void *) = (bool (*)(void *))(i->func);
		return f(obj);
	}
#endif
}

int asCScriptEngine::CallObjectMethodRetInt(void *obj, int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	asSSystemFunctionInterface *i = s->sysFuncIntf;

#ifdef __GNUC__
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(int*)gen.GetReturnPointer();
	}
	else if( i->callConv == ICC_VIRTUAL_THISCALL )
	{
		// For virtual thiscalls we must call the method as a true class method so that the compiler will lookup the function address in the vftable
		union
		{
			asSIMPLEMETHOD_t mthd;
			struct
			{
				asFUNCTION_t func;
				asDWORD baseOffset;
			} f;
		} p;
		p.f.func = (void (*)())(i->func);
		p.f.baseOffset = i->baseOffset;
		int (asCSimpleDummy::*f)() = (int (asCSimpleDummy::*)())(p.mthd);
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else /*if( i->callConv == ICC_THISCALL || i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		// Non-virtual thiscall can be called just like any global function, passing the object as the first parameter
		int (*f)(void *) = (int (*)(void *))(i->func);
		return f(obj);
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
		int (asCSimpleDummy::*f)() = (int (asCSimpleDummy::*)())p.mthd;
		return (((asCSimpleDummy*)obj)->*f)();
	}
	else
#endif
	if( i->callConv == ICC_GENERIC_METHOD )
	{
		asCGeneric gen(this, s, obj, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(int*)gen.GetReturnPointer();
	}
	else /*if( i->callConv == ICC_CDECL_OBJLAST || i->callConv == ICC_CDECL_OBJFIRST )*/
	{
		int (*f)(void *) = (int (*)(void *))(i->func);
		return f(obj);
	}
#endif
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(int func)
{
	asCScriptFunction *s = scriptFunctions[func];
	return CallGlobalFunctionRetPtr(s->sysFuncIntf, s);
}

void *asCScriptEngine::CallGlobalFunctionRetPtr(asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		void *(*f)() = (void *(*)())(i->func);
		return f();
	}
	else if( i->callConv == ICC_STDCALL )
	{
		void *(STDCALL *f)() = (void *(STDCALL *)())(i->func);
		return f();
	}
	else
	{
		asCGeneric gen(this, s, 0, 0);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(void**)gen.GetReturnPointer();
	}
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

bool asCScriptEngine::CallGlobalFunctionRetBool(void *param1, void *param2, asSSystemFunctionInterface *i, asCScriptFunction *s)
{
	if( i->callConv == ICC_CDECL )
	{
		bool (*f)(void *, void *) = (bool (*)(void *, void *))(i->func);
		return f(param1, param2);
	}
	else if( i->callConv == ICC_STDCALL )
	{
		bool (STDCALL *f)(void *, void *) = (bool (STDCALL *)(void *, void *))(i->func);
		return f(param1, param2);
	}
	else
	{
		asCGeneric gen(this, s, 0, (asDWORD*)&param1);
		void (*f)(asIScriptGeneric *) = (void (*)(asIScriptGeneric *))(i->func);
		f(&gen);
		return *(bool*)gen.GetReturnPointer();
	}
}

void *asCScriptEngine::CallAlloc(asCObjectType *type)
{
#if defined(AS_DEBUG) && !defined(AS_NO_USER_ALLOC)
	return ((asALLOCFUNCDEBUG_t)(userAlloc))(type->size, __FILE__, __LINE__);
#else
	return userAlloc(type->size);
#endif
}

void asCScriptEngine::CallFree(void *obj)
{
	userFree(obj);
}

void asCScriptEngine::NotifyGarbageCollectorOfNewObject(void *obj, int typeId)
{
	asCObjectType *objType = GetObjectTypeFromTypeId(typeId);
	AddScriptObjectToGC(obj, objType);
}

void asCScriptEngine::AddScriptObjectToGC(void *obj, asCObjectType *objType)
{
	CallObjectMethod(obj, objType->beh.addref);
	asSObjTypePair ot = {obj, objType};
	gcObjects.PushLast(ot);
}

int asCScriptEngine::GarbageCollect(bool doFullCycle)
{
	if( doFullCycle )
	{
		// Reset GC
		gcState = 0;
		toMark.SetLength(0);
		gcMap.EraseAll();

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

enum egcState
{
	destroyGarbage_init = 0,
	destroyGarbage_loop,
	destroyGarbage_haveMore,
	clearCounters_init,
	clearCounters_loop,
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

int asCScriptEngine::GCInternal()
{
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
			// application touching the objects during collection.

			// Destroy all objects that have refCount == 1. If any objects are
			// destroyed, go over the list again, because it may have made more
			// objects reach refCount == 1.
			while( ++gcIdx < gcObjects.GetLength() )
			{
				if( CallObjectMethodRetInt(gcObjects[gcIdx].obj, gcObjects[gcIdx].type->beh.gcGetRefCount) == 1 )
				{
					// Release the object immediately

					// Make sure the refCount is really 0, because the 
					// destructor may have increased the refCount again.

					bool addRef = false;
					if( gcObjects[gcIdx].type->flags & asOBJ_SCRIPT_STRUCT )
					{
						// Script structs may actually be resurrected in the destructor
						int refCount = ((asCScriptStruct*)gcObjects[gcIdx].obj)->Release();
						if( refCount > 0 ) addRef = true;
					}
					else
						CallObjectMethod(gcObjects[gcIdx].obj, gcObjects[gcIdx].type->beh.release);

					// Was the object really destroyed?
					if( !addRef )
					{
						if( gcIdx == gcObjects.GetLength() - 1 )
							gcObjects.PopLast();
						else
							gcObjects[gcIdx] = gcObjects.PopLast();
						gcIdx--;
					}
					else
					{
						// Since the object was resurrected in the 
						// destructor, we must add our reference again
						CallObjectMethod(gcObjects[gcIdx].obj, gcObjects[gcIdx].type->beh.addref);
					}

					gcState = destroyGarbage_haveMore;

					// Allow the application to work a little
					return 1;
				}
			}

			// Only move to the next step if no garbage was detected in this step
			if( gcState == destroyGarbage_haveMore )
				gcState = destroyGarbage_init;
			else
				gcState = clearCounters_init;
		}
		break;

		case clearCounters_init:
		{
			gcMap.EraseAll();
			gcState = clearCounters_loop;
		}
		break;

		case clearCounters_loop:
		{
			// TODO: gc
			// This step can be incremental as newly created objects are
			// added to the end of the gcObjects array.
			// :ODOT

			// Build a map of objects that will be checked, the map will
			// hold the object pointer as key, and the gcCount and the 
			// objects index in the gcObjects array as value. As objects are 
			// added to the map the gcFlag must be set in the objects, so
			// we can be verify if the object is accessed during the GC cycle.
			for( asUINT n = 0; n < gcObjects.GetLength(); n++ )
			{
				// Add the gc count for this object
				int refCount = CallObjectMethodRetInt(gcObjects[n].obj, gcObjects[n].type->beh.gcGetRefCount);
				asSIntTypePair it = {refCount-1, gcObjects[n].type};
				gcMap.Insert(gcObjects[n].obj, it);

				// Mark the object so that we can
				// see if it has changed since read
				CallObjectMethod(gcObjects[n].obj, gcObjects[n].type->beh.gcSetFlag);
			}

			gcState = countReferences_init;
		}
		break;

		case countReferences_init:
		{
			gcIdx = (asUINT)-1;
			gcMap.MoveFirst(&gcMapCursor);
			gcState = countReferences_loop;
		}
		break;

		case countReferences_loop:
		{
			// Call EnumReferences on all objects in the map, to count the number
			// of references reachable from between objects in the map. If all
			// references for an object in the map is reachable from other objects 
			// in the map, then we know that no outside references are held for 
			// this object, thus it is a potential dead object in a circular reference.

			// If the gcFlag is cleared for an object we consider the object alive 
			// and referenced from outside the GC, thus we don't enumerate its references.

			// Any new objects created after this step in the GC cycle won't be 
			// in the map, and is thus automatically considered alive.
			while( gcMapCursor )
			{
				void *obj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;
				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				if( CallObjectMethodRetBool(obj, type->beh.gcGetFlag) )
				{
					CallObjectMethod(obj, this, type->beh.gcEnumReferences);

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
			gcMap.MoveFirst(&gcMapCursor);
			gcState = detectGarbage_loop1;
		}
		break;

		case detectGarbage_loop1:
		{
			// All objects that are known not to be dead must be removed from the map,
			// along with all objects they reference. What remains in the map after 
			// this pass is sure to be dead objects in circular references.

			// An object is considered alive if its gcFlag is cleared, or all the 
			// references where not found in the map.

			// Add all alive objects from the map to the toMark array
			while( gcMapCursor )
			{
				asSMapNode<void*, asSIntTypePair> *cursor = gcMapCursor;
				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				void *obj = gcMap.GetKey(cursor);
				asSIntTypePair it = gcMap.GetValue(cursor);

				bool gcFlag = CallObjectMethodRetBool(obj, it.type->beh.gcGetFlag);
				if( !gcFlag || it.i > 0 )
				{
					toMark.PushLast(obj);

					// Allow the application to work a little
					return 1;
				}
			}

			gcState = detectGarbage_loop2;
		}
		break;

		case detectGarbage_loop2:
		{
			// In this step we are actually removing the alive objects from the map.
			// As the object is removed, all the objects it references are added to the
			// toMark list, by calling EnumReferences. Only objects still in the map
			// will be added to the toMark list.
			while( toMark.GetLength() )
			{
				void *gcObj = toMark.PopLast();
				asCObjectType *type = 0;

				// Remove the object from the map to mark it as alive
				asSMapNode<void*, asSIntTypePair> *cursor = 0;
				if( gcMap.MoveTo(&cursor, gcObj) )
				{
					type = gcMap.GetValue(cursor).type;
					gcMap.Erase(cursor);

					// Enumerate all the object's references so that they too can be marked as alive
					CallObjectMethod(gcObj, this, type->beh.gcEnumReferences);
				}

				// Allow the application to work a little
				return 1;
			}

			gcState = verifyUnmarked;
		}
		break;

		case verifyUnmarked:
		{
			// In this step we must make sure that none of the objects still in the map
			// has been touched by the application. If they have then we must run the
			// detectGarbage loop once more.
			gcMap.MoveFirst(&gcMapCursor);
			while( gcMapCursor )
			{
				void *gcObj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;

				bool gcFlag = CallObjectMethodRetBool(gcObj, type->beh.gcGetFlag);
				if( !gcFlag )
				{
					// The unmarked object was touched, rerun the detectGarbage loop
					gcState = detectGarbage_init;
					return 1;
				}

				gcMap.MoveNext(&gcMapCursor, gcMapCursor);
			}

			// No unmarked object was touched, we can now be sure
			// that objects that have gcCount == 0 really is garbage
			gcState = breakCircles_init;
		}
		break;

		case breakCircles_init:
		{
			gcIdx = (asUINT)-1;
			gcMap.MoveFirst(&gcMapCursor);
			gcState = breakCircles_loop;
		}
		break;

		case breakCircles_loop:
		case breakCircles_haveGarbage:
		{
			// All objects in the map are now known to be dead objects
			// kept alive through circular references. To be able to free
			// these objects we need to force the breaking of the circle
			// by having the objects release their references.
			while( gcMapCursor )
			{
				//gcMap.GetKey(gcMapCursor)->ReleaseAllHandles();
				void *gcObj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;
				CallObjectMethod(gcObj, this, type->beh.gcReleaseAllReferences);

				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				gcState = breakCircles_haveGarbage;

				// Allow the application to work a little
				return 1;
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

void asCScriptEngine::GCEnumCallback(void *reference)
{
	if( gcState == countReferences_loop )
	{
		// Find the reference in the map
		asSMapNode<void*, asSIntTypePair> *cursor = 0;
		if( gcMap.MoveTo(&cursor, (asCGCObject*)reference) )
		{
			// Decrease the counter in the map for the reference
			gcMap.GetValue(cursor).i--;
		}
	}
	else if( gcState == detectGarbage_loop2 )
	{
		// Find the reference in the map
		asSMapNode<void*, asSIntTypePair> *cursor = 0;
		if( gcMap.MoveTo(&cursor, (asCGCObject*)reference) )
		{
			// Add the object to the list of objects to mark as alive
			toMark.PushLast((asCGCObject*)reference);
		}
	}
}

int asCScriptEngine::GetTypeIdFromDataType(const asCDataType &dt)
{
	// Find the existing type id
	asSMapNode<int,asCDataType*> *cursor = 0;
	mapTypeIdToDataType.MoveFirst(&cursor);
	while( cursor )
	{
		if( mapTypeIdToDataType.GetValue(cursor)->IsEqualExceptRefAndConst(dt) )
			return mapTypeIdToDataType.GetKey(cursor);

		mapTypeIdToDataType.MoveNext(&cursor, cursor);
	}

	// The type id doesn't exist, create it

	// Setup the basic type id
	int typeId = typeIdSeqNbr++;
	if( dt.GetObjectType() )
	{
		if( dt.GetObjectType()->flags & asOBJ_SCRIPT_STRUCT ) typeId |= asTYPEID_SCRIPTSTRUCT;
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
	asSMapNode<int,asCDataType*> *cursor = 0;
	if( mapTypeIdToDataType.MoveTo(&cursor, typeId) )
		return mapTypeIdToDataType.GetValue(cursor);

	return 0;
}

asCObjectType *asCScriptEngine::GetObjectTypeFromTypeId(int typeId)
{
	asSMapNode<int,asCDataType*> *cursor = 0;
	if( mapTypeIdToDataType.MoveTo(&cursor, typeId) )
		return mapTypeIdToDataType.GetValue(cursor)->GetObjectType();

	return 0;
}

void asCScriptEngine::RemoveFromTypeIdMap(asCObjectType *type)
{
	asSMapNode<int,asCDataType*> *cursor = 0;
	mapTypeIdToDataType.MoveFirst(&cursor);
	while( cursor )
	{
		asCDataType *dt = mapTypeIdToDataType.GetValue(cursor);
		asSMapNode<int,asCDataType*> *old = cursor;
		mapTypeIdToDataType.MoveNext(&cursor, cursor);
		if( dt->GetObjectType() == type )
		{
			DELETE(dt,asCDataType);
			mapTypeIdToDataType.Erase(old);
		}
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

	if( length ) *length = (int)tempString->GetLength();

	return tempString->AddressOf();
}

int asCScriptEngine::GetSizeOfPrimitiveType(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return 0;
	if( !dt->IsPrimitive() ) return 0;

	return dt->GetSizeInMemoryBytes();
}

void *asCScriptEngine::CreateScriptObject(int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return 0;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return 0;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return 0;

	// Allocate the memory
	asCObjectType *objType = dt->GetObjectType();
	void *ptr = 0;

	// Construct the object
	if( objType->flags & asOBJ_SCRIPT_STRUCT )
		ptr = ScriptStructFactory(objType, this);
	else if( objType->flags & asOBJ_SCRIPT_ARRAY )
		ptr = ArrayObjectFactory(objType);
	else if( objType->flags & asOBJ_REF )
		ptr = CallGlobalFunctionRetPtr(objType->beh.construct);
	else
	{
		ptr = CallAlloc(objType);
		int funcIndex = objType->beh.construct;
		if( funcIndex )
			CallObjectMethod(ptr, funcIndex);
	}

	return ptr;
}

void *asCScriptEngine::CreateScriptObjectCopy(void *origObj, int typeId)
{
	void *newObj = CreateScriptObject(typeId);
	if( newObj == 0 ) return 0;

	CopyScriptObject(newObj, origObj, typeId);

	return newObj;
}

void asCScriptEngine::CopyScriptObject(void *dstObj, void *srcObj, int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	// Copy the contents from the original object, using the assignment operator
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();
	if( objType->beh.copy )
	{
		CallObjectMethod(dstObj, srcObj, objType->beh.copy);
	}
	else if( objType->size )
	{
		memcpy(dstObj, srcObj, objType->size);
	}
}

int asCScriptEngine::CompareScriptObjects(bool &result, int behaviour, void *leftObj, void *rightObj, int typeId)
{
	// Make sure the type id is for an object type, and not a primitive or a handle
	if( (typeId & (asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) != typeId ) return asINVALID_TYPE;
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return asINVALID_TYPE;

	// Is the behaviour valid?
	if( behaviour < (int)asBEHAVE_EQUAL || behaviour > (int)asBEHAVE_GEQUAL ) return asINVALID_ARG;

	// Get the object type information so we can find the comparison behaviours
	asCObjectType *ot = GetObjectTypeFromTypeId(typeId);
	if( !ot ) return asINVALID_TYPE;
	asCDataType dt = asCDataType::CreateObject(ot, true);
	dt.MakeReference(true);

	// Find the behaviour
	unsigned int n;
	for( n = 0; n < globalBehaviours.operators.GetLength(); n += 2 )
	{
		// Is it the right comparison operator?
		if( globalBehaviours.operators[n] == behave_dual_token[behaviour - asBEHAVE_FIRST_DUAL] )
		{
			// Is it the right datatype?
			int func = globalBehaviours.operators[n+1];
			if( scriptFunctions[func]->parameterTypes[0].IsEqualExceptConst(dt) &&
				scriptFunctions[func]->parameterTypes[1].IsEqualExceptConst(dt) )
			{
				// Call the comparison function and return the result
				result = CallGlobalFunctionRetBool(leftObj, rightObj, scriptFunctions[func]->sysFuncIntf, scriptFunctions[func]);
				return 0;
			}
		}
	}

	// If the behaviour is not supported we return an error
	result = false;
	return asNOT_SUPPORTED;
}

void asCScriptEngine::AddRefScriptObject(void *obj, int typeId)
{
	// Make sure it is not a null pointer
	if( obj == 0 ) return;

	// Make sure the type id is for an object type or a handle
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();

	if( objType->beh.addref )
	{
		// Call the addref behaviour
		CallObjectMethod(obj, objType->beh.addref);
	}
}

void asCScriptEngine::ReleaseScriptObject(void *obj, int typeId)
{
	// Make sure it is not a null pointer
	if( obj == 0 ) return;

	// Make sure the type id is for an object type or a handle
	if( (typeId & asTYPEID_MASK_OBJECT) == 0 ) return;

	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return;

	asCObjectType *objType = dt->GetObjectType();

	if( objType->beh.release )
	{
		// Call the release behaviour
		CallObjectMethod(obj, objType->beh.release);
	}
	else
	{
		// Call the destructor
		if( objType->beh.destruct )
			CallObjectMethod(obj, objType->beh.destruct);

		// Then free the memory
		CallFree(obj);
	}
}

bool asCScriptEngine::IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId)
{
	// if equal, then it is obvious they are compatible
	if( objTypeId == handleTypeId )
		return true;

	// Get the actual data types from the type ids
	const asCDataType *objDt = GetDataTypeFromTypeId(objTypeId);
	const asCDataType *hdlDt = GetDataTypeFromTypeId(handleTypeId);

	// A handle to const cannot be passed to a handle that is not referencing a const object
	if( objDt->IsHandleToConst() && !hdlDt->IsHandleToConst() )
		return false;

	if( objDt->GetObjectType() == hdlDt->GetObjectType() )
	{
		// The object type is equal
		return true;
	}
	else if( objDt->IsScriptStruct() && obj )
	{
		// There's still a chance the object implements the requested interface
		asCObjectType *objType = ((asCScriptStruct*)obj)->objType;
		if( objType->Implements(hdlDt->GetObjectType()) )
			return true;
	}

	return false;
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

int asCScriptEngine::RegisterTypedef(const char *type, const char *decl)
{
	if( type == 0 ) return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ )
	{
		if( objectTypes[n] && objectTypes[n]->name == type )
			return asALREADY_REGISTERED;
	}

	// Grab the data type
	asCTokenizer t;
	size_t tokenLen;
	eTokenType token;
	asCDataType dataType;

	//	Create the data type
	token = t.GetToken(decl, strlen(decl), &tokenLen);
	switch(token) 
	{
	case ttBool:
	case ttInt:
	case ttInt8:
	case ttInt16:
	case ttInt64:
	case ttUInt:
	case ttUInt8:
	case ttUInt16:
	case ttUInt64:
	case ttFloat:
	case ttDouble:
		if( strlen(decl) != tokenLen ) 
		{
			return ConfigError(asINVALID_TYPE);
		}
		break;

	default:
		return ConfigError(asINVALID_TYPE);
	}

	dataType = asCDataType::CreatePrimitive(token, false);

	// Make sure the name is not a reserved keyword
	token = t.GetToken(type, strlen(type), &tokenLen);
	if( token != ttIdentifier || strlen(type) != tokenLen )
		return ConfigError(asINVALID_NAME);

	asCBuilder bld(this, 0);
	int r = bld.CheckNameConflict(type, 0, 0);
	if( r < 0 )
		return ConfigError(asNAME_TAKEN);

	// Don't have to check against members of object
	// types as they are allowed to use the names

	// Put the data type in the list
	asCObjectType *object= NEW(asCObjectType)(this);
	object->arrayType = 0;
	object->flags = asOBJ_NAMED_PSEUDO;
	object->size = dataType.GetSizeInMemoryBytes();
	object->name = type;
	object->tokenType = dataType.GetTokenType();

	objectTypes.PushLast(object);

	currentGroup->objTypes.PushLast(object);

	return asSUCCESS;
}

int asCScriptEngine::RegisterEnum(const char *name)
{
	//	Check the name
	if( NULL == name ) 
		return ConfigError(asINVALID_NAME);

	// Verify if the name has been registered as a type already
	asUINT n;
	for( n = 0; n < objectTypes.GetLength(); n++ ) 
		if( objectTypes[n] && objectTypes[n]->name == name ) 
			return asALREADY_REGISTERED;

	// Use builder to parse the datatype
	asCDataType dt;
	asCBuilder bld(this, 0);
	bool oldMsgCallback = msgCallback; msgCallback = false;
	int r = bld.ParseDataType(name, &dt);
	msgCallback = oldMsgCallback;
	if( r >= 0 ) 
		return ConfigError(asERROR);

	// Make sure the name is not a reserved keyword
	asCTokenizer t;
	size_t tokenLen;
	int token = t.GetToken(name, strlen(name), &tokenLen);
	if( token != ttIdentifier || strlen(name) != tokenLen ) 
		return ConfigError(asINVALID_NAME);

	r = bld.CheckNameConflict(name, 0, 0);
	if( r < 0 ) 
		return ConfigError(asNAME_TAKEN);

	asCObjectType *st = NEW(asCObjectType)(this);

	asCDataType dataType;
	dataType.CreatePrimitive(ttInt, false);

	st->arrayType = 0;
	st->flags = asOBJ_NAMED_ENUM;
	st->size = dataType.GetSizeInMemoryBytes();
	st->name = name;
	st->tokenType = ttIdentifier;

	objectTypes.PushLast(st);

	currentGroup->objTypes.PushLast(st);

	return asSUCCESS;
}

int asCScriptEngine::RegisterEnumValue(const char *typeName, const char *valueName, int value)
{
	// Verify that the correct config group is used
	if( currentGroup->FindType(typeName) == 0 )
		return asWRONG_CONFIG_GROUP;

	asCDataType dt;
	int r;
	asCBuilder bld(this, 0);
	r = bld.ParseDataType(typeName, &dt);
	if( r < 0 )
		return ConfigError(r);

	// Store the enum value
	asCObjectType *ot = dt.GetObjectType();
	if( ot == 0 || !(ot->flags & asOBJ_NAMED_ENUM) )
		return ConfigError(asINVALID_TYPE);

	if( NULL == valueName ) 
		return ConfigError(asINVALID_NAME);

	for( unsigned int n = 0; n < ot->enumValues.GetLength(); n++ )
	{
		if( ot->enumValues[n]->name == valueName )
			return ConfigError(asALREADY_REGISTERED);
	}

	asSEnumValue *e = NEW(asSEnumValue);
	e->name = valueName;
	e->value = value;

	ot->enumValues.PushLast(e);

	return asSUCCESS;
}

int asCScriptEngine::GetObjectTypeCount()
{
	return (int)classTypes.GetLength();
}

asIObjectType *asCScriptEngine::GetObjectTypeByIndex(asUINT index)
{	
	if( index >= classTypes.GetLength() )
		return 0;

	return classTypes[index];
}

asIObjectType *asCScriptEngine::GetObjectTypeById(int typeId)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);

	// Is the type id valid?
	if( !dt ) return 0;

	return dt->GetObjectType();
}

//	Additional functionality for to access internal objects
const asIScriptFunction *asCScriptEngine::GetMethodDescriptorByIndex(int typeId, int index)
{
	const asCDataType *dt = GetDataTypeFromTypeId(typeId);
	if( dt == 0 ) return 0;

	asCObjectType *ot = dt->GetObjectType();
	if( ot == 0 ) return 0;

	if( index < 0 || (unsigned)index >= ot->methods.GetLength() ) return 0;

	return scriptFunctions[ot->methods[index]];
}

const asIScriptFunction *asCScriptEngine::GetFunctionDescriptorByIndex(const char *module, int index)
{
	asCModule *mod = GetModule(module, false);
	if( mod == 0 ) return 0;

	return mod->scriptFunctions[index];
}

END_AS_NAMESPACE

