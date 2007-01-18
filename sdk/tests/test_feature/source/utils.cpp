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

static int numAllocs            = 0;
static int numFrees             = 0;
static size_t currentMemAlloc   = 0;
static size_t maxMemAlloc       = 0;
static int maxNumAllocsSameTime = 0;
static asQWORD sumAllocSize     = 0;

static map<void*,size_t> memSize;
static map<void*,int> memCount;
static map<size_t,int> meanSize;

void *MyAllocWithStats(size_t size)
{
	numAllocs++;
	sumAllocSize += size;

	currentMemAlloc += size;
	if( currentMemAlloc > maxMemAlloc ) maxMemAlloc = currentMemAlloc;

	void *ptr = new asBYTE[size];
	memSize.insert(map<void*,size_t>::value_type(ptr,size));

	memCount.insert(map<void*,int>::value_type(ptr,numAllocs));

	if( numAllocs - numFrees > maxNumAllocsSameTime )
		maxNumAllocsSameTime = numAllocs - numFrees;

	map<size_t,int>::iterator i = meanSize.find(size);
	if( i != meanSize.end() )
		i->second++;
	else
		meanSize.insert(map<size_t,int>::value_type(size,1));

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

	printf("---------\n");
	printf("MEMORY STATISTICS\n");
	printf("number of allocations                 : %d\n", numAllocs);
	printf("max allocated memory                  : %d\n", maxMemAlloc);
	printf("max number of simultaneous allocations: %d\n", maxNumAllocsSameTime);
	printf("medium size of allocations            : %d\n", (int)sumAllocSize/numAllocs);

	// Find the mean size of allocations
	map<size_t,int>::iterator i = meanSize.begin();
	int n = 0;
	int meanAllocSize = 0;
	while( i != meanSize.end() )
	{
		if( n + i->second > numAllocs / 2 )
		{
			meanAllocSize = (int)i->first;
			break;
		}

		n += i->second;
		i++;
	}
	printf("mean size of allocations              : %d\n", meanAllocSize);
	printf("smallest allocation size              : %d\n", meanSize.begin()->first);
	printf("largest allocation size               : %d\n", meanSize.rbegin()->first);
	printf("number of different allocation sizes  : %d\n", meanSize.size());

	// Print allocation sizes
	i = meanSize.begin();
	while( i != meanSize.end() )
	{
		if( i->second >= 1000 )
			printf("alloc size %d: %d\n", i->first, i->second);
		i++;
	}

	asResetGlobalMemoryFunctions();
}
