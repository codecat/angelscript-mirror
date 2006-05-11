/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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

#include <stdarg.h>     // va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <assert.h>     // assert()
#include <stdio.h>      // _vsnprintf()
#include <memory.h>     // memcpy()
#include <string.h>     // some compilers declare memcpy() here

#include "as_config.h"

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif

#include "as_string_util.h"

// Returns the number of characters written or -1 if the buffer was too small
int asStringFormat(char *string, size_t maxLength, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int r = vsnprintf(string, maxLength, format, args);

	va_end(args);

	return r;
}

double asStringScanDouble(const char *string, size_t *numScanned)
{
	char *end;

	double res = strtod(string, &end);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

int asStringScanInt(const char *string, int base, size_t *numScanned)
{
	assert(base > 0);

	char *end;

	int res = strtol(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

acUINT asStringScanUInt(const char *string, int base, size_t *numScanned)
{
	assert(base > 0);

	char *end;

	acUINT res = strtoul(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

void asStringCopy(const char *source, size_t srcLength, char *dest, size_t destLength)
{
	size_t min = srcLength < destLength ? srcLength : destLength;

	memcpy(dest, source, min);
	dest[min] = 0;
}

