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
// angelscript_c.h
//
// The script engine interface for the C language.
//
// The idea is that the library should be compiled with a C++ compiler with the AS_C_INTERFACE 
// preprocessor word defined. The C application will then be able to link with the library and
// use this header file to interact with it.
//
// Note: This header file is not yet complete. I'd appreciate any help with completing it.
//

#ifndef ANGELSCRIPT_C_H
#define ANGELSCRIPT_C_H

typedef unsigned char  asBYTE;
typedef unsigned short asWORD;
typedef unsigned int   asUINT;
typedef size_t         asPWORD;
#ifdef __LP64__
    typedef unsigned int  asDWORD;
    typedef unsigned long asQWORD;
    typedef long asINT64;
#else
    typedef unsigned long asDWORD;
  #if defined(__GNUC__) || defined(__MWERKS__)
    typedef unsigned long long asQWORD;
    typedef long long asINT64;
  #else
    typedef unsigned __int64 asQWORD;
    typedef __int64 asINT64;
  #endif
#endif

typedef struct asIScriptEngine asIScriptEngine;
typedef struct asIScriptContext asIScriptContext;
typedef struct asIScriptGeneric asIScriptGeneric;
typedef struct asIScriptStruct asIScriptStruct;
typedef struct asIScriptArray asIScriptArray;
typedef struct asIObjectType asIObjectType;
typedef struct asIScriptFunction asIScriptFunction;
typedef struct asIBinaryStream asIBinaryStream;

typedef void (*asBINARYREADFUNC_t)(void *ptr, asUINT size, void *param);
typedef void (*asBINARYWRITEFUNC_t)(const void *ptr, asUINT size, void *param);


AS_API asIScriptEngine * asCreateScriptEngine(asDWORD version);
AS_API const char * asGetLibraryVersion();
AS_API const char * asGetLibraryOptions();
AS_API asIScriptContext * asGetActiveContext();
AS_API int asThreadCleanup();
AS_API int asSetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc);
AS_API int asResetGlobalMemoryFunctions();

