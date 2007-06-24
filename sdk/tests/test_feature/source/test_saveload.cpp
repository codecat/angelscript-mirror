//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include <vector>
#include "utils.h"

namespace TestSaveLoad
{

using namespace std;

#define TESTNAME "TestSaveLoad"


class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream() {wpointer = 0;rpointer = 0;}

	void Write(const void *ptr, asUINT size) {if( size == 0 ) return; buffer.resize(buffer.size() + size); memcpy(&buffer[wpointer], ptr, size); wpointer += size;}
	void Read(void *ptr, asUINT size) {memcpy(ptr, &buffer[rpointer], size); rpointer += size;}

	int rpointer;
	int wpointer;
	std::vector<asBYTE> buffer;
};


static const char *script1 =
"import void Test() from \"DynamicModule\";   \n"
"OBJ g_obj;                                   \n"
"A @gHandle;                                  \n"
"void main()                                  \n"
"{                                            \n"
"  Test();                                    \n"
"  TestStruct();                              \n"
"  TestArray();                               \n"
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

static const char *script4 = "      \n\
interface Actor {  }				\n\
InGame g_inGame;                    \n\
class InGame						\n\
{									\n\
	Ship _ship;						\n\
	bool Initialize(int level)		\n\
	{								\n\
		if (!_ship.Initialize())    \n\
			return false;           \n\
									\n\
		return true;                \n\
	}								\n\
}									\n\
class Ship : Actor					\n\
{									\n\
	bool Initialize()				\n\
	{								\n\
		return true;				\n\
	}								\n\
}									\n";



bool fail = false;
int number = 0;
COutStream out;
asIScriptEngine *ConfigureEngine()
{
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalProperty("int number", &number);
	engine->RegisterObjectType("OBJ", sizeof(int), asOBJ_PRIMITIVE);

	return engine;
}

void TestScripts(asIScriptEngine *engine)
{
	int r;

	// Bind the imported functions
	r = engine->BindAllImportedFunctions(0); assert( r >= 0 );

	// Verify if handles are properly resolved
	int funcID = engine->GetFunctionIDByDecl(0, "void TestHandle(string @)");
	if( funcID < 0 ) 
	{
		printf("%s: Failed to identify function with handle\n", TESTNAME);
		fail = true;
	}

	engine->ExecuteString(0, "main()");

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		fail = true;
	}

	// Call an interface method on a class that implements the interface
	int typeId = engine->GetTypeIdByDecl(0, "MyClass");
	asIScriptStruct *obj = (asIScriptStruct*)engine->CreateScriptObject(typeId);

	int intfTypeId = engine->GetTypeIdByDecl(0, "MyIntf");
	int funcId = engine->GetMethodIDByDecl(intfTypeId, "void test()");
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


bool Test()
{
	int r;
		
 	asIScriptEngine *engine = ConfigureEngine();

	engine->AddScriptSection(0, TESTNAME ":1", script1, strlen(script1), 0);
	engine->Build(0);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	TestScripts(engine);

	// Save the compiled byte code
	CBytecodeStream stream;
	engine->SaveByteCode(0, &stream);

	// Test loading without releasing the engine first
	engine->LoadByteCode(0, &stream);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	TestScripts(engine);

	// Test loading for a new engine
	engine->Release();
	engine = ConfigureEngine();

	stream.rpointer = 0;
	engine->LoadByteCode(0, &stream);

	engine->AddScriptSection("DynamicModule", TESTNAME ":2", script2, strlen(script2), 0);
	engine->Build("DynamicModule");

	TestScripts(engine);

	engine->Release();

	//-----------------------------------------
	// A different case
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->AddScriptSection(0, "script3", script3, strlen(script3));
	engine->Build(0);
	CBytecodeStream stream2;
	engine->SaveByteCode(0, &stream2);

	engine->Release();
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->LoadByteCode(0, &stream2);
	engine->ExecuteString(0, "Test(3)");

	engine->Release();

	//-----------------------------------
	// save/load with overloaded array types should work as well
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		int r = engine->RegisterObjectType("float[]", sizeof(vector<float>), asOBJ_CLASS_CDA); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructFloatArray, (vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructFloatArray, (int, vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructFloatArray), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_ASSIGNMENT, "float[] &f(float[]&in)", asMETHODPR(vector<float>, operator=, (const std::vector<float> &), vector<float>&), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_INDEX, "float &f(int)", asMETHODPR(vector<float>, operator[], (vector<float>::size_type), float &), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "int length()", asMETHOD(vector<float>, size), asCALL_THISCALL); assert(r >= 0);
		
		engine->AddScriptSection(0, "script3", script3, strlen(script3));
		engine->Build(0);
		
		CBytecodeStream stream3;
		engine->SaveByteCode(0, &stream3);
		
		engine->Discard(0);
		
		engine->LoadByteCode(0, &stream3);
		engine->ExecuteString(0, "Test(3)");
		
		engine->Release();
	}

	//---------------------------------
	// Must be possible to load scripts with classes declared out of order
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	engine->AddScriptSection(0, "script", script4, strlen(script4));
	r = engine->Build(0);
	if( r < 0 ) 
		fail = true;
	else
	{
		// Test the script with compiled byte code
		r = engine->ExecuteString(0, "g_inGame.Initialize(0);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// Save the bytecode
		CBytecodeStream stream4;
		engine->SaveByteCode(0, &stream4);
		engine->Release();

		// Now load the bytecode into a fresh engine and test the script again
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->LoadByteCode(0, &stream4);
		r = engine->ExecuteString(0, "g_inGame.Initialize(0);");
		if( r != asEXECUTION_FINISHED )
			fail = true;
	}
	engine->Release();

	// Success
	return fail;
}

} // namespace

