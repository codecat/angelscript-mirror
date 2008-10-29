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
// as_scriptfunction.cpp
//
// A container for a compiled script function
//



#include "as_config.h"
#include "as_scriptfunction.h"
#include "as_tokendef.h"
#include "as_scriptengine.h"
#include "as_callfunc.h"
#include "as_bytecode.h"

BEGIN_AS_NAMESPACE

asCScriptFunction::asCScriptFunction(asCScriptEngine *engine, asCModule *mod)
{
	this->engine = engine;
	funcType    = -1;
	module      = mod; 
	objectType  = 0; 
	name        = ""; 
	isReadOnly  = false;
	stackNeeded = 0;
	sysFuncIntf = 0;
	signatureId = 0;
}

asCScriptFunction::~asCScriptFunction()
{
	ReleaseReferences();

	for( asUINT n = 0; n < variables.GetLength(); n++ )
	{
		asDELETE(variables[n],asSScriptVariable);
	}

	if( sysFuncIntf )
	{
		asDELETE(sysFuncIntf,asSSystemFunctionInterface);
	}
}

const char *asCScriptFunction::GetModuleName(int *length) const
{
	if( module )
	{
		if( length ) *length = (int)module->name.GetLength();
		return module->name.AddressOf();
	}

	return 0;
}

asIObjectType *asCScriptFunction::GetObjectType() const
{
	return objectType;
}

const char *asCScriptFunction::GetObjectName(int *length) const 
{
	if( objectType )
		return objectType->GetName(length);

	return 0;
}

const char *asCScriptFunction::GetName(int *length) const
{
	if( length )
		*length = (int)name.GetLength();
	return name.AddressOf();
}

bool asCScriptFunction::IsClassMethod() const
{
	return objectType && objectType->IsInterface() == false;
}

bool asCScriptFunction::IsInterfaceMethod() const
{
	return objectType && objectType->IsInterface();
}

int asCScriptFunction::GetSpaceNeededForArguments()
{
	// We need to check the size for each type
	int s = 0;
	for( asUINT n = 0; n < parameterTypes.GetLength(); n++ )
		s += parameterTypes[n].GetSizeOnStackDWords();

	return s;
}

int asCScriptFunction::GetSpaceNeededForReturnValue()
{
	return returnType.GetSizeOnStackDWords();
}

asCString asCScriptFunction::GetDeclarationStr() const
{
	asCString str;

	str = returnType.Format();
	str += " ";
	if( objectType )
	{
		if( objectType->name != "" )
			str += objectType->name + "::";
		else
			str += "?::";
	}
	if( name == "" )
		str += "?(";
	else
		str += name + "(";

	if( parameterTypes.GetLength() > 0 )
	{
		asUINT n;
		for( n = 0; n < parameterTypes.GetLength() - 1; n++ )
		{
			str += parameterTypes[n].Format();
			if( parameterTypes[n].IsReference() && inOutFlags.GetLength() > n )
			{
				if( inOutFlags[n] == 1 ) str += "in";
				else if( inOutFlags[n] == 2 ) str += "out";
				else if( inOutFlags[n] == 3 ) str += "inout";
			}
			str += ", ";
		}

		str += parameterTypes[n].Format();
		if( parameterTypes[n].IsReference() && inOutFlags.GetLength() > n )
		{
			if( inOutFlags[n] == 1 ) str += "in";
			else if( inOutFlags[n] == 2 ) str += "out";
			else if( inOutFlags[n] == 3 ) str += "inout";
		}
	}

	str += ")";

	if( isReadOnly )
		str += " const";

	return str;
}

