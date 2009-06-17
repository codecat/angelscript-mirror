#include "utils.h"

namespace TestOperator
{


bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

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
		"void main()                        \n"
		"{                                  \n"
		"  Test a,b,c;                      \n"
		"  a.value = 0;                     \n"
		"  b.value = 0;                     \n"
		"  c.value = 1;                     \n"
		"  assert( a == b );                \n"  // a.opEquals(b)
		"  assert( a.opEquals(b) );         \n"  // Same as a == b
		"  assert( a == 0 );                \n"  // a.opEquals(0)
		"  assert( a == 0.1f );             \n"  // a.opEquals(int(0.1f))
		"  assert( a != c );                \n"  // !a.opEquals(c)
		"  assert( a != 1 );                \n"  // !a.opEquals(1)
		"  assert( !a.opEquals(c) );        \n"  // Same as a != c
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



	// TODO: Test with inverted arguments, e.g. 0 == a
	//       This is supported by implictly casting a to the type of the left operand

	// TODO: If opEquals isn't implemented the compiler should try opCmp instead

	// Success
	return fail;
}

}