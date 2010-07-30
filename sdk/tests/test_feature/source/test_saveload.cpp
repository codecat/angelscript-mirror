//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include <memory>
#include <vector>
#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"


namespace TestSaveLoad
{

using namespace std;

static const char * const TESTNAME = "TestSaveLoad";



static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"OBJ g_obj;                                   \n"
"A @gHandle;                                  \n"
"enum ETest {}                                \n"
"void main()                                  \n"
"{                                            \n"
"  Test();                                    \n"
"  TestStruct();                              \n"
"  TestArray();                               \n"
"  GlobalCharArray.resize(1);                 \n"
"  string @s = ARRAYTOHEX(GlobalCharArray);   \n"
"}                                            \n"
"void TestObj(OBJ &out obj)                   \n"
"{                                            \n"
"}                                            \n"
"void TestStruct()                            \n"
"{                                            \n"
"  A a;                                       \n"
"  a.a = 2;                                   \n"
"  A@ b = @a;                                 \n"
"}                                            \n"
"void TestArray()                             \n"
"{                                            \n"
"  A[] c(3);                                  \n"
"  int[] d(2);                                \n"
"  A[]@[] e(1);                               \n"
"  @e[0] = @c;                                \n"
"}                                            \n"
"class A                                      \n"
"{                                            \n"
"  int a;                                     \n"
"  ETest e;                                   \n"
"};                                           \n"
"void TestHandle(string @str)                 \n"
"{                                            \n"
"}                                            \n"
"interface MyIntf                             \n"
"{                                            \n"
"  void test();                               \n"
"}                                            \n"
"class MyClass : MyIntf                       \n"
"{                                            \n"
"  void test() {number = 1241;}               \n"
"}                                            \n";

static const char *script2 =
"void Test()                               \n"
"{                                         \n"
"  int[] a(3);                             \n"
"  a[0] = 23;                              \n"
"  a[1] = 13;                              \n"
"  a[2] = 34;                              \n"
"  if( a[0] + a[1] + a[2] == 23+13+34 )    \n"
"    number = 1234567890;                  \n"
"}                                         \n";

static const char *script3 = 
"float[] f(5);       \n"
"void Test(int a) {} \n";

static const char *script4 = 
"class CheckCollision                          \n"
"{                                             \n"
"	Actor@[] _list1;                           \n"
"                                              \n"
"	void Initialize() {                        \n"
"		_list1.resize(1);                      \n"
"	}                                          \n"
"                                              \n"
"	void Register(Actor@ entity){              \n"
"		@_list1[0] = @entity;                  \n"
"	}                                          \n"
"}                                             \n"
"                                              \n"
"CheckCollision g_checkCollision;              \n"
"                                              \n"
"class Shot : Actor {                          \n"
"	void Initialize() {                        \n"
"		g_checkCollision.Register(this);       \n"
"	}                                          \n"
"}                                             \n"
"interface Actor {  }				           \n"
"InGame g_inGame;                              \n"
"class InGame					   	           \n"
"{									           \n"
"	Ship _ship;						           \n"
"	void Initialize(int level)		           \n"
"	{								           \n"
"		g_checkCollision.Initialize();         \n"
"		_ship.Initialize();	                   \n"
"	}						   		           \n"
"}									           \n"
"class Ship : Actor							   \n"
"{											   \n"
"   Shot@[] _shots;							   \n"
"	void Initialize()						   \n"
"	{										   \n"
"		_shots.resize(5);					   \n"
"                                              \n"
"		for (int i=0; i < 5; i++)              \n"
"		{                                      \n"
"			Shot shot;						   \n"
"			@_shots[i] = @shot;                \n"
"			_shots[i].Initialize();	           \n"
"		}                                      \n"
"	}										   \n"
"}											   \n";

// Make sure the handle can be explicitly taken for class properties, array members, and global variables
static const char *script5 =
"IsoMap      _iso;                                      \n"
"IsoSprite[] _sprite;                                   \n"
"                                                       \n"
"int which = 0;                                         \n"
"                                                       \n"
"bool Initialize() {                                    \n"
"  if (!_iso.Load(\"data/iso/map.imp\"))                \n"
"    return false;                                      \n"
"                                                       \n"
"  _sprite.resize(100);                                 \n"
"                                                       \n"
"  if (!_sprite[0].Load(\"data/iso/pacman.spr\"))       \n"
"    return false;                                      \n"
"                                                       \n"
"  for (int i=1; i < 100; i++) {                        \n"
"    if (!_sprite[i].Load(\"data/iso/residencia1.spr\"))\n"
"      return false;                                    \n"
"  }                                                    \n"
"                                                       \n"
"                                                       \n"
"   _iso.AddEntity(_sprite[0], 0, 0, 0);                \n"
"                                                       \n"
"   return true;                                        \n"
"}                                                      \n";

bool fail = false;
int number = 0;
int number2 = 0;
COutStream out;
asIScriptArray* GlobalCharArray = 0;

void ArrayToHexStr(asIScriptGeneric *gen)
{
}

asIScriptEngine *ConfigureEngine(int version)
{
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);

