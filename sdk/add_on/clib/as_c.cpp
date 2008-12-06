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
// as_c.cpp
//
// A C interface to the library 
//

// Include the C++ interface header so we can call the proper methods
#include "../../angelscript/include/angelscript.h"

BEGIN_AS_NAMESPACE

typedef void (*asBINARYREADFUNC_t)(void *ptr, asUINT size, void *param);
typedef void (*asBINARYWRITEFUNC_t)(const void *ptr, asUINT size, void *param);

class asCBinaryStreamC : public asIBinaryStream
{
public:
	asCBinaryStreamC(asBINARYWRITEFUNC_t write, asBINARYREADFUNC_t read, void *param) {this->write = write; this->read = read; this->param = param;}

	void Write(const void *ptr, asUINT size) { write(ptr, size, param); }
	void Read(void *ptr, asUINT size) { read(ptr, size, param); }

	asBINARYREADFUNC_t read;
	asBINARYWRITEFUNC_t write;
	void *param;
};

extern "C"
{

// Implement a global wrapper function for each of the library's interface methods
AS_API int                asEngine_AddRef(asIScriptEngine *e)                                                                                                                                      { return e->AddRef(); }
AS_API int                asEngine_Release(asIScriptEngine *e)                                                                                                                                     { return e->Release(); }
AS_API int                asEngine_SetEngineProperty(asIScriptEngine *e, asEEngineProp property, asPWORD value)                                                                                    { return e->SetEngineProperty(property, value); }
AS_API asPWORD            asEngine_GetEngineProperty(asIScriptEngine *e, asEEngineProp property)                                                                                                   { return e->GetEngineProperty(property); }
AS_API int                asEngine_SetMessageCallback(asIScriptEngine *e, asFUNCTION_t callback, void *obj, asDWORD callConv)                                                                      { return e->SetMessageCallback(asFUNCTION(callback), obj, callConv); }
AS_API int                asEngine_ClearMessageCallback(asIScriptEngine *e)                                                                                                                        { return e->ClearMessageCallback(); }
AS_API int                asEngine_WriteMessage(asIScriptEngine *e, const char *section, int row, int col, asEMsgType type, const char *message)                                                   { return e->WriteMessage(section, row, col, type, message); }
AS_API int                asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags)                                                                           { return e->RegisterObjectType(name, byteSize, flags); }
AS_API int                asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset)                                                            { return e->RegisterObjectProperty(obj, declaration, byteOffset); }
AS_API int                asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)                                  { return e->RegisterObjectMethod(obj, declaration, asFUNCTION(funcPointer), callConv); }
AS_API int                asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *datatype, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv) { return e->RegisterObjectBehaviour(datatype, behaviour, declaration, asFUNCTION(funcPointer), callConv); }
AS_API int                asEngine_RegisterGlobalProperty(asIScriptEngine *e, const char *declaration, void *pointer)                                                                              { return e->RegisterGlobalProperty(declaration, pointer); }
AS_API int                asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)                                                 { return e->RegisterGlobalFunction(declaration, asFUNCTION(funcPointer), callConv); }
AS_API int                asEngine_RegisterGlobalBehaviour(asIScriptEngine *e, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)                       { return e->RegisterGlobalBehaviour(behaviour, declaration, asFUNCTION(funcPointer), callConv); }
AS_API int                asEngine_RegisterInterface(asIScriptEngine *e, const char *name)                                                                                                         { return e->RegisterInterface(name); }
AS_API int                asEngine_RegisterInterfaceMethod(asIScriptEngine *e, const char *intf, const char *declaration)                                                                          { return e->RegisterInterfaceMethod(intf, declaration); }
AS_API int                asEngine_RegisterEnum(asIScriptEngine *e, const char *type)                                                                                                              { return e->RegisterEnum(type); }
AS_API int                asEngine_RegisterEnumValue(asIScriptEngine *e, const char *type, const char *name, int value)                                                                            { return e->RegisterEnumValue(type,name,value); }
AS_API int                asEngine_RegisterTypedef(asIScriptEngine *e, const char *type, const char *decl)                                                                                         { return e->RegisterTypedef(type,decl); }
AS_API int                asEngine_RegisterStringFactory(asIScriptEngine *e, const char *datatype, asFUNCTION_t factoryFunc, asDWORD callConv)                                                     { return e->RegisterStringFactory(datatype, asFUNCTION(factoryFunc), callConv); }
AS_API int                asEngine_BeginConfigGroup(asIScriptEngine *e, const char *groupName)                                                                                                     { return e->BeginConfigGroup(groupName); }
AS_API int                asEngine_EndConfigGroup(asIScriptEngine *e)                                                                                                                              { return e->EndConfigGroup(); }
AS_API int                asEngine_RemoveConfigGroup(asIScriptEngine *e, const char *groupName)                                                                                                    { return e->RemoveConfigGroup(groupName); }
AS_API int                asEngine_SetConfigGroupModuleAccess(asIScriptEngine *e, const char *groupName, const char *module, bool haveAccess)                                                      { return e->SetConfigGroupModuleAccess(groupName, module, haveAccess); }
AS_API asIScriptModule  * asEngine_GetModule(asIScriptEngine *e, const char *module, asEGMFlags flag) { return e->GetModule(module, flag); }
AS_API int                asEngine_DiscardModule(asIScriptEngine *e, const char *module) { return e->DiscardModule(module); }
AS_API asIScriptFunction *asEngine_GetFunctionDescriptorById(asIScriptEngine *e, int funcId)                                                                                                      { return e->GetFunctionDescriptorById(funcId); }
AS_API int                asEngine_GetTypeIdByDecl(asIScriptEngine *e, const char *decl)                                                                                                           { return e->GetTypeIdByDecl(decl); }
AS_API const char *       asEngine_GetTypeDeclaration(asIScriptEngine *e, int typeId, int *length)                                                                                                 { return e->GetTypeDeclaration(typeId, length); }
AS_API int                asEngine_GetSizeOfPrimitiveType(asIScriptEngine *e, int typeId)                                                                                                          { return e->GetSizeOfPrimitiveType(typeId); }
AS_API asIObjectType *    asEngine_GetObjectTypeById(asIScriptEngine *e, int typeId)                                                                                                               { return e->GetObjectTypeById(typeId); }
AS_API asIObjectType *    asEngine_GetObjectTypeByIndex(asIScriptEngine *e, asUINT index)                                                                                                          { return e->GetObjectTypeByIndex(index); }
AS_API int                asEngine_GetObjectTypeCount(asIScriptEngine *e)                                                                                                                          { return e->GetObjectTypeCount(); }
AS_API asIScriptContext * asEngine_CreateContext(asIScriptEngine *e)                                                                                                                               { return e->CreateContext(); }
AS_API void *             asEngine_CreateScriptObject(asIScriptEngine *e, int typeId)                                                                                                              { return e->CreateScriptObject(typeId); }
AS_API void *             asEngine_CreateScriptObjectCopy(asIScriptEngine *e, void *obj, int typeId)                                                                                               { return e->CreateScriptObjectCopy(obj, typeId); }
AS_API void               asEngine_CopyScriptObject(asIScriptEngine *e, void *dstObj, void *srcObj, int typeId)                                                                                    { e->CopyScriptObject(dstObj, srcObj, typeId); }
AS_API void               asEngine_ReleaseScriptObject(asIScriptEngine *e, void *obj, int typeId)                                                                                                  { e->ReleaseScriptObject(obj, typeId); }
AS_API void               asEngine_AddRefScriptObject(asIScriptEngine *e, void *obj, int typeId)                                                                                                   { e->AddRefScriptObject(obj, typeId); }
AS_API bool               asEngine_IsHandleCompatibleWithObject(asIScriptEngine *e, void *obj, int objTypeId, int handleTypeId)                                                                    { return e->IsHandleCompatibleWithObject(obj, objTypeId, handleTypeId); }
AS_API int                asEngine_CompareScriptObjects(asIScriptEngine *e, bool &result, int behaviour, void *leftObj, void *rightObj, int typeId)                                                { return e->CompareScriptObjects(result, behaviour, leftObj, rightObj, typeId); }
AS_API int                asEngine_ExecuteString(asIScriptEngine *e, const char *module, const char *script, asIScriptContext **ctx, asDWORD flags)                                                { return e->ExecuteString(module, script, ctx, flags); }
AS_API int                asEngine_GarbageCollect(asIScriptEngine *e, asEGCFlags flags)                                                                                                            { return e->GarbageCollect(flags); }
AS_API void               asEngine_GetGCStatistics(asIScriptEngine *e, asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected)                                                         { e->GetGCStatistics(currentSize, totalDestroyed, totalDetected); }
AS_API void               asEngine_NotifyGarbageCollectorOfNewObject(asIScriptEngine *e, void *obj, int typeId)                                                                                    { e->NotifyGarbageCollectorOfNewObject(obj, typeId); }
AS_API void               asEngine_GCEnumCallback(asIScriptEngine *e, void *obj)                                                                                                                   { e->GCEnumCallback(obj); }
AS_API void *             asEngine_SetUserData(asIScriptEngine *e, void *data) {return e->SetUserData(data);}
AS_API void *             asEngine_GetUserData(asIScriptEngine *e) {return e->GetUserData();}

