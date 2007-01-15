
#include <stdarg.h>
#include "utils.h"
#pragma warning (disable:4786)
#include <map>

using namespace std;

namespace TestCustomMem
{

#define TESTNAME "TestCustomMem"

int objectsAllocated = 0;
void *MyAlloc(size_t size)
{
	objectsAllocated++;

	void *mem = new asBYTE[size];
//	printf("MyAlloc(%d) %X\n", size, mem);
	return mem;
}

void MyFree(void *mem)
{
	objectsAllocated--;

//	printf("MyFree(%X)\n", mem);
	delete[] mem;
}

int ReturnObj()
{
	return 0;
}

void ReturnObjGeneric(asIScriptGeneric *gen)
{
	int v = 0;
	gen->SetReturnObject(&v);
}


int numAllocs       = 0;
int numFrees        = 0;
size_t currentMemAlloc = 0;
size_t maxMemAlloc     = 0;

map<void*,size_t> memSize;
map<void*,int> memCount;

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
		memCount.erase(i2);
	}
	else
		assert(false);

	free(address);
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


static const char *script =
"void test(obj o) { }";

bool Test()
{
	bool fail = false;

	int r;

	asSetGlobalMemoryFunctions(MyAllocWithStats, MyFreeWithStats);
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("obj", 4, asOBJ_PRIMITIVE); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("obj", asBEHAVE_ALLOC, "obj &f(uint)", asFUNCTION(MyAlloc), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("obj", asBEHAVE_FREE, "void f(obj &in)", asFUNCTION(MyFree), asCALL_CDECL); assert( r >= 0 );
	if( !strstr(asGetLibraryOptions(),"AS_MAX_PORTABILITY") )
	{
		r = engine->RegisterGlobalFunction("obj retObj()", asFUNCTION(ReturnObj), asCALL_CDECL); assert( r >= 0 );
	}
	r = engine->RegisterGlobalFunction("obj retObj2(obj)", asFUNCTION(ReturnObjGeneric), asCALL_GENERIC); assert( r >= 0 );

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->ExecuteString(0, "obj o");

	if( !strstr(asGetLibraryOptions(),"AS_MAX_PORTABILITY") )
		engine->ExecuteString(0, "retObj()");

	engine->ExecuteString(0, "obj o; retObj2(o)");

	engine->ExecuteString(0, "obj[] o(2)");

	engine->AddScriptSection(0, 0, script, strlen(script));
	engine->Build(0);
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByName(0, "test"));
	int v = 0;
	ctx->SetArgObject(0, &v);
	ctx->Execute();
	ctx->Release();

	engine->Release();

	if( objectsAllocated )
	{
		printf("%s: Failed\n", TESTNAME);
		fail = true;
	}

	asThreadCleanup();
	assert( numAllocs == numFrees );
	assert( currentMemAlloc == 0 );

	asResetGlobalMemoryFunctions();
	PrintAllocIndices();

	// Success
	return fail;
}

} // namespace