	// Register a property with the built-in array type
	GlobalCharArray = (asIScriptArray*)engine->CreateScriptObject(engine->GetTypeIdByDecl("uint8[]"));
	int r = engine->RegisterGlobalProperty("uint8[] GlobalCharArray", GlobalCharArray); assert( r >= 0 );

	// Register function that use the built-in array type
	r = engine->RegisterGlobalFunction("string@ ARRAYTOHEX(uint8[] &in)", asFUNCTION(ArrayToHexStr), asCALL_GENERIC); assert( r >= 0 );


	if( version == 1 )
	{
		// The order of the properties shouldn't matter
		engine->RegisterGlobalProperty("int number", &number);
		engine->RegisterGlobalProperty("int number2", &number2);
	}
	else
	{
		// The order of the properties shouldn't matter
		engine->RegisterGlobalProperty("int number2", &number2);
		engine->RegisterGlobalProperty("int number", &number);
	}
	engine->RegisterObjectType("OBJ", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

	return engine;
}

void TestScripts(asIScriptEngine *engine)
{
	int r;

	// Bind the imported functions
	asIScriptModule *mod = engine->GetModule(0);
	r = mod->BindAllImportedFunctions(); assert( r >= 0 );

	// Verify if handles are properly resolved
	int funcID = mod->GetFunctionIdByDecl("void TestHandle(string @)");
	if( funcID < 0 ) 
	{
		printf("%s: Failed to identify function with handle\n", TESTNAME);
		fail = true;
	}

	ExecuteString(engine, "main()", mod);

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Call an interface method on a class that implements the interface
	int typeId = engine->GetModule(0)->GetTypeIdByDecl("MyClass");
	asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(typeId);

	int intfTypeId = engine->GetModule(0)->GetTypeIdByDecl("MyIntf");
	asIObjectType *type = engine->GetObjectTypeById(intfTypeId);
	int funcId = type->GetMethodIdByDecl("void test()");
	asIScriptContext *ctx = engine->CreateContext();
	r = ctx->Prepare(funcId);
	if( r < 0 ) fail = true;
	ctx->SetObject(obj);
	ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		fail = true;

	if( ctx ) ctx->Release();
	if( obj ) obj->Release();

	if( number != 1241 )
	{
		printf("%s: Interface method failed\n", TESTNAME);
		fail = true;
	}
}

void ConstructFloatArray(vector<float> *p)
{
	new(p) vector<float>;
}

void ConstructFloatArray(int s, vector<float> *p)
{
	new(p) vector<float>(s);
}

void DestructFloatArray(vector<float> *p)
{
	p->~vector<float>();
}

void IsoMapFactory(asIScriptGeneric *gen)
{
	*(int**)gen->GetAddressOfReturnLocation() = new int(1);
}

void IsoSpriteFactory(asIScriptGeneric *gen)
{
	*(int**)gen->GetAddressOfReturnLocation() = new int(1);
}

void DummyAddref(asIScriptGeneric *gen)
{
	int *object = (int*)gen->GetObject();
	(*object)++;
}

void DummyRelease(asIScriptGeneric *gen)
{
	int *object = (int*)gen->GetObject();
	(*object)--;
	if( *object == 0 )
		delete object;
}

void Dummy(asIScriptGeneric *gen)
{
}

static string _out;
void output(asIScriptGeneric *gen)
{
	string *str = (string*)gen->GetArgAddress(0);
	_out += *str;
}

bool Test2();

bool Test()
{
	int r;
	COutStream out;
		
	Test2();

 	asIScriptEngine *engine = ConfigureEngine(0);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":1", script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
		fail = true;

	// Validate the number of global functions
	if( mod->GetFunctionCount() != 5 )
		fail = true;

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":2", script2, strlen(script2), 0);
	mod->Build();

	TestScripts(engine);
	asUINT currentSize, totalDestroyed, totalDetected;
	engine->GetGCStatistics(&currentSize, &totalDestroyed, &totalDetected);

	// Save the compiled byte code
	CBytecodeStream stream(__FILE__"1");
	mod = engine->GetModule(0);
	mod->SaveByteCode(&stream);