AS_API asIScriptEngine   *asModule_GetEngine(asIScriptModule *m) { return m->GetEngine(); }
AS_API void               asModule_SetName(asIScriptModule *m, const char *name) { m->SetName(name); }
AS_API const char        *asModule_GetName(asIScriptModule *m, int *length /*= 0*/) { return m->GetName(length); }
AS_API int                asModule_AddScriptSection(asIScriptModule *m, const char *name, const char *code, size_t codeLength /*= 0*/, int lineOffset /*= 0*/) { return m->AddScriptSection(name, code, codeLength, lineOffset); }
AS_API int                asModule_Build(asIScriptModule *m) { return m->Build(); }
AS_API int                asModule_GetFunctionCount(asIScriptModule *m) { return m->GetFunctionCount(); }
AS_API int                asModule_GetFunctionIdByIndex(asIScriptModule *m, int index) { return m->GetFunctionIdByIndex(index); }
AS_API int                asModule_GetFunctionIdByName(asIScriptModule *m, const char *name) { return m->GetFunctionIdByName(name); }
AS_API int                asModule_GetFunctionIdByDecl(asIScriptModule *m, const char *decl) { return m->GetFunctionIdByDecl(decl); }
AS_API asIScriptFunction *asModule_GetFunctionDescriptorByIndex(asIScriptModule *m, int index) { return m->GetFunctionDescriptorByIndex(index); }
AS_API asIScriptFunction *asModule_GetFunctionDescriptorById(asIScriptModule *m, int funcId) { return m->GetFunctionDescriptorById(funcId); }
AS_API int                asModule_ResetGlobalVars(asIScriptModule *m) { return m->ResetGlobalVars(); }
AS_API int                asModule_GetGlobalVarCount(asIScriptModule *m) { return m->GetGlobalVarCount(); }
AS_API int                asModule_GetGlobalVarIndexByName(asIScriptModule *m, const char *name) { return m->GetGlobalVarIndexByName(name); }
AS_API int                asModule_GetGlobalVarIndexByDecl(asIScriptModule *m, const char *decl) { return m->GetGlobalVarIndexByDecl(decl); }
AS_API const char        *asModule_GetGlobalVarDeclaration(asIScriptModule *m, int index, int *length /* = 0 */) { return m->GetGlobalVarDeclaration(index, length); }
AS_API const char        *asModule_GetGlobalVarName(asIScriptModule *m, int index, int *length /* = 0 */) { return m->GetGlobalVarName(index, length); }
AS_API int                asModule_GetGlobalVarTypeId(asIScriptModule *m, int index) { return m->GetGlobalVarTypeId(index); }
AS_API void              *asModule_GetAddressOfGlobalVar(asIScriptModule *m, int index) { return m->GetAddressOfGlobalVar(index); }
AS_API int                asModule_GetObjectTypeCount(asIScriptModule *m) { return m->GetObjectTypeCount(); }
AS_API asIObjectType     *asModule_GetObjectTypeByIndex(asIScriptModule *m, asUINT index) { return m->GetObjectTypeByIndex(index); }
AS_API int                asModule_GetTypeIdByDecl(asIScriptModule *m, const char *decl) { return m->GetTypeIdByDecl(decl); }
AS_API int                asModule_GetImportedFunctionCount(asIScriptModule *m) { return m->GetImportedFunctionCount(); }
AS_API int                asModule_GetImportedFunctionIndexByDecl(asIScriptModule *m, const char *decl) { return m->GetImportedFunctionIndexByDecl(decl); }
AS_API const char        *asModule_GetImportedFunctionDeclaration(asIScriptModule *m, int importIndex, int *length /* = 0 */) { return m->GetImportedFunctionDeclaration(importIndex, length); }
AS_API const char        *asModule_GetImportedFunctionSourceModule(asIScriptModule *m, int importIndex, int *length /* = 0 */) { return m->GetImportedFunctionSourceModule(importIndex, length); }
AS_API int                asModule_BindImportedFunction(asIScriptModule *m, int importIndex, int funcId) { return m->BindImportedFunction(importIndex, funcId); }
AS_API int                asModule_UnbindImportedFunction(asIScriptModule *m, int importIndex) { return m->UnbindImportedFunction(importIndex); }
AS_API int                asModule_BindAllImportedFunctions(asIScriptModule *m) { return m->BindAllImportedFunctions(); }
AS_API int                asModule_UnbindAllImportedFunctions(asIScriptModule *m) { return m->UnbindAllImportedFunctions(); }
AS_API int                asModule_SaveByteCode(asIScriptModule *m, asIBinaryStream *out) { return m->SaveByteCode(out); }
AS_API int                asModule_LoadByteCode(asIScriptModule *m, asIBinaryStream *in) { return m->LoadByteCode(in); }

