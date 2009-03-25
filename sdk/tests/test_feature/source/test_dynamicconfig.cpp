#include "utils.h"
#include "../../../add_on/scriptany/scriptany.h"

namespace TestDynamicConfig
{

#define TESTNAME "TestDynamicConfig"

static const char *script1 =
"void Test()           \n"
"{                     \n"
"  MyFunc();           \n"
"}                     \n";

static const char *script2 =
"void Test()           \n"
"{                     \n"
"  global = 1;         \n"
"}                     \n";

static const char *script3 =
"void Test()           \n"
"{                     \n"
"  mytype var;         \n"
"}                     \n";

static const char *script4 =
"void Test()           \n"
"{                     \n"
"  mytype var;         \n"
"  string a;           \n"
"  a = a + var;        \n"
"}                     \n";

static const char *script5 =
"void Test()           \n"
"{                     \n"
"  mytype var;         \n"
"  g_any.store(@var);  \n"
"}                     \n";

static const char *script6 =
"void Test()           \n"
"{                     \n"
"  int[] a;            \n"
"}                     \n";

static const char *script7 =
"class mystruct        \n"
"{                     \n"
"  mytype var;         \n"
"};                    \n";

static const char *script8 =
"void Test(mytype&in)  \n"
"{                     \n"
"}                     \n";

static const char *script9 =
"void Test()           \n"
"{                     \n"
"   mytype[] a;        \n"
"}                     \n";

static const char *script10 =
"void Test()           \n"
"{                     \n"
"  mytype[] var;       \n"
"  g_any.store(@var);  \n"
"}                     \n";

static void MyFunc(asIScriptGeneric *)
{
}

static void Construct(asIScriptGeneric *gen)
{
	int *o = (int*)gen->GetObject();
	*o = 1;
}

static void AddRef(asIScriptGeneric *gen)
{
	int *o = (int*)gen->GetObject();
	(*o)++;
}

static void Release(asIScriptGeneric *gen)
{
	int *o = (int*)gen->GetObject();
	(*o)--;
	if( *o == 0 ) delete o;
}

bool Test2();

bool Test()
{
	bool fail = Test2();
	int r;

	//------------
	// Test global function
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void MyFunc()", asFUNCTION(MyFunc), asCALL_GENERIC); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 1) : Info    : Compiling void Test()\n"
                                 "TestDynamicConfig (3, 3) : Error   : No matching signatures to 'MyFunc()'\n" )
	{
		fail = true;
	}

	engine->Release();

	//----------------
	// Test global property
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("int global", 0); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 1) : Info    : Compiling void Test()\n"
                                 "TestDynamicConfig (3, 3) : Error   : 'global' is not declared\n" )
	{
		fail = true;
	}

	// Try registering the property again
	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("int global", 0); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	engine->Release();

	//-------------
	// Test object types
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 1) : Info    : Compiling void Test()\n"
                                 "TestDynamicConfig (3, 10) : Error   : Expected ';'\n" )
	{
		fail = true;
	}

	engine->Release();

	//------------------
	// Test global behaviours
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString_Generic(engine);
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "string@ f(const string &in, const mytype &in)", asFUNCTION(MyFunc), asCALL_GENERIC); assert( r >= 0 );
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 1) : Info    : Compiling void Test()\n"
                                 "TestDynamicConfig (5, 9) : Error   : No matching operator that takes the types 'string&' and 'mytype&' found\n" )
	{
		fail = true;
	}

	engine->Release();

	//------------------
	// Test object types held by external variable, i.e. any

	// TODO: The application needs a way to tell the engine that the type is in use so that it won't be removed

/*	CScriptAny *any = 0;
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptAny(engine);

	engine->BeginConfigGroup("group1");
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_PRIMITIVE);
	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Construct), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_ADDREF, "void f()", asFUNCTION(AddRef), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_RELEASE, "void f()", asFUNCTION(Release), asCALL_GENERIC);

	any = (CScriptAny*)engine->CreateScriptObject(engine->GetTypeIdByDecl(0, "any"));

	r = engine->RegisterGlobalProperty("any g_any", any);
	engine->EndConfigGroup();

	mod->AddScriptSection(0, TESTNAME, script5, strlen(script5), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build(0);
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Discard(0);
	engine->GarbageCollect();

	int *o = 0;
	any->Retrieve(&o, engine->GetTypeIdByDecl(0, "mytype@"));
	if( o == 0 )
		fail = true;
	if( --(*o) != 1 )
		fail = true;

	// The mytype variable is still stored in the any variable so we shouldn't be allowed to remove it's configuration group
	r = engine->RemoveConfigGroup("group1"); assert( r < 0 );

	any->Release();
	engine->GarbageCollect();

	// Now it should be possible to remove the group
	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	engine->Release();
*/
	//-------------
	// Test array types
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterObjectType("int[]", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	engine->Release();

	//-------------
	// Test object types in struct members
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (3, 3) : Error   : Identifier 'mytype' is not a data type\n" )
	{
		fail = true;
	}

	engine->Release();

	//-------------
	// Test object types in function declarations
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script8, strlen(script8), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script8, strlen(script8), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
								 "TestDynamicConfig (1, 1) : Info    : Compiling void Test(int&in)\n"
		                         "TestDynamicConfig (1, 11) : Error   : Identifier 'mytype' is not a data type\n" )
	{
		fail = true;
	}

	engine->Release();

	//-------------
	// Test object types in script arrays
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("group1"); assert( r >= 0 );
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script9, strlen(script9), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->DiscardModule(0);

	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script9, strlen(script9), 0);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = mod->Build();
	if( r >= 0 || bout.buffer != "TestDynamicConfig (1, 1) : Info    : Compiling void Test()\n"
                                 "TestDynamicConfig (3, 11) : Error   : Expected expression value\n" )
	{
		fail = true;
	}

	engine->Release();


	//------------------
	// Test object types held by external variable, i.e. any