	if( stream.buffer.size() != 1438 ) 
	{
		// Originally this was 3213
		printf("The saved byte code is not of the expected size. It is %d bytes\n", stream.buffer.size());
		fail = true;
	}

	asUINT zeroes = stream.CountZeroes();
	if( zeroes != 442 )
	{
		printf("The saved byte code contains a different amount of zeroes than expected. Counted %d\n", zeroes);
	}

	// Test loading without releasing the engine first
	mod->LoadByteCode(&stream);

	if( mod->GetFunctionCount() != 5 )
		fail = true;

	if( string(mod->GetFunctionDescriptorByIndex(0)->GetScriptSectionName()) != ":1" )
		fail = true;

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":2", script2, strlen(script2), 0);
	mod->Build();

	TestScripts(engine);

	// Test loading for a new engine
	GlobalCharArray->Release();
	GlobalCharArray = 0;

	engine->Release();
	engine = ConfigureEngine(1);

	stream.Restart();
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->LoadByteCode(&stream);

	if( mod->GetFunctionCount() != 5 )
		fail = true;

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":2", script2, strlen(script2), 0);
	mod->Build();

	TestScripts(engine);
	asUINT currentSize2, totalDestroyed2, totalDetected2;
	engine->GetGCStatistics(&currentSize2, &totalDestroyed2, &totalDetected2);
	assert( currentSize == currentSize2 &&
		    totalDestroyed == totalDestroyed2 &&
			totalDetected == totalDetected2 );

	GlobalCharArray->Release();
	GlobalCharArray = 0;
	engine->Release();

	//-----------------------------------------
	// A different case
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script3", script3, strlen(script3));
	mod->Build();
	CBytecodeStream stream2(__FILE__"2");
	mod->SaveByteCode(&stream2);

	engine->Release();
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->LoadByteCode(&stream2);
	ExecuteString(engine, "Test(3)", mod);

	engine->Release();

	//-----------------------------------
	// save/load with overloaded array types should work as well
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		int r = engine->RegisterObjectType("float[]", sizeof(vector<float>), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructFloatArray, (vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructFloatArray, (int, vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructFloatArray), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "float[] &opAssign(float[]&in)", asMETHODPR(vector<float>, operator=, (const std::vector<float> &), vector<float>&), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_INDEX, "float &f(int)", asMETHODPR(vector<float>, operator[], (vector<float>::size_type), float &), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "int length()", asMETHOD(vector<float>, size), asCALL_THISCALL); assert(r >= 0);
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script3", script3, strlen(script3));
		mod->Build();
		
		CBytecodeStream stream3(__FILE__"3");
		mod->SaveByteCode(&stream3);
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream3);
		ExecuteString(engine, "Test(3)", mod);
		
		engine->Release();
	}

	//---------------------------------
	// Must be possible to load scripts with classes declared out of order
	// Built-in array types must be able to be declared even though the complete script structure hasn't been loaded yet
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r < 0 ) 
		fail = true;
	else
	{
		// Test the script with compiled byte code
		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "g_inGame.Initialize(0);", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
			fail = true;
		}
		if( ctx ) ctx->Release();

		// Save the bytecode
		CBytecodeStream stream4(__FILE__"4");
		mod = engine->GetModule(0);
		mod->SaveByteCode(&stream4);
		engine->Release();

		// Now load the bytecode into a fresh engine and test the script again
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream4);
		r = ExecuteString(engine, "g_inGame.Initialize(0);", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}
	engine->Release();
	//----------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("IsoSprite", sizeof(int), asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_FACTORY, "IsoSprite@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("IsoSprite", "IsoSprite &opAssign(const IsoSprite &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("IsoSprite", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("IsoMap", sizeof(int), asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_FACTORY, "IsoMap@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("IsoMap", "IsoMap &opAssign(const IsoMap &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("IsoMap", "bool AddEntity(const IsoSprite@+, int col, int row, int layer)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("IsoMap", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script5, strlen(script5));
	r = mod->Build();
	if( r < 0 ) 
		fail = true;
	else
	{
		// Test the script with compiled byte code
		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "Initialize();", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
			fail = true;
		}
		if( ctx ) ctx->Release();

		// Save the bytecode
		CBytecodeStream stream(__FILE__"5");
		mod = engine->GetModule(0);
		mod->SaveByteCode(&stream);
		engine->Release();

		// Now load the bytecode into a fresh engine and test the script again
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("IsoSprite", sizeof(int), asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_FACTORY, "IsoSprite@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoSprite", "IsoSprite &opAssign(const IsoSprite &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoSprite", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("IsoMap", sizeof(int), asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_FACTORY, "IsoMap@ f()", asFUNCTION(IsoMapFactory), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "IsoMap &opAssign(const IsoMap &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "bool AddEntity(const IsoSprite@+, int col, int row, int layer)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream);
		r = ExecuteString(engine, "Initialize();", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}
	engine->Release();

	//------------------------------
	// Test to make sure the script constants are stored correctly
	{
		const char *script = "void main()                 \n"
		"{                                                \n"
		"	int i = 123;                                  \n"
		"                                                 \n"
		"   output( ' i = (' + i + ')' + 'aaa' + 'bbb' ); \n"
		"}                                                \n";
		
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		r = engine->RegisterGlobalFunction("void output(const string &in)", asFUNCTION(output), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		ExecuteString(engine, "main()", mod);
		if( _out != " i = (123)aaabbb" )
			fail = true;

		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		r = engine->RegisterGlobalFunction("void output(const string &in)", asFUNCTION(output), asCALL_GENERIC); assert( r >= 0 );
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		
		mod->LoadByteCode(&stream);

		_out = "";
		ExecuteString(engine, "main()", mod);
		if( _out != " i = (123)aaabbb" )
			fail = true;

		engine->Release();
	}

	//-------------------------------
	// Test that registered template classes are stored correctly
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"void main() \n"
			"{ \n"
			"	array< int > intArray = {0,1,2}; \n"
			"	uint tmp = intArray.length(); \n"
			"   assert( tmp == 3 ); \n"
			"}; \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream);

		if( ExecuteString(engine, "main()", mod) != asEXECUTION_FINISHED )
			fail = true;
		
		engine->Release();
	}

	// Test loading script with out of order template declarations 
	{
		const char *script = 
			"class HogeManager \n"
			"{ \n"
			"  array< Hoge >@ hogeArray; \n"
			"} \n"
			"class Hoge {}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			fail = true;

		engine->Release();
	}

	// Test loading script with out of order template declarations 
	{
		const char *script = 
			"class HogeManager \n"
			"{ \n"
			"  HogeManager() \n"
			"  { \n"
			"    array< Hoge >@ hogeArray; \n"
			"  } \n"
			"} \n"
			"class Hoge {}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			fail = true;

		engine->Release();
	}

	// Test loading byte code that uses the enum types
	{
		const char *script = 
			"array< ColorKind > COLOR_KIND_TABLE = { ColorKind_Red }; \n"
			"enum ColorKind { ColorKind_Red }; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			fail = true;

		engine->Release();
	}

	// Test loading and executing bytecode
	{
		const char *script = 
			"interface IObj {}; \n"
		    "class Hoge : IObj {}; \n"
		    "void main() \n"
		    "{ \n"
		    "    Hoge h; \n"
		    "    IObj@ objHandle = h; \n"
		    "    Hoge@ hogeHandle = cast< Hoge@ >( objHandle ); \n"
		    "}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			fail = true;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->Release();
	}

	// Test that property offsets are properly mapped
	{
		asQWORD test;

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterObjectType("test", 8, asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectProperty("test", "int a", 0);
		engine->RegisterObjectProperty("test", "int b", 4);

		engine->RegisterGlobalProperty("test t", &test);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, "void func() { t.a = 1; t.b = 2; }");
		mod->Build();

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		if( *(int*)(&test) != 1 || *((int*)(&test)+1) != 2 )
			fail = true;

		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterObjectType("test", 8, asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectProperty("test", "int a", 4); // Switch order of the properties
		engine->RegisterObjectProperty("test", "int b", 0);

		engine->RegisterGlobalProperty("test t", &test);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			fail = true;

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		if( *(int*)(&test) != 2 || *((int*)(&test)+1) != 1 )
			fail = true;

		engine->Release();
	}

	// Success
	return fail;
}

bool Test2()
{
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	const char *script = 
		"enum ENUM1{          \n"
		"_ENUM_1 = 1          \n"
		"}                    \n"
		"void main()          \n"
		"{                    \n"
		"int item = _ENUM_1;  \n"
		"}                    \n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		fail = true;

	CBytecodeStream stream(__FILE__"6");
	mod = engine->GetModule(0);
	r = mod->SaveByteCode(&stream);
	if( r < 0 )
		fail = true;
	engine->Release();

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->LoadByteCode(&stream);
	if( r < 0 )
		fail = true;

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		fail = true;

	engine->Release();

	return fail;
}

} // namespace