AS_API int              asContext_AddRef(asIScriptContext *c)                                                             { return c->AddRef(); }
AS_API int              asContext_Release(asIScriptContext *c)                                                            { return c->Release(); }
AS_API asIScriptEngine *asContext_GetEngine(asIScriptContext *c)                                                          { return c->GetEngine(); }
AS_API int              asContext_GetState(asIScriptContext *c)                                                           { return c->GetState(); }
AS_API int              asContext_Prepare(asIScriptContext *c, int funcID)                                                { return c->Prepare(funcID); }
AS_API int              asContext_SetArgByte(asIScriptContext *c, asUINT arg, asBYTE value)                               { return c->SetArgByte(arg, value); } 
AS_API int              asContext_SetArgWord(asIScriptContext *c, asUINT arg, asWORD value)                               { return c->SetArgWord(arg, value); } 
AS_API int              asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value)                             { return c->SetArgDWord(arg, value); } 
AS_API int              asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value)                             { return c->SetArgQWord(arg, value); }
AS_API int              asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value)                               { return c->SetArgFloat(arg, value); }
AS_API int              asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value)                             { return c->SetArgDouble(arg, value); }
AS_API int              asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr)                              { return c->SetArgAddress(arg, addr); }
AS_API int              asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj)                                { return c->SetArgObject(arg, obj); }
AS_API void *           asContext_GetArgPointer(asIScriptContext *c, asUINT arg)                                          { return c->GetArgPointer(arg); }
AS_API int              asContext_SetObject(asIScriptContext *c, void *obj)                                               { return c->SetObject(obj); }
AS_API asBYTE           asContext_GetReturnByte(asIScriptContext *c)                                                      { return c->GetReturnByte(); }
AS_API asWORD           asContext_GetReturnWord(asIScriptContext *c)                                                      { return c->GetReturnWord(); }
AS_API asDWORD          asContext_GetReturnDWord(asIScriptContext *c)                                                     { return c->GetReturnDWord(); }
AS_API asQWORD          asContext_GetReturnQWord(asIScriptContext *c)                                                     { return c->GetReturnQWord(); }
AS_API float            asContext_GetReturnFloat(asIScriptContext *c)                                                     { return c->GetReturnFloat(); }
AS_API double           asContext_GetReturnDouble(asIScriptContext *c)                                                    { return c->GetReturnDouble(); }
AS_API void *           asContext_GetReturnAddress(asIScriptContext *c)                                                   { return c->GetReturnAddress(); }
AS_API void *           asContext_GetReturnObject(asIScriptContext *c)                                                    { return c->GetReturnObject(); }
AS_API void *           asContext_GetAddressOfReturnValue(asIScriptContext *c)                                            { return c->GetAddressOfReturnValue(); }
AS_API int              asContext_Execute(asIScriptContext *c)                                                            { return c->Execute(); }
AS_API int              asContext_Abort(asIScriptContext *c)                                                              { return c->Abort(); }
AS_API int              asContext_Suspend(asIScriptContext *c)                                                            { return c->Suspend(); }
AS_API int              asContext_GetCurrentLineNumber(asIScriptContext *c, int *column)                                  { return c->GetCurrentLineNumber(column); }
AS_API int              asContext_GetCurrentFunction(asIScriptContext *c)                                                 { return c->GetCurrentFunction(); }
AS_API int              asContext_SetException(asIScriptContext *c, const char *string)                                   { return c->SetException(string); }
AS_API int              asContext_GetExceptionLineNumber(asIScriptContext *c, int *column)                                { return c->GetExceptionLineNumber(column); }
AS_API int              asContext_GetExceptionFunction(asIScriptContext *c)                                               { return c->GetExceptionFunction(); }
AS_API const char *     asContext_GetExceptionString(asIScriptContext *c, int *length)                                    { return c->GetExceptionString(length); }
AS_API int              asContext_SetLineCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv)      { return c->SetLineCallback(asFUNCTION(callback), obj, callConv); }
AS_API void             asContext_ClearLineCallback(asIScriptContext *c)                                                  { c->ClearLineCallback(); }
AS_API int              asContext_SetExceptionCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv) { return c->SetExceptionCallback(asFUNCTION(callback), obj, callConv); }
AS_API void             asContext_ClearExceptionCallback(asIScriptContext *c)                                             { c->ClearExceptionCallback(); }
AS_API int              asContext_GetCallstackSize(asIScriptContext *c)                                                   { return c->GetCallstackSize(); }
AS_API int              asContext_GetCallstackFunction(asIScriptContext *c, int index)                                    { return c->GetCallstackFunction(index); }
AS_API int              asContext_GetCallstackLineNumber(asIScriptContext *c, int index, int *column)                     { return c->GetCallstackLineNumber(index, column); }
AS_API int              asContext_GetVarCount(asIScriptContext *c, int stackLevel)                                        { return c->GetVarCount(stackLevel); }
AS_API const char *     asContext_GetVarName(asIScriptContext *c, int varIndex, int *length, int stackLevel)              { return c->GetVarName(varIndex, length, stackLevel); }
AS_API const char *     asContext_GetVarDeclaration(asIScriptContext *c, int varIndex, int *length, int stackLevel)       { return c->GetVarDeclaration(varIndex, length, stackLevel); }
AS_API int              asContext_GetVarTypeId(asIScriptContext *c, int varIndex, int stackLevel)                         { return c->GetVarTypeId(varIndex, stackLevel); }
AS_API void *           asContext_GetAddressOfVar(asIScriptContext *c, int varIndex, int stackLevel)                      { return c->GetAddressOfVar(varIndex, stackLevel); }
AS_API int              asContext_GetThisTypeId(asIScriptContext *c, int stackLevel)                                      { return c->GetThisTypeId(stackLevel); }
AS_API void *           asContext_GetThisPointer(asIScriptContext *c, int stackLevel)                                     { return c->GetThisPointer(stackLevel); }
AS_API void *           asContext_SetUserData(asIScriptContext *c, void *data)                                            { return c->SetUserData(data); }
AS_API void *           asContext_GetUserData(asIScriptContext *c)                                                        { return c->GetUserData(); }

