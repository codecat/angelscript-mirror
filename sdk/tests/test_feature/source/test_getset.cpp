#include "utils.h"

namespace TestGetSet
{


bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	COutStream out;
	asIScriptModule *mod;
 	asIScriptEngine *engine;
	

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// main1 and main2 should produce the same bytecode
	const char *script1 = 
		"class Test                            \n"
		"{                                     \n"
		"  int get_prop() { return _prop; }    \n"
		"  void set_prop(int v) { _prop = v; } \n"
        "  int _prop;                          \n"
		"}                                     \n"
		"void main1()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.set_prop(42);                     \n"
		"  assert( t.get_prop() == 42 );       \n"
		"}                                     \n"
		"void main2()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.prop = 42;                        \n"
		"  assert( t.prop == 42 );             \n"
		"}                                     \n";
		
	mod->AddScriptSection("script", script1);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	r = engine->ExecuteString(0, "main1()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = engine->ExecuteString(0, "main2()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test compound assignment with accessors (not allowed)
	const char *script2 = 
		"class Test                            \n"
		"{                                     \n"
		"  void set_prop(int v) { _prop = v; } \n"
        "  int _prop;                          \n"
		"}                                     \n"
		"void main1()                          \n"
		"{                                     \n"
		"  Test t;                             \n"
		"  t.prop += 42;                       \n"
		"}                                     \n";

	mod->AddScriptSection("script", script2);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "script (6, 1) : Info    : Compiling void main1()\n"
	                   "script (9, 10) : Error   : Compound assignments with property accessors are not allowed\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with boolean operators
	const char *script3 = 
		"class Test                              \n"
		"{                                       \n"
		"  bool get_boolProp() { return true; }  \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  if( t.boolProp ) {}                   \n"
		"  if( t.boolProp && true ) {}           \n"
		"  if( false || t.boolProp ) {}          \n"
		"  if( t.boolProp ^^ t.boolProp ) {}     \n"
		"  if( !t.boolProp ) {}                  \n"
		"  t.boolProp ? t.boolProp : t.boolProp; \n"
		"}                                       \n";

	mod->AddScriptSection("script", script3);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with math operators
	const char *script4 = 
		"class Test                              \n"
		"{                                       \n"
		"  float get_prop() { return 1.0f; }     \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  float f = t.prop * 1;                 \n"
		"  10 / t.prop;                          \n"
		"  -t.prop;                              \n"
		"}                                       \n";

	mod->AddScriptSection("script", script4);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test get accessor with bitwise operators
	const char *script5 = 
		"class Test                              \n"
		"{                                       \n"
		"  uint get_prop() { return 1; }         \n"
		"}                                       \n"
		"void main1()                            \n"
		"{                                       \n"
		"  Test t;                               \n"
		"  t.prop << t.prop;                     \n"
		"  t.prop & t.prop;                      \n"
		"  ~t.prop;                              \n"
		"}                                       \n";

	mod->AddScriptSection("script", script5);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test multiple get accessors for same property. Should give error
	// Test multiple set accessors for same property. Should give error
	const char *script6 = 
		"class Test                  \n"
		"{                           \n"
		"  uint get_p() {return 0;}  \n"
		"  float get_p() {return 0;} \n"
		"  void set_s(float) {}      \n"
		"  void set_s(uint) {}       \n"
		"}                           \n"
		"void main()                 \n"
		"{                           \n"
		"  Test t;                   \n"
		"  t.p;                      \n"
		"  t.s = 0;                  \n"
		"}                           \n";
	mod->AddScriptSection("script", script6);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "script (8, 1) : Info    : Compiling void main()\n"
	                   "script (11, 4) : Error   : Found multiple get accessors for property 'p'\n"
	                   "script (11, 4) : Info    : uint Test::get_p()\n"
	                   "script (11, 4) : Info    : float Test::get_p()\n"
	                   "script (12, 4) : Error   : Found multiple set accessors for property 's'\n"
	                   "script (12, 4) : Info    : void Test::set_s(float)\n"
	                   "script (12, 4) : Info    : void Test::set_s(uint)\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test mismatching type between get accessor and set accessor. Should give error
	const char *script7 = 
		"class Test                  \n"
		"{                           \n"
		"  uint get_p() {return 0;}  \n"
		"  void set_p(float) {}      \n"
		"}                           \n"
		"void main()                 \n"
		"{                           \n"
		"  Test t;                   \n"
		"  t.p;                      \n"
		"}                           \n";
	mod->AddScriptSection("script", script7);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "script (6, 1) : Info    : Compiling void main()\n"
                       "script (9, 4) : Error   : The property 'p' has mismatching types for the get and set accessors\n"
                       "script (9, 4) : Info    : uint Test::get_p()\n"
                       "script (9, 4) : Info    : void Test::set_p(float)\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test only set accessor for read expression
	// Test only get accessor for write expression
	const char *script8 = 
		"class Test                  \n"
		"{                           \n"
		"  uint get_g() {return 0;}  \n"
		"  void set_s(float) {}      \n"
		"}                           \n"
		"void main()                 \n"
		"{                           \n"
		"  Test t;                   \n"
		"  t.g = 0;                  \n"
        "  t.s + 1;                  \n"
		"}                           \n";
	mod->AddScriptSection("script", script8);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "script (6, 1) : Info    : Compiling void main()\n"
					   "script (9, 7) : Error   : The property has no set accessor\n"
					   "script (10, 7) : Error   : The property has no get accessor\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test pre and post ++. Should fail, since the expression is not a variable
	const char *script9 = 
		"class Test                  \n"
		"{                           \n"
		"  uint get_p() {return 0;}  \n"
		"  void set_p(uint) {}       \n"
		"}                           \n"
		"void main()                 \n"
		"{                           \n"
		"  Test t;                   \n"
		"  t.p++;                    \n"
        "  --t.p;                    \n"
		"}                           \n";
	mod->AddScriptSection("script", script9);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
	{
		fail = true;
		printf("Didn't fail to compile the script\n");
	}
	if( bout.buffer != "script (6, 1) : Info    : Compiling void main()\n"
					   "script (9, 6) : Error   : Not a valid reference\n"
				 	   "script (10, 3) : Error   : Not a valid reference\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test using property accessors from within class methods without 'this'
	// Test accessor where the object is a handle
	const char *script10 = 
		"class Test                 \n"
		"{                          \n"
		"  uint get_p() {return 0;} \n"
		"  void set_p(uint) {}      \n"
		"  void test()              \n"
		"  {                        \n"
		"    p = 0;                 \n"
		"    int a = p;             \n"
		"  }                        \n"
		"}                          \n"
		"void func()                \n"
		"{                          \n"
		"  Test @a = Test();        \n"
		"  a.p = 1;                 \n"
		"  int b = a.p;             \n"
		"}                          \n";
	mod->AddScriptSection("script", script10);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test accessors with function arguments (by value, in ref, out ref)
	const char *script11 = 
		"class Test                 \n"
		"{                          \n"
		"  uint get_p() {return 0;} \n"
		"  void set_p(uint) {}      \n"
		"}                          \n"
		"void func()                \n"
		"{                          \n"
		"  Test a();                \n"
		"  byVal(a.p);              \n"
		"  inArg(a.p);              \n"
		"  outArg(a.p);             \n"
		"}                          \n"
		"void byVal(int v) {}       \n"
		"void inArg(int &in v) {}   \n"
		"void outArg(int &out v) {} \n";
	mod->AddScriptSection("script", script11);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// When the property is an object type, then the set accessor should be 
	// used instead of the overloaded assignment operator to set the value. 
	// Properties of object properties, must allow having different 
	// types for get and set. IsEqualExceptConstAndRef should be used.
	const char *script12 = 
		"class Test                                   \n"
		"{                                            \n"
		"  string get_s() {return _s;}                \n"
		"  void set_s(const string &in n) {_s = n;}   \n"
		"  string _s;                                 \n"
		"}                          \n"
		"void func()                \n"
		"{                          \n"
		"  Test t;                  \n"
		"  t.s = 'hello';           \n"
		"  assert(t.s == 'hello');  \n"
		"}                          \n";
	mod->AddScriptSection("script", script12);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Compound assignments for object properties will not be allowed
	r = engine->ExecuteString(0, "Test t; t.s += 'hello';");
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "ExecuteString (1, 13) : Error   : Compound assignments with property accessors are not allowed\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test @t.prop = @obj; Property is a handle, and the property is assigned a new handle. Should work
	const char *script13 = 
		"class Test                                   \n"
		"{                                            \n"
		"  string@ get_s() {return _s;}               \n"
		"  void set_s(string @n) {@_s = @n;}          \n"
		"  string@ _s;                                \n"
		"}                          \n"
		"void func()                \n"
		"{                          \n"
		"  Test t;                  \n"
		"  string s = 'hello';      \n"
		"  @t.s = @s;               \n" // handle assignment
		"  assert(t.s is s);        \n"
		"  t.s = 'other';           \n" // value assignment
		"  assert(s == 'other');    \n"
		"}                          \n";
	mod->AddScriptSection("script", script13);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test accessing members of an object property
	const char *script14 = 
		"class Test                                   \n"
		"{                                            \n"
		"  string get_s() {return _s;}                \n"
		"  void set_s(string n) {_s = n;}             \n"
		"  string _s;                                 \n"
		"}                            \n"
		"void func()                  \n"
		"{                            \n"
		"  Test t;                    \n"
		"  t.s = 'hello';             \n" // value assignment
		"  assert(t.s == 'hello');    \n"
		"  assert(t.s.length() == 5); \n" // this should work as length is const
		"}                            \n";
	mod->AddScriptSection("script", script14);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test accessing a non-const method on an object through a get accessor
	// Should at least warn since the object is just a temporary one
	bout.buffer.c_str();
	r = engine->ExecuteString(0, "Test t; t.s.resize(4);");
	if( r < 0 )
		fail = true;
	if( bout.buffer != "ExecuteString (1, 13) : Warning : A non-const method is called on temporary object. Changes to the object may be lost.\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test opNeg for object through get accessor
	const char *script15 = 
		"class Val { int opNeg() const { return -1; } } \n"
		"class Test                          \n"
		"{                                   \n"
		"  Val get_s() const {return Val();} \n"
		"}                                   \n"
		"void func()                  \n"
		"{                            \n"
		"  Test t;                    \n"
		"  assert( -t.s == -1 );      \n"
		"}                            \n";
	mod->AddScriptSection("script", script15);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}	

	// Test index operator for object through get accessor
	const char *script16 = 
		"class Test                          \n"
		"{                                   \n"
		"  int[] get_s() const { int[] a(1); a[0] = 42; return a; } \n"
		"}                                   \n"
		"void func()                  \n"
		"{                            \n"
		"  Test t;                    \n"
		"  assert( t.s[0] == 42 );    \n"
		"}                            \n";
	mod->AddScriptSection("script", script16);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}	

	// Test accessing normal properties for object through get accessor
	const char *script17 = 
		"class Val { int val; } \n"
		"class Test                          \n"
		"{                                   \n"
		"  Val get_s() const { Val v; v.val = 42; return v;} \n"
		"}                                   \n"
		"void func()                  \n"
		"{                            \n"
		"  Test t;                    \n"
		"  assert( t.s.val == 42 );   \n"
		"}                            \n";
	mod->AddScriptSection("script", script17);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
		printf("Failed to compile the script\n");
	}
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	r = engine->ExecuteString(0, "func()");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}	

	// Test const/non-const get accessor
	const char *script18 = 
		"class Test                          \n"
		"{                                   \n"
		"  int get_p() { return 42; }        \n"
		"  int get_c() const { return 42; }  \n"
		"}                                   \n"
		"void func()                  \n"
		"{                            \n"
		"  const Test @t = @Test();   \n"
		"  assert( t.p == 42 );       \n" // Fail
		"  assert( t.c == 42 );       \n" // Success
		"}                            \n";
	mod->AddScriptSection("script", script18);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (6, 1) : Info    : Compiling void func()\n"
	                   "script (9, 15) : Error   : Non-const method call on read-only object reference\n"
	                   "script (9, 15) : Info    : int Test::get_p()\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	
	// TODO: Test non-const get accessor for object type with const overloaded dual operator
	
	// TODO: Test get accessor that returns a reference (only from application func to start with)
		
	// TODO: Test property accessor with inout references. Shouldn't be allowed as the value is not a valid reference

	// TODO: Test set accessor with parameter declared as out ref (shouldn't be found)

	// TODO: What should be done with expressions like t.prop; Should the get accessor be called even though 
	//       the value is never used?

	// TODO: Accessing a class member from within the property accessor with the same name as the property 
	//       shouldn't call the accessor again. Instead it should access the real member. FindPropertyAccessor() 
	//       shouldn't find any if the function being compiler is the property accessor itself

	engine->Release();

	// Test property accessor on temporary object handle
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);

		const char *script = "class Obj { void set_opacity(float v) {} }\n"
			                 "Obj @GetObject() { return @Obj(); } \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			fail = true;

		r = engine->ExecuteString(0, "GetObject().opacity = 1.0f;");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->Release();
	}

	// Test bug reported by Scarabus2
	// The bug was an incorrect reusage of temporary variable by the  
	// property get accessor when compiling a binary operator
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"class Object { \n"
			"  Object() {rot = 0;} \n"
			"  void set_rotation(float r) {rot = r;} \n"
			"  float get_rotation() const {return rot;} \n"
			"  float rot; } \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			fail = true;

		r = engine->ExecuteString(0, "Object obj; \n"
								     "float elapsed = 1.0f; \n"
									 "float temp = obj.rotation + elapsed * 1.0f; \n"
									 "obj.rotation = obj.rotation + elapsed * 1.0f; \n"
									 "assert( obj.rot == 1 ); \n");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

