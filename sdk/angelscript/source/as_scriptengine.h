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
#include "as_bstr_util.h"
#include "as_module.h"
#include "as_restore.h"
#include "as_callfunc.h"

#define EXECUTESTRINGID 0x7FFFFFFFul

struct asSTypeBehaviour
{
	asCDataType type;
	int construct;
	int destruct;
	int copy;
	asCArray<int> constructors;
	asCArray<int> operators;
};

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

	// Script building
	int RegisterObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterObjectProperty(const char *objname, const char *declaration, int byteOffset);
	int RegisterObjectMethod(const char *objname, const char *declaration, asUPtr funcPointer, asDWORD callConv);
	int RegisterObjectBehaviour(const char *objname, asDWORD behaviour, const char *decl, asUPtr funcPointer, asDWORD callConv);

	int RegisterGlobalProperty(const char *declaration, void *pointer);
	int RegisterGlobalFunction(const char *declaration, asUPtr funcPointer, asDWORD callConv);
	int RegisterGlobalBehaviour(asDWORD behaviour, const char *decl, asUPtr funcPointer, asDWORD callConv);

	int RegisterStringFactory(const char *datatype, asUPtr factoryFunc, asDWORD callConv);

	int AddScriptSection(const char *module, const char *name, const char *code, int codeLength, int lineOffset);
	int Build(const char *module, asIOutputStream *out);
	int Discard(const char *module);
	int GetModuleIndex(const char *module);
	const char *GetModuleNameFromIndex(int index, int *length);

	int GetFunctionCount(const char *module);
	int GetFunctionIDByIndex(const char *module, int index);
	int GetFunctionIDByName(const char *module, const char *name);
	int GetFunctionIDByDecl(const char *module, const char *decl);
	const char *GetFunctionDeclaration(int funcID, int *length);
	const char *GetFunctionName(int funcID, int *length);

	int GetGlobalVarCount(const char *module);
	int GetGlobalVarIDByIndex(const char *module, int index);
	int GetGlobalVarIDByName(const char *module, const char *name);
	int GetGlobalVarIDByDecl(const char *module, const char *decl);
	const char *GetGlobalVarDeclaration(int gvarID, int *length);
	const char *GetGlobalVarName(int gvarID, int *length);
	int GetGlobalVarPointer(int gvarID, void **pointer);

	// Dynamic binding between modules
	int GetImportedFunctionCount(const char *module);
	int GetImportedFunctionIndexByDecl(const char *module, const char *decl);
	const char *GetImportedFunctionDeclaration(const char *module, int index, int *length);
	const char *GetImportedFunctionSourceModule(const char *module, int index, int *length);
	int BindImportedFunction(const char *module, int index, int funcID);
	int UnbindImportedFunction(const char *module, int index);

	int BindAllImportedFunctions(const char *module);
	int UnbindAllImportedFunctions(const char *module);

	// Script execution
	int SetDefaultContextStackSize(asUINT initial, asUINT maximum);
	int CreateContext(asIScriptContext **context);

	// String interpretation
	int ExecuteString(const char *module, const char *script, asIOutputStream *out, asIScriptContext **ctx, asDWORD flags);

	// Bytecode Saving/Restoring
	int SaveByteCode(const char* module, asIBinaryStream* out);
	int LoadByteCode(const char* module, asIBinaryStream* in);

	asCObjectType *GetArrayType(asCDataType &type);

#ifdef AS_DEPRECATED
	int ExecuteString(const char *module, const char *script, asIOutputStream *out, asDWORD flags);
	asIScriptContext *GetContextForExecuteString();
	int GetFunctionDeclaration(int funcID, char *buffer, int bufferSize);
	int GetFunctionName(int funcID, char *buffer, int bufferSize);
	int GetGlobalVarDeclaration(int gvarID, char *buffer, int bufferSize);
	int GetGlobalVarName(int gvarID, char *buffer, int bufferSize);
	int GetImportedFunctionDeclaration(const char *module, int index, char *buffer, int bufferSize);
#endif

//protected:
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCDataType;
	friend class asCModule;
	friend class asCRestore;
	friend int CallSystemFunction(int id, asCContext *context);
	friend int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine);
#ifdef USE_ASM_VM
	friend asDWORD getGlobalPropAddress(asCScriptEngine& engine, int index);
#endif

	int RegisterSpecialObjectType(const char *objname, int byteSize, asDWORD flags);
	int RegisterSpecialObjectMethod(const char *objname, const char *declaration, asUPtr funcPointer, int callConv);
	int RegisterSpecialObjectBehaviour(const char *objname, asDWORD behaviour, const char *decl, asUPtr funcPointer, int callConv);


	void Reset();
	void PrepareEngine();
	bool isPrepared;

	int CreateContext(asIScriptContext **context, bool isInternal);

	int AddObjectType(const char *type, int byteSize);
	asCObjectType *GetObjectType(const char *type, int pointerLevel = 0, int arrayDimensions = 0);

	int AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal);

	asCString GetFunctionDeclaration(int funcID);

	asSTypeBehaviour *GetBehaviour(const asCDataType *type, bool notDefault = false);
	int GetBehaviourIndex(const asCDataType *type);

	asCScriptFunction *GetScriptFunction(int funcID);

	int initialContextStackSize;
	int maximumContextStackSize;

	// Information registered by host
	asSTypeBehaviour globalBehaviours;
	asCObjectType *defaultArrayObjectType;
	asCArray<asCObjectType *> objectTypes;
	asCArray<asCObjectType *> arrayTypes;
	asCArray<asCProperty *> globalProps;
	asCArray<void *> globalPropAddresses;
	asCArray<asSTypeBehaviour *> typeBehaviours;
	asSTypeBehaviour *defaultArrayObjectBehaviour;
	asCArray<asCScriptFunction *> systemFunctions;
	asCArray<asSSystemFunctionInterface *> systemFunctionInterfaces;
	asCScriptFunction *stringFactory;

	int ConfigError(int err);
	bool configFailed;

	// Script modules
	asCModule *GetModule(const char *name, bool create);
	asCModule *GetModule(int id);

	// These resources must be protected for multiple accesses
	int refCount;
	asCArray<asCModule *> scriptModules;
	asCModule *lastModule;
#ifdef AS_DEPRECATED
	asCContext *stringContext;
#endif

	// Critical sections for threads
	DECLARECRITICALSECTION(engineCritical);
	DECLARECRITICALSECTION(moduleCritical);
};

#endif
