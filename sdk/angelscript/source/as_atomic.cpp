/*
   AngelCode Scripting Library
   Copyright (c) 2003-2008 Andreas Jonsson

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
// as_gc.cpp
//
// The implementation of the garbage collector
//

#include "as_atomic.h"

BEGIN_AS_NAMESPACE

asCAtomic::asCAtomic()
{
	value = 0;
}

asDWORD asCAtomic::get() const
{
	return value;
}

void asCAtomic::set(asDWORD val)
{
	value = val;
}

//
// The following code implements the atomicInc and atomicDec on different platforms
//
#ifdef AS_NO_THREADS

asDWORD asCAtomic::atomicInc()
{
	return ++value;
}

asDWORD asCAtomic::atomicDec()
{
	return --value;
}

#elif AS_WIN

END_AS_NAMESPACE
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
BEGIN_AS_NAMESPACE

asDWORD asCAtomic::atomicInc()
{
	return InterlockedIncrement((LONG*)&value);
}

asDWORD asCAtomic::atomicDec()
{
	return InterlockedDecrement((LONG*)&value);
}

#elif AS_LINUX

END_AS_NAMESPACE
#include <asm/atomic.h>
BEGIN_AS_NAMESPACE

asDWORD asCAtomic::atomicInc()
{
	return atomic_inc_and_test(value);
}

asDWORD asCAtomic::atomicDec()
{
	return atomic_dec_and_test(value);
}

#else

asDWORD asCAtomic::atomicInc()
{
	asDWORD v;
	ENTERCRITICALSECTION(cs);
	v = ++value;
	LEAVECRITICALSECTION(cs);
	return v;
}

asDWORD asCAtomic::atomicDec()
{
	asDWORD v;
	ENTERCRITICALSECTION(cs);
	v = --value;
	LEAVECRITICALSECTION(cs);
	return v;
}

#endif

END_AS_NAMESPACE