AS_API asIScriptEngine *asGeneric_GetEngine(asIScriptGeneric *g)                    { return g->GetEngine(); }
AS_API int              asGeneric_GetFunctionId(asIScriptGeneric *g)                { return g->GetFunctionId(); }
AS_API void *           asGeneric_GetObject(asIScriptGeneric *g)                    { return g->GetObject(); }
AS_API int              asGeneric_GetObjectTypeId(asIScriptGeneric *g)              { return g->GetObjectTypeId(); }
AS_API int              asGeneric_GetArgCount(asIScriptGeneric *g)                  { return g->GetArgCount(); }
AS_API asBYTE           asGeneric_GetArgByte(asIScriptGeneric *g, asUINT arg)       { return g->GetArgByte(arg); }
AS_API asWORD           asGeneric_GetArgWord(asIScriptGeneric *g, asUINT arg)       { return g->GetArgWord(arg); }
AS_API asDWORD          asGeneric_GetArgDWord(asIScriptGeneric *g, asUINT arg)      { return g->GetArgDWord(arg); }
AS_API asQWORD          asGeneric_GetArgQWord(asIScriptGeneric *g, asUINT arg)      { return g->GetArgQWord(arg); }
AS_API float            asGeneric_GetArgFloat(asIScriptGeneric *g, asUINT arg)      { return g->GetArgFloat(arg); }
AS_API double           asGeneric_GetArgDouble(asIScriptGeneric *g, asUINT arg)     { return g->GetArgDouble(arg); }
AS_API void *           asGeneric_GetArgAddress(asIScriptGeneric *g, asUINT arg)    { return g->GetArgAddress(arg); }
AS_API void *           asGeneric_GetArgObject(asIScriptGeneric *g, asUINT arg)     { return g->GetArgObject(arg); }
AS_API void *           asGeneric_GetAddressOfArg(asIScriptGeneric *g, asUINT arg)  { return g->GetAddressOfArg(arg); }
AS_API int              asGeneric_GetArgTypeId(asIScriptGeneric *g, asUINT arg)     { return g->GetArgTypeId(arg); }
AS_API int              asGeneric_SetReturnByte(asIScriptGeneric *g, asBYTE val)    { return g->SetReturnByte(val); }
AS_API int              asGeneric_SetReturnWord(asIScriptGeneric *g, asWORD val)    { return g->SetReturnWord(val); }
AS_API int              asGeneric_SetReturnDWord(asIScriptGeneric *g, asDWORD val)  { return g->SetReturnDWord(val); }
AS_API int              asGeneric_SetReturnQWord(asIScriptGeneric *g, asQWORD val)  { return g->SetReturnQWord(val); }
AS_API int              asGeneric_SetReturnFloat(asIScriptGeneric *g, float val)    { return g->SetReturnFloat(val); }
AS_API int              asGeneric_SetReturnDouble(asIScriptGeneric *g, double val)  { return g->SetReturnDouble(val); }
AS_API int              asGeneric_SetReturnAddress(asIScriptGeneric *g, void *addr) { return g->SetReturnAddress(addr); }
AS_API int              asGeneric_SetReturnObject(asIScriptGeneric *g, void *obj)   { return g->SetReturnObject(obj); }
AS_API void *           asGeneric_GetReturnPointer(asIScriptGeneric *g)             { return g->GetReturnPointer(); }
AS_API int              asGeneric_GetReturnTypeId(asIScriptGeneric *g)              { return g->GetReturnTypeId(); }

