#include "utils.h"

namespace TestForEach
{

// Foreach support
// https://www.gamedev.net/forums/topic/717525-implementing-the-range-based-for-loop-aka-foreach/
// ref: https://en.wikipedia.org/wiki/Foreach_loop


// TODO: Make a test case for showing how to deal with probable dangling pointer for a registered type
// For example, given a iterator type
// struct iterator
// {
//   container_t* ptr;
//   uint idx;
// };
//
// And in the registered opForValue (and maybe other operators where applicable),
// check the iterator
// value_t& container_t::opForValue(iterator& iter)
// {
//    if(iter.ptr != this) {
//      /* Raise exception about invalid iterator */
//      return nullptr;
//    }
//    return data[iter.idx];
// }

std::string g_printBuffer;
void Print_Generic(asIScriptGeneric* gen)
{
	std::string *str = (std::string*)gen->GetArgAddress(0);
//	PRINTF("%s", str->c_str());
	g_printBuffer += *str;
}

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;

	// Test const overload
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		const char* range_script =
			"class Range\n"
			"{\n"
			"  int[] data(4);\n"
			"  Range() { data = {1, 2, 3, 4}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) { return iter == 4; }\n"
			"  int& opForValue(uint iter) { return data[iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"  uint opForBegin() const { return 4; }\n" // Make the const version iterate backwards to distinguish
			"  bool opForEnd(uint iter) const { return iter == 0; }\n"
			"  const int& opForValue(uint iter) const { return data[iter-1]; }\n"
			"  uint opForNext(uint iter) const { return iter - 1; }\n"
			"};\n";

		const char* foreach1234 =
			"void Test()\n"
			"{\n"
			"  Range r;\n"
			"  string s;\n"
			"  foreach(int i : r)\n"
			"    s += i;\n"
			"  const Range @c = r; \n"
			"  foreach(int i : c)\n"
			"    s += i;\n"
			"  Print('foreach: ' + s + '\\n');\n"
			"  Assert(s == '12344321');\n"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("range", range_script);
		mod->AddScriptSection("foreach1234", foreach1234);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (g_printBuffer != "foreach: 12344321\n")
		{
			TEST_FAILED;
			PRINTF("%s", g_printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Test complex iterator type
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);

		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Range { \n"
			"  Iter @opForBegin() {return Iter();}\n"
			"  bool opForEnd(Iter@ i) {return i.i >= 4;}\n"
			"  Iter @opForNext(Iter@ i) {i.i++; return i;}\n"
			"  int opForValue(Iter@ i) {return i.i;}\n"
			"} \n"
			"class Iter { \n"
			"  int i = 0; \n"
			"}\n"
			"void equivalent() \n"
			"{ \n"
			"  int sum = 0; \n"
			"  auto@ r = Range(); \n"
			"  for( auto@ i = r.opForBegin(); !r.opForEnd(i); @i = r.opForNext(i) ) \n"
			"  { \n"
			"    auto v = r.opForValue(i); \n"
			"    sum += v; \n"
			"    Print(' ' + v); \n"
			"  } \n"
			"  Assert( sum == 6 ); \n"
			"} \n"
			"void func() \n"
			"{ \n"
			"  int sum = 0; \n"
			"  foreach( auto v : Range() ) \n"
			"  { \n"
			"    sum += v; \n"
			"    Print(' ' + v); \n"
			"  } \n"
			"  Assert( sum == 6 ); \n"
			"} \n");
			
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		asIScriptContext* ctx = engine->CreateContext();
		r = ExecuteString(engine, "equivalent()", mod, ctx);
		if (r != asEXECUTION_FINISHED)
		{
			TEST_FAILED;
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("%s\n", GetExceptionInfo(ctx, false).c_str());
		}
		r = ExecuteString(engine, "func()", mod, ctx);
		if (r != asEXECUTION_FINISHED)
		{
			TEST_FAILED;
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("%s\n", GetExceptionInfo(ctx, false).c_str());
		}
		ctx->Release();

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Test foreach on complex expressions (i.e. not local variable)
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		RegisterScriptArray(engine, false);

		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char* range_script =
			"class Range\n"
			"{\n"
			"  array<int> data(4);\n"
			"  Range() { data = {1, 2, 3, 4}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) const { return iter == 4; }\n"
			"  int& opForValue(uint iter) { return data[iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"};\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("range", range_script);
		mod->AddScriptSection("test",
			"Range global; \n"
			"void func() \n"
			"{ \n"
			"  int sum = 0; \n"
			"  foreach( auto v : global ) \n"
			"    sum += v; \n"
			"  Assert( sum == 10 ); \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		asIScriptContext* ctx = engine->CreateContext();
		r = ExecuteString(engine, "func()", mod, ctx);
		if (r != asEXECUTION_FINISHED)
		{
			TEST_FAILED;
			if (r == asEXECUTION_EXCEPTION)
				PRINTF("%s\n", GetExceptionInfo(ctx, false).c_str());
		}
		ctx->Release();

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Test foreach on invalid expressions
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"void func() { \n"
			"  int a; \n"
			"  foreach( auto v : a ) {} \n"
			"  HalfRange r; \n"
			"  foreach( auto v : r ) {} \n"
			"  BadSignature s; \n"
			"  foreach( auto v : s ) {} \n"
			"} \n"
			"class HalfRange { \n"
			"  int opForBegin() { return 0; } \n"
			"} \n" // missing the methods for opFor...
			"class BadSignature { \n" // methods are available but with wrong function signature
			"  int opForBegin(int) {return 0;} \n"
			"  bool opForEnd(int, int) {return false;} \n"
			"  int opForNext(int, int) {return 0;} \n"
			"  int opForValue(int, int) {return 0;} \n"
			"} \n");
		r = mod->Build();
		if (r >= 0)
			TEST_FAILED;

