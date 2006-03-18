#include "utils.h"
#include "../../add_on/std_vector/stdvector.h"


namespace TestStdVector
{

#define TESTNAME "TestStdVector"


static void print(std::string &str)
{
	printf("%s", str.c_str());
}

static void print(int num)
{
	printf("%d", num);
}

static void Assert(bool expr)
{
	if( !expr )
	{
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Assert failed");
	}
}

static const char *script1 =
"void Test()                         \n"
"{                                   \n"
"   TestInt();                       \n"
"   TestChar();                      \n"
"   Test2D();                        \n"
"}                                   \n"
"                                    \n"
"void TestInt()                      \n"
"{                                   \n"
"   int[] A(5);                      \n"
"   Assert(A.size() == 5);           \n"
"	A.push_back(6);                  \n"
"   Assert(A.size() == 6);           \n"
"	int[] B(A);                      \n"
"   Assert(B.size() == 6);           \n"
"	A.pop_back();                    \n"
"   Assert(B.size() == 6);           \n"
"   Assert(A.size() == 5);           \n"
"	A = B;                           \n"
"   Assert(A.size() == 6);           \n"
"	A.resize(8);                     \n"
"   Assert(A.size() == 8);           \n"
"	A[1] = 20;                       \n"
"	Assert(A[1] == 20);              \n"
"}                                   \n"
"                                    \n"
"void TestChar()                     \n"
"{                                   \n"
"   int8[] A(5);                     \n"
"   Assert(A.size() == 5);           \n"
"   A.push_back(6);                  \n"
"   Assert(A.size() == 6);           \n"
"   int8[] B(A);                     \n"
"   Assert(B.size() == 6);           \n"
"   A.pop_back();                    \n"
"   Assert(B.size() == 6);           \n"
"   Assert(A.size() == 5);           \n"
"   A = B;                           \n"
"   Assert(A.size() == 6);           \n"
"   A.resize(8);                     \n"
"   Assert(A.size() == 8);           \n"
"   A[1] = 20;                       \n"
"   Assert(A[1] == 20);              \n"
"}                                   \n"
"                                    \n"
"void Test2D()                       \n"
"{                                   \n"
"   int[][] A(2);                    \n"
"   int[] B(2);                      \n"
"   A[0] = B;                        \n"
"   A[1] = B;                        \n"
"                                    \n"
"   A[0][0] = 0;                     \n"
"   A[0][1] = 1;                     \n"
"   A[1][0] = 2;                     \n"
"   A[1][1] = 3;                     \n"
"                                    \n"
"   Assert(A[0][0] == 0);            \n"
"   Assert(A[0][1] == 1);            \n"
"   Assert(A[1][0] == 2);            \n"
"   Assert(A[1][1] == 3);            \n"
"}                                   \n";

using namespace std;

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterStdString(engine);
	RegisterVector<char>("int8[]", "int8", engine);
	RegisterVector<int>("int[]", "int", engine);
#ifdef __GNUC__
	RegisterVector< std::vector<int> >("int[][]", "int[]", engine);
#else
	// There is something going wrong when registering the following.
	// It looks like it is a linker problem, but I can't be sure.
	printf("%s: MSVC can't register vector< vector<int> >\n", TESTNAME);
#endif
	engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTIONPR(print, (std::string&), void), asCALL_CDECL);
	engine->RegisterGlobalFunction("void Print(int)", asFUNCTIONPR(print, (int), void), asCALL_CDECL);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script1, strlen(script1), 0);
	int r = engine->Build(0, &out);
	if( r < 0 )
	{
		fail = true;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	asIScriptContext *ctx = 0;
	r = engine->ExecuteString(0, "Test()", 0, &ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failed to execute script\n", TESTNAME);

		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		
		fail = true;
	}
	
	if( ctx ) ctx->Release();
	engine->Release();

	// Success
	return fail;
}

} // namespace

