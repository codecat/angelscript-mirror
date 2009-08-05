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
#include "as_texts.h"

BEGIN_AS_NAMESPACE

// internal
asCScriptFunction::asCScriptFunction(asCScriptEngine *engine, asCModule *mod)
{
	refCount.set(1);
	this->engine           = engine;
	funcType               = -1;
	module                 = mod; 
	objectType             = 0; 
	name                   = ""; 
	isReadOnly             = false;
	stackNeeded            = 0;
	sysFuncIntf            = 0;
	signatureId            = 0;
	scriptSectionIdx       = -1;
	dontCleanUpOnException = false;
	vfTableIdx             = -1;
	jitFunction            = 0;
}

// internal
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

// interface
int asCScriptFunction::AddRef()
{
	return refCount.atomicInc();
}

int asCScriptFunction::Release()
{
	return refCount.atomicDec();
}

// interface
const char *asCScriptFunction::GetModuleName() const
{
	if( module )
	{
		return module->name.AddressOf();
	}

	return 0;
}

// interface
asIObjectType *asCScriptFunction::GetObjectType() const
{
	return objectType;
}

// interface
const char *asCScriptFunction::GetObjectName() const 
{
	if( objectType )
		return objectType->GetName();

	return 0;
}

// interface
const char *asCScriptFunction::GetName() const
{
	return name.AddressOf();
}

// interface
bool asCScriptFunction::IsClassMethod() const
{
	return objectType && objectType->IsInterface() == false;
}

// interface
bool asCScriptFunction::IsInterfaceMethod() const
{
	return objectType && objectType->IsInterface();
}

// interface
bool asCScriptFunction::IsReadOnly() const
{
	return isReadOnly;
}

// internal
int asCScriptFunction::GetSpaceNeededForArguments()
{
	// We need to check the size for each type
	int s = 0;
	for( asUINT n = 0; n < parameterTypes.GetLength(); n++ )
		s += parameterTypes[n].GetSizeOnStackDWords();

	return s;
}

// internal
int asCScriptFunction::GetSpaceNeededForReturnValue()
{
	return returnType.GetSizeOnStackDWords();
}

// internal
asCString asCScriptFunction::GetDeclarationStr(bool includeObjectName) const
{
	asCString str;

	str = returnType.Format();
	str += " ";
	if( objectType && includeObjectName )
	{
		if( objectType->name != "" )
			str += objectType->name + "::";
		else
			str += "_unnamed_type_::";
	}
	asASSERT(name.GetLength() > 0);
	if( name == "" )
		str += "_unnamed_function_(";
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
				if( inOutFlags[n] == asTM_INREF ) str += "in";
				else if( inOutFlags[n] == asTM_OUTREF ) str += "out";
				else if( inOutFlags[n] == asTM_INOUTREF ) str += "inout";
			}
			str += ", ";
		}

		str += parameterTypes[n].Format();
		if( parameterTypes[n].IsReference() && inOutFlags.GetLength() > n )
		{
			if( inOutFlags[n] == asTM_INREF ) str += "in";
			else if( inOutFlags[n] == asTM_OUTREF ) str += "out";
			else if( inOutFlags[n] == asTM_INOUTREF ) str += "inout";
		}
	}

	str += ")";

	if( isReadOnly )
		str += " const";

	return str;
}

// internal
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

// internal
void asCScriptFunction::AddVariable(asCString &name, asCDataType &type, int stackOffset)
{
	asSScriptVariable *var = asNEW(asSScriptVariable);
	var->name        = name;
	var->type        = type;
	var->stackOffset = stackOffset;
	variables.PushLast(var);
}

// internal
void asCScriptFunction::ComputeSignatureId()
{
	// This function will compute the signatureId based on the 
	// function name, return type, and parameter types. The object 
	// type for methods is not used, so that class methods and  
	// interface methods match each other.
	for( asUINT n = 0; n < engine->signatureIds.GetLength(); n++ )
	{
		if( !IsSignatureEqual(engine->signatureIds[n]) ) continue;

		signatureId = engine->signatureIds[n]->signatureId;
		return;
	}

	signatureId = id;
	engine->signatureIds.PushLast(this);
}

// internal
bool asCScriptFunction::IsSignatureEqual(const asCScriptFunction *func) const
{
	if( name              != func->name              ) return false;
	if( returnType        != func->returnType        ) return false;
	if( isReadOnly        != func->isReadOnly        ) return false;
	if( inOutFlags        != func->inOutFlags        ) return false;
	if( parameterTypes    != func->parameterTypes    ) return false;
	if( (objectType != 0) != (func->objectType != 0) ) return false;

	return true;
}