AS_API asIScriptEngine *asStruct_GetEngine(asIScriptStruct *s)                        { return s->GetEngine(); }
AS_API int              asStruct_AddRef(asIScriptStruct *s)                           { return s->AddRef(); }
AS_API int              asStruct_Release(asIScriptStruct *s)                          { return s->Release(); }
AS_API int              asStruct_GetStructTypeId(asIScriptStruct *s)                  { return s->GetStructTypeId(); }
AS_API asIObjectType *  asStruct_GetObjectType(asIScriptStruct *s)                    { return s->GetObjectType(); }
AS_API int              asStruct_GetPropertyCount(asIScriptStruct *s)                 { return s->GetPropertyCount(); }
AS_API int              asStruct_GetPropertyTypeId(asIScriptStruct *s, asUINT prop)   { return s->GetPropertyTypeId(prop); }
AS_API const char *     asStruct_GetPropertyName(asIScriptStruct *s, asUINT prop)     { return s->GetPropertyName(prop); }
AS_API void *           asStruct_GetPropertyPointer(asIScriptStruct *s, asUINT prop)  { return s->GetPropertyPointer(prop); }
AS_API int              asStruct_CopyFrom(asIScriptStruct *s, asIScriptStruct *other) { return s->CopyFrom(other); }

AS_API asIScriptEngine *asArray_GetEngine(asIScriptArray *a)                       { return a->GetEngine(); }                
AS_API int              asArray_AddRef(asIScriptArray *a)                          { return a->AddRef(); }
AS_API int              asArray_Release(asIScriptArray *a)                         { return a->Release(); }
AS_API int              asArray_GetArrayTypeId(asIScriptArray *a)                  { return a->GetArrayTypeId(); }
AS_API int              asArray_GetElementTypeId(asIScriptArray *a)                { return a->GetElementTypeId(); }
AS_API asUINT           asArray_GetElementCount(asIScriptArray *a)                 { return a->GetElementCount(); }
AS_API void *           asArray_GetElementPointer(asIScriptArray *a, asUINT index) { return a->GetElementPointer(index); }
AS_API void             asArray_Resize(asIScriptArray *a, asUINT size)             { a->Resize(size); }
AS_API int              asArray_CopyFrom(asIScriptArray *a, asIScriptArray *other) { return a->CopyFrom(other); }

