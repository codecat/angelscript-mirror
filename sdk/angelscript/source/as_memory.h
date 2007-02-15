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
// as_memory.h
//
// Overload the default memory management functions so that we
// can let the application decide how to do it.
//



#ifndef AS_MEMORY_H
#define AS_MEMORY_H

#include <new>

#include "as_config.h"

BEGIN_AS_NAMESPACE

extern asALLOCFUNC_t userAlloc;
extern asFREEFUNC_t  userFree;

// We don't overload the new operator as that would affect the application as well

#ifndef AS_DEBUG

#define NEW(x)        new(userAlloc(sizeof(x))) x
#define DELETE(ptr,x) {void *tmp = ptr; (ptr)->~x(); userFree(tmp);}

#define NEWARRAY(x,cnt)  (x*)userAlloc(sizeof(x)*cnt)
#define DELETEARRAY(ptr) userFree(&ptr[0])

// When new[] is used to allocate an array of objects with destructors it adds a word at the 
// beginning to hold the size of the array. This is done so that delete[] will know how many
// objects there are that needs to have their destructor called.
#define NEWOBJARRAY(x,cnt)  new(userAlloc(sizeof(x)*cnt+sizeof(size_t))) x[cnt]
#define DELETEOBJARRAY(ptr) userFree(((char*)&ptr[0])-sizeof(size_t))

#else

typedef void *(*asALLOCFUNCDEBUG_t)(size_t, const char *, unsigned int);

#define NEW(x)        new(((asALLOCFUNCDEBUG_t)(userAlloc))(sizeof(x), __FILE__, __LINE__)) x
#define DELETE(ptr,x) {void *tmp = ptr; (ptr)->~x(); userFree(tmp);}

#define NEWARRAY(x,cnt)  (x*)((asALLOCFUNCDEBUG_t)(userAlloc))(sizeof(x)*cnt, __FILE__, __LINE__)
#define DELETEARRAY(ptr) userFree(&ptr[0])

// When new[] is used to allocate an array of objects with destructors it adds a word at the 
// beginning to hold the size of the array. This is done so that delete[] will know how many
// objects there are that needs to have their destructor called.
#define NEWOBJARRAY(x,cnt)  new(((asALLOCFUNCDEBUG_t)(userAlloc))(sizeof(x)*cnt+sizeof(size_t), __FILE__, __LINE__)) x[cnt]
#define DELETEOBJARRAY(ptr) userFree(((char*)&ptr[0])-sizeof(size_t))

#endif

END_AS_NAMESPACE

#endif