// internal
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
	for( asUINT n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_ALLOC:
		case asBC_REFCPY:
			{
                asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->AddRef();
			}
			break;

		// Global variables
		case asBC_LDG:
		case asBC_PGA:
		case asBC_PshG4:
		case asBC_SetG4:
		case asBC_CpyVtoG4:
			if( module )
			{
				int gvarId = asBC_WORDARG0(&byteCode[n]);
				asCConfigGroup *group = module->GetConfigGroupByGlobalVarId(gvarId);
				if( group != 0 ) group->AddRef();
			}
			break;

		case asBC_LdGRdR4:
		case asBC_CpyGtoV4:
			if( module )
			{
				int gvarId = asBC_WORDARG1(&byteCode[n]);
				asCConfigGroup *group = module->GetConfigGroupByGlobalVarId(gvarId);
				if( group != 0 ) group->AddRef();
			}
			break;

		// System functions
		case asBC_CALLSYS:
			if( module )
			{
				int funcId = asBC_INTARG(&byteCode[n]);
				asCConfigGroup *group = module->engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->AddRef();
			}
			break;
		}
	}
}

// internal
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
	for( asUINT n = 0; n < byteCode.GetLength(); n += asBCTypeSize[asBCInfo[*(asBYTE*)&byteCode[n]].type] )
	{
		switch( *(asBYTE*)&byteCode[n] )
		{
		// Object types
		case asBC_OBJTYPE:
		case asBC_FREE:
		case asBC_ALLOC:
		case asBC_REFCPY:
			{
				asCObjectType *objType = (asCObjectType*)(size_t)asBC_PTRARG(&byteCode[n]);
				objType->Release();
			}
			break;

		// Global variables
		case asBC_LDG:
		case asBC_PGA:
		case asBC_PshG4:
		case asBC_SetG4:
		case asBC_CpyVtoG4:
			if( module )
			{
				int gvarId = asBC_WORDARG0(&byteCode[n]);
				asCConfigGroup *group = module->GetConfigGroupByGlobalVarId(gvarId);
				if( group != 0 ) group->Release();
			}
			break;

		case asBC_LdGRdR4:
		case asBC_CpyGtoV4:
			if( module )
			{
				int gvarId = asBC_WORDARG1(&byteCode[n]);
				asCConfigGroup *group = module->GetConfigGroupByGlobalVarId(gvarId);
				if( group != 0 ) group->Release();
			}
			break;

		// System functions
		case asBC_CALLSYS:
			if( module )
			{
				int funcId = asBC_INTARG(&byteCode[n]);
				asCConfigGroup *group = module->engine->FindConfigGroupForFunction(funcId);
				if( group != 0 ) group->Release();
			}
			break;
		}
	}

	// Release the jit compiled function
	if( jitFunction )
		engine->jitCompiler->ReleaseJITFunction(jitFunction);
	jitFunction = 0;
}

// interface
int asCScriptFunction::GetReturnTypeId() const
{
	return engine->GetTypeIdFromDataType(returnType);
}

// interface
int asCScriptFunction::GetParamCount() const
{
	return (int)parameterTypes.GetLength();
}

// interface
int asCScriptFunction::GetParamTypeId(int index, asDWORD *flags) const
{
	if( index < 0 || (unsigned)index >= parameterTypes.GetLength() )
		return asINVALID_ARG;

	if( flags )
		*flags = inOutFlags[index];

	return engine->GetTypeIdFromDataType(parameterTypes[index]);
}

// interface
asIScriptEngine *asCScriptFunction::GetEngine() const
{
	return engine;
}

// interface
const char *asCScriptFunction::GetDeclaration(bool includeObjectName) const
{
	asASSERT(threadManager);
	asCString *tempString = &threadManager->GetLocalData()->string;
	*tempString = GetDeclarationStr(includeObjectName);
	return tempString->AddressOf();
}

// interface
const char *asCScriptFunction::GetScriptSectionName() const
{
	if( module && scriptSectionIdx >= 0 )
	{
		return module->scriptSections[scriptSectionIdx]->AddressOf();
	}
	
	return 0;
}

// interface
const char *asCScriptFunction::GetConfigGroup() const
{
	asCConfigGroup *group = engine->FindConfigGroupForFunction(id);
	if( group == 0 )
		return 0;

	return group->groupName.AddressOf();
}

// internal
void asCScriptFunction::JITCompile()
{
    asIJITCompiler *jit = engine->GetJITCompiler();
    if( !jit )
        return;

	// Release the previous function, if any
    if( jitFunction )
    {
        engine->jitCompiler->ReleaseJITFunction(jitFunction);
        jitFunction = 0;
    }

	// Compile for native system
	int r = jit->CompileFunction(this, &jitFunction);
	if( r < 0 )
	{
		asASSERT( jitFunction == 0 );
	}
}

// interface
asDWORD *asCScriptFunction::GetByteCode(asUINT *length)
{
	if( length )
		*length = (asUINT)byteCode.GetLength();

	if( byteCode.GetLength() )
	{
		return byteCode.AddressOf();
	}

	return 0;
}

END_AS_NAMESPACE

