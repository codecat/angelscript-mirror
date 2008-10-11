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
// as_objecttype.h
//
// A class for storing object type information
//



#ifndef AS_OBJECTTYPE_H
#define AS_OBJECTTYPE_H

#include "as_string.h"
#include "as_property.h"
#include "as_array.h"
#include "as_scriptfunction.h"

BEGIN_AS_NAMESPACE

// Additional flag to the class object type
const asDWORD asOBJ_SCRIPT_STRUCT    = 0x10000;
const asDWORD asOBJ_TEMPLATE         = 0x20000;
const asDWORD asOBJ_IMPLICIT_HANDLE  = 0x40000;
const asDWORD asOBJ_NAMED_TYPE       = 0x80000000;
const asDWORD asOBJ_NAMED_PSEUDO     = 0x40000000 | asOBJ_NAMED_TYPE;
const asDWORD asOBJ_NAMED_ENUM       = 0x10000000 | asOBJ_NAMED_TYPE;

// asOBJ_GC is used to indicate that the type can potentially 
// form circular references, thus is garbage collected.

// The fact that an object is garbage collected doesn't imply that an object that 
// can references it also must be garbage collected, only if the garbage collected 
// object can reference it as well.

// For registered types however, we set the flag asOBJ_GC if the GC 
// behaviours are registered. For script types that contain any such type we 
// automatically make garbage collected as well, because we cannot know what type
// of references that object can contain, and must assume the worst.

struct asSTypeBehaviour
{
	asSTypeBehaviour() {construct = 0; destruct = 0; copy = 0; addref = 0; release = 0; gcGetRefCount = 0; gcSetFlag = 0; gcGetFlag = 0; gcEnumReferences = 0; gcReleaseAllReferences = 0;}

	int construct;
	int destruct;
	int copy;
	int addref;
	int release;
	
	// GC behaviours
	int gcGetRefCount;
	int gcSetFlag;
	int gcGetFlag;
	int gcEnumReferences;
	int gcReleaseAllReferences;
	
	asCArray<int> constructors;
	asCArray<int> operators;
};

struct asSEnumValue
{
	asCString name;
	int       value;
};

class asCScriptEngine;

// TODO: Need to minimize used memory here, because not all types use all properties of the class
class asCObjectType : public asIObjectType
{
public:
	asCObjectType(); 
	asCObjectType(asCScriptEngine *engine);
	~asCObjectType();

	asIScriptEngine *GetEngine() const;
	const char *GetName(int *length = 0) const;
	asIObjectType *GetSubType() const;
	virtual int GetInterfaceCount() const;
	asIObjectType *GetInterface(asUINT index) const;
	bool IsInterface() const;
	bool Implements(const asCObjectType *objType);

	// Methods
	int                      GetMethodCount() const;
	int                      GetMethodIdByIndex(int index) const;
	int                      GetMethodIdByName(const char *name) const;
	int                      GetMethodIdByDecl(const char *decl) const;
	asIScriptFunction       *GetMethodDescriptorByIndex(int index) const;

	// Properties
	int         GetPropertyCount();
	int         GetPropertyTypeId(asUINT prop);
	const char *GetPropertyName(asUINT prop, int *length = 0);

	asCString   name;
	eTokenType  tokenType;
	int         arrayType;
	int         size;
	asCArray<asCProperty*> properties;
	asCArray<int> methods;
	asCArray<asCObjectType*> interfaces;
	asCArray<asSEnumValue*> enumValues;

	asDWORD flags;

	asSTypeBehaviour beh;

	asCObjectType *subType;

	asCScriptEngine *engine;
	int refCount;
};

END_AS_NAMESPACE

#endif