int asCScriptFunction::GetLineNumber(int programPosition)
{
	if( lineNumbers.GetLength() == 0 ) return 0;

	// Do a binary search in the buffer
	int max = (int)lineNumbers.GetLength()/2 - 1;
	int min = 0;
	int i = max/2;

	for(;;)
	{
		if( lineNumbers[i*2] < programPosition )
		{
			// Have we found the largest number < programPosition?
			if( max == i ) return lineNumbers[i*2+1];
			if( lineNumbers[i*2+2] > programPosition ) return lineNumbers[i*2+1];

			min = i + 1;
			i = (max + min)/2; 
		}
		else if( lineNumbers[i*2] > programPosition )
		{
			// Have we found the smallest number > programPosition?
			if( min == i ) return lineNumbers[i*2+1];

			max = i - 1;
			i = (max + min)/2;
		}
		else
		{
			// We found the exact position
			return lineNumbers[i*2+1];
		}
	}
}

void asCScriptFunction::AddVariable(asCString &name, asCDataType &type, int stackOffset)
{
	asSScriptVariable *var = asNEW(asSScriptVariable);
	var->name = name;
	var->type = type;
	var->stackOffset = stackOffset;
	variables.PushLast(var);
}

void asCScriptFunction::ComputeSignatureId()
{
	// This function will compute the signatureId based on the 
	// function name, return type, and parameter types. The object 
	// type for methods is not used, so that class methods and  
	// interface methods match each other.
	for( asUINT n = 0; n < engine->signatureIds.GetLength(); n++ )
	{
		if( name != engine->signatureIds[n]->name ) continue;
		if( returnType != engine->signatureIds[n]->returnType ) continue;
		if( isReadOnly != engine->signatureIds[n]->isReadOnly ) continue;
		if( inOutFlags != engine->signatureIds[n]->inOutFlags ) continue;
		if( parameterTypes != engine->signatureIds[n]->parameterTypes ) continue;

		signatureId = engine->signatureIds[n]->signatureId;
		return;
	}

	signatureId = id;
	engine->signatureIds.PushLast(this);
}

#define INTARG(x)    (int(*(x+1)))
#define PTRARG(x)    (asPTRWORD(*(x+1)))
#define WORDARG0(x)   (*(((asWORD*)x)+1))
#define WORDARG1(x)   (*(((asWORD*)x)+2))

void asCScriptFunction::AddReferences()
{
	// Only count references if there is any bytecode
	if( byteCode.GetLength() ) 
	{
		if( returnType.IsObject() )
			returnType.GetObjectType()->AddRef();

		for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
			if( parameterTypes[p].IsObject() )
				parameterTypes[p].GetObjectType()->AddRef();
	}

	// Go through the byte code and add references to all resources used by the function
	for( asUINT n = 0; n < byteCode.GetLength(); n += asCByteCode::SizeOfType(bcTypes[*(asBYTE*)&byteCode[n]]) )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case BC_OBJTYPE:
		case BC_FREE:
		case BC_ALLOC:
		case BC_REFCPY:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)PTRARG(&byteCode[n]);
				objType->AddRef();
				break;
			}

		// Global variables
		case BC_LDG:
		case BC_PGA:
		case BC_PshG4:
		case BC_SetG4:
		case BC_CpyVtoG4:
			{
				int gvarId = WORDARG0(&byteCode[n]);
				void *gvarPtr = module->globalVarPointers[gvarId];

				gvarId = 0;
				for( asUINT g = 0; g < module->engine->globalPropAddresses.GetLength(); g++ )
				{
					if( module->engine->globalPropAddresses[g] == gvarPtr )
					{
						gvarId = -int(g) - 1;
						break;
					}
				}

				if( gvarId < 0 )
				{
					// Find the config group from the property id
					asCConfigGroup *group = module->engine->FindConfigGroupForGlobalVar(gvarId);
					if( group != 0 ) group->AddRef();
				}

				break;
			}

		case BC_LdGRdR4:
		case BC_CpyGtoV4:
			{
				int gvarId = WORDARG1(&byteCode[n]);
				void *gvarPtr = module->globalVarPointers[gvarId];

				gvarId = 0;
				for( asUINT g = 0; g < module->engine->globalPropAddresses.GetLength(); g++ )
				{
					if( module->engine->globalPropAddresses[g] == gvarPtr )
					{
						gvarId = -int(g) - 1;
						break;
					}
				}

				if( gvarId < 0 )
				{
					// Find the config group from the property id
					asCConfigGroup *group = module->engine->FindConfigGroupForGlobalVar(gvarId);
					if( group != 0 ) group->AddRef();
				}

				break;
			}

		// System functions
		case BC_CALLSYS:
			{
				int funcId = INTARG(&byteCode[n]);
				asCConfigGroup *group = module->engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->AddRef();
				break;
			}
		}
	}
}