AS_API int               asEngine_AddRef(asIScriptEngine *e);
AS_API int               asEngine_Release(asIScriptEngine *e);
AS_API int               asEngine_SetEngineProperty(asIScriptEngine *e, asEEngineProp property, asPWORD value);
AS_API asPWORD           asEngine_GetEngineProperty(asIScriptEngine *e, asEEngineProp property);
AS_API int               asEngine_SetMessageCallback(asIScriptEngine *e, asFUNCTION_t callback, void *obj, asDWORD callConv);
AS_API int               asEngine_ClearMessageCallback(asIScriptEngine *e);
AS_API int               asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags);
AS_API int               asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset);
AS_API int               asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
AS_API int               asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *datatype, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
AS_API int               asEngine_RegisterGlobalProperty(asIScriptEngine *e, const char *declaration, void *pointer);
AS_API int               asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
AS_API int               asEngine_RegisterGlobalBehaviour(asIScriptEngine *e, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv);
AS_API int               asEngine_RegisterInterface(asIScriptEngine *e, const char *name);
AS_API int               asEngine_RegisterInterfaceMethod(asIScriptEngine *e, const char *intf, const char *declaration);
AS_API int               asEngine_RegisterEnum(asIScriptEngine *e, const char *type);
AS_API int               asEngine_RegisterEnumValue(asIScriptEngine *e, const char *type, const char *name, int value);
AS_API int               asEngine_RegisterTypedef(asIScriptEngine *e, const char *type, const char *decl);
AS_API int               asEngine_RegisterStringFactory(asIScriptEngine *e, const char *datatype, asFUNCTION_t factoryFunc, asDWORD callConv);
AS_API int               asEngine_BeginConfigGroup(asIScriptEngine *e, const char *groupName);
AS_API int               asEngine_EndConfigGroup(asIScriptEngine *e);
AS_API int               asEngine_RemoveConfigGroup(asIScriptEngine *e, const char *groupName);
AS_API int               asEngine_SetConfigGroupModuleAccess(asIScriptEngine *e, const char *groupName, const char *module, bool hasAccess);
AS_API int               asEngine_AddScriptSection(asIScriptEngine *e, const char *module, const char *name, const char *code, int codeLength, int lineOffset /* = 0 */);
AS_API int               asEngine_Build(asIScriptEngine *e, const char *module);
AS_API int               asEngine_Discard(asIScriptEngine *e, const char *module);
AS_API int               asEngine_GetFunctionCount(asIScriptEngine *e, const char *module);
AS_API int               asEngine_GetFunctionIDByIndex(asIScriptEngine *e, const char *module, int index);
AS_API int               asEngine_GetFunctionIDByName(asIScriptEngine *e, const char *module, const char *name);
AS_API int               asEngine_GetFunctionIDByDecl(asIScriptEngine *e, const char *module, const char *decl);
#ifdef AS_DEPRECATED
AS_API const char *      asEngine_GetFunctionDeclaration(asIScriptEngine *e, int funcID, int *length /* = 0 */);
AS_API const char *      asEngine_GetFunctionName(asIScriptEngine *e, int funcID, int *length /* = 0 */);
AS_API const char *      asEngine_GetFunctionModule(asIScriptEngine *e, int funcID, int *length /* = 0 */);
AS_API const char *      asEngine_GetFunctionSection(asIScriptEngine *e, int funcID, int *length /* = 0 */);
#endif
AS_API asIScriptFunction *asEngine_GetFunctionDescriptorByIndex(asIScriptEngine *e, const char *module, int index);
AS_API asIScriptFunction *asEngine_GetFunctionDescriptorById(asIScriptEngine *e, int funcId);
#ifdef AS_DEPRECATED
AS_API int               asEngine_GetMethodCount(asIScriptEngine *e, int typeId);
AS_API int               asEngine_GetMethodIDByIndex(asIScriptEngine *e, int typeId, int index);
AS_API int               asEngine_GetMethodIDByName(asIScriptEngine *e, int typeId, const char *name);
AS_API int               asEngine_GetMethodIDByDecl(asIScriptEngine *e, int typeId, const char *decl);
AS_API asIScriptFunction *asEngine_GetMethodDescriptorByIndex(asIScriptEngine *e, int typeId, int index);
#endif
AS_API int               asEngine_GetGlobalVarCount(asIScriptEngine *e, const char *module);
AS_API int               asEngine_GetGlobalVarIDByIndex(asIScriptEngine *e, const char *module, int index);
AS_API int               asEngine_GetGlobalVarIDByName(asIScriptEngine *e, const char *module, const char *name);
AS_API int               asEngine_GetGlobalVarIDByDecl(asIScriptEngine *e, const char *module, const char *decl);
AS_API const char *      asEngine_GetGlobalVarDeclaration(asIScriptEngine *e, int gvarID, int *length /* = 0 */);
AS_API const char *      asEngine_GetGlobalVarName(asIScriptEngine *e, int gvarID, int *length /* = 0 */);
AS_API void *            asEngine_GetGlobalVarPointer(asIScriptEngine *e, int gvarID);
AS_API int               asEngine_GetImportedFunctionCount(asIScriptEngine *e, const char *module);
AS_API int               asEngine_GetImportedFunctionIndexByDecl(asIScriptEngine *e, const char *module, const char *decl);
AS_API const char *      asEngine_GetImportedFunctionDeclaration(asIScriptEngine *e, const char *module, int importIndex, int *length /* = 0 */);
AS_API const char *      asEngine_GetImportedFunctionSourceModule(asIScriptEngine *e, const char *module, int importIndex, int *length /* = 0 */);
AS_API int               asEngine_BindImportedFunction(asIScriptEngine *e, const char *module, int importIndex, int funcID);
AS_API int               asEngine_UnbindImportedFunction(asIScriptEngine *e, const char *module, int importIndex);
AS_API int               asEngine_BindAllImportedFunctions(asIScriptEngine *e, const char *module);
AS_API int               asEngine_UnbindAllImportedFunctions(asIScriptEngine *e, const char *module);
AS_API int               asEngine_GetTypeIdByDecl(asIScriptEngine *e, const char *module, const char *decl);
AS_API const char *      asEngine_GetTypeDeclaration(asIScriptEngine *e, int typeId, int *length /* = 0 */);
AS_API int               asEngine_GetSizeOfPrimitiveType(asIScriptEngine *e, int typeId);
AS_API asIObjectType *   asEngine_GetObjectTypeById(asIScriptEngine *e, int typeId);
AS_API asIObjectType *   asEngine_GetObjectTypeByIndex(asIScriptEngine *e, asUINT index);
AS_API int               asEngine_GetObjectTypeCount(asIScriptEngine *e);
#ifdef AS_DEPRECATED
AS_API int               asEngine_SetDefaultContextStackSize(asIScriptEngine *e, asUINT initial, asUINT maximum);
#endif
AS_API asIScriptContext *asEngine_CreateContext(asIScriptEngine *e);
AS_API void *            asEngine_CreateScriptObject(asIScriptEngine *e, int typeId);
AS_API void *            asEngine_CreateScriptObjectCopy(asIScriptEngine *e, void *obj, int typeId);
AS_API void              asEngine_CopyScriptObject(asIScriptEngine *e, void *dstObj, void *srcObj, int typeId);
AS_API void              asEngine_ReleaseScriptObject(asIScriptEngine *e, void *obj, int typeId);
AS_API void              asEngine_AddRefScriptObject(asIScriptEngine *e, void *obj, int typeId);
AS_API bool              asEngine_IsHandleCompatibleWithObject(asIScriptEngine *e, void *obj, int objTypeId, int handleTypeId);
AS_API int               asEngine_CompareScriptObjects(asIScriptEngine *e, bool &result, int behaviour, void *leftObj, void *rightObj, int typeId);
AS_API int               asEngine_ExecuteString(asIScriptEngine *e, const char *module, const char *script, asIScriptContext **ctx, asDWORD flags);
AS_API int               asEngine_GarbageCollect(asIScriptEngine *e, bool doFullCycle /* = true */);
AS_API int               asEngine_GetObjectsInGarbageCollectorCount(asIScriptEngine *e);
AS_API void              asEngine_NotifyGarbageCollectorOfNewObject(asIScriptEngine *e, void *obj, int typeId);
AS_API void              asEngine_GCEnumCallback(asIScriptEngine *e, void *obj);
AS_API int               asEngine_SaveByteCode(asIScriptEngine *e, const char *module, asBINARYWRITEFUNC_t outFunc, void *outParam);
AS_API int               asEngine_LoadByteCode(asIScriptEngine *e, const char *module, asBINARYREADFUNC_t inFunc, void *inParam);

