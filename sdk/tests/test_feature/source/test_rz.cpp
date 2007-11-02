#include "utils.h"
#include "../../../add_on/scriptany/scriptany.h"


namespace TestRZ
{

#define TESTNAME "TestRZ"

const char *script1 = "\n"
"MyGame @global;       \n"
"class MyGame          \n"
"{                     \n"
// Cause GC to keep a reference (for testing purposes)
"  MyGame@ ref;        \n"
"  MyGame@[] array;    \n"
"}                     \n"
"any@ CreateInstance() \n"
"{                     \n"
"  any res;            \n"
"  MyGame obj;         \n"
"  @global = @obj;     \n"
"  res.store(@obj);    \n"
"  return res;         \n"
"}                     \n";


bool Test1()
{
	bool fail = false;
	int r = 0;
	COutStream out;
 	asIScriptEngine *engine;
	int refCount;

	asIScriptStruct *myGame = 0;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptAny(engine);

	engine->AddScriptSection(0, "script", script1, strlen(script1));
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
	}

	// Calling the garbage collector mustn't free the object types, even though they are not used yet
	int tid1 = engine->GetTypeIdByDecl(0, "MyGame@[]");
	engine->GarbageCollect(true);
	int tid2 = engine->GetTypeIdByDecl(0, "MyGame@[]");

	if( tid1 != tid2 )
	{
		printf("Object type was released incorrectly by GC\n");
		fail = true;
	}

	// Make sure ref count is properly updated
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByName(0, "CreateInstance"));
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		printf("execution failed\n");
		fail = true;
	}
	else
	{
		CScriptAny *any = *(CScriptAny**)ctx->GetReturnPointer();
		int typeId = any->GetTypeId();
		if( !(typeId & asTYPEID_OBJHANDLE) )
		{
			printf("not a handle\n");
			fail = true;
		}

		// Retrieve will increment the reference count for us
		any->Retrieve(&myGame, typeId);

		// What is the refcount?
		myGame->AddRef();
		refCount = myGame->Release();

		// GC, any, global, application
		if( refCount != 4 )
		{
			printf("ref count is wrong\n");
			fail = true;
		}

		// Clear the reference that the any object holds (this is not necessary)
		double zero = 0.0;
		any->Store(zero);

		// What is the refcount?
		myGame->AddRef();
		refCount = myGame->Release();

		// GC, global, application
		if( refCount != 3 )
		{
			printf("ref count is wrong\n");
			fail = true;
		}
	}

	// Call abort on the context to free up resources (this is not necessary)
	ctx->Abort();

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// GC, global, application
	if( refCount != 3 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release the context
	ctx->Release();
	ctx = 0;

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// GC, global, application
	if( refCount != 3 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Call garbage collection
	engine->GarbageCollect(true);

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// GC, global, application
	if( refCount != 3 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Discard the module, freeing the global variable
	engine->Discard(0);

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// GC, application
	if( refCount != 2 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release the game object
	refCount = myGame->Release();

	// GC
	if( refCount != 1 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release engine
	engine->Release();
	engine = 0;

	// Success
 	return fail;
}

const char *script2 = "\n"
"MyGame @global;       \n"
"class MyGame          \n"
"{                     \n"
"}                     \n"
"any@ CreateInstance() \n"
"{                     \n"
"  any res;            \n"
"  MyGame obj;         \n"
"  @global = @obj;     \n"
"  res.store(@obj);    \n"
"  return res;         \n"
"}                     \n";


bool Test2()
{
	bool fail = false;
	int r = 0;
	COutStream out;
 	asIScriptEngine *engine;
	int refCount;

	asIScriptStruct *myGame = 0;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptAny(engine);

	engine->AddScriptSection(0, "script", script2, strlen(script2));
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
	}

	// Make sure ref count is properly updated
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(engine->GetFunctionIDByName(0, "CreateInstance"));
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		printf("execution failed\n");
		fail = true;
	}
	else
	{
		CScriptAny *any = *(CScriptAny**)ctx->GetReturnPointer();
		int typeId = any->GetTypeId();
		if( !(typeId & asTYPEID_OBJHANDLE) )
		{
			printf("not a handle\n");
			fail = true;
		}

		// Retrieve will increment the reference count for us
		any->Retrieve(&myGame, typeId);

		// What is the refcount?
		myGame->AddRef();
		refCount = myGame->Release();

		// any, global, application
		if( refCount != 3 )
		{
			printf("ref count is wrong\n");
			fail = true;
		}

		// Clear the reference that the any object holds (this is not necessary)
		double zero = 0.0;
		any->Store(zero);

		// What is the refcount?
		myGame->AddRef();
		refCount = myGame->Release();

		// global, application
		if( refCount != 2 )
		{
			printf("ref count is wrong\n");
			fail = true;
		}
	}

	// Call abort on the context to free up resources (this is not necessary)
	ctx->Abort();

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// global, application
	if( refCount != 2 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release the context
	ctx->Release();
	ctx = 0;

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// global, application
	if( refCount != 2 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Call garbage collection
	engine->GarbageCollect(true);

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// global, application
	if( refCount != 2 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Discard the module, freeing the global variable
	engine->Discard(0);

	// What is the refcount?
	myGame->AddRef();
	refCount = myGame->Release();

	// application
	if( refCount != 1 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release the game object
	refCount = myGame->Release();

	// nobody
	if( refCount != 0 )
	{
		printf("ref count is wrong\n");
		fail = true;
	}

	// Release engine
	engine->Release();
	engine = 0;

	// Success
 	return fail;
}

bool Test()
{
	if( Test1() ) return true;

	if( Test2() ) return true;

	return false;
}

} // namespace

