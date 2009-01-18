/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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
#include "as_atomic.h"
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
//=============================================================
// From asIScriptEngine
//=============================================================
public:
	// Memory management
	virtual int AddRef();
	virtual int Release();

	// Engine configuration
	virtual int     SetEngineProperty(asEEngineProp property, asPWORD value);
	virtual asPWORD GetEngineProperty(asEEngineProp property);

	virtual int SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv);
	virtual int ClearMessageCallback();
	virtual int WriteMessage(const char *section, int row, int col, asEMsgType type, const char *message);

	virtual int RegisterObjectType(const char *obj, int byteSize, asDWORD flags);
	virtual int RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset);
	virtual int RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	virtual int RegisterObjectBehaviour(const char *obj, asEBehaviours behaviour, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);

	virtual int RegisterGlobalProperty(const char *declaration, void *pointer);
	virtual int RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	virtual int RegisterGlobalBehaviour(asEBehaviours behaviour, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);

	virtual int RegisterInterface(const char *name);
	virtual int RegisterInterfaceMethod(const char *intf, const char *declaration);

	virtual int RegisterEnum(const char *type);
	virtual int RegisterEnumValue(const char *type, const char *name, int value);

	virtual int RegisterTypedef(const char *type, const char *decl);

	virtual int RegisterStringFactory(const char *datatype, const asSFuncPtr &factoryFunc, asDWORD callConv);

	virtual int BeginConfigGroup(const char *groupName);
	virtual int EndConfigGroup();
	virtual int RemoveConfigGroup(const char *groupName);
	virtual int SetConfigGroupModuleAccess(const char *groupName, const char *module, bool hasAccess);

	// Script modules
	virtual asIScriptModule *GetModule(const char *module, asEGMFlags flag);
	virtual int              DiscardModule(const char *module);

	// Script functions
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId);

	// Type identification
	virtual int            GetObjectTypeCount();
	virtual asIObjectType *GetObjectTypeByIndex(asUINT index);
	virtual asIObjectType *GetObjectTypeById(int typeId);
	virtual int            GetTypeIdByDecl(const char *decl);
	virtual const char    *GetTypeDeclaration(int typeId, int *length = 0);
	virtual int            GetSizeOfPrimitiveType(int typeId);

	// Script execution
	virtual asIScriptContext *CreateContext();
	virtual void             *CreateScriptObject(int typeId);
	virtual void             *CreateScriptObjectCopy(void *obj, int typeId);
	virtual void              CopyScriptObject(void *dstObj, void *srcObj, int typeId);
	virtual void              ReleaseScriptObject(void *obj, int typeId);
	virtual void              AddRefScriptObject(void *obj, int typeId);
	virtual bool              IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId);
	virtual int               CompareScriptObjects(bool &result, int behaviour, void *leftObj, void *rightObj, int typeId);

	// String interpretation
	virtual asETokenClass ParseToken(const char *string, size_t stringLength = 0, int *tokenLength = 0);
	virtual int           ExecuteString(const char *module, const char *script, asIScriptContext **ctx, asDWORD flags);

	// Garbage collection
	virtual int  GarbageCollect(asEGCFlags flags = asGC_FULL_CYCLE);
	virtual void GetGCStatistics(asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected);
	virtual void NotifyGarbageCollectorOfNewObject(void *obj, int typeId);
	virtual void GCEnumCallback(void *reference);

	// User data
	virtual void *SetUserData(void *data);
	virtual void *GetUserData();

#ifdef AS_DEPRECATED
	virtual int                AddScriptSection(const char *module, const char *name, const char *code, size_t codeLength, int lineOffset);
	virtual int                Build(const char *module);
	virtual int                Discard(const char *module);
	virtual int                ResetModule(const char *module);
	virtual int                GetFunctionCount(const char *module);
	virtual int                GetFunctionIDByIndex(const char *module, int index);
	virtual int                GetFunctionIDByName(const char *module, const char *name);
	virtual int                GetFunctionIDByDecl(const char *module, const char *decl);
	virtual asIScriptFunction *GetFunctionDescriptorByIndex(const char *module, int index);
	virtual int                GetGlobalVarCount(const char *module);
	virtual int                GetGlobalVarIndexByName(const char *module, const char *name);
	virtual int                GetGlobalVarIndexByDecl(const char *module, const char *decl);
	virtual const char        *GetGlobalVarDeclaration(const char *module, int index, int *length = 0);
	virtual const char        *GetGlobalVarName(const char *module, int index, int *length = 0);
	virtual void              *GetAddressOfGlobalVar(const char *module, int index);
	virtual int                GetGlobalVarIDByIndex(const char *module, int index);
	virtual int                GetGlobalVarIDByName(const char *module, const char *name);
	virtual int                GetGlobalVarIDByDecl(const char *module, const char *decl);
	virtual const char        *GetGlobalVarDeclaration(int gvarID, int *length);
	virtual const char        *GetGlobalVarName(int gvarID, int *length);
	virtual void              *GetGlobalVarPointer(int gvarID);
	virtual int                GetImportedFunctionCount(const char *module);
	virtual int                GetImportedFunctionIndexByDecl(const char *module, const char *decl);
	virtual const char        *GetImportedFunctionDeclaration(const char *module, int importIndex, int *length);
	virtual const char        *GetImportedFunctionSourceModule(const char *module, int importIndex, int *length);
	virtual int                BindImportedFunction(const char *module, int importIndex, int funcId);
	virtual int                UnbindImportedFunction(const char *module, int importIndex);
	virtual int                BindAllImportedFunctions(const char *module);
	virtual int                UnbindAllImportedFunctions(const char *module);
	virtual int                GetTypeIdByDecl(const char *module, const char *decl);
	virtual int                GetObjectsInGarbageCollectorCount();
	virtual int                SaveByteCode(const char *module, asIBinaryStream *out);
	virtual int                LoadByteCode(const char *module, asIBinaryStream *in);