AS_API int              asContext_AddRef(asIScriptContext *c);
AS_API int              asContext_Release(asIScriptContext *c);
AS_API asIScriptEngine *asContext_GetEngine(asIScriptContext *c);
AS_API int              asContext_GetState(asIScriptContext *c);
AS_API int              asContext_Prepare(asIScriptContext *c, int funcID);
AS_API int              asContext_SetArgByte(asIScriptContext *c, asUINT arg, asBYTE value);
AS_API int              asContext_SetArgWord(asIScriptContext *c, asUINT arg, asWORD value);
AS_API int              asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value);
AS_API int              asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value);
AS_API int              asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value);
AS_API int              asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value);
AS_API int              asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr);
AS_API int              asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj);
AS_API void *           asContext_GetArgPointer(asIScriptContext *c, asUINT arg);
AS_API int              asContext_SetObject(asIScriptContext *c, void *obj);
AS_API asBYTE           asContext_GetReturnByte(asIScriptContext *c);
AS_API asWORD           asContext_GetReturnWord(asIScriptContext *c);
AS_API asDWORD          asContext_GetReturnDWord(asIScriptContext *c);
AS_API asQWORD          asContext_GetReturnQWord(asIScriptContext *c);
AS_API float            asContext_GetReturnFloat(asIScriptContext *c);
AS_API double           asContext_GetReturnDouble(asIScriptContext *c);
AS_API void *           asContext_GetReturnAddress(asIScriptContext *c);
AS_API void *           asContext_GetReturnObject(asIScriptContext *c);
AS_API void *           asContext_GetReturnPointer(asIScriptContext *c);
AS_API int              asContext_Execute(asIScriptContext *c);
AS_API int              asContext_Abort(asIScriptContext *c);
AS_API int              asContext_Suspend(asIScriptContext *c);
AS_API int              asContext_GetCurrentLineNumber(asIScriptContext *c, int *column /* = 0 */);
AS_API int              asContext_GetCurrentFunction(asIScriptContext *c);
AS_API int              asContext_SetException(asIScriptContext *c, const char *string);
AS_API int              asContext_GetExceptionLineNumber(asIScriptContext *c, int *column /* = 0 */);
AS_API int              asContext_GetExceptionFunction(asIScriptContext *c);
AS_API const char *     asContext_GetExceptionString(asIScriptContext *c, int *length /* = 0 */);
AS_API int              asContext_SetLineCallback(asIScriptContext *c, asSFuncPtr callback, void *obj, int callConv);
AS_API void             asContext_ClearLineCallback(asIScriptContext *c);
AS_API int              asContext_SetExceptionCallback(asIScriptContext *c, asSFuncPtr callback, void *obj, int callConv);
AS_API void             asContext_ClearExceptionCallback(asIScriptContext *c);
AS_API int              asContext_GetCallstackSize(asIScriptContext *c);
AS_API int              asContext_GetCallstackFunction(asIScriptContext *c, int index);
AS_API int              asContext_GetCallstackLineNumber(asIScriptContext *c, int index, int *column /* = 0 */);
AS_API int              asContext_GetVarCount(asIScriptContext *c, int stackLevel /* = 0 */);
AS_API const char *     asContext_GetVarName(asIScriptContext *c, int varIndex, int *length /* = 0 */, int stackLevel /* = 0 */);
AS_API const char *     asContext_GetVarDeclaration(asIScriptContext *c, int varIndex, int *length /* = 0 */, int stackLevel /* = 0 */);
AS_API int              asContext_GetVarTypeId(asIScriptContext *c, int varIndex, int stackLevel /* = -1 */);
AS_API void *           asContext_GetVarPointer(asIScriptContext *c, int varIndex, int stackLevel /* = 0 */);
AS_API int              asContext_GetThisTypeId(asIScriptContext *c, int stackLevel /* = -1 */);
AS_API void *           asContext_GetThisPointer(asIScriptContext *c, int stackLevel /* = -1 */);
AS_API void *           asContext_SetUserData(asIScriptContext *c, void *data);
AS_API void *           asContext_GetUserData(asIScriptContext *c);

