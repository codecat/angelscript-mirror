#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <math.h>

#include <angelscript.h>

#include "../../../add_on/scriptstring/scriptstring.h"

#ifdef AS_USE_NAMESPACE
using namespace AngelScript;
#endif


class COutStream
{
public:
	void Callback(asSMessageInfo *msg) 
	{ 
		const char *msgType = 0;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
	}
};

class CBufferedOutStream
{
public:
	void Callback(asSMessageInfo *msg) 
	{ 
		const char *msgType = 0;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		char buf[256];
		#ifdef _MSC_VER
		_snprintf(buf, 255, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
		#else
		snprintf(buf, 255, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
		#endif
		buf[255] = '\0';

		buffer += buf;
	}

	std::string buffer;
};

void PrintException(asIScriptContext *ctx);
void Assert(asIScriptGeneric *gen);

void InstallMemoryManager();
void RemoveMemoryManager();


#if defined(_MSC_VER) && _MSC_VER <= 1200 // MSVC++ 6
	#define I64(x) x##l
#else // MSVC++ 7, GNUC, etc
	#define I64(x) x##ll
#endif

#endif

inline bool CompareDouble(double a,double b)
{
	if( fabs( a - b ) > 0.00000001 )
		return false;
	return true;
}

#define UNUSED_VAR(x) (x)=(x)