#endif

//===========================================================
// internal methods
//===========================================================
public:
	asCScriptEngine();
	virtual ~asCScriptEngine();

	asCObjectType *GetArrayTypeFromSubType(asCDataType &subType);

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
	void  CallFree(void *obj);
	void *CallGlobalFunctionRetPtr(int func);
	void *CallGlobalFunctionRetPtr(asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void  CallObjectMethod(void *obj, int func);
	void  CallObjectMethod(void *obj, void *param, int func);
	void  CallObjectMethod(void *obj, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void  CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool  CallObjectMethodRetBool(void *obj, int func);
	int   CallObjectMethodRetInt(void *obj, int func);
	void  CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool  CallGlobalFunctionRetBool(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);

	void ClearUnusedTypes();
	void RemoveArrayType(asCObjectType *t);
	void RemoveTypeAndRelatedFromList(asCArray<asCObjectType*> &types, asCObjectType *ot);

	asCConfigGroup *FindConfigGroup(asCObjectType *ot);
	asCConfigGroup *FindConfigGroupForFunction(int funcId);
	asCConfigGroup *FindConfigGroupForGlobalVar(int gvarId);
	asCConfigGroup *FindConfigGroupForObjectType(asCObjectType *type);

	int  RequestBuild();
	void BuildCompleted();

	void Reset();
	void PrepareEngine();
	bool isPrepared;

	int CreateContext(asIScriptContext **context, bool isInternal);

	asCObjectType *GetObjectType(const char *type);
	asCObjectType *GetArrayType(const char *type);

	int AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal);

	asCString GetFunctionDeclaration(int funcID);

	asCScriptFunction *GetScriptFunction(int funcID);

	asCModule *GetModule(const char *name, bool create);
	asCModule *GetModule(int id);
	asCModule *GetModuleFromFuncId(int funcId);

	int  GetMethodIDByDecl(const asCObjectType *ot, const char *decl, asCModule *mod);

	int  GetNextScriptFunctionId();
	void SetScriptFunction(asCScriptFunction *func);
	void DeleteScriptFunction(int id);

	int ConfigError(int err);

	int                GetTypeIdFromDataType(const asCDataType &dt);
	const asCDataType *GetDataTypeFromTypeId(int typeId);
	asCObjectType     *GetObjectTypeFromTypeId(int typeId);
	void               RemoveFromTypeIdMap(asCObjectType *type);

//===========================================================
// internal properties
//===========================================================
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

	// This array stores the type and names of the registered global properties
	asCArray<asCGlobalProperty *>  globalProps;

	// This array stores pointers to each registered global property. It allows the virtual 
	// machine to directly find the value of the global property using an index into this array.
	asCArray<void *>               globalPropAddresses;

	asCScriptFunction             *stringFactory;

	bool configFailed;

	// Script modules
	asCArray<asCScriptFunction *> scriptFunctions;
	asCArray<int>                 freeScriptFunctionIds;
	asCArray<asCScriptFunction *> signatureIds;

	// These resources must be protected for multiple accesses
	asCAtomic              refCount;
	asCArray<asCModule *>  scriptModules;
	asCModule             *lastModule;
	bool                   isBuilding;

	// Stores script declared object types
	asCArray<asCObjectType *> classTypes;
	asCArray<asCObjectType *> scriptArrayTypes;

	// Type identifiers
	int                       typeIdSeqNbr;
	asCMap<int, asCDataType*> mapTypeIdToDataType;

	// Garbage collector
	asCGarbageCollector gc;

	// Dynamic groups
	asCConfigGroup             defaultGroup;
	asCArray<asCConfigGroup*>  configGroups;
	asCConfigGroup            *currentGroup;

	// Message callback
	bool                        msgCallback;
	asSSystemFunctionInterface  msgCallbackFunc;
	void                       *msgCallbackObj;

	// User data
	void *userData;

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
