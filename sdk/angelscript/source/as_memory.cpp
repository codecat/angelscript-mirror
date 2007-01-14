/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jönsson

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
// as_memory.cpp
//
// Overload the default memory management functions so that we
// can let the application decide how to do it.
//

#include <stdlib.h>

#include "as_config.h"
#include "as_memory.h"

BEGIN_AS_NAMESPACE

// By default we'll use the standard memory management functions
asALLOCFUNC_t userAlloc = malloc;
asFREEFUNC_t  userFree  = free;

int asSetGlobalMemoryFunctions(asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc)
{
	userAlloc = allocFunc;
	userFree  = freeFunc;

	return 0;
}

int asResetGlobalMemoryFunctions()
{
	asThreadCleanup();

	userAlloc = malloc;
	userFree  = free;

	return 0;
}

END_AS_NAMESPACE



