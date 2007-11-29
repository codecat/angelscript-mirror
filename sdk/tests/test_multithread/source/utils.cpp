#include "utils.h"

void PrintException(asIScriptContext *ctx)
{
	asIScriptEngine *engine = ctx->GetEngine();

	int funcID = ctx->GetExceptionFunction();
	printf("mdle : %s\n", engine->GetFunctionModule(funcID));
	printf("func : %s\n", engine->GetFunctionName(funcID));
	printf("line : %d\n", ctx->GetExceptionLineNumber());
	printf("desc : %s\n", ctx->GetExceptionString());
}