AS_API asIScriptEngine *asGeneric_GetEngine(asIScriptGeneric *g);
AS_API int              asGeneric_GetFunctionId(asIScriptGeneric *g);
AS_API void *           asGeneric_GetObject(asIScriptGeneric *g);
AS_API int              asGeneric_GetObjectTypeId(asIScriptGeneric *g);
AS_API int              asGeneric_GetArgCount(asIScriptGeneric *g);
AS_API asBYTE           asGeneric_GetArgByte(asIScriptGeneric *g, asUINT arg);
AS_API asWORD           asGeneric_GetArgWord(asIScriptGeneric *g, asUINT arg);
AS_API asDWORD          asGeneric_GetArgDWord(asIScriptGeneric *g, asUINT arg);
AS_API asQWORD          asGeneric_GetArgQWord(asIScriptGeneric *g, asUINT arg);
AS_API float            asGeneric_GetArgFloat(asIScriptGeneric *g, asUINT arg);
AS_API double           asGeneric_GetArgDouble(asIScriptGeneric *g, asUINT arg);
AS_API void *           asGeneric_GetArgAddress(asIScriptGeneric *g, asUINT arg);
AS_API void *           asGeneric_GetArgObject(asIScriptGeneric *g, asUINT arg);
AS_API void *           asGeneric_GetArgPointer(asIScriptGeneric *g, asUINT arg);
AS_API int              asGeneric_GetArgTypeId(asIScriptGeneric *g, asUINT arg);
AS_API int              asGeneric_SetReturnByte(asIScriptGeneric *g, asBYTE val);
AS_API int              asGeneric_SetReturnWord(asIScriptGeneric *g, asWORD val);
AS_API int              asGeneric_SetReturnDWord(asIScriptGeneric *g, asDWORD val);
AS_API int              asGeneric_SetReturnQWord(asIScriptGeneric *g, asQWORD val);
AS_API int              asGeneric_SetReturnFloat(asIScriptGeneric *g, float val);
AS_API int              asGeneric_SetReturnDouble(asIScriptGeneric *g, double val);
AS_API int              asGeneric_SetReturnAddress(asIScriptGeneric *g, void *addr);
AS_API int              asGeneric_SetReturnObject(asIScriptGeneric *g, void *obj);
AS_API void *           asGeneric_GetReturnPointer(asIScriptGeneric *g);
AS_API int              asGeneric_GetReturnTypeId(asIScriptGeneric *g);

