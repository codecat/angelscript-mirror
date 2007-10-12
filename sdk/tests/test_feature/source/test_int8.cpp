#include "utils.h"

namespace TestInt8
{

#define TESTNAME "TestInt8"

char RetInt8(char in)
{
	if( in != 1 )
	{
		printf("failed to pass parameter correctly\n");
	}
	return 1;
}

class TestInt8Class
{
public:
	TestInt8Class()
	{
		m_fail = false;
	}

	void Test1(char value)
	{
		if( value != 1 )
		{
			m_fail = true;
		}
	}

	void Test0(char value)
	{
		if( value != 0 )
		{
			m_fail = true;
		}
	}

	bool m_fail;
};

static const char *script3 =
"void TestInt8ToMember()           \n"
"{                                 \n"
"   int8 flag = 1;                 \n"
"   TestInt8Class.Test1(flag);     \n"
"   flag = 0;                      \n"
"   TestInt8Class.Test0(flag);     \n"
"}                                 \n";


bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	int r;
	bool fail = false;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	// We'll test two things with this function
	// 1. The native interface is able to pass byte parameters correctly
	// 2. The native interface is able to return byte values correctly
	engine->RegisterGlobalFunction("int8 RetInt8(int8)", asFUNCTION(RetInt8), asCALL_CDECL);

	char var = 0;
	engine->RegisterGlobalProperty("int8 gvar", &var);

	engine->ExecuteString(0, "gvar = RetInt8(1)");
	if( var != 1 )
	{
		printf("failed to return value correctly\n");
		fail = true;
	}
	
	engine->ExecuteString(0, "Assert(RetInt8(1) == 1)");

	
	// Test to make sure bools can be passed to member functions properly
	engine->RegisterObjectType("Int8Tester", 0, asOBJ_REF | asOBJ_NOHANDLE);
	engine->RegisterObjectMethod("Int8Tester", "void Test1(int8)", asMETHOD(TestInt8Class, Test1), asCALL_THISCALL);
	engine->RegisterObjectMethod("Int8Tester", "void Test0(int8)", asMETHOD(TestInt8Class, Test0), asCALL_THISCALL);	
	TestInt8Class testInt8;
	r = engine->RegisterGlobalProperty("Int8Tester TestInt8Class", &testInt8 );
	if( r < 0 ) fail = true;
	engine->AddScriptSection(0, "script", script3, strlen(script3));
	r = engine->Build(0);
	if( r < 0 )
	{
		fail = true;
	}
	else
	{
		r = engine->ExecuteString(0, "TestInt8ToMember();");
		if( r != asEXECUTION_FINISHED ) fail = true;

		if( testInt8.m_fail ) fail = true;
	}
	
	
	engine->Release();

	return fail;
}

} // namespace

