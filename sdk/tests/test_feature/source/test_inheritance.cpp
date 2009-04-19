#include "utils.h"

namespace TestInheritance
{

bool Test2();

bool TestModule(const char *module, asIScriptEngine *engine);

bool Test()
{
	bool fail = false;
	int r;

	asIScriptModule *mod = 0;
	COutStream out;
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	const char *script =
		"bool baseDestructorCalled = false;               \n"
		"bool baseConstructorCalled = false;              \n"
		"bool baseFloatConstructorCalled = false;         \n"
		"class Base : Intf                                \n"
		"{                                                \n"
		"  int a;                                         \n"
		"  void f1() { a = 1; }                           \n"
		"  void f2() { a = 0; }                           \n"
		"  void f3() { a = 3; }                           \n"
		"  Base() { baseConstructorCalled = true; }       \n"
		"  Base(float) { baseFloatConstructorCalled = true; } \n"
		"  ~Base() { baseDestructorCalled = true; }       \n"
		"}                                                \n"
		"bool derivedDestructorCalled = false;            \n"
		"bool derivedConstructorCalled = false;           \n"
		"class Derived : Base                             \n"
		"{                                                \n"
		   // overload f2()
		"  void f2() { a = 2; }                           \n"
		   // overload f3()
		"  void f3() { a = 2; }                           \n"
		"  void func()                                    \n"
		"  {                                              \n"
		     // call Base::f1()
		"    f1();                                        \n"
		"    assert(a == 1);                              \n"
		     // call Derived::f2()
		"    f2();                                           \n"
		"    assert(a == 2);                                 \n"
		     // call Base::f3() 
		"    Base::f3();                                     \n"
		"    assert(a == 3);                                 \n"
		"  }                                                 \n"
		"  Derived(int) { derivedConstructorCalled = true; } \n"
		"  ~Derived() { derivedDestructorCalled = true; }    \n"
		"}                                                \n"
		"void foo( Base &in a )                           \n"
		"{                                                \n"
		"  assert( cast<Derived>(a) is null );            \n"
		"}                                                \n"
		// Must be possible to call the default constructor, even if not declared
		"class DerivedGC : BaseGC { DerivedGC() { super(); } }  \n"
		"class BaseGC { BaseGC @b; }                      \n"
		"class DerivedS : Base                            \n"
		"{                                                \n"
		"  DerivedS(float)                                \n"
		"  {                                              \n"
	  	     // Call Base::Base(float)
		"    if( true )                                   \n"
		"      super(1.4f);                               \n"
		"    else                                         \n"
		"      super();                                   \n"
		"  }                                              \n"
		"}                                                \n"
		// Must handle inheritance where the classes have been declared out of order
		"void func()                                      \n"
		"{                                                \n"
		"   Intf@ a = C();                                \n"
		"}                                                \n"
		"class C : B {}                                   \n"
		"interface Intf {}                                \n"
		"class B : Intf {}                                \n";

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
	{
		fail = true;
	}

	if( TestModule(0, engine) )
	{
		fail = true;
	}

	// Must make sure that the inheritance path is stored/restored with the saved byte code
	{ 
		CBytecodeStream stream(__FILE__"1");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
		{
			fail = true;
		}

		asIScriptModule *mod2 = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod2->LoadByteCode(&stream);
		if( r < 0 )
		{
			fail = true;
		}

		// Both modules should have the same number of functions
		if( mod->GetFunctionCount() != mod2->GetFunctionCount() )
		{
			fail = true;

			asUINT n;
			printf("First module's functions\n");
			for( n = 0; n < (asUINT)mod->GetFunctionCount(); n++ )
			{
				asIScriptFunction *f = mod->GetFunctionDescriptorByIndex(n);
				printf("%s\n", f->GetDeclaration());
			}
			printf("\nSecond module's functions\n");
			for( n = 0; n < (asUINT)mod2->GetFunctionCount(); n++ )
			{
				asIScriptFunction *f = mod2->GetFunctionDescriptorByIndex(n);
				printf("%s\n", f->GetDeclaration());
			}
		}

		if( TestModule("2", engine) )
		{
			fail = true;
		}

		engine->DiscardModule("2");
	}

	engine->Release();

	fail = Test2() || fail;

	// Success
	return fail;
}

bool TestModule(const char *module, asIScriptEngine *engine)
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	COutStream out;
	asIScriptModule *mod = engine->GetModule(module);