/*	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptAny(engine);

	engine->BeginConfigGroup("group1");
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_PRIMITIVE);

	any = (CScriptAny*)engine->CreateScriptObject(engine->GetTypeIdByDecl(0, "any"));

	r = engine->RegisterGlobalProperty("any g_any", any);
	engine->EndConfigGroup();

	mod->AddScriptSection(0, TESTNAME, script10, strlen(script10), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build(0);
	if( r < 0 )
		fail = true;

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Discard(0);
	engine->GarbageCollect();

	asIScriptArray *array = 0;
	any->Retrieve(&array, engine->GetTypeIdByDecl(0, "mytype[]@"));
	if( array == 0 )
		fail = true;
	else
		array->Release();

	engine->GarbageCollect();

	// The mytype variable is still stored in the any variable so we shouldn't be allowed to remove it's configuration group
	r = engine->RemoveConfigGroup("group1"); assert( r < 0 );

	any->Release();
	engine->GarbageCollect();

	// Now it should be possible to remove the group
	r = engine->RemoveConfigGroup("group1"); assert( r >= 0 );

	engine->Release();
*/
	//-------------------
	// Test references between config groups
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->BeginConfigGroup("group1");
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	engine->EndConfigGroup();

	engine->BeginConfigGroup("group2");
	r = engine->RegisterGlobalFunction("void func(mytype)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
	engine->EndConfigGroup();

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	r = engine->RemoveConfigGroup("group2"); assert( r <= 0 );

	r = engine->RemoveConfigGroup("group1"); assert( r <= 0 );

	engine->Release();

	//--------------------
	// Test situation where the default group references a dynamic group. It will then be impossible
	// to remove the dynamic group, but the application must still be able to release the engine.
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->BeginConfigGroup("group1");
	r = engine->RegisterObjectType("mytype", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	engine->EndConfigGroup();

	r = engine->RegisterGlobalFunction("void func(mytype)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RemoveConfigGroup("group1"); assert( r == asCONFIG_GROUP_IS_IN_USE );

	engine->Release();

	//----------------------
	// Test that it is possible to repeat the registration of the config group
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("g1"); assert( r >= 0 );
	RegisterScriptString_Generic(engine);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->ExecuteString(0, "string a = \"test\""); assert( r == asEXECUTION_FINISHED );

	r = engine->DiscardModule(0);  assert( r >= 0 );
	r = engine->GarbageCollect(); assert( r >= 0 );

	r = engine->RemoveConfigGroup("g1"); assert( r >= 0 );

	// again..
	r = engine->BeginConfigGroup("g1"); assert( r >= 0 );
	RegisterScriptString_Generic(engine);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->ExecuteString(0, "string a = \"test\""); assert( r == asEXECUTION_FINISHED );

	r = engine->DiscardModule(0);  assert( r >= 0 );
	r = engine->GarbageCollect(); assert( r >= 0 );

	r = engine->RemoveConfigGroup("g1"); assert( r >= 0 );

	engine->Release();


	//-----------------------------
	// Test that it isn't possible to register the sample property in two different groups
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->BeginConfigGroup("a");

	r = engine->RegisterGlobalProperty("int a", 0); assert( r >= 0 );

	engine->EndConfigGroup();

	r = engine->RegisterGlobalProperty("int a", 0); assert( r < 0 );

	engine->Release();

	//------------------------------
	// Test that ExecuteString doesn't lock dynamic config groups
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->BeginConfigGroup("g1"); assert( r >= 0 );
	RegisterScriptString_Generic(engine);
	r = engine->EndConfigGroup(); assert( r >= 0 );

	r = engine->ExecuteString(0, "string a = \"test\""); assert( r == asEXECUTION_FINISHED );

	// Garbage collect and remove config group before discarding module
	r = engine->GarbageCollect(); assert( r >= 0 );
	r = engine->RemoveConfigGroup("g1"); assert( r >= 0 );

	engine->Release();

	// Success
	return fail;
}

// This test was reported by Zeu5 on 2009-03-24
// It used to crash on the second call to Build()
bool Test2()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->BeginConfigGroup("MyGroup");
	engine->RegisterInterface("MyHostDefinedInterface");
	engine->RegisterInterfaceMethod("MyHostDefinedInterface", "void doSomething()");
	engine->EndConfigGroup();

	const char *script = "class MyScriptedClass : MyHostDefinedInterface\n"
						 "{\n"
						 "   void doSomething() { /* nothing */ }\n"
						 "};\n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	engine->DiscardModule(0);
	r = engine->RemoveConfigGroup("MyGroup");
	if( r < 0 )
	{
		fail = true;
	}

	// Now do everything again
	engine->BeginConfigGroup("MyGroup");
	engine->RegisterInterface("MyHostDefinedInterface");
	engine->RegisterInterfaceMethod("MyHostDefinedInterface", "void doSomething()");
	engine->EndConfigGroup();

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}

}
