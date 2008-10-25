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
// as_scriptengine.h
//
// The implementation of the script engine interface
//



#ifndef AS_SCRIPTENGINE_H
#define AS_SCRIPTENGINE_H

#include "as_config.h"
#include "as_thread.h"
#include "as_scriptfunction.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_objecttype.h"
#include "as_module.h"
#include "as_restore.h"
#include "as_callfunc.h"
#include "as_configgroup.h"
#include "as_memory.h"
#include "as_gc.h"

BEGIN_AS_NAMESPACE

#define EXECUTESTRINGID 0x7FFFFFFFul

class asCBuilder;
class asCContext;

class asCScriptEngine : public asIScriptEngine
{
public:
	asCScriptEngine();
	virtual ~asCScriptEngine();

	// Memory management
	int AddRef();
	int Release();

	// Configuration
	int     SetEngineProperty(asEEngineProp property, asPWORD value);
	asPWORD GetEngineProperty(asEEngineProp property);

	// Script building
	int SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv);
	int ClearMessageCallback();
	int WriteMessage(const char *section, int row, int col, asEMsgType type, const char *message);

	int RegisterTypedef(const char *type, const char *decl);

	int RegisterEnum(const char *type);
	int RegisterEnumValue(const char *type, const char *name, int value);

	int RegisterObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterObjectProperty(const char *objname, const char *declaration, int byteOffset);
	int RegisterObjectMethod(const char *objname, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	int RegisterObjectBehaviour(const char *objname, asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv);

	int RegisterGlobalProperty(const char *declaration, void *pointer);
	int RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	int RegisterGlobalBehaviour(asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv);

	int RegisterInterface(const char *name);
	int RegisterInterfaceMethod(const char *intf, const char *declaration);

	int RegisterStringFactory(const char *datatype, const asSFuncPtr &factoryFunc, asDWORD callConv);

	int BeginConfigGroup(const char *groupName);
	int EndConfigGroup();
	int RemoveConfigGroup(const char *groupName);
	int SetConfigGroupModuleAccess(const char *groupName, const char *module, bool haveAccess);

	int AddScriptSection(const char *module, const char *name, const char *code, size_t codeLength, int lineOffset);
	int Build(const char *module);
	int Discard(const char *module);
	int ResetModule(const char *module);
	int GetFunctionCount(const char *module);
	int GetFunctionIDByIndex(const char *module, int index);
	int GetFunctionIDByName(const char *module, const char *name);
	int GetFunctionIDByDecl(const char *module, const char *decl);
#ifdef AS_DEPRECATED
	const char *GetFunctionDeclaration(int funcID, int *length);
	const char *GetFunctionName(int funcID, int *length);
	const char *GetFunctionModule(int funcID, int *length);
	const char *GetFunctionSection(int funcID, int *length);
#endif
	asIScriptFunction *GetFunctionDescriptorByIndex(const char *module, int index);
	asIScriptFunction *GetFunctionDescriptorById(int funcId);

#ifdef AS_DEPRECATED
	int GetMethodCount(int typeId);
	int GetMethodIDByIndex(int typeId, int index);
	int GetMethodIDByName(int typeId, const char *name);
	int GetMethodIDByDecl(int typeId, const char *decl);
	asIScriptFunction *GetMethodDescriptorByIndex(int typeId, int index);
#endif

	int         GetGlobalVarCount(const char *module);
	int         GetGlobalVarIndexByName(const char *module, const char *name);
	int         GetGlobalVarIndexByDecl(const char *module, const char *decl);
	const char *GetGlobalVarDeclaration(const char *module, int index, int *length = 0);
	const char *GetGlobalVarName(const char *module, int index, int *length = 0);
	void       *GetAddressOfGlobalVar(const char *module, int index);
#ifdef AS_DEPRECATED
	int GetGlobalVarIDByIndex(const char *module, int index);
	int GetGlobalVarIDByName(const char *module, const char *name);
	int GetGlobalVarIDByDecl(const char *module, const char *decl);
	const char *GetGlobalVarDeclaration(int gvarID, int *length);
	const char *GetGlobalVarName(int gvarID, int *length);
	void *GetGlobalVarPointer(int gvarID);
#endif

	// Dynamic binding between modules
	int GetImportedFunctionCount(const char *module);
	int GetImportedFunctionIndexByDecl(const char *module, const char *decl);
	const char *GetImportedFunctionDeclaration(const char *module, int index, int *length);
	const char *GetImportedFunctionSourceModule(const char *module, int index, int *length);
	int BindImportedFunction(const char *module, int index, int funcID);
	int UnbindImportedFunction(const char *module, int index);

	int BindAllImportedFunctions(const char *module);
	int UnbindAllImportedFunctions(const char *module);

	// Type identification
	int GetTypeIdByDecl(const char *module, const char *decl);
	const char *GetTypeDeclaration(int typeId, int *length = 0);
	int GetSizeOfPrimitiveType(int typeId);
	asIObjectType *GetObjectTypeById(int typeId);
	asIObjectType *GetObjectTypeByIndex(asUINT index);
	int GetObjectTypeCount();

	// Script execution
#ifdef AS_DEPRECATED
	int SetDefaultContextStackSize(asUINT initial, asUINT maximum);
#endif
	asIScriptContext *CreateContext();
	void *CreateScriptObject(int typeId);
	void *CreateScriptObjectCopy(void *obj, int typeId);
	void CopyScriptObject(void *dstObj, void *srcObj, int typeId);
	void ReleaseScriptObject(void *obj, int typeId);
	void AddRefScriptObject(void *obj, int typeId);
	bool IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId);
	int CompareScriptObjects(bool &result, int behaviour, void *leftObj, void *rightObj, int typeId);

	// String interpretation
	int ExecuteString(const char *module, const char *script, asIScriptContext **ctx, asDWORD flags);

	// Bytecode Saving/Restoring
	int SaveByteCode(const char *module, asIBinaryStream *out);
	int LoadByteCode(const char *module, asIBinaryStream *in);


	asCObjectType *GetArrayTypeFromSubType(asCDataType &subType);


	int GarbageCollect(bool doFullCycle);
	int GetObjectsInGarbageCollectorCount();
	void NotifyGarbageCollectorOfNewObject(void *obj, int typeId);
	void GCEnumCallback(void *reference);

