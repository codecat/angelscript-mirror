#include "utils.h"

void PrintException(asIScriptContext *ctx)
{
	asIScriptEngine *engine = ctx->GetEngine();

	int funcID = ctx->GetExceptionFunction();
	printf("mdle : %s\n", engine->GetModuleNameFromIndex(funcID>>16));
	printf("func : %s\n", engine->GetFunctionName(funcID));
	printf("line : %d\n", ctx->GetExceptionLineNumber());
	printf("desc : %s\n", ctx->GetExceptionString());
}

void Assert(bool expr)
{
	if( !expr )
	{
		printf("---------------------------------------------\n");
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			printf("func: %s\n", engine->GetFunctionDeclaration(ctx->GetCurrentFunction()));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
		}
		printf("---------------------------------------------\n");
	}
}
