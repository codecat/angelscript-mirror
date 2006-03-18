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
// as_restore.cpp
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be

#include "as_config.h"
#include "as_restore.h"
#include "as_bytecodedef.h"

#define WRITE_NUM(N) stream->Write(&(N), sizeof(N))
#define READ_NUM(N) stream->Read(&(N), sizeof(N))

asCRestore::asCRestore(asCModule* _module, asIBinaryStream* _stream, asCScriptEngine* _engine)
 : module(_module), stream(_stream), engine(_engine)
{
#ifdef USE_ASM_VM
	for( int n = 0; n < BC_MAXBYTECODE; n++ )
		relocToBC.Insert(relocTable[n], n);
#endif
}

int asCRestore::Save() 
{
	unsigned long i, count;
	
	// initFunction
	WriteFunction(&module->initFunction);

	// exitFunction
	WriteFunction(&module->exitFunction);

	// scriptFunctions[]
	count = module->scriptFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
		WriteFunction(module->scriptFunctions[i]);

	// scriptGlobals[]
	count = module->scriptGlobals.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteProperty(module->scriptGlobals[i]);

	// globalMem size (can restore data using @init())
	count = module->globalMem.GetLength();
	WRITE_NUM(count);

	// stringConstants[]
	count = module->stringConstants.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteBSTR(&module->stringConstants[i]);

	// importedFunctions[] and bindInformations[]
	count = module->importedFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteFunction(module->importedFunctions[i]);
		WRITE_NUM(module->bindInformations[i].importFrom);
	}

	return asSUCCESS;
}

int asCRestore::Restore() 
{
	// Before starting the load, make sure that 
	// any existing resources have been freed
	module->Reset();

	unsigned long i, count;

	asCScriptFunction* func;
	asCProperty* prop;
	asBSTR bstr;

	// initFunction
	ReadFunction(&module->initFunction);

	// exitFunction
	ReadFunction(&module->exitFunction);

	// scriptFunctions[]
	READ_NUM(count);
	module->scriptFunctions.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->scriptFunctions.PushLast(func);
	}

	// scriptGlobals[]
	READ_NUM(count);
	module->scriptGlobals.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		prop = new asCProperty;
		ReadProperty(prop);
		module->scriptGlobals.PushLast(prop);
	}

	// globalMem size
	READ_NUM(count);
	module->globalMem.Allocate(count, 0);

	// stringConstants[]
	READ_NUM(count);
	module->stringConstants.Allocate(count, 0);
	for(i=0;i<count;++i) 
	{
		bstr = ReadBSTR();
		module->stringConstants.PushLast(bstr);
	}
	
	// importedFunctions[] and bindInformations[]
	READ_NUM(count);
	module->importedFunctions.Allocate(count, 0);
	module->bindInformations.SetLength(count);
	for(i=0;i<count;++i)
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->importedFunctions.PushLast(func);

		READ_NUM(module->bindInformations[i].importFrom);
		module->bindInformations[i].importedFunction = -1;
	}

	// Fake building
	module->isBuildWithoutErrors = true;

	// Init system functions properly
	engine->PrepareEngine();

	module->CallInit();

	return 0;
}

void asCRestore::WriteString(asCString* str) 
{
	unsigned long len = str->GetLength();
	WRITE_NUM(len);
	stream->Write(str->AddressOf(), len);
}

void asCRestore::WriteFunction(asCScriptFunction* func) 
{
	int i, count;

	WriteString(&func->name);

	WriteDataType(&func->returnType);

	count = func->parameterTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteDataType(&func->parameterTypes[i]);

	WRITE_NUM(func->id);
	
	count = func->byteCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->byteCode.AddressOf(), count);

	count = func->cleanCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->cleanCode.AddressOf(), count);

	WRITE_NUM(func->stackNeeded);

	WriteObjectType(func->objectType);

	int length = func->lineNumbers.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->lineNumbers[i]);

	length = func->exceptionIDs.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->exceptionIDs[i]);
}

void asCRestore::WriteProperty(asCProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->index);
}

void asCRestore::WriteBSTR(asBSTR* str) 
{
	unsigned long count;

	count = strlen((char*)*str);
	WRITE_NUM(count);

	stream->Write(*str, count);
}