//protected:
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCDataType;
	friend class asCModule;
	friend class asCRestore;
	friend class asCByteCode;
	friend int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine);

	int RegisterSpecialObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterSpecialObjectMethod(const char *objname, const char *declaration, const asSFuncPtr &funcPointer, int callConv);
	int RegisterSpecialObjectBehaviour(asCObjectType *objType, asDWORD behaviour, const char *decl, const asSFuncPtr &funcPointer, int callConv);

	int VerifyVarTypeNotInFunction(asCScriptFunction *func);

	void *CallAlloc(asCObjectType *objType);
	void CallFree(void *obj);
	void *CallGlobalFunctionRetPtr(int func);
	void *CallGlobalFunctionRetPtr(asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void CallObjectMethod(void *obj, int func);
	void CallObjectMethod(void *obj, void *param, int func);
	void CallObjectMethod(void *obj, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool CallObjectMethodRetBool(void *obj, int func);
	int  CallObjectMethodRetInt(void *obj, int func);
	void CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool CallGlobalFunctionRetBool(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);

	void ClearUnusedTypes();
	void RemoveArrayType(asCObjectType *t);
	void RemoveTypeAndRelatedFromList(asCArray<asCObjectType*> &types, asCObjectType *ot);

	asCConfigGroup *FindConfigGroup(asCObjectType *ot);
	asCConfigGroup *FindConfigGroupForFunction(int funcId);
	asCConfigGroup *FindConfigGroupForGlobalVar(int gvarId);
	asCConfigGroup *FindConfigGroupForObjectType(asCObjectType *type);

	void Reset();
	void PrepareEngine();
	bool isPrepared;

	int CreateContext(asIScriptContext **context, bool isInternal);

	asCObjectType *GetObjectType(const char *type);
	asCObjectType *GetArrayType(const char *type);

	int AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal);

	asCString GetFunctionDeclaration(int funcID);

	asCScriptFunction *GetScriptFunction(int funcID);

	asCMemoryMgr memoryMgr;

	int initialContextStackSize;

	// Information registered by host
	asSTypeBehaviour globalBehaviours;
	asCObjectType   *defaultArrayObjectType;
	asCObjectType    scriptTypeBehaviours;

	// Stores all known object types, both application registered, and script declared
	asCArray<asCObjectType *>      objectTypes;
	// Store information about registered array types
	asCArray<asCObjectType *>      arrayTypes;
	asCArray<asCProperty *>        globalProps;
	asCArray<void *>               globalPropAddresses;
	asCScriptFunction             *stringFactory;

	int ConfigError(int err);
	bool configFailed;

	// Script modules
	asCModule *GetModule(const char *name, bool create);
	asCModule *GetModule(int id);
	asCModule *GetModuleFromFuncId(int funcId);

	int GetMethodIDByDecl(const asCObjectType *ot, const char *decl, asCModule *mod);

	int GetNextScriptFunctionId();
	void SetScriptFunction(asCScriptFunction *func);
	void DeleteScriptFunction(int id);
	asCArray<asCScriptFunction *> scriptFunctions;
	asCArray<int> freeScriptFunctionIds;
	asCArray<asCScriptFunction *> signatureIds;

	// These resources must be protected for multiple accesses
	int refCount;
	asCArray<asCModule *> scriptModules;
	asCModule *lastModule;
	bool isBuilding;

	// Stores script declared object types
	asCArray<asCObjectType *> classTypes;
	asCArray<asCObjectType *> scriptArrayTypes;

	// Type identifiers
	int typeIdSeqNbr;
	asCMap<int, asCDataType*> mapTypeIdToDataType;
	int GetTypeIdFromDataType(const asCDataType &dt);
	const asCDataType *GetDataTypeFromTypeId(int typeId);
	asCObjectType *GetObjectTypeFromTypeId(int typeId);
	void RemoveFromTypeIdMap(asCObjectType *type);

	// Garbage collector
	asCGarbageCollector gc;

	// Dynamic groups
	asCConfigGroup defaultGroup;
	asCArray<asCConfigGroup*> configGroups;
	asCConfigGroup *currentGroup;

	// Message callback
	bool msgCallback;
	asSSystemFunctionInterface msgCallbackFunc;
	void *msgCallbackObj;

	// Critical sections for threads
	DECLARECRITICALSECTION(engineCritical);

	// Engine properties
	struct
	{
		bool allowUnsafeReferences;
		bool optimizeByteCode;
		bool copyScriptSections;
		int  maximumContextStackSize;
		bool useCharacterLiterals;
		bool allowMultilineStrings;
		bool allowImplicitHandleTypes;
	} ep;
};

END_AS_NAMESPACE

#endif
