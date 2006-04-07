#include "utils.h"

namespace TestScriptClassMethod
{

#define TESTNAME "TestScriptClassMethod"

// Normal structure
static const char *script1 =
"void Test()                                     \n"
"{                                               \n"
"   myclass a;                                   \n"
"   a.mthd(1);                                   \n"
"   Assert( a.c == 4 );                          \n"
"   mthd2(2);                                    \n"
"   @g = myclass();                              \n"
"   g.deleteGlobal();                            \n"
"}                                               \n"
"class myclass                                   \n"
"{                                               \n"
"   void deleteGlobal()                          \n"
"   {                                            \n"
"      @g = null;                                \n"
"      Analyze(any(@this));                      \n"
"   }                                            \n"
"   void mthd(int a)                             \n"
"   {                                            \n"
"      int b = 3;                                \n"
"      print(\"class:\"+a+\":\"+b);              \n"
"      myclass tmp;                              \n"
"      this = tmp;                               \n"
"      this.c = 4;                               \n"
"   }                                            \n"
"   void mthd2(int a)                            \n"
"   {                                            \n"
"      print(\"class:\"+a);                      \n"
"   }                                            \n"
"   int c;                                       \n"
"};                                              \n"
"void mthd2(int a) { print(\"global:\"+a); }     \n"
"myclass @g;                                     \n";

static const char *script2 =
"class myclass                                   \n"
"{                                               \n"
"  myclass()                                     \n"
"  {                                             \n"
"    print(\"Default constructor\");             \n"
"    this.value = 1;                             \n"
"  }                                             \n"
"  myclass(int a)                                \n"
"  {                                             \n"
"    print(\"Constructor(\"+a+\")\");            \n"
"    this.value = 2;                             \n"
"  }                                             \n"
"  void method()                                 \n"
"  {                                             \n"
"    this.value = 3;                             \n"
"  }                                             \n"
"  int value;                                    \n"
"};                                              \n"
"void Test()                                     \n"
"{                                               \n"
"  myclass c;                                    \n"
"  Assert(c.value == 1);                         \n"
"  myclass d(1);                                 \n"
"  Assert(d.value == 2);                         \n"
"  c = myclass(2);                               \n"
"  Assert(c.value == 2);                         \n"
"}                                               \n";

static const char *script3 = 
"class myclass                                   \n"
"{                                               \n"
"  void func() {}                                \n"
"  void func(int x, int y) {}                    \n"
"};                                              \n"
"myclass c;                                      \n";

void print(std::string &s)
{
//	printf("%s\n", s.c_str());
}

void Analyze(asIScriptAny *a)
{
	int myclassId = a->GetTypeId();
	asIScriptStruct *s = 0;
	a->Retrieve(&s, myclassId);
	s->Release();
}

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);
	engine->RegisterGlobalFunction("void Analyze(any &inout)", asFUNCTION(Analyze), asCALL_CDECL);

	COutStream out;
	CBufferedOutStream bout;
	engine->SetCommonMessageStream(&out);

	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "Test()", &ctx);
	if( r != asEXECUTION_FINISHED ) 
	{
		if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
		fail = true;
	}
	if( ctx ) ctx->Release();

	// Make sure that the error message for wrong constructor name works
	bout.buffer = "";
	engine->SetCommonMessageStream(&bout);
	engine->AddScriptSection(0, TESTNAME, "class t{ s() {} };", 18, 0, false);
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "TestScriptClassMethod (1, 10) : Error   : The constructor name must be the same as the class\n" ) fail = true;

	// Make sure the default constructor can be overloaded
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	int typeId = engine->GetTypeIdByDecl(0, "myclass");
	asIScriptStruct *s = (asIScriptStruct*)engine->CreateScriptObject(typeId);
	if( s == 0 ) 
		fail = true;
	else
	{
		// Validate the property
		int *v = 0;
		int n = s->GetPropertyCount();
		for( int c = 0; c < n; c++ )
		{
			std::string str = "value";
			if( str == s->GetPropertyName(c) )
			{	
				v = (int*)s->GetPropertyPointer(c);
				if( *v != 1 ) fail = true;
			}
		}

		// Call the script class method
		if( engine->GetMethodCount(0, "myclass") != 1 ) fail = true;
		int methodId = engine->GetMethodIDByDecl(0, "myclass", "void method()");
		if( methodId < 0 ) 
			fail = true;
		else
		{
			asIScriptContext *ctx = engine->CreateContext();
			ctx->Prepare(methodId);
			ctx->SetObject(s);
			int r = ctx->Execute();
			if( r != asEXECUTION_FINISHED )
				fail = true;

			if( !v || *v != 3 ) fail = true;

			ctx->Release();
		}

		s->Release();
	}

	engine->Release();

	//----------------------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetCommonMessageStream(&out);
	engine->AddScriptSection(0, "test3", script3, strlen(script3), 0, false);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	int mtdId = engine->GetMethodIDByDecl(0, "myclass", "void func()");
	void *obj = engine->GetGlobalVarPointer(engine->GetGlobalVarIDByName(0, "c"));

	if( mtdId < 0 || obj == 0 ) fail = true;
	else
	{
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mtdId);
		ctx->SetObject(obj);
		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED ) fail = true;
		ctx->Release();
	}

	mtdId = engine->GetMethodIDByDecl(0, "myclass", "void func(int, int)");
	if( mtdId < 0 || obj == 0 ) fail = true;
	else
	{
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mtdId);
		ctx->SetObject(obj);
		ctx->SetArgDWord(0, 1);
		ctx->SetArgDWord(1, 1);
		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED ) fail = true;
		ctx->Release();
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

