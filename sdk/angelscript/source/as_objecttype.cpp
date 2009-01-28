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
// as_objecttype.cpp
//
// A class for storing object type information
//

#include <stdio.h>

#include "as_config.h"
#include "as_objecttype.h"
#include "as_configgroup.h"
#include "as_scriptengine.h"

BEGIN_AS_NAMESPACE

asCObjectType::asCObjectType()
{
	engine      = 0; 
	refCount.set(0); 
	subType     = 0;
	derivedFrom = 0;
}

asCObjectType::asCObjectType(asCScriptEngine *engine) 
{
	this->engine = engine; 
	refCount.set(0); 
	subType      = 0;
	derivedFrom  = 0;
}

void asCObjectType::AddRef()
{
	refCount.atomicInc();
}

void asCObjectType::Release()
{
	refCount.atomicDec();
}

int asCObjectType::GetRefCount()
{
	return refCount.get();
}

asCObjectType::~asCObjectType()
{
	if( subType )
		subType->Release();

	asUINT n;
	for( n = 0; n < properties.GetLength(); n++ )
		if( properties[n] ) 
		{
			if( flags & asOBJ_SCRIPT_STRUCT )
			{
				// Release the config group for script structures that are being destroyed
				asCConfigGroup *group = engine->FindConfigGroupForObjectType(properties[n]->type.GetObjectType());
				if( group != 0 ) group->Release();
			}

			asDELETE(properties[n],asCObjectProperty);
		}

	properties.SetLength(0);

	methods.SetLength(0);

	for( n = 0; n < enumValues.GetLength(); n++ )
	{
		if( enumValues[n] )
			asDELETE(enumValues[n],asSEnumValue);
	}

	enumValues.SetLength(0);
}

bool asCObjectType::Implements(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	for( asUINT n = 0; n < interfaces.GetLength(); n++ )
		if( interfaces[n] == objType ) return true;

	return false;
}

bool asCObjectType::DerivesFrom(const asCObjectType *objType) const
{
	if( this == objType )
		return true;

	asCObjectType *base = derivedFrom;
	while( base )
	{
		if( base == objType )
			return true;

		base = base->derivedFrom;
	}

	return false;
}

const char *asCObjectType::GetName(int *length) const
{
	if( length ) *length = (int)name.GetLength();
	return name.AddressOf();
}

asIObjectType *asCObjectType::GetSubType() const
{
	return subType;
}

int asCObjectType::GetInterfaceCount() const
{
	return (int)interfaces.GetLength();
}

asIObjectType *asCObjectType::GetInterface(asUINT index) const
{
	assert(index < interfaces.GetLength());

	return interfaces[index];
}

asIScriptEngine *asCObjectType::GetEngine() const
{
	return engine;
}

bool asCObjectType::IsInterface() const
{
	if( (flags & asOBJ_SCRIPT_STRUCT) && size == 0 )
		return true;

	return false;
}

int asCObjectType::GetMethodCount() const
{
	return (int)methods.GetLength();
}

int asCObjectType::GetMethodIdByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() )
		return asINVALID_ARG;

	return methods[index];
}

int asCObjectType::GetMethodIdByName(const char *name) const
{
	int id = -1;
	for( size_t n = 0; n < methods.GetLength(); n++ )
	{
		if( engine->scriptFunctions[methods[n]]->name == name )
		{
			if( id == -1 )
				id = methods[n];
			else
				return asMULTIPLE_FUNCTIONS;
		}
	}

	if( id == -1 ) return asNO_FUNCTION;

	return id;
}

int asCObjectType::GetMethodIdByDecl(const char *decl) const
{
	// Get the module from one of the methods
	if( methods.GetLength() == 0 )
		return asNO_FUNCTION;

	asCModule *mod = engine->scriptFunctions[methods[0]]->module;
	if( mod == 0 )
	{
		if( engine->scriptFunctions[methods[0]]->funcType == asFUNC_INTERFACE )
			return engine->GetMethodIDByDecl(this, decl, 0);

		return asNO_MODULE;
	}

	return engine->GetMethodIDByDecl(this, decl, mod);
}

asIScriptFunction *asCObjectType::GetMethodDescriptorByIndex(int index) const
{
	if( index < 0 || (unsigned)index >= methods.GetLength() ) 
		return 0;

	return engine->scriptFunctions[methods[index]];
}

int asCObjectType::GetPropertyCount()
{
	return (int)properties.GetLength();
}

int asCObjectType::GetPropertyTypeId(asUINT prop)
{
	if( prop >= properties.GetLength() )
		return asINVALID_ARG;

	return engine->GetTypeIdFromDataType(properties[prop]->type);
}

const char *asCObjectType::GetPropertyName(asUINT prop, int *length)
{
	if( prop >= properties.GetLength() )
		return 0;

	if( length ) *length = (int)properties[prop]->name.GetLength();
	return properties[prop]->name.AddressOf();
}

END_AS_NAMESPACE



