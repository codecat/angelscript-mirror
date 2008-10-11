#include "utils.h"
#include "../../../add_on/scriptany/scriptany.h"

namespace TestAny
{

#define TESTNAME "TestAny"

// Normal functionality
static const char *script1 =
"string g_str = \"test\";               \n"
"any g_any(@g_str);                     \n"
"void TestAny()                         \n"
"{                                      \n"
"  any a, b;                            \n"
"  string str = \"test\";               \n"
"  b.store(@str);                       \n"
"  a = b;                               \n"
"  string @s;                           \n"
"  a.retrieve(@s);                      \n"
"  Assert(s == str);                    \n"
"  Assert(@s == @str);                  \n"
"  int[]@ c;                            \n"
"  a.retrieve(@c);                      \n"
"  Assert(@c == null);                  \n"
"  a = any(@str);                       \n"
"  a.retrieve(@s);                      \n"
"  Assert(s == str);                    \n"
"  any d(@str);                         \n"
"  d.retrieve(@s);                      \n"
"  Assert(s == str);                    \n"
"  g_any.retrieve(@s);                  \n"
"  Assert(@s == @g_str);                \n"
// If the container holds a handle to a const object, it must not copy this to a handle to a non-const object
"  const string @cs = str;              \n"
"  a.store(@cs);                        \n"
"  a.retrieve(@s);                      \n"
"  Assert(@s == null);                  \n"
"  @cs = null;                          \n"
"  a.retrieve(@cs);                     \n"
"  Assert(@cs == @str);                 \n"
// If the container holds a handle to a non-const object, it should be able to copy it to a handle to a const object
"  @s = str;                            \n"
"  a.store(@s);                         \n"
"  a.retrieve(@cs);                     \n"
"  Assert(@cs == @str);                 \n"
// Allow storing null
"  a.store(null);                       \n"
"}                                      \n";

// Test circular references with any
static const char *script2 =
"class s                                \n"
"{                                      \n"
"  any a;                               \n"
"};                                     \n"
"void TestAny()                         \n"
"{                                      \n"
"  any a;                               \n"
"  a.store(@a);                         \n"
"  any b,c;                             \n"
"  b.store(@c);                         \n"
"  c.store(@b);                         \n"
"  any[] d(1);                          \n"
"  d[0].store(@d);                      \n"
"  s e;                                 \n"
"  e.a.store(@e);                       \n"
"}                                      \n";

// Don't allow a ref to const in retrieve()
static const char *script3 =
"class s                   \n"
"{                         \n"
"  string @a;              \n"
"};                        \n"
"void TestAny()            \n"
"{                         \n"
"  const s a;              \n"
"  any c;                  \n"
"  c.retrieve(@a.a);       \n"
"}                         \n"; 

static const char *script4 =
"void TestAny()            \n"
"{                         \n"
"  string s = \"test\";    \n"
"  any a(@s);              \n"
"  SetMyAny(a);            \n"
"}                         \n";

static CScriptAny *myAny = 0;
void SetMyAny(asIScriptGeneric *gen)
{
	CScriptAny *a = (CScriptAny *)gen->GetArgObject(0);
	a->AddRef();
	if( myAny ) myAny->Release();
	myAny = a;
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	asIScriptContext *ctx;
	asIScriptEngine *engine;
	CBufferedOutStream bout;

	// ---------------------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString_Generic(engine);
	RegisterScriptAny(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	r = engine->RegisterGlobalFunction("void SetMyAny(any@)", asFUNCTION(SetMyAny), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	r = engine->ExecuteString(0, "TestAny()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		fail = true;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( ctx ) ctx->Release();
	engine->Release();

	//--------------------------------------------------
	// Verify that the GC can handle circles with any structures
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString_Generic(engine);
	RegisterScriptAny(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	r = engine->RegisterGlobalFunction("void SetMyAny(any@)", asFUNCTION(SetMyAny), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	r = engine->ExecuteString(0, "TestAny()", &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		fail = true;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( ctx ) ctx->Release();
	
	int gc = engine->GetObjectsInGarbageCollectorCount();
	engine->GarbageCollect();
	gc = engine->GetObjectsInGarbageCollectorCount();

	engine->Release();

	//-------------------------------------------------------
	// Don't allow const handle to retrieve()
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString_Generic(engine);
	RegisterScriptAny(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	r = engine->RegisterGlobalFunction("void SetMyAny(any@)", asFUNCTION(SetMyAny), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, TESTNAME, script3, strlen(script3), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to Build()\n", TESTNAME);
	}
	if( bout.buffer != "TestAny (5, 1) : Info    : Compiling void TestAny()\n"
	                   "TestAny (9, 14) : Warning : Argument cannot be assigned. Output will be discarded.\n" )
	{
		fail = true;
	}

	engine->Release();

	//--------------------------------------------------------
	// Make sure it is possible to pass any to the application
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptString_Generic(engine);
	RegisterScriptAny(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	r = engine->RegisterGlobalFunction("void SetMyAny(any@)", asFUNCTION(SetMyAny), asCALL_GENERIC); assert( r >= 0 );

	engine->AddScriptSection(0, TESTNAME, script4, strlen(script4), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile\n", TESTNAME);
	}
	
	r = engine->ExecuteString(0, "TestAny()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
		printf("%s: Failed to execute\n", TESTNAME);
	}

	if( myAny )
	{
		int typeId = myAny->GetTypeId();

		if( !(typeId & asTYPEID_OBJHANDLE) )
			fail = true;
		if( (typeId & asTYPEID_MASK_OBJECT) != asTYPEID_APPOBJECT )
			fail = true;

		const char *decl = engine->GetTypeDeclaration(typeId);
		if( (decl == 0) || (strcmp(decl, "string@") != 0) )
		{
			fail = true;
			printf("%s: Failed to return the correct type\n", TESTNAME);
		}

		int typeId2 = engine->GetTypeIdByDecl(0, "string@");
		if( typeId != typeId2 )
		{
			fail = true;
			printf("%s: Failed to return the correct type\n", TESTNAME);
		}

		asCScriptString *str = 0;
		myAny->Retrieve((void*)&str, typeId);

		if( str->buffer != "test" )
		{
			fail = true;
			printf("%s: Failed to set the string correctly\n", TESTNAME);
		}

		if( str ) str->Release();

		myAny->Release();
		myAny = 0;
	}
	else
		fail = true;

	//--------------------------------------
	// Make sure the any type can store primitives as well
	r = engine->ExecuteString(0, "any a; a.store(1); int b; a.retrieve(b); Assert(b == 1);");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();

	// Success
 	return fail;
}

} // namespace

