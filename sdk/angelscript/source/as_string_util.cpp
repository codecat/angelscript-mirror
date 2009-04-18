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

#include <stdarg.h>     // va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <stdio.h>      // _vsnprintf()
#include <string.h>     // some compilers declare memcpy() here
#include <locale.h>     // setlocale()

#include "as_config.h"

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif

#include "as_string.h"
#include "as_string_util.h"

// Returns the number of characters written or -1 if the buffer was too small
int asStringFormat(char *string, size_t maxLength, const char *format, ...)
{
	va_list args;
	va_start(args, format);

#if defined(_MSC_VER) && _MSC_VER >= 1500
	int r = vsnprintf_s(string, maxLength, maxLength, format, args);
#else
	int r = vsnprintf(string, maxLength, format, args);
#endif

	va_end(args);

	return r;
}

double asStringScanDouble(const char *string, size_t *numScanned)
{
	char *end;

	// Set the locale to C so that we are guaranteed to parse the float value correctly
	asCString orig = setlocale(LC_NUMERIC, 0);
	setlocale(LC_NUMERIC, "C");

	double res = strtod(string, &end);

	// Restore the locale
	setlocale(LC_NUMERIC, orig.AddressOf());

	if( numScanned )
		*numScanned = end - string;

	return res;
}

int asStringScanInt(const char *string, int base, size_t *numScanned)
{
	asASSERT(base > 0);

	char *end;

	int res = strtol(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

asUINT asStringScanUInt(const char *string, int base, size_t *numScanned)
{
	asASSERT(base > 0);

	char *end;

	asUINT res = strtoul(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

asQWORD asStringScanUInt64(const char *string, int base, size_t *numScanned)
{
	asASSERT(base == 10 || base == 16);

	const char *end = string;

	asQWORD res = 0;
	if( base == 10 )
	{
		while( *end >= '0' && *end <= '9' )
		{
			res *= 10;
			res += *end++ - '0';
		}
	}
	else if( base == 16 )
	{
		while( (*end >= '0' && *end <= '9') ||
		       (*end >= 'a' && *end <= 'f') ||
		       (*end >= 'A' && *end <= 'F') )
		{
			res *= 16;
			if( *end >= '0' && *end <= '9' )
				res += *end++ - '0';
			else if( *end >= 'a' && *end <= 'f' )
				res += *end++ - 'a' + 10;
			else if( *end >= 'A' && *end <= 'F' )
				res += *end++ - 'A' + 10;
		}
	}

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