AS_API int            asStruct_AddRef(asIScriptStruct *s);
AS_API int            asStruct_Release(asIScriptStruct *s);
AS_API int            asStruct_GetStructTypeId(asIScriptStruct *s);
AS_API asIObjectType *asStruct_GetObjectType(asIScriptStruct *s);
AS_API int            asStruct_GetPropertyCount(asIScriptStruct *s);
AS_API int            asStruct_GetPropertyTypeId(asIScriptStruct *s, asUINT prop);
AS_API const char *   asStruct_GetPropertyName(asIScriptStruct *s, asUINT prop);
AS_API void *         asStruct_GetPropertyPointer(asIScriptStruct *s, asUINT prop);
AS_API int            asStruct_CopyFrom(asIScriptStruct *s, asIScriptStruct *other);

AS_API int    asArray_AddRef(asIScriptArray *a);
AS_API int    asArray_Release(asIScriptArray *a);
AS_API int    asArray_GetArrayTypeId(asIScriptArray *a);
AS_API int    asArray_GetElementTypeId(asIScriptArray *a);
AS_API asUINT asArray_GetElementCount(asIScriptArray *a);
AS_API void * asArray_GetElementPointer(asIScriptArray *a, asUINT index);
AS_API void   asArray_Resize(asIScriptArray *a, asUINT size);
AS_API int    asArray_CopyFrom(asIScriptArray *a, asIScriptArray *other);

AS_API asIScriptEngine         *asObjectType_GetEngine(const asIObjectType *o);
AS_API const char              *asObjectType_GetName(const asIObjectType *o);
AS_API asIObjectType           *asObjectType_GetSubType(const asIObjectType *o);
AS_API int                      asObjectType_GetInterfaceCount(const asIObjectType *o);
AS_API asIObjectType           *asObjectType_GetInterface(const asIObjectType *o, asUINT index);
AS_API bool                     asObjectType_IsInterface(const asIObjectType *o);
AS_API int                      asObjectType_GetMethodCount(const asIObjectType *o);
AS_API int                      asObjectType_GetMethodIdByIndex(const asIObjectType *o, int index);
AS_API int                      asObjectType_GetMethodIdByName(const asIObjectType *o, const char *name);
AS_API int                      asObjectType_GetMethodIdByDecl(const asIObjectType *o, const char *decl);
AS_API asIScriptFunction       *asObjectType_GetMethodDescriptorByIndex(const asIObjectType *o, int index);

AS_API asIScriptEngine     *asScriptFunction_GetEngine(const asIScriptFunction *f);
AS_API const char          *asScriptFunction_GetModuleName(const asIScriptFunction *f);
AS_API asIObjectType       *asScriptFunction_GetObjectType(const asIScriptFunction *f);
AS_API const char          *asScriptFunction_GetObjectName(const asIScriptFunction *f);
AS_API const char          *asScriptFunction_GetName(const asIScriptFunction *f);
AS_API bool                 asScriptFunction_IsClassMethod(const asIScriptFunction *f);
AS_API bool                 asScriptFunction_IsInterfaceMethod(const asIScriptFunction *f);
AS_API int                  asScriptFunction_GetParamCount(const asIScriptFunction *f);
AS_API int                  asScriptFunction_GetParamTypeId(const asIScriptFunction *f, int index);
AS_API int                  asScriptFunction_GetReturnTypeId(const asIScriptFunction *f);

#endif