void asCScriptFunction::ReleaseReferences()
{
	// Only count references if there is any bytecode
	if( byteCode.GetLength() )
	{
		if( returnType.IsObject() )
			returnType.GetObjectType()->Release();

		for( asUINT p = 0; p < parameterTypes.GetLength(); p++ )
			if( parameterTypes[p].IsObject() )
				parameterTypes[p].GetObjectType()->Release();
	}

	// Go through the byte code and release references to all resources used by the function
	for( asUINT n = 0; n < byteCode.GetLength(); n += asCByteCode::SizeOfType(bcTypes[*(asBYTE*)&byteCode[n]]) )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case BC_OBJTYPE:
		case BC_FREE:
		case BC_ALLOC:
		case BC_REFCPY:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)PTRARG(&byteCode[n]);
				objType->Release();
				break;
			}

		// Global variables
		case BC_LDG:
		case BC_PGA:
		case BC_PshG4:
		case BC_SetG4:
		case BC_CpyVtoG4:
			{
				int gvarId = WORDARG0(&byteCode[n]);
				void *gvarPtr = module->globalVarPointers[gvarId];

				gvarId = 0;
				for( asUINT g = 0; g < module->engine->globalPropAddresses.GetLength(); g++ )
				{
					if( module->engine->globalPropAddresses[g] == gvarPtr )
					{
						gvarId = -int(g) - 1;
						break;
					}
				}

				if( gvarId < 0 )
				{
					// Find the config group from the property id
					asCConfigGroup *group = module->engine->FindConfigGroupForGlobalVar(gvarId);
					if( group != 0 ) group->Release();
				}

				break;
			}

		case BC_LdGRdR4:
		case BC_CpyGtoV4:
			{
				int gvarId = WORDARG1(&byteCode[n]);
				void *gvarPtr = module->globalVarPointers[gvarId];

				gvarId = 0;
				for( asUINT g = 0; g < module->engine->globalPropAddresses.GetLength(); g++ )
				{
					if( module->engine->globalPropAddresses[g] == gvarPtr )
					{
						gvarId = -int(g) - 1;
						break;
					}
				}

				if( gvarId < 0 )
				{
					// Find the config group from the property id
					asCConfigGroup *group = module->engine->FindConfigGroupForGlobalVar(gvarId);
					if( group != 0 ) group->Release();
				}

				break;
			}

		// System functions
		case BC_CALLSYS:
			{
				int funcId = INTARG(&byteCode[n]);
				asCConfigGroup *group = module->engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->Release();
				break;
			}
		}
	}
}

int asCScriptFunction::GetReturnTypeId() const
{
	return engine->GetTypeIdFromDataType(returnType);
}

int asCScriptFunction::GetParamCount() const
{
	return (int)parameterTypes.GetLength();
}

int asCScriptFunction::GetParamTypeId(int index) const
{
	if( index < 0 || (unsigned)index >= parameterTypes.GetLength() )
		return asINVALID_ARG;

	return engine->GetTypeIdFromDataType(parameterTypes[index]);
}

asIScriptEngine *asCScriptFunction::GetEngine() const
{
	return engine;
}

const char *asCScriptFunction::GetDeclaration(int *length) const
{
	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = GetDeclarationStr();
	if( length ) *length = (int)tempString->GetLength();
	return tempString->AddressOf();
}

const char *asCScriptFunction::GetScriptSectionName(int *length) const
{
	if( module )
	{
		if( length ) *length = (int)module->scriptSections[scriptSectionIdx]->GetLength();
		return module->scriptSections[scriptSectionIdx]->AddressOf();
	}
	
	return 0;
}

END_AS_NAMESPACE

