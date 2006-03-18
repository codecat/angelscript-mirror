/*
   AngelCode Scripting Library
   Copyright (c) 2003-2005 Andreas Jönsson

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
// as_bstr_util.cpp
//
// Some functions for working with bstrs
//

#include <memory.h> // memcpy(), memcmp()
#include <string.h> // some compilers declare memcmp() here

#include "as_config.h"
#include "as_bstr_util.h"

asBSTR asBStrAlloc(asUINT length)
{
	unsigned char *str = new unsigned char[length+4+1];
	*((asUINT*)str)  = length; // Length of string
	str[length+4] = 0;      // Null terminated

	return str + 4;
}

void asBStrFree(asBSTR str)
{
	if( str == 0 ) return;

	unsigned char *p = str-4;
	delete[] p;
}

asUINT asBStrLength(asBSTR str)
{
	if( str == 0 ) return 0;

	unsigned char *p = str-4;

	return *(asUINT*)(p);
}
