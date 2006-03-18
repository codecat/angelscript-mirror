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

struct asSTypeBehaviour;

class asCObjectType
{
public:
	asCObjectType() {beh = 0;}
	~asCObjectType();

	asCString   name;
	eTokenType  tokenType;
	int         pointerLevel;
	int         arrayDimensions;
	int         size;
	asCArray<asCProperty*> properties;
	asCArray<int> methods;

	asDWORD flags;

	asSTypeBehaviour *beh;
};

#endif