void asCRestore::WriteDataType(asCDataType* dt) 
{
	WRITE_NUM(dt->tokenType);
	WriteObjectType(dt->extendedType);
	WRITE_NUM(dt->pointerLevel);
	WRITE_NUM(dt->arrayDimensions);
	WRITE_NUM(dt->isReference);
	WRITE_NUM(dt->isReadOnly);
	WriteObjectType(dt->objectType);
}

void asCRestore::WriteObjectType(asCObjectType* ot) 
{
	// Only write the object type name
	if( ot )
		WriteString(&ot->name);
	else
	{
		// Write a null string
		asDWORD null = 0;
		WRITE_NUM(null);
	}
}

void asCRestore::ReadString(asCString* str) 
{
	unsigned long len;
	READ_NUM(len);
	str->SetLength(len);
	stream->Read(str->AddressOf(), len);
	str->RecalculateLength();
}

void asCRestore::ReadFunction(asCScriptFunction* func) 
{
	int i, count;
	asCDataType dt;

	ReadString(&func->name);

	ReadDataType(&func->returnType);

	READ_NUM(count);
	func->parameterTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadDataType(&dt);
		func->parameterTypes.PushLast(dt);
	}

	READ_NUM(func->id);
	
	READ_NUM(count);
	func->byteCode.Allocate(count, 0);
	ReadByteCode(func->byteCode.AddressOf(), count);
	func->byteCode.SetLength(count);

	READ_NUM(count);
	func->cleanCode.Allocate(count, 0);
	ReadByteCode(func->cleanCode.AddressOf(), count);
	func->cleanCode.SetLength(count);

	READ_NUM(func->stackNeeded);

	func->objectType = ReadObjectType();

	int length;
	READ_NUM(length);
	func->lineNumbers.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->lineNumbers[i]);

	READ_NUM(length);
	func->exceptionIDs.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->exceptionIDs[i]);

}

void asCRestore::ReadProperty(asCProperty* prop) 
{
	ReadString(&prop->name);
	ReadDataType(&prop->type);
	READ_NUM(prop->index);
}

asBSTR asCRestore::ReadBSTR() 
{
	unsigned long count;

	READ_NUM(count);
	asBSTR str = asBStrAlloc(count);

	stream->Read(str, count);
	
	return str;
}

void asCRestore::ReadDataType(asCDataType* dt) 
{
	READ_NUM(dt->tokenType);
	dt->extendedType = ReadObjectType();
	READ_NUM(dt->pointerLevel);
	READ_NUM(dt->arrayDimensions);
	READ_NUM(dt->isReference);
	READ_NUM(dt->isReadOnly);
	dt->objectType = ReadObjectType();
}

asCObjectType* asCRestore::ReadObjectType() 
{
	// Read the object type name
	asCString typeName;
	ReadString(&typeName);

	// Find the registered object type in the engine
	return engine->GetObjectType(typeName);
}

void asCRestore::WriteByteCode(asBYTE *bc, int length)
{
	while( length )
	{
#ifdef USE_ASM_VM
		asDWORD reloc = *(asDWORD*)bc;
		asBYTE c = FindByteCode(reloc);
#else
		asBYTE c = *bc;
#endif
		bc += 4;
		WRITE_NUM(c);
		for( int n = 4; n < bcSize[c]; n++ )
			WRITE_NUM(*bc++);

		length -= bcSize[c];
	}
}

void asCRestore::ReadByteCode(asBYTE *bc, int length)
{
	while( length )
	{
		asBYTE c;
		READ_NUM(c);
#ifdef USE_ASM_VM
		*(asDWORD*)bc = relocTable[c];
#else
		*(asDWORD*)bc = asDWORD(c);
#endif
		bc += 4;
		for( int n = 4; n < bcSize[c]; n++ )
			READ_NUM(*bc++);

		length -= bcSize[c];
	}
}

#ifdef USE_ASM_VM
asBYTE asCRestore::FindByteCode(asDWORD relocation)
{
	asDWORD bc;
	relocToBC.MoveTo(relocation);
	bc = relocToBC.GetValue();

	return asBYTE(bc);
}
#endif

