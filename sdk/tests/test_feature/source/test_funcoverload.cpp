#include "utils.h"

namespace TestFuncOverload
{
static const char* const TESTNAME = "TestFuncOverload";

class Obj
{
public:
	void* p;
	void* Value() { return p; }
	void Set(const std::string&, void*) {}
};

static Obj o;

void FuncVoid()
{
}

void FuncInt(int)
{
}

bool Test2();

bool Test()
{
	bool fail = Test2();
	COutStream out;
	CBufferedOutStream bout;

	// Test function overload between obj@ and const obj@
	// Reported by Patrick Jeeves
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);


		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		const char* script1 =
			"int called = 0; \n"
			"MeshComponent@ GetMesh(GameObject@, int part = 0) { called = 1; return null; } \n"
			"const MeshComponent@ GetMesh(const GameObject@, int part = 0) { called = 2; return null; } \n"
			"class MeshComponent {} \n"
			"class GameObject {} \n";
		mod->AddScriptSection(TESTNAME, script1);
		int r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		r = ExecuteString(engine, "GameObject @obj; const GameObject @cobj = obj; GetMesh(cobj); assert( called == 2 );", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		r = ExecuteString(engine, "GameObject @obj; GetMesh(obj); assert( called == 1 );", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test function overload where one option is ?&in
	// https://www.gamedev.net/forums/topic/715439-recent-change-fixing-funtion-overload-between-obj-and-const-obj-broke-a-lot-of-bindings/5460950/
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		RegisterStdString(engine);

		engine->RegisterGlobalFunction("void PushID(const ?&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void PushID(const string &in)", asFUNCTION(0), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		const char* script1 =
			"class wstring { string opImplConv() const { return ''; } } \n"
			"void main() { \n"
			"  wstring s; \n"
			"  PushID(s); \n"
			"} \n";
		mod->AddScriptSection(TESTNAME, script1);
		int r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test function overload when there are two options, one taking obj@ and another taking const obj@
	// Reported by Patrick Jeeves
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		const char* script1 =
			"class Foo { int k; }; \n"
			"int Func0(Foo@) { return 1; } \n"
			"int Func0(const Foo@) { return 2; } \n"
			"void main() { Foo f; \nassert( Func0(f) == 1 ); \nassert( Func0(cast<const Foo>(f)) == 2 ); } \n";
		mod->AddScriptSection(TESTNAME, script1);
		int r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "main()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		engine->ShutDownAndRelease();
	}

	// Test basic function overload
	SKIP_ON_MAX_PORT
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);

		engine->RegisterObjectType("Data", sizeof(void*), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

		engine->RegisterObjectType("Obj", sizeof(Obj), asOBJ_REF | asOBJ_NOHANDLE);
		engine->RegisterObjectMethod("Obj", "Data &Value()", asMETHOD(Obj, Value), asCALL_THISCALL);
		engine->RegisterObjectMethod("Obj", "void Set(string &in, Data &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
		engine->RegisterObjectMethod("Obj", "void Set(string &in, string &in)", asMETHOD(Obj, Set), asCALL_THISCALL);
		engine->RegisterGlobalProperty("Obj TX", &o);

		engine->RegisterGlobalFunction("void func()", asFUNCTION(FuncVoid), asCALL_CDECL);
		engine->RegisterGlobalFunction("void func(int)", asFUNCTION(FuncInt), asCALL_CDECL);

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		const char* script1 =
			"void Test()                               \n"
			"{                                         \n"
			"  TX.Set(\"user\", TX.Value());           \n"
			"}                                         \n";
		mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
		int r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		r = ExecuteString(engine, "func(func(3));", mod);
		if (r != asEXECUTION_FINISHED) TEST_FAILED;

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		const char* script2 =
			"void ScriptFunc(void m)                   \n"
			"{                                         \n"
			"}                                         \n";
		mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
		r = mod->Build();
		if (r >= 0)
			TEST_FAILED;
		if (bout.buffer != "TestFuncOverload (1, 1) : Info    : Compiling void ScriptFunc(void)\n"
			"TestFuncOverload (1, 1) : Error   : Parameter type can't be 'void', because the type cannot be instantiated.\n")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Permit void parameter list
		r = engine->RegisterGlobalFunction("void func2(void)", asFUNCTION(FuncVoid), asCALL_CDECL); assert(r >= 0);

		// Don't permit void parameters
		r = engine->RegisterGlobalFunction("void func3(void n)", asFUNCTION(FuncVoid), asCALL_CDECL); assert(r < 0);

		engine->Release();
	}

	return fail;
}

// This test verifies that it is possible to find a best match even if the first argument
// may give a better match for another function. Also the order of the function declarations
// should not affect the result.
bool Test2()
{
	bool fail = false;
	COutStream out;
	int r;

	asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	const char* script1 =
		"class A{}  \n"
		"class B{}  \n"
		"int choice; \n"
		"void func(A&in, A&in) { choice = 1; } \n"
		"void func(const A&in, const B&in) { choice = 2; } \n"
		"void test()  \n"
		"{ \n"
		"  A a; B b; \n"
		"  func(a,b); \n"
		"  assert( choice == 2 ); \n"
		"}\n";

	asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script1, strlen(script1));
	r = mod->Build();
	if (r < 0)
		TEST_FAILED;
	r = ExecuteString(engine, "test()", mod);
	if (r != asEXECUTION_FINISHED)
		TEST_FAILED;

	const char* script2 =
		"class A{}  \n"
		"class B{}  \n"
		"int choice; \n"
		"void func(const A&in, const B&in) { choice = 1; } \n"
		"void func(A&in, A&in) { choice = 2; } \n"
		"void test()  \n"
		"{ \n"
		"  A a; B b; \n"
		"  func(a,b); \n"
		"  assert( choice == 1 ); \n"
		"}\n";

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script2, strlen(script2));
	r = mod->Build();
	if (r < 0)
		TEST_FAILED;
	r = ExecuteString(engine, "test()", mod);
	if (r != asEXECUTION_FINISHED)
		TEST_FAILED;