AS_API asIScriptEngine         *asObjectType_GetEngine(const asIObjectType *o)                             { return o->GetEngine(); }
AS_API const char              *asObjectType_GetName(const asIObjectType *o)                               { return o->GetName(); }
AS_API asIObjectType           *asObjectType_GetSubType(const asIObjectType *o)                            { return o->GetSubType(); }
AS_API int                      asObjectType_GetInterfaceCount(const asIObjectType *o)                     { return o->GetInterfaceCount(); }
AS_API asIObjectType           *asObjectType_GetInterface(const asIObjectType *o, asUINT index)            { return o->GetInterface(index); }
AS_API bool                     asObjectType_IsInterface(const asIObjectType *o)                           { return o->IsInterface(); }
AS_API int                      asObjectType_GetMethodCount(const asIObjectType *o)                        { return o->GetMethodCount(); }
AS_API int                      asObjectType_GetMethodIdByIndex(const asIObjectType *o, int index)         { return o->GetMethodIdByIndex(index); }
AS_API int                      asObjectType_GetMethodIdByName(const asIObjectType *o, const char *name)   { return o->GetMethodIdByName(name); }
AS_API int                      asObjectType_GetMethodIdByDecl(const asIObjectType *o, const char *decl)   { return o->GetMethodIdByDecl(decl); }
AS_API asIScriptFunction       *asObjectType_GetMethodDescriptorByIndex(const asIObjectType *o, int index) { return o->GetMethodDescriptorByIndex(index); }

