#include "utils.h"

#pragma warning (disable:4786)
#include <map>

using namespace std;


void PrintException(asIScriptContext *ctx)
{
	asIScriptEngine *engine = ctx->GetEngine();
	int funcID = ctx->GetExceptionFunction();
	printf("func: %s\n", engine->GetFunctionDeclaration(funcID));
	printf("modl: %s\n", engine->GetFunctionModule(funcID));
	printf("sect: %s\n", engine->GetFunctionSection(funcID));
	printf("line: %d\n", ctx->GetExceptionLineNumber());
	printf("desc: %s\n", ctx->GetExceptionString());
}

void Assert(asIScriptGeneric *gen)
{
	bool expr = gen->GetArgDWord(0) ? true : false;
	if( !expr )
	{
		printf("--- Assert failed ---\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			int funcID = ctx->GetCurrentFunction();
			printf("func: %s\n", engine->GetFunctionDeclaration(funcID));
			printf("mdle: %s\n", engine->GetFunctionModule(funcID));
			printf("sect: %s\n", engine->GetFunctionSection(funcID));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
			printf("---------------------\n");
		}
	}
}

static int numAllocs       = 0;
static int numFrees        = 0;
static size_t currentMemAlloc = 0;
static size_t maxMemAlloc     = 0;

static map<void*,size_t> memSize;
static map<void*,int> memCount;

void *MyAllocWithStats(size_t size)
{
	numAllocs++;

	currentMemAlloc += size;
	if( currentMemAlloc > maxMemAlloc ) maxMemAlloc = currentMemAlloc;

	void *ptr = new asBYTE[size];
	memSize.insert(map<void*,size_t>::value_type(ptr,size));

	memCount.insert(map<void*,int>::value_type(ptr,numAllocs));

	return ptr;
}

void MyFreeWithStats(void *address)
{
	numFrees++;

	map<void*,size_t>::iterator i = memSize.find(address);
	if( i != memSize.end() )
	{
		currentMemAlloc -= i->second;
		memSize.erase(i);
	}
	else
		assert(false);

	map<void*,int>::iterator i2 = memCount.find(address);
	if( i2 != memCount.end() )
	{
		int numAlloc = i2->second;
		memCount.erase(i2);
	}
	else
		assert(false);

	free(address);
}

void InstallMemoryManager()
{
	asSetGlobalMemoryFunctions(MyAllocWithStats, MyFreeWithStats);
}

void PrintAllocIndices()
{
	map<void*,int>::iterator i = memCount.begin();
	while( i != memCount.end() )
	{
		printf("%d\n", i->second);
		i++;
	}
}

void RemoveMemoryManager()
{
	asThreadCleanup();

	PrintAllocIndices();

	assert( numAllocs == numFrees ); 
	assert( currentMemAlloc == 0 );

	asResetGlobalMemoryFunctions();
}