	const char* script3 =
		"int choice = 0; \n"
		"void func(int, float, double) { choice = 1; } \n"
		"void func(float, int, double) { choice = 2; } \n"
		"void func(double, float, int) { choice = 3; } \n"
		"void main() \n"
		"{ \n"
		"  func(1, 1.0f, 1.0); assert( choice == 1 ); \n" // perfect match
		"  func(1.0f, 1, 1.0); assert( choice == 2 ); \n" // perfect match
		"  func(1.0, 1.0f, 1); assert( choice == 3 ); \n" // perfect match
		"  func(1.0, 1, 1); assert( choice == 3 ); \n" // second arg converted
		"  func(1.0f, 1.0, 1.0); assert( choice == 2 ); \n" // second arg converted
		"  func(1.0f, 1.0f, 1); assert( choice == 3 ); \n" // first arg converted
		"} \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script3);
	r = mod->Build();
	if (r < 0)
		TEST_FAILED;
	r = ExecuteString(engine, "main()", mod);
	if (r != asEXECUTION_FINISHED)
		TEST_FAILED;

	const char* script4 =
		"class A { \n"
		"  void func(int) { choice = 1; } \n"
		"  void func(int) const { choice = 2; } \n"
		"} \n"
		"int choice; \n"
		"void main() \n"
		"{ \n"
		"  A@ a = A(); \n"
		"  const A@ b = A(); \n"
		"  a.func(1); assert( choice == 1 ); \n"
		"  b.func(1); assert( choice == 2 ); \n"
		"} \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script4);
	r = mod->Build();
	if (r < 0)
		TEST_FAILED;
	r = ExecuteString(engine, "main()", mod);
	if (r != asEXECUTION_FINISHED)
		TEST_FAILED;

	const char* script5 =
		"void func(int8, double a = 1.0) { choice = 1; } \n"
		"void func(float) { choice = 2; } \n"
		"int choice; \n"
		"void main() \n"
		"{ \n"
		"  func(1); assert( choice == 1 ); \n"
		"} \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script5);
	r = mod->Build();
	if (r < 0)
		TEST_FAILED;
	r = ExecuteString(engine, "main()", mod);
	if (r != asEXECUTION_FINISHED)
		TEST_FAILED;

	engine->Release();

	return fail;
}

} // end namespace