AS_API asIScriptEngine     *asScriptFunction_GetEngine(const asIScriptFunction *f)                 { return f->GetEngine(); }
AS_API const char          *asScriptFunction_GetModuleName(const asIScriptFunction *f)             { return f->GetModuleName(); }
AS_API asIObjectType       *asScriptFunction_GetObjectType(const asIScriptFunction *f)             { return f->GetObjectType(); }
AS_API const char          *asScriptFunction_GetObjectName(const asIScriptFunction *f)             { return f->GetObjectName(); }
AS_API const char          *asScriptFunction_GetName(const asIScriptFunction *f)                   { return f->GetName(); }
AS_API bool                 asScriptFunction_IsClassMethod(const asIScriptFunction *f)             { return f->IsClassMethod(); }
AS_API bool                 asScriptFunction_IsInterfaceMethod(const asIScriptFunction *f)         { return f->IsInterfaceMethod(); }
AS_API int                  asScriptFunction_GetParamCount(const asIScriptFunction *f)             { return f->GetParamCount(); }
AS_API int                  asScriptFunction_GetParamTypeId(const asIScriptFunction *f, int index) { return f->GetParamTypeId(index); }
AS_API int                  asScriptFunction_GetReturnTypeId(const asIScriptFunction *f)           { return f->GetReturnTypeId(); }

}

END_AS_NAMESPACE



