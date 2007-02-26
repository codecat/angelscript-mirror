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

// TODO: This macro is only temporary and should be removed once it
// has been established that the macros are cross platform compatible
#ifdef AS_NO_USER_ALLOC

	#define NEW(x)        new x
	#define DELETE(ptr,x) delete ptr

	#define NEWARRAY(x,cnt)  new x[cnt]
	#define DELETEARRAY(ptr) delete[] ptr

#else

	#ifndef AS_DEBUG

		#define NEW(x)        new(userAlloc(sizeof(x))) x
		#define DELETE(ptr,x) {void *tmp = ptr; (ptr)->~x(); userFree(tmp);}

		#define NEWARRAY(x,cnt)  (x*)userAlloc(sizeof(x)*cnt)
		#define DELETEARRAY(ptr) userFree(ptr)

	#else

		typedef void *(*asALLOCFUNCDEBUG_t)(size_t, const char *, unsigned int);

		#define NEW(x)        new(((asALLOCFUNCDEBUG_t)(userAlloc))(sizeof(x), __FILE__, __LINE__)) x
		#define DELETE(ptr,x) {void *tmp = ptr; (ptr)->~x(); userFree(tmp);}

		#define NEWARRAY(x,cnt)  (x*)((asALLOCFUNCDEBUG_t)(userAlloc))(sizeof(x)*cnt, __FILE__, __LINE__)
		#define DELETEARRAY(ptr) userFree(ptr)

	#endif
#endif

END_AS_NAMESPACE

#include "as_array.h"

BEGIN_AS_NAMESPACE

class asCMemoryMgr
{
public:
	asCMemoryMgr();
	~asCMemoryMgr();

	void FreeUnusedMemory();

	void *AllocScriptNode();
	void FreeScriptNode(void *ptr);

protected:
	asCArray<void *> scriptNodePool;
};

END_AS_NAMESPACE

#endif
