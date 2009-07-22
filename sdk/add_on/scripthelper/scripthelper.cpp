#include <string.h>
#include "scripthelper.h"

BEGIN_AS_NAMESPACE

int CompareRelation(asIScriptEngine *engine, void *lobj, void *robj, int typeId, int &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when sorting an array, 
    //       then the method id and context should be cached between calls.
    
	int retval = -1;
	int funcId = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opCmp method
		for( int n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *func = ot->GetMethodDescriptorByIndex(n);
			if( strcmp(func->GetName(), "opCmp") == 0 &&
				func->GetReturnTypeId() == asTYPEID_INT32 &&
				func->GetParamCount() == 1 )
			{
				asDWORD flags;
				int paramTypeId = func->GetParamTypeId(0, &flags);
				
				// The parameter must be an input reference of the same type
				if( flags != asTM_INREF || typeId != paramTypeId )
					break;

				// Found the method
				funcId = ot->GetMethodIdByIndex(n);
				break;
			}
		}
	}

	if( funcId )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(funcId);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = (int)ctx->GetReturnDWord();

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}

	return retval;
}

int CompareEquality(asIScriptEngine *engine, void *lobj, void *robj, int typeId, bool &result)
{
    // TODO: If a lot of script objects are going to be compared, e.g. when searching for an
	//       entry in a set, then the method id and context should be cached between calls.
    
	int retval = -1;
	int funcId = 0;

	asIObjectType *ot = engine->GetObjectTypeById(typeId);
	if( ot )
	{
		// Check if the object type has a compatible opEquals method
		for( int n = 0; n < ot->GetMethodCount(); n++ )
		{
			asIScriptFunction *func = ot->GetMethodDescriptorByIndex(n);
			if( strcmp(func->GetName(), "opEquals") == 0 &&
				func->GetReturnTypeId() == asTYPEID_BOOL &&
				func->GetParamCount() == 1 )
			{
				asDWORD flags;
				int paramTypeId = func->GetParamTypeId(0, &flags);
				
				// The parameter must be an input reference of the same type
				if( flags != asTM_INREF || typeId != paramTypeId )
					break;

				// Found the method
				funcId = ot->GetMethodIdByIndex(n);
				break;
			}
		}
	}

	if( funcId )
	{
		// Call the method
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(funcId);
		ctx->SetObject(lobj);
		ctx->SetArgAddress(0, robj);
		int r = ctx->Execute();
		if( r == asEXECUTION_FINISHED )
		{
			result = ctx->GetReturnByte() ? true : false;

			// The comparison was successful
			retval = 0;
		}
		ctx->Release();
	}
	else
	{
		// If the opEquals method doesn't exist, then we try with opCmp instead
		int relation;
		retval = CompareRelation(engine, lobj, robj, typeId, relation);
		if( retval >= 0 )
			result = relation == 0 ? true : false;
	}

	return retval;
}

END_AS_NAMESPACE
