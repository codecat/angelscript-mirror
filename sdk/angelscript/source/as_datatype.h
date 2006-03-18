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
// as_datatype.h
//
// This class describes the datatype for expressions during compilation
//



#ifndef AS_DATATYPE_H
#define AS_DATATYPE_H

#include "as_tokendef.h"
#include "as_string.h"

class asCScriptEngine;
class asCObjectType;

class asCDataType
{
public:
	asCDataType() : tokenType(ttUnrecognizedToken), extendedType(0), pointerLevel(0), arrayDimensions(0), objectType(0), isReference(false), isReadOnly(false) {};
	asCDataType(eTokenType tt, bool cnst, bool ref) : tokenType(tt), extendedType(0), pointerLevel(0), arrayDimensions(0), objectType(0), isReference(ref), isReadOnly(cnst) {};

	asCString Format() const;

	bool IsDefaultArrayType(asCScriptEngine *engine) const;
	void SetAsDefaultArray(asCScriptEngine *engine);
	asCDataType GetSubType(asCScriptEngine *engine);

	eTokenType tokenType;
	asCObjectType *extendedType;
	int  pointerLevel;
	int  arrayDimensions;
	asCObjectType *objectType;
	bool isReference;
	bool isReadOnly;

	bool IsPrimitive() const;
	bool IsComplex(asCScriptEngine *engine) const;

	bool IsIntegerType() const;
	bool IsUnsignedType() const;
	bool IsFloatType() const;
	bool IsDoubleType() const;
	bool IsBitVectorType() const;
	bool IsBooleanType() const;
	int  GetSizeOnStackDWords() const;
	int  GetSizeInMemoryBytes() const;
	int  GetSizeInMemoryDWords() const;

	bool IsSameBaseType(const asCDataType &dt) const;
	bool IsEqualExceptRef(const asCDataType &) const;
	bool IsEqualExceptRefAndConst(const asCDataType &) const;
	bool IsEqualExceptConst(const asCDataType &) const;

	asCDataType &operator =(const asCDataType &);
	bool operator ==(const asCDataType &) const;
	bool operator !=(const asCDataType &) const;
};


#endif
