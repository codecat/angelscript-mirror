#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>

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
		const char *msgType;
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
		const char *msgType;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		char buf[256];

		sprintf(buf, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);

		buffer += buf;
	}

	std::string buffer;
};

void PrintException(asIScriptContext *ctx);
void Assert(asIScriptGeneric *gen);

#endif

