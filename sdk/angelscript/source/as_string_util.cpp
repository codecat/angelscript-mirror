/*
   ac_string_util.cpp - version 1.0, June 25h, 2003

   Some string utility functions, that doesn't necessitate
   use of acCString.

   Copyright (c) 2003 Andreas Jönsson

   This software is provided 'as-is', without any form of
   warranty. In no case will the author be held responsible
   for any damage caused by its use.

   Permission is granted to anyone to use the software for
   for any purpose, including commercial. It is also allowed
   to modify the software and redistribute it free of charge.
   The permission is granted with the following restrictions:

   1. The origin of the software may not be misrepresented.
      It must be plainly understandable who is the author of
      the original software.
   2. Altered versions must not be misrepresented as the
      original software, i.e. must be plainly marked as
      altered.
   3. This notice may not be removed or altered from any
      distribution of the software, altered or not.

   Andreas Jönsson
   andreas@angelcode.com
*/

#include <stdarg.h>		// va_list, va_start(), etc
#include <stdlib.h>     // strtod(), strtol()
#include <assert.h>     // assert()
#include <stdio.h>      // _vsnprintf()
#include <memory.h>     // memcpy()

#include "as_config.h"
#include "as_string_util.h"

// Returns the number of characters written or -1 if the buffer was too small
int asStringFormat(char *string, int maxLength, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	int r = vsnprintf(string, maxLength, format, args);

	va_end(args);

	return r;
}

double asStringScanDouble(const char *string, int *numScanned)
{
	char *end;

	double res = strtod(string, &end);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

int asStringScanInt(const char *string, int base, int *numScanned)
{
	assert(base > 0);

	char *end;

	int res = strtol(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

acUINT asStringScanUInt(const char *string, int base, int *numScanned)
{
	assert(base > 0);

	char *end;

	acUINT res = strtoul(string, &end, base);

	if( numScanned )
		*numScanned = end - string;

	return res;
}

void asStringCopy(const char *source, int srcLength, char *dest, int destLength)
{
	int min = srcLength < destLength ? srcLength : destLength;

	memcpy(dest, source, min);
	dest[min] = 0;
}