		if (bout.buffer !=
			"test (1, 1) : Info    : Compiling void func()\n"
			"test (3, 25) : Error   : Type 'int' is not valid type for foreach loops\n"
			"test (5, 25) : Error   : Type 'HalfRange@' is not valid type for foreach loops\n"
			"test (5, 25) : Info    : Missing or invalid definition of 'opForEnd'\n"
			"test (5, 25) : Info    : Missing or invalid definition of 'opForNext'\n"
			"test (5, 25) : Info    : Missing or invalid definition of 'opForValue#'\n"
			"test (7, 25) : Error   : Type 'BadSignature@' is not valid type for foreach loops\n"
			"test (7, 25) : Info    : Missing or invalid definition of 'opForBegin'\n")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Test turning off foreach support
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// When turned on 'foreach' is a reserved keyword
		const char* script = "void foreach() {}";
		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if (r >= 0)
			TEST_FAILED;

		// When turned off 'foreach' can be used normally
		engine->SetEngineProperty(asEP_FOREACH_SUPPORT, false);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != 
			"test (1, 6) : Error   : Expected identifier\n"
			"test (1, 6) : Error   : Instead found reserved keyword 'foreach'\n")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Basic foreach
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		const char* range_script =
			"class Range\n"
			"{\n"
			"  int[] data(4);\n"
			"  Range() { data = {1, 2, 3, 4}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) const { return iter == 4; }\n"
			"  int& opForValue(uint iter) { return data[iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"};\n";

		const char* foreach1234 =
			"void Test()\n"
			"{\n"
			"  Range r;\n"
			"  string s;\n"
			"  foreach(int i : r)\n"
			"    s += i;\n"
			"  Print('foreach: ' + s + '\\n');\n"
			"  Assert(s == '1234');\n"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("range", range_script);
		mod->AddScriptSection("foreach1234", foreach1234);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (g_printBuffer != "foreach: 1234\n")
		{
			TEST_FAILED;
			PRINTF("%s", g_printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}
	
	// auto value type
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		const char* range_script =
			"class Range\n"
			"{\n"
			"  int[] data(4);\n"
			"  Range() { data = {1, 2, 3, 4}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) const { return iter == 4; }\n"
			"  int& opForValue(uint iter) { return data[iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"};\n";

		const char* foreach1234_auto =
			"void Test()\n"
			"{\n"
			"  Range r;\n"
			"  string s;\n"
			"  foreach(auto i : r)\n"
			"    s += i;\n"
			"  Print('auto foreach: ' + s + '\\n');\n"
			"  Assert(s == '1234');\n"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("range", range_script);
		mod->AddScriptSection("foreach1234_auto", foreach1234_auto);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (g_printBuffer != "auto foreach: 1234\n")
		{
			TEST_FAILED;
			PRINTF("%s", g_printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Generator range
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		// Instead of having real elements, this range generates value during iteration,
		// simulating an array.
		// For reference: iota_view in ranges library since C++20
		// https://en.cppreference.com/w/cpp/ranges/iota_view
		const char* generator_script =
			"class Range\n"
			"{\n"
			"  private int start;\n"
			"  private int sentinel;\n"
			"  Range(int count = 4, int init_val = 1) { \n"
			"    start = init_val; sentinel = init_val + count;\n"
			"    Print('generator ' + start + '->' + sentinel + '\\n');\n"
			"  }\n"
			"  int opForBegin() { return start; }\n"
			"  bool opForEnd(int iter) const { return iter == sentinel; }\n"
			"  int opForValue(int iter) { return iter; }\n"
			"  int opForNext(int iter) { return iter + 1; }\n"
			"};\n";

		const char* foreach1234 =
			"void Test()\n"
			"{\n"
			"  Range r;\n"
			"  string s;\n"
			"  foreach(int i : r)\n"
			"    s += i;\n"
			"  Print('foreach: ' + s + '\\n');\n"
			"  Assert(s == '1234');\n"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("generator_range", generator_script);
		mod->AddScriptSection("foreach1234", foreach1234);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (g_printBuffer != 
			"generator 1->5\n"
			"foreach: 1234\n")
		{
			TEST_FAILED;
			PRINTF("%s", g_printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Multiple values
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print_Generic), asCALL_GENERIC);
		g_printBuffer = "";

		// The iterator value is an (index, value) pair
		// This kind of range is called "enumerate"
		// See: https://en.cppreference.com/w/cpp/ranges/enumerate_view
		const char* enum_range_script =
			"class Enumerate\n"
			"{\n"
			"  int[] data(4);\n"
			"  Enumerate() { data = {101, 102, 103, 104}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) const { return iter == 4; }\n"
			"  uint opForValue0(uint iter) { return iter; }\n"
			"  int& opForValue1(uint iter) { return data[iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"};\n"
			"void Test(){\n"
			"  Enumerate r;\n"
			"  string s;\n"
			"  foreach(uint idx, int val : r)\n"
			"  {\n"
			"    if(idx != 0) s += ', ';\n"
			"    s += idx + ': ' + val;\n"
			"  }\n"
			"  Print('foreach(idx, val : r): ' + s + '\\n');\n"
			"  Assert(s == '0: 101, 1: 102, 2: 103, 3: 104');"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("enum_range", enum_range_script);

		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (g_printBuffer != "foreach(idx, val : r): 0: 101, 1: 102, 2: 103, 3: 104\n")
		{
			TEST_FAILED;
			PRINTF("%s", g_printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Multiple values, more tests
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		RegisterScriptArray(engine, true);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		// The dataset value is an (index, value1, value2)
		const char* enum_range_script =
			"class Dataset\n"
			"{\n"
			"  int[] data(4);\n"
			"  Dataset() { data = {101, 102, 103, 104}; }\n"
			"  uint opForBegin() { return 0; }\n"
			"  bool opForEnd(uint iter) const { return iter == 4; }\n"
			"  uint opForValue0(uint iter) { return iter; }\n"
			"  int opForValue1(uint iter) { return data[iter]; }\n"
			"  int opForValue2(uint iter) { return data[3-iter]; }\n"
			"  uint opForNext(uint iter) { return iter + 1; }\n"
			"};\n"
			"void Test(){\n"
			"  Dataset r;\n"
			"  string s;\n"
			"  foreach(uint idx, int val1, int val2 : r)\n"
			"  {\n"
			"    if(idx != 0) s += ', ';\n"
			"    s += idx + ': ' + val1 + '.' + val2; \n"
			"  }\n"
			"  Assert(s == '0: 101.104, 1: 102.103, 2: 103.102, 3: 104.101');"
			"  s = '';\n"
			"  foreach(uint idx : r)\n"
			"  {\n"
			"    s += idx + ' ';\n"
			"  }\n"
			"  Assert( s == '0 1 2 3 ' );\n"
			"}\n";

		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("enum_range", enum_range_script);
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if (r != asEXECUTION_FINISHED)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			TEST_FAILED;
			PRINTF("%s", bout.buffer.c_str());
		}

		engine->ShutDownAndRelease();
	}

	// Success
	return fail;
}
}