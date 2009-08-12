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
// as_property.h
//
// A class for storing object property information
//



#ifndef AS_PROPERTY_H
#define AS_PROPERTY_H

#include "as_string.h"
#include "as_datatype.h"

BEGIN_AS_NAMESPACE

class asCObjectProperty
{
public:
	asCString   name;
	asCDataType type;
	int         byteOffset;
};



//
// TODO: global:
// The global property structure is reference counted, so that the
// engine can keep track of how many references to the property there are.
//
// It is possible to unregister a property without updating all referrers.
// When this happens the address pointing to the value is cleared. When 
// the referrer tries to access the value it will throw an exception.
//
// The global property structure is responsible for allocating the storage
// method for script declared variables. Each allocation is independent of
// other global properties, so that variables can be added and removed at
// any time.
//
class asCGlobalProperty
{
public:
	asCGlobalProperty() { memory = 0; memoryAllocated = false; realAddress = 0; }
	~asCGlobalProperty() { if( memoryAllocated ) { asDELETEARRAY(memory); } }

	asCString   name;
	asCDataType type;
	int         id;
	bool        memoryAllocated;

	// This is only stored for registered properties, and keeps the pointer given by the application
	void       *realAddress;

	void SetAddressOfValue(void *p) { memory = p; }
	void *GetAddressOfValue() { return (memoryAllocated || realAddress) ? memory : &storage; }

	union
	{
		void       *memory;
		asQWORD     storage;
	};
};

END_AS_NAMESPACE

#endif
