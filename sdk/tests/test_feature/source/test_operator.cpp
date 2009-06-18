#include "utils.h"

namespace TestOperator
{


class AppVal
{
public:
	AppVal() { value = 0; }
	int value;

	bool operator == (const AppVal &o) const
	{
		return value == o.value;
	}
};

void Construct(AppVal *a)
{
	new(a) AppVal();
}

AppVal g_AppVal;
AppVal &GetAppValRef(AppVal &a)
{
	a.value = 1;
	return g_AppVal;
}

bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;
	asIScriptEngine *engine = 0;
	asIScriptModule *mod = 0;

	//----------------------------------------------
	// opEquals for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operator ==
			"  bool opEquals(const Test &in o) const \n"
			"  {                                     \n"
			"    return value == o.value;            \n"
			"  }                                     \n"
			// The operator can be overloaded for different types
			"  bool opEquals(int o)             \n"
			"  {                                \n"
			"    return value == o;             \n"
			"  }                                \n"
			// opEquals that don't return bool are ignored
			"  int opEquals(float o)            \n"
			"  {                                \n"
			"    return 0;                      \n"
			"  }                                \n"
			"}                                  \n"
			"Test func()                        \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return a;                        \n"
			"}                                  \n"
			"Test @funcH()                      \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return @a;                       \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test a,b,c;                      \n"
			"  a.value = 0;                     \n"
			"  b.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  assert( a == b );                \n"  // a.opEquals(b)
			"  assert( a.opEquals(b) );         \n"  // Same as a == b
			"  assert( a == 0 );                \n"  // a.opEquals(0)
			"  assert( 0 == a );                \n"  // a.opEquals(0)
			"  assert( a == 0.1f );             \n"  // a.opEquals(int(0.1f))
			"  assert( a != c );                \n"  // !a.opEquals(c)
			"  assert( a != 1 );                \n"  // !a.opEquals(1)
			"  assert( 1 != a );                \n"  // !a.opEquals(1)
			"  assert( !a.opEquals(c) );        \n"  // Same as a != c
			"  assert( a == func() );           \n"
			"  assert( a == funcH() );          \n"
			"  assert( func() == a );           \n"
			"  assert( funcH() == a );          \n"
			"}                                  \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			fail = true;
		}
		
		r = engine->ExecuteString(0, "main()");
		if( r != asEXECUTION_FINISHED )
		{
			fail = true;
		}

		// Test const correctness. opEquals(int) isn't const so it must not be allowed
		bout.buffer = "";
		r = engine->ExecuteString(0, "Test a; const Test @h = a; assert( h == 0 );");
		if( r >= 0 )
		{
			fail = true;
		}
		if( bout.buffer != "ExecuteString (1, 38) : Error   : No conversion from 'const Test@&' to 'uint' available.\n" )
		{
			printf(bout.buffer.c_str());
		}

		engine->Release();
	}

	//--------------------------------------------
	// opEquals for application classes
	//
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = engine->RegisterObjectType("AppVal", sizeof(AppVal), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("AppVal", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Construct, (AppVal*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectMethod("AppVal", "bool opEquals(const AppVal &in) const", asMETHODPR(AppVal, operator ==, (const AppVal &) const, bool), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectProperty("AppVal", "int value", offsetof(AppVal, value)); assert( r >= 0 );

		r = engine->RegisterGlobalFunction("AppVal &GetAppValRef(AppVal &out)", asFUNCTIONPR(GetAppValRef, (AppVal &), AppVal &), asCALL_CDECL); assert( r >= 0 );
		g_AppVal.value = 0;

		const char *script = 			
			"void main()                        \n"
			"{                                  \n"
			"  AppVal a,b,c;                    \n"
			"  a.value = 0;                     \n"
			"  b.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  assert( a == b );                \n"  // a.opEquals(b)
			"  assert( a.opEquals(b) );         \n"  // Same as a == b
			"  assert( a != c );                \n"  // !a.opEquals(c)
			"  assert( !a.opEquals(c) );        \n"  // Same as a != c
			"  assert( a == GetAppValRef(b) );  \n"
			"  assert( b == c );                \n"
			"  b.value = 0;                     \n"
			"  assert( GetAppValRef(b) == a );  \n"
			"  assert( c == b );                \n"
			"  assert( AppVal() == a );         \n"
			"}                                  \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			fail = true;
		}
		if( bout.buffer != "" )
		{
			printf(bout.buffer.c_str());
		}

		r = engine->ExecuteString(0, "main()");
		if( r != asEXECUTION_FINISHED )
		{
			fail = true;
		}

		engine->Release();
	}



	// TODO: If opEquals isn't implemented the compiler should try opCmp instead




	// Success
	return fail;
}

}