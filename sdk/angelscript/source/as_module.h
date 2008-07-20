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
// as_module.h
//
// A class that holds a script module
//

#ifndef AS_MODULE_H
#define AS_MODULE_H

#include "as_config.h"
#include "as_thread.h"
#include "as_string.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_scriptfunction.h"
#include "as_property.h"

BEGIN_AS_NAMESPACE

const int asFUNC_INIT   = 0xFFFF;
const int asFUNC_STRING = 0xFFFE;

const int FUNC_IMPORTED = 0x40000000;

class asCScriptEngine;
class asCCompiler;
class asCBuilder;
class asCContext;
class asCConfigGroup;

struct sBindInfo
{
	asDWORD importFrom;
	int importedFunction;
};

// TODO: Move this to angelscript.h
class asIScriptModule
{
public:
	virtual asIScriptEngine *GetEngine() = 0;
	virtual void SetName(const char *name) = 0;
	virtual const char *GetName(int *length = 0) = 0; 

	virtual int  AddScriptSection(const char *name, const char *code, size_t codeLength, int lineOffset = 0) = 0;
	virtual int  Build() = 0;
    virtual void Discard() = 0;
	virtual int  Reinitialize() = 0;

	// Script functions
	virtual int GetFunctionCount() = 0;
	virtual int GetFunctionIdByIndex(int index) = 0;
	virtual int GetFunctionIdByName(const char *name) = 0;
	virtual int GetFunctionIdByDecl(const char *decl) = 0;
	virtual asIScriptFunction *GetFunctionDescriptorByIndex(int index) = 0;
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId) = 0;

	// Script global variables
	virtual int GetGlobalVarCount() = 0;
	virtual int GetGlobalVarIndexByName(const char *name) = 0;
	virtual int GetGlobalVarIndexByDecl(const char *decl) = 0;
	virtual const char *GetGlobalVarDeclaration(int index, int *length = 0) = 0;
	virtual const char *GetGlobalVarName(int index, int *length = 0) = 0;
	virtual void *GetAddressOfGlobalVar(int index) = 0;

	// Dynamic binding between modules
	virtual int GetImportedFunctionCount() = 0;
	virtual int GetImportedFunctionIndexByDecl(const char *decl) = 0;
//	virtual const char *GetImportedFunctionDeclaration(int importIndex, int *length = 0) = 0;
//	virtual const char *GetImportedFunctionSourceModule(int importIndex, int *length = 0) = 0;
	virtual int BindImportedFunction(int importIndex, int funcId) = 0;
//	virtual int UnbindImportedFunction(int importIndex) = 0;

//	virtual int BindAllImportedFunctions() = 0;
//	virtual int UnbindAllImportedFunctions() = 0;

	// Type identification
//	virtual int GetTypeIdByDecl(const char *decl) = 0;
//	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) = 0;
//	virtual int GetObjectTypeCount() = 0;

protected:
	virtual ~asIScriptModule() {}
};

class asCModule : public asIScriptModule
{
public:
	asCModule(const char *name, int id, asCScriptEngine *engine);
	~asCModule();

	asIScriptEngine *GetEngine();
	void             SetName(const char *name);
	const char      *GetName(int *length);

	int  AddScriptSection(const char *name, const char *code, size_t codeLength, int lineOffset);
	int  Build();
	void Discard();

	int  Reinitialize();

	int  GetFunctionCount();
	int  GetFunctionIdByIndex(int index);
	int  GetFunctionIdByName(const char *name);
	int  GetFunctionIdByDecl(const char *decl);
	asIScriptFunction *GetFunctionDescriptorByIndex(int index);
	asIScriptFunction *GetFunctionDescriptorById(int funcId);

	int  GetGlobalVarCount();
	int  GetGlobalVarIndexByName(const char *name);
	int  GetGlobalVarIndexByDecl(const char *decl);
	const char *GetGlobalVarDeclaration(int index, int *length);
	const char *GetGlobalVarName(int index, int *length);
	void *GetAddressOfGlobalVar(int index);

	asCString name;

//protected:
	friend class asCScriptEngine;
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCRestore;

	void InternalReset();

	int  AddContextRef();
	int  ReleaseContextRef();
	int  contextCount;

	int  AddModuleRef();
	int  ReleaseModuleRef();
	int  moduleCount;

	void CallInit();
	void CallExit();
	bool isGlobalVarInitialized;

	bool IsUsed();

	int  AddConstantString(const char *str, size_t length);
	const asCString &GetConstantString(int id);

	int  AllocGlobalMemory(int size);

	int  GetNextFunctionId();
	int  AddScriptFunction(int sectionIdx, int id, const char *name, const asCDataType &returnType, asCDataType *params, int *inOutFlags, int paramCount, bool isInterface, asCObjectType *objType = 0);
	int  AddImportedFunction(int id, const char *name, const asCDataType &returnType, asCDataType *params, int *inOutFlags, int paramCount, int moduleNameStringID);

	bool CanDeleteAllReferences(asCArray<asCModule*> &modules);

	void UpdateGlobalVarPointer(void *pold, void *pnew);

	int  GetNextImportedFunctionId();
	int  GetImportedFunctionCount();
	int  GetImportedFunctionIndexByDecl(const char *decl);
	const char *GetImportedFunctionSourceModule(int index);
	int  BindImportedFunction(int index, int sourceID);

	asCScriptFunction *GetImportedFunction(int funcID);
	asCScriptFunction *GetScriptFunction(int funcID);
	asCScriptFunction *GetSpecialFunction(int funcID);

	asCObjectType *GetObjectType(const char *type);

	int  GetScriptSectionIndex(const char *name);
	bool CanDelete();

	int GetGlobalVarIndex(int propIdx);

	asCScriptEngine *engine;
	asCBuilder      *builder;
	bool             isBuildWithoutErrors;

	int  moduleID;
	bool isDiscarded;

	asCScriptFunction             *initFunction;
	asCArray<asCString *>          scriptSections;
	asCArray<asCScriptFunction *>  scriptFunctions;
	asCArray<asCScriptFunction *>  importedFunctions;
	asCArray<sBindInfo>            bindInformations;
	asCArray<asCProperty *>        scriptGlobals;
	asCArray<size_t>               globalMem;
	asCArray<void*>                globalVarPointers;
	asCArray<asCString*>           stringConstants;
	asCArray<asCObjectType*>       classTypes;

	DECLARECRITICALSECTION(criticalSection);
};

END_AS_NAMESPACE

#endif