	// Test that it is possible to declare a class that inherits from another
	// Test that the inherited properties are available in the derived class
	// Test that the inherited methods are available in the derived class
	// Test that it is possible to override the inherited methods
	// Test that it is possible to call base class methods from within overridden methods in derived class 
	asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("Derived"));
	asIScriptContext *ctx = engine->CreateContext();
	ctx->Prepare(obj->GetObjectType()->GetMethodIdByDecl("void func()"));
	ctx->SetObject(obj);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}
	ctx->Release();
	obj->Release();

	// Test that implicit cast from derived to base is working
	r = engine->ExecuteString(module, "Derived d; Base @b = @d; assert( b !is null );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that cast from base to derived require explicit cast
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = engine->ExecuteString(module, "Base b; Derived @d = @b;");
	if( r >= 0 )
	{
		fail = true;
	}
	if( bout.buffer != "ExecuteString (1, 22) : Error   : Can't implicitly convert from 'Base@const&' to 'Derived@&'.\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is possible to explicitly cast to derived class
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	r = engine->ExecuteString(module, "Derived d; Base @b = @d; assert( cast<Derived>(b) !is null );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test the explicit cast behaviour for a non-handle script object
	r = engine->ExecuteString(module, "Base b; assert( cast<Derived>(b) is null );");
	if( r != asEXECUTION_FINISHED )
	{
		fail= true;
	}

	// Test that it is possible to implicitly assign derived class to base class
	r = engine->ExecuteString(module, "Derived d; Base b = d;");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that it is possible to pass a derived class to a function expecting a reference to the base class
	// This actually creates an instance of the Base class and assigns the Derived instance to it.
	// This is because the parameter is &in and not const &in
	// TODO: May be able to avoid this by having a specific behaviour for 
	//       duplicating objects, rather than using assignment
	r = engine->ExecuteString(module, "Derived d; foo(d);");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test polymorphing
	r = engine->ExecuteString(module, "Derived d; Base @b = @d; b.a = 3; b.f2(); assert( b.a == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Base class' destructor must be called when object is destroyed
	r = engine->ExecuteString(module, "baseDestructorCalled = derivedDestructorCalled = false; { Derived d; }\n"
								      "assert( derivedDestructorCalled ); \n"
		                              "assert( baseDestructorCalled );\n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// If the base class is garbage collected, then the derived class must also be garbage collected
	// This also tests that it is possible to call the default constructor of the base class, even though it is not declared
	engine->GarbageCollect();
	r = engine->ExecuteString(module, "DerivedGC b; @b.b = @b;");
	if( r != asEXECUTION_FINISHED ) 
	{
		fail = true;
	}

	asUINT gcSize;
	engine->GetGCStatistics(&gcSize);
	assert( gcSize == 1 );
	engine->GarbageCollect();
	engine->GetGCStatistics(&gcSize);
	assert( gcSize == 0 );

	// Test that the derived class inherits the interfaces that the base class implements
	r = engine->ExecuteString(module, "Intf @a; Derived b; @a = @b;");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that an implemented constructor calls the base class' default constructor
	r = engine->ExecuteString(module, "baseConstructorCalled = derivedConstructorCalled = false; Derived d(1); \n"
		                              "assert( baseConstructorCalled ); \n"
									  "assert( derivedConstructorCalled ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that the default constructor calls the base class' default constructor
	r = engine->ExecuteString(module, "baseConstructorCalled = false; Derived d; \n"
		                              "assert( baseConstructorCalled ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that it is possible to manually call the base class' constructor
	// Test that the default constructor for the base class isn't called 
	//   when a manual call to another constructor is made
	r = engine->ExecuteString(module, "baseConstructorCalled = baseFloatConstructorCalled = false; DerivedS d(1.4f); \n"
		                              "assert( baseFloatConstructorCalled ); \n"
									  "assert( !baseConstructorCalled ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that it is possible to determine base class from object type interface
	asIObjectType *d = engine->GetObjectTypeById(mod->GetTypeIdByDecl("Derived"));
	if( d == 0 )
		fail = true;
	else if( d->GetBaseType() == 0 )
		fail = true;
	else if( strcmp(d->GetBaseType()->GetName(), "Base") != 0 )
		fail = true;

	// Test factory id
	if( d->GetFactoryCount() != 2 )
		fail = true;
	int fid = d->GetFactoryIdByDecl("Derived@ Derived(int)");
	if( fid < 0 )
		fail = true;
	if( fid != d->GetFactoryIdByIndex(1) )
		fail = true;

	// TODO: not related to inheritance, but it should be possible to call another constructor from within a constructor. 
	//       We can follow D's design of using this(args) to call the constructor

	return fail;
}

bool Test2()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;
	asIScriptModule *mod;
	asIScriptEngine *engine;
	const char *script;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Test that it is not possible to inherit from application registered type
	script = "class A : string {} \n";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 11) : Error   : Can't inherit from 'string'\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to inherit from multiple script classes
	script = "class B {} class C {} class D {} class A : B, C, D {} \n";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 47) : Error   : Can't inherit from multiple classes\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to inherit from a class that in turn inherits from this class
	script = "class A : C {} class B : A {} class C : B {}\n";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 41) : Error   : Can't inherit from itself, or another class that inherits from this class\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to inherit from self
	script = "class A : A {}\n";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 11) : Error   : Can't inherit from itself, or another class that inherits from this class\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that derived classes can't overload properties
	// TODO: In C++ it is possible to overload properties, in which case the base class property is hidden. Should we adopt this for AngelScript too?
	script = "class A { int a; } class B : A { double a; }\n";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	// TODO: The error should explain that the original property is from the base class
	if( bout.buffer != "script (1, 41) : Error   : Name conflict. 'a' is an object property.\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super() when not deriving from any class
	script = "class A { A() { super(); } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	// TODO: The error message should explain that it is not possible to call super 
	//       because the class doesn't derived from another class
	if( bout.buffer != "script (1, 11) : Info    : Compiling void A::A()\n"
					   "script (1, 17) : Error   : No matching signatures to 'A::super()'\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super() multiple times within the constructor
	script = "class A {} class B : A { B() { super(); super(); } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 ) 
		fail = true;
	if( bout.buffer != "script (1, 26) : Info    : Compiling void B::B()\n"
					   "script (1, 41) : Error   : Can't call a constructor multiple times\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super() in a loop
	script = "class A {} class B : A { B() { while(true) { super(); } } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build(); 
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 26) : Info    : Compiling void B::B()\n"
					   "script (1, 46) : Error   : Can't call a constructor in loops\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super() in a switch
	// TODO: Should allow call in switch, but all possibilities must call it once.
	script = "class A {} class B : A { B() { switch(2) { case 2: super(); } } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build(); 
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 26) : Info    : Compiling void B::B()\n"
					   "script (1, 52) : Error   : Can't call a constructor in switch\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that all (or none) control paths must call super()
	script = "class A {} class B : A { \n"
		     "B(int) { if( true ) super(); } \n"
			 "B(float) { if( true ) {} else super(); } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build(); 
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (2, 1) : Info    : Compiling void B::B(int)\n"
					   "script (2, 10) : Error   : Both conditions must call constructor\n"
					   "script (3, 1) : Info    : Compiling void B::B(float)\n"
				   	   "script (3, 12) : Error   : Both conditions must call constructor\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super() outside of the constructor
	script = "class A {} class B : A { void mthd() { super(); } }";

	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build(); 
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 26) : Info    : Compiling void B::mthd()\n"
					   "script (1, 40) : Error   : No matching signatures to 'super()'\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that a base class can't have a derived class as member (except as handle)
	script = "class A { B b; } class B : A {}";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	// TODO: The message could be improved to mention which member
	if( bout.buffer != "script (1, 24) : Error   : Illegal member type\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that it is not possible to call super with any scope prefix
	script = "class A { } class B : A { B() { ::super(); } }";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 27) : Info    : Compiling void B::B()\n"
					   "script (1, 33) : Error   : No matching signatures to '::super()'\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that the error message for calling missing method with scope is correct
	script = "class A { void method() { B::test(); A::method(2); A::method(); method(3.15); B::A::a(); } }";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r >= 0 )
		fail = true;
	if( bout.buffer != "script (1, 11) : Info    : Compiling void A::method()\n"
					   "script (1, 27) : Error   : No matching signatures to 'B::test()'\n"
					   "script (1, 38) : Error   : No matching signatures to 'A::method(const uint)'\n"
					   "script (1, 65) : Error   : No matching signatures to 'A::method(const double)'\n"
					   "script (1, 83) : Error   : Invalid scope resolution\n"
					   "script (1, 79) : Error   : No matching signatures to 'B::a()'\n" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}

	// Test that calling the constructor from within the constructor 
	// using the class name will create a new object. 
	script = "A @a1, a2; class A { A() { @a1 = this; A(1); } A(int) { @a2 = this; } }";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		fail = true;
		printf(bout.buffer.c_str());
	}
	r = engine->ExecuteString(0, "A a; assert( a1 !is a2 ); assert( a1 !is null ); assert( a2 !is null );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}


	engine->Release();

	return fail;
}

} // namespace

