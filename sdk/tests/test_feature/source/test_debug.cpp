
#include <stdarg.h>
#include "utils.h"
#include "../../add_on/scriptany/scriptany.h"

namespace TestDebug
{

static const char *script1 =
"import void Test2() from \"Module2\";  \n"
"funcdef void func_t();                 \n"
"void main()                            \n"
"{                                      \n"
"  int a = 1;                           \n"
"  string s = \"text\";                 \n" // 6
"  c _c; _c.Test1();                    \n" // 7
"  func_t @t = main;                    \n"
"  Test2();                             \n"
"}                                      \n"
"class c                                \n" 
"{                                      \n"
"  void Test1()                         \n" // 13
"  {                                    \n"
"    int d = 4;                         \n"
"  }                                    \n"
"}                                      \n";

static const char *script2 =
"void Test2()           \n"
"{                      \n"
"  int b = 2;           \n"
"  Test3();             \n" // 4
"}                      \n"
"void Test3()           \n"
"{                      \n"
"  int c = 3;           \n"
"  int[] a;             \n" // 9
"  a[0] = 0;            \n" // 10
"}                      \n";


std::string printBuffer;

static const char *correct =
"Module1:void main():5,3\n"
"Module1:void main():5,3\n"
"Module1:void main():6,3\n"
"Module1:void main():7,3\n"
" Module1:c@ c():0,0\n"
"  Module1:c::c():0,0\n"
"Module1:void main():7,9\n"
" Module1:void c::Test1():15,5\n"
" Module1:void c::Test1():15,5\n"
" Module1:void c::Test1():16,4\n"
"Module1:void main():8,3\n"
"Module1:void main():9,3\n"
" Module2:void Test2():3,3\n"
" Module2:void Test2():3,3\n"
" Module2:void Test2():4,3\n"
"  Module2:void Test3():8,3\n"
"  Module2:void Test3():8,3\n"
"  Module2:void Test3():9,3\n"
"   (null):int[]@ $fact():0,0\n"
"  Module2:void Test3():10,3\n"
"--- exception ---\n"
"desc: Index out of bounds\n"
"func: void Test3()\n"
"modl: Module2\n"
"sect: :2\n"
"line: 10,3\n"
" (in scope) int c = 3\n"
" (in scope) int[] a = {...}\n"
"--- call stack ---\n"
"Module2:void Test2():4,3\n"
" (in scope) int b = 2\n"
"Module1:void main():9,3\n"
" (in scope) int a = 1\n"
" (in scope) string s = 'text'\n"
" (in scope) c _c = {...}\n"
" (in scope) func_t@ t = {...}\n"
"--- exception ---\n"
"desc: Index out of bounds\n"
"func: void Test3()\n"
"modl: Module2\n"
"sect: :2\n"
"line: 10,3\n"
" (in scope) int c = 3\n"
" (in scope) int[] a = {...}\n"
"--- call stack ---\n"
"Module2:void Test2():4,3\n"
" (in scope) int b = 2\n"
"Module1:void main():9,3\n"
" (in scope) int a = 1\n"
" (in scope) string s = 'text'\n"
" (in scope) c _c = {...}\n"
" (in scope) func_t@ t = {...}\n";

void print(const char *format, ...)
{
	char buf[256];
	va_list args;
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	printBuffer += buf;
}

void PrintVariables(asIScriptContext *ctx, asUINT stackLevel);

void LineCallback(asIScriptContext *ctx, void * /*param*/)
{
	if( ctx->GetState() != asEXECUTION_ACTIVE )
		return;

	int col;
	int line = ctx->GetLineNumber(0, &col);
	int indent = ctx->GetCallstackSize();
	for( int n = 1; n < indent; n++ )
		print(" ");
	const asIScriptFunction *function = ctx->GetFunction();
	print("%s:%s:%d,%d\n", function->GetModuleName(),
	                    function->GetDeclaration(),
	                    line, col);

//	PrintVariables(ctx, 0);
}

void LineCallback2(asIScriptContext* ctx, void* /*param*/)
{
	if (ctx->GetState() != asEXECUTION_ACTIVE)
		return;

	int col;
	int line = ctx->GetLineNumber(0, &col);
	int indent = ctx->GetCallstackSize();
	for (int n = 1; n < indent; n++)
		print(" ");
	const asIScriptFunction* function = ctx->GetFunction();
	print("%s:%s:%d,%d\n", function->GetModuleName(),
		function->GetDeclaration(),
		line, col);

	if( line == 9 || line == 14 )
		PrintVariables(ctx, 0);
}

void LineCallback3(asIScriptContext* ctx, void* /*param*/)
{
	if (ctx->GetState() != asEXECUTION_ACTIVE)
		return;

	int col;
	int line = ctx->GetLineNumber(0, &col);
	int indent = ctx->GetCallstackSize();
	for (int n = 1; n < indent; n++)
		print(" ");
	const asIScriptFunction* function = ctx->GetFunction();
	print("%s:%s:%d,%d\n", function->GetModuleName(),
		function->GetDeclaration(),
		line, col);

	if (line == 8 || line == 13 || line == 18)
		PrintVariables(ctx, 0);
}

void LineCallback4(asIScriptContext* ctx, void* /*param*/)
{
	if (ctx->GetState() != asEXECUTION_ACTIVE)
		return;

	int col;
	int line = ctx->GetLineNumber(0, &col);
	int indent = ctx->GetCallstackSize();
	for (int n = 1; n < indent; n++)
		print(" ");
	const asIScriptFunction* function = ctx->GetFunction();
	print("%s:%s:%d,%d\n", function->GetModuleName(),
		function->GetDeclaration(),
		line, col);

	if (line == 6)
	{
//		PrintVariables(ctx, 0); // print variables in Test()
		PrintVariables(ctx, 1); // print variables in main()
	}
}

bool doSkipTemporary = true;
void PrintVariables(asIScriptContext *ctx, asUINT stackLevel)
{
	asIScriptEngine *engine = ctx->GetEngine();

	int typeId = ctx->GetThisTypeId(stackLevel);
	void *varPointer = ctx->GetThisPointer(stackLevel);
	if( typeId )
	{
		print(" this = 0x%x\n", varPointer);
	}

	int numVars = ctx->GetVarCount(stackLevel);
	for( int n = 0; n < numVars; n++ )
	{
		// Skip temporary variables
		const char* name;
		ctx->GetVar(n, stackLevel, &name, &typeId);
		if ((name == 0 || strlen(name) == 0) && doSkipTemporary)
			continue;

		bool inScope = false;
		if (ctx->IsVarInScope(n, stackLevel))
		{
			print(" (in scope)");
			inScope = true;
		}
		else
			print(" (no scope)");

		varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if( typeId == engine->GetTypeIdByDecl("int") )
		{
			if (inScope)
				print(" %s%s = %d\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "", *(int*)varPointer);
			else
				print(" %s%s = {uninitialized}\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
		else if (typeId == engine->GetTypeIdByDecl("uint"))
		{
			if (inScope)
				print(" %s%s = %u\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "", *(asUINT*)varPointer);
			else
				print(" %s%s = {uninitialized}\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
		else if( typeId == engine->GetTypeIdByDecl("string") )
		{
			CScriptString *str = (CScriptString*)varPointer;
			if( str )
				print(" %s%s = '%s'\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "", str->buffer.c_str());
			else
				print(" %s%s = <null>\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
		else if (typeId == engine->GetTypeIdByDecl("foo"))
		{
			if (varPointer)
				print(" %s%s = %d\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "", *(int*)varPointer);
			else
				print(" %s%s = <null>\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
		else if(typeId == engine->GetTypeIdByDecl("vec3_t"))
		{
			if (varPointer)
				print(" %s%s = {%f,%f,%f}\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "", *(float*)varPointer, *((((float*)varPointer))+1), *((((float*)varPointer)) + 2));
			else
				print(" %s%s = <null>\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
		else
		{
			if( varPointer )
				print(" %s%s = {...}\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
			else
				print(" %s%s = <null>\n", ctx->GetVarDeclaration(n, stackLevel), (name == 0 || strlen(name) == 0) ? "(temp)" : "");
		}
	}
}

void ExceptionCallback(asIScriptContext *ctx, void * /*param*/)
{
	const asIScriptFunction *function = ctx->GetExceptionFunction();

	print("--- exception ---\n");
	print("desc: %s\n", ctx->GetExceptionString());
	print("func: %s\n", function->GetDeclaration());
	print("modl: %s\n", function->GetModuleName());
	const char* sectionName = 0;
	int col, line = ctx->GetExceptionLineNumber(&col, &sectionName);
	print("sect: %s\n", sectionName ? sectionName : "");
	print("line: %d,%d\n", line, col);

	// Print the variables in the current function
	PrintVariables(ctx, 0);

	// Show the call stack with the variables
	print("--- call stack ---\n");
	for( asUINT n = 1; n < ctx->GetCallstackSize(); n++ )
	{
		const asIScriptFunction *func = ctx->GetFunction(n);
		line = ctx->GetLineNumber(n,&col);
		print("%s:%s:%d,%d\n", func->GetModuleName(),
		                       func->GetDeclaration(),
							   line, col);
		PrintVariables(ctx, n);
	}
}

class TestLN
{
public:
void *TestLineNumber()
{
    asIScriptContext *ctx = asGetActiveContext();
    const char *script_section;
    /*int line =*/ ctx->GetLineNumber(0, 0, &script_section);
	assert( std::string(script_section) == "a" );
    return 0;
}
};

int& foo_opAssign(int val, int* _this)
{
	*_this = val;
	return *_this;
}

bool Test2();

struct vec3_t
{
	float x, y, z;
};

void vec3_fff(float x, float y, float z, vec3_t*v)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

void vec3_v3(const vec3_t& o, vec3_t* v)
{
	*v = o;
}

bool Test()
{
	int r;
	bool fail = Test2();

	// Test GetAddressOfVar for object variables whose stack position is reused in multiple scopes
	// Reported by Paril
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("vec3_t", sizeof(vec3_t), asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectBehaviour("vec3_t", asBEHAVE_CONSTRUCT, "void vec3_t(float, float, float)", asFUNCTION(vec3_fff), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectBehaviour("vec3_t", asBEHAVE_CONSTRUCT, "void vec3_t(const vec3_t &in)", asFUNCTION(vec3_v3), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectProperty("vec3_t", "float x", asOFFSET(vec3_t, x));
		engine->RegisterObjectProperty("vec3_t", "float y", asOFFSET(vec3_t, y));
		engine->RegisterObjectProperty("vec3_t", "float z", asOFFSET(vec3_t, z));


		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", R"script(
void Test(vec3_t v)
{
    vec3_t a, b, c;

    v.x = 50 + b.x + a.x + c.x;
}

bool Check() { return true; }

void main(bool is_cgame)
{
    vec3_t(1, 2, 3);
    vec3_t(1, 2, 3);

    {
        vec3_t test_a(1,2,3);
        test_a.x++;
    }

    if (Check())
    {
        if (!is_cgame)
        {
            vec3_t test(3,2,1);
            Test(test);
        }
    }
}
)script");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		printBuffer = "";
		asIScriptContext* ctx = engine->CreateContext();
		doSkipTemporary = true;
		ctx->SetLineCallback(asFUNCTION(LineCallback4), 0, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), 0, asCALL_CDECL);
		ctx->Prepare(mod->GetFunctionByDecl("void main(bool)"));
		ctx->SetArgByte(0, false);
		r = ctx->Execute();
		if (r == asEXECUTION_EXCEPTION)
		{
			// It is possible to examine the callstack even after the Execute() method has returned
			ExceptionCallback(ctx, 0);
		}
		ctx->Release();
		doSkipTemporary = true;
		engine->ShutDownAndRelease();

		if (printBuffer !=
			R"out(test:void main(bool):13,5
test:void main(bool):13,5
test:void main(bool):14,5
test:void main(bool):17,9
test:void main(bool):18,9
test:void main(bool):21,5
 test:bool Check():9,16
 test:bool Check():9,16
test:void main(bool):23,9
test:void main(bool):25,13
test:void main(bool):26,13
 test:void Test(vec3_t):4,5
 test:void Test(vec3_t):4,5
 test:void Test(vec3_t):6,5
 (in scope) bool is_cgame = {...}
 (no scope) vec3_t test_a = <null>
 (no scope) vec3_t test = <null>
 test:void Test(vec3_t):7,2
test:void main(bool):29,2
)out")
		{
			TEST_FAILED;
			PRINTF("%s", printBuffer.c_str());
		}

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	// Test GetDeclaredAt for functions
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"int a; \n"
			"void func() { \n"
			"  a = a + 1; \n"
			"} \n"
			"class C { \n"
			"  void method() { \n"
			"    a = a - 1; \n"
			"  } \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		const char* scriptSection;
		int row;
		int col;
		r = mod->GetFunctionByName("func")->GetDeclaredAt(&scriptSection, &row, &col);
		if (r < 0)
			TEST_FAILED;
		if ( std::string(scriptSection) != "test" || row != 2 || col != 1 )
		{
			PRINTF("Got %s, %d, %d\n", scriptSection, row, col);
			TEST_FAILED;
		}

		r = mod->GetTypeInfoByName("C")->GetMethodByName("method")->GetDeclaredAt(&scriptSection, &row, &col);
		if (r >= 0) // virtual methods do not have a place to store the declared at as they are just stubs
			TEST_FAILED;
		if (scriptSection != 0 || row != 0 || col != 0)
		{
			PRINTF("Got %s, %d, %d\n", scriptSection, row, col);
			TEST_FAILED;
		}

		r = mod->GetTypeInfoByName("C")->GetMethodByName("method",false)->GetDeclaredAt(&scriptSection, &row, &col);
		if (r < 0)
			TEST_FAILED;
		if (std::string(scriptSection) != "test" || row != 6 || col != 3)
		{
			PRINTF("Got %s, %d, %d\n", scriptSection, row, col);
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test GetLineNumber right after Prepare
	// Reported by Sam Tupy
	{
		asIScriptEngine* engine = asCreateScriptEngine();
		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"int a; \n"
			"void func() { \n"
			"  a = a + 1; \n"
			"} \n"
			"class C { \n"
			"  void method() { \n"
			"    a = a - 1; \n"
			"  } \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		if (bout.buffer != "")
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByName("func"));
		int ln = ctx->GetLineNumber();
		ctx->Release();

		if (ln != 3) // this is the first line of actual code in the function
		{
			PRINTF("Got %d\n", ln);
			TEST_FAILED;
		}

		ctx = engine->CreateContext();
		ctx->Prepare(mod->GetTypeInfoByName("C")->GetMethodByName("method"));
		ln = ctx->GetLineNumber();
		ctx->Release();

		if (ln != 0)  // this is the virtual method, it has no code
		{
			PRINTF("Got %d\n", ln);
			TEST_FAILED;
		}

		ctx = engine->CreateContext();
		ctx->Prepare(mod->GetTypeInfoByName("C")->GetMethodByName("method", false));
		ln = ctx->GetLineNumber();
		ctx->Release();

		if (ln != 7)  // now it is the actual method, so the line number should be the first line with code
		{
			PRINTF("Got %d\n", ln);
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Test IsVarInScope for variables declared in for-loop
	// Reported by gmp3 labs
	{
		printBuffer = "";

		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		asIScriptModule* mod = engine->GetModule("Module1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":1",
			"void main() \n"
			"{ \n"
			"	for (int i = 0; i < 1; i++) \n"  // 3
			"	{ \n"
			"		bool hey = false; \n"        // 5
			"	} \n"
			"	for (int j = 0; j < 1; j++) \n"  // 7
			"	{ \n"
			"		bool hello = false; \n"      // 9
			"	} \n"
			"	int foo = 9; \n"                 // 11
			"	print(666); \n"  //<<<DEBUG HERE // 12
			"} \n"                               // 13
			"void print(int) {} \n");            // 14
		mod->Build();

		asIScriptContext* ctx = engine->CreateContext();
		doSkipTemporary = false;
		ctx->SetLineCallback(asFUNCTION(LineCallback3), 0, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), 0, asCALL_CDECL);
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();
		if (r == asEXECUTION_EXCEPTION)
		{
			// It is possible to examine the callstack even after the Execute() method has returned
			ExceptionCallback(ctx, 0);
		}
		ctx->Release();
		doSkipTemporary = true;
		engine->ShutDownAndRelease();

		if (printBuffer !=
			"Module1:void main():3,7\n"
			"Module1:void main():3,7\n"
			"Module1:void main():3,18\n"
			"Module1:void main():3,7\n"
			"Module1:void main():5,3\n"
			"Module1:void main():3,25\n"
			"Module1:void main():3,18\n"
			"Module1:void main():7,7\n"
			"Module1:void main():7,18\n"
			"Module1:void main():7,7\n"
			"Module1:void main():9,3\n"
			"Module1:void main():7,25\n"
			"Module1:void main():7,18\n"
			"Module1:void main():11,2\n"
			"Module1:void main():12,2\n"
			" Module1:void print(int):14,19\n"
			" Module1:void print(int):14,19\n"
			"Module1:void main():13,2\n"
			" (no scope) int i = {uninitialized}\n"
			" (no scope) bool hey = {...}\n"
			" (no scope) int j = {uninitialized}\n"
			" (no scope) bool hello = {...}\n"
			" (in scope) int foo = 9\n"
			" (in scope) int (temp) = 0\n")
		{
			TEST_FAILED;
			PRINTF("%s", printBuffer.c_str());
		}
	}

	// Test IsVarInScope and GetAddresOfVar for registered value types when variable slot is reused in multiple scopes
	// https://www.gamedev.net/forums/topic/712251-debugger-and-var-in-scope/
	{
		printBuffer = "";

		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);
		RegisterScriptAny(engine);
		engine->RegisterObjectType("foo", 4, asOBJ_VALUE | asOBJ_POD);
#ifndef AS_MAX_PORTABILITY
		r = engine->RegisterObjectMethod("foo", "foo &opAssign(int)", asFUNCTION(foo_opAssign), asCALL_CDECL_OBJLAST); assert(r >= 0);
#else
		r = engine->RegisterObjectMethod("foo", "foo &opAssign(int)", WRAP_OBJ_LAST(foo_opAssign), asCALL_GENERIC); assert(r >= 0);
#endif

		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		asIScriptModule* mod = engine->GetModule("Module1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":1",
			"interface IUnknown {} \n"
			"class Script { void addNamedItem(const string &in, IUnknown @, foo) {}}"
			"Script script;"
			"void main() { \n"
			"  array<array<any>@> connectedAddins; \n"
			"  foo f = 42; array<any>@ i = {any(null),any('test'),any(f)}; \n"
			"  connectedAddins.insertLast(@i); \n"
			"  { \n"
			"    string name2 = 'pre-scope'; \n"
			"    string test2; \n"
			"  } \n"
			"  for (uint k = 0, km = connectedAddins.length(); k < km; k++) { \n"
			"    array<any>@ a = connectedAddins[k]; \n"
			"    IUnknown@ obj; \n"
			"    string name; \n"
			"    foo global; \n"
			"    a[0].retrieve(@obj); \n"
			"    a[1].retrieve(name); \n"
			"    a[2].retrieve(global); \n"
			"    script.addNamedItem(name, obj, global); \n" // << there is breakpoint and check variable 'name' and 'global'
			"  } \n"
			"} \n");
		mod->Build();

		asIScriptContext* ctx = engine->CreateContext();
		ctx->SetLineCallback(asFUNCTION(LineCallback3), 0, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), 0, asCALL_CDECL);
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();
		if (r == asEXECUTION_EXCEPTION)
		{
			// It is possible to examine the callstack even after the Execute() method has returned
			ExceptionCallback(ctx, 0);
		}
		ctx->Release();
		engine->ShutDownAndRelease();

		if (printBuffer !=
			"Module1:void main():3,3\n"
			"Module1:void main():3,3\n"
			" (null):array<array<any>@>@ $fact():0,0\n"
			"Module1:void main():4,3\n"
			"Module1:void main():4,15\n"
			" (null):array<any>@ $list(int&in) { repeat any }:0,0\n"
			"Module1:void main():5,3\n"
			"Module1:void main():7,5\n"
			"Module1:void main():8,5\n"
			" (in scope) array<array<any>@> connectedAddins = {...}\n"
			" (in scope) foo f = 42\n"
			" (in scope) array<any>@ i = {...}\n"
			" (in scope) string name2 = 'pre-scope'\n"
			" (no scope) string test2 = <null>\n"
			" (no scope) uint k = {uninitialized}\n"
			" (no scope) uint km = {uninitialized}\n"
			" (no scope) array<any>@ a = {...}\n"
			" (no scope) IUnknown@ obj = {...}\n"
			" (no scope) string name = <null>\n"
			" (no scope) foo global = <null>\n"
			"Module1:void main():10,8\n"
			"Module1:void main():10,51\n"
			"Module1:void main():10,8\n"
			"Module1:void main():11,5\n"
			"Module1:void main():13,5\n"
			" (in scope) array<array<any>@> connectedAddins = {...}\n"
			" (in scope) foo f = 42\n"
			" (in scope) array<any>@ i = {...}\n"
			" (no scope) string name2 = <null>\n"
			" (no scope) string test2 = <null>\n"
			" (in scope) uint k = 0\n"
			" (in scope) uint km = 1\n"
			" (in scope) array<any>@ a = {...}\n"
			" (in scope) IUnknown@ obj = {...}\n"
			" (no scope) string name = <null>\n"
			" (no scope) foo global = <null>\n"
			"Module1:void main():14,5\n"
			"Module1:void main():15,5\n"
			"Module1:void main():16,5\n"
			"Module1:void main():17,5\n"
			"Module1:void main():18,5\n"
			" (in scope) array<array<any>@> connectedAddins = {...}\n"
			" (in scope) foo f = 42\n"
			" (in scope) array<any>@ i = {...}\n"
			" (no scope) string name2 = <null>\n"
			" (no scope) string test2 = <null>\n"
			" (in scope) uint k = 0\n"
			" (in scope) uint km = 1\n"
			" (in scope) array<any>@ a = {...}\n"
			" (in scope) IUnknown@ obj = {...}\n"
			" (in scope) string name = 'test'\n"
			" (in scope) foo global = 42\n"
			" Module1:void Script::addNamedItem(const string&in, IUnknown@, foo):2,71\n"
			" Module1:void Script::addNamedItem(const string&in, IUnknown@, foo):2,71\n"
			"Module1:void main():10,59\n"
			"Module1:void main():10,51\n"
			"Module1:void main():20,2\n")
		{
			TEST_FAILED;
			PRINTF("%s", printBuffer.c_str());
		}
	}

	// Test IsVarInScope and GetAddresOfVar for registered value types
	// https://www.gamedev.net/forums/topic/712251-debugger-and-var-in-scope/
	{
		printBuffer = "";

		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);
		RegisterScriptAny(engine);
		engine->RegisterObjectType("foo", 4, asOBJ_VALUE | asOBJ_POD);
#ifndef AS_MAX_PORTABILITY
		engine->RegisterObjectMethod("foo", "foo &opAssign(int)", asFUNCTION(foo_opAssign), asCALL_CDECL_OBJLAST);
#else
		engine->RegisterObjectMethod("foo", "foo &opAssign(int)", WRAP_OBJ_LAST(foo_opAssign), asCALL_GENERIC);
#endif

		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		asIScriptModule* mod = engine->GetModule("Module1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":1", 
			"interface IUnknown {} \n"
			"class Script { void addNamedItem(const string &in, IUnknown @, foo) {}}"
			"Script script;"
			"void main() { \n"
			"  array<array<any>@> connectedAddins; \n"
			"  foo f = 42; array<any>@ i = {any(null),any('test'),any(f)}; \n"
			"  connectedAddins.insertLast(@i); \n"
			"  for (uint k = 0, km = connectedAddins.length(); k < km; k++) { \n"
			"    array<any>@ a = connectedAddins[k]; \n"
			"    IUnknown@ obj; \n"
			"    string name; \n"
			"    foo global; \n"
			"    a[0].retrieve(@obj); \n"
			"    a[1].retrieve(name); \n"
			"    a[2].retrieve(global); \n"
			"    script.addNamedItem(name, obj, global); \n" // << there is breakpoint and check variable 'name' and 'global'
			"  } \n"
			"} \n");
		mod->Build();

		asIScriptContext* ctx = engine->CreateContext();
		ctx->SetLineCallback(asFUNCTION(LineCallback2), 0, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), 0, asCALL_CDECL);
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();
		if (r == asEXECUTION_EXCEPTION)
		{
			// It is possible to examine the callstack even after the Execute() method has returned
			ExceptionCallback(ctx, 0);
		}
		ctx->Release();
		engine->ShutDownAndRelease();

		if( printBuffer != 
			"Module1:void main():3,3\n"
			"Module1:void main():3,3\n"
			" (null):array<array<any>@>@ $fact():0,0\n"
			"Module1:void main():4,3\n"
			"Module1:void main():4,15\n"
			" (null):array<any>@ $list(int&in) { repeat any }:0,0\n"
			"Module1:void main():5,3\n"
			"Module1:void main():6,8\n"
			"Module1:void main():6,51\n"
			"Module1:void main():6,8\n"
			"Module1:void main():7,5\n"
			"Module1:void main():9,5\n"
			" (in scope) array<array<any>@> connectedAddins = {...}\n"
			" (in scope) foo f = 42\n"
			" (in scope) array<any>@ i = {...}\n"
			" (in scope) uint k = 0\n"
			" (in scope) uint km = 1\n"
			" (in scope) array<any>@ a = {...}\n"
			" (in scope) IUnknown@ obj = {...}\n"
			" (no scope) string name = <null>\n"
			" (no scope) foo global = <null>\n"
			"Module1:void main():10,5\n"
			"Module1:void main():11,5\n"
			"Module1:void main():12,5\n"
			"Module1:void main():13,5\n"
			"Module1:void main():14,5\n"
			" (in scope) array<array<any>@> connectedAddins = {...}\n"
			" (in scope) foo f = 42\n"
			" (in scope) array<any>@ i = {...}\n"
			" (in scope) uint k = 0\n"
			" (in scope) uint km = 1\n"
			" (in scope) array<any>@ a = {...}\n"
			" (in scope) IUnknown@ obj = {...}\n"
			" (in scope) string name = 'test'\n"
			" (in scope) foo global = 42\n"
			" Module1:void Script::addNamedItem(const string&in, IUnknown@, foo):2,71\n"
			" Module1:void Script::addNamedItem(const string&in, IUnknown@, foo):2,71\n"
			"Module1:void main():6,59\n"
			"Module1:void main():6,51\n"
			"Module1:void main():16,2\n" )
		{
			TEST_FAILED;
			PRINTF("%s", printBuffer.c_str());
		}
	}

	// GetTypeByDeclaration shouldn't write message on incorrect declaration
	// http://www.gamedev.net/topic/649322-getting-rid-of-angelscript-prints/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterStdString(engine);

		asITypeInfo *t = engine->GetTypeInfoByName("wrong decl");
		if( t != 0 ) TEST_FAILED;
		int i = engine->GetTypeIdByDecl("wrong decl");
		if( i >= 0 ) TEST_FAILED;
		asIScriptFunction *func = engine->GetGlobalFunctionByDecl("wrong decl");
		if( func != 0 ) TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test FindNextLineWithCode
	// Reported by Scott Bean
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		asIScriptModule *mod = engine->GetModule("Test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"int Function() \n" // 1
			"{ \n"              // 2
			"    // comment \n" // 3
			"    return 1; \n"  // 4
			"} \n");            // 5
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptFunction *func = mod->GetFunctionByName("Function");
		int line = func->FindNextLineWithCode(0);
		if( line != -1 )
			TEST_FAILED;
		line = func->FindNextLineWithCode(1);
		if( line != 4 )
			TEST_FAILED;
		line = func->FindNextLineWithCode(2);
		if( line != 4 )
			TEST_FAILED;
		line = func->FindNextLineWithCode(3);
		if( line != 4 )
			TEST_FAILED;
		line = func->FindNextLineWithCode(4);
		if( line != 4 )
			TEST_FAILED;
		line = func->FindNextLineWithCode(5);
		if( line != -1 )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test FindNextLineWithCode for class destructors
	// Reported by Scott Bean
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		RegisterStdString(engine);

		asIScriptModule* mod = engine->GetModule("Test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class ssNode \n"
			"{ \n"
			"    ssNode()  \n"
			"    { \n"
			"        n = 42; \n"
			"    } \n"
			"    int n; \n"
			"    string str; \n"
			"} \n");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		asITypeInfo* type = mod->GetTypeInfoByName("ssNode");
		asEBehaviours behave;
		asUINT behaveCount = type->GetBehaviourCount();
		if (behaveCount != 10)
			TEST_FAILED;
		asIScriptFunction* func = type->GetBehaviourByIndex(8, &behave);
		if (behave != asBEHAVE_CONSTRUCT)
			TEST_FAILED;
		int line = func->FindNextLineWithCode(5);
		if (line != 5)
			TEST_FAILED;
		line = func->FindNextLineWithCode(8);
		if (line != 8)
			TEST_FAILED;

		engine->Release();
	}

	// Test crash in GetLineNumber
	// http://www.gamedev.net/topic/638656-crash-in-ctx-getlinenumber/
	SKIP_ON_MAX_PORT
	{
		asIScriptEngine * engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->RegisterInterface("foo");
		engine->RegisterObjectType("Test", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectMethod("Test", "foo @TestLineNumber()", asMETHOD(TestLN, TestLineNumber), asCALL_THISCALL);

		TestLN t;
		engine->RegisterGlobalProperty("Test test", &t);

		asIScriptModule* mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("b"," // nothing to compile");
		mod->AddScriptSection("a","foo @f = test.TestLineNumber();");
		r = mod->Build();
		if (r < 0)
			TEST_FAILED;

		CBytecodeStream stream("test");
		mod->SaveByteCode(&stream);

		engine->Release();

		// Load the bytecode
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->RegisterInterface("foo");
		engine->RegisterObjectType("Test", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectMethod("Test", "foo @TestLineNumber()", asMETHOD(TestLN, TestLineNumber), asCALL_THISCALL);
		engine->RegisterGlobalProperty("Test test", &t);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if (r < 0)
			TEST_FAILED;

		engine->Release();
	}

	// Test basic debugging capability
	{
		printBuffer = "";
		int number = 0;

		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, true);
		RegisterScriptString_Generic(engine);

		// Test GetTypeIdByDecl
		if (engine->GetTypeIdByDecl("int") != engine->GetTypeIdByDecl("const int"))
			TEST_FAILED;
		if (engine->GetTypeIdByDecl("string") != engine->GetTypeIdByDecl("const string"))
			TEST_FAILED;

		// A handle to a const is different from a handle to a non-const
		if (engine->GetTypeIdByDecl("string@") == engine->GetTypeIdByDecl("const string@"))
			TEST_FAILED;

		// Test debugging
		engine->RegisterGlobalProperty("int number", &number);

		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		asIScriptModule* mod = engine->GetModule("Module1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":1", script1, strlen(script1), 0);
		mod->Build();

		mod = engine->GetModule("Module2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":2", script2, strlen(script2), 0);
		mod->Build();

		// Bind all functions that the module imports
		mod = engine->GetModule("Module1");
		mod->BindAllImportedFunctions();

		asIScriptContext* ctx = engine->CreateContext();
		ctx->SetLineCallback(asFUNCTION(LineCallback), 0, asCALL_CDECL);
		ctx->SetExceptionCallback(asFUNCTION(ExceptionCallback), 0, asCALL_CDECL);
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();
		if (r == asEXECUTION_EXCEPTION)
		{
			// It is possible to examine the callstack even after the Execute() method has returned
			ExceptionCallback(ctx, 0);
		}
		ctx->Release();
		engine->Release();

		if (printBuffer != correct)
		{
			TEST_FAILED;
			PRINTF("%s", printBuffer.c_str());
		}
	}

	// Success
	return fail;
}

//----------------------------------------------

// In this test we'll use the debug functions to update a script parameter directly on the stack

void DebugCall()
{
	asIScriptContext *ctx = asGetActiveContext();

	// Get the address of the output parameter
	void *varPointer = ctx->GetAddressOfVar(0, 0);

	// We got the address to the handle
	CScriptString **str = (CScriptString**)varPointer;

	// Set the handle to point to a new string
	*str = new CScriptString("test");

	// It is possible to determine the function that is being called too
	asIScriptFunction *func = ctx->GetSystemFunction();
	assert( std::string(func->GetDeclaration()) == "void debugCall()" );
}

bool Test2()
{
	RET_ON_MAX_PORT

	bool fail = false;
	COutStream out;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->RegisterGlobalFunction("void debugCall()", asFUNCTION(DebugCall), asCALL_CDECL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	const char *script = 
		"void func(string@ &out output) \n"
		"{ \n"
		"  debugCall(); \n"
		"  assert( output == 'test' ); \n"
		"} \n";

	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	r = ExecuteString(engine, "string @o; func(o); assert( o == 'test' );", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();

	return fail;
}

} // namespace

