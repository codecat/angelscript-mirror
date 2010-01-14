#include "utils.h"

namespace TestImplicitCast
{

void Type_construct0(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*a = 0;
}

void Type_construct1(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*a = *(int*)gen->GetAddressOfArg(0);;
}

void Type_castInt(asIScriptGeneric *gen)
{
	int *a = (int*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = *a;
}

bool Type_equal(int &a, int &b)
{
	return a == b;
}

// Class A is the base class
class A
{
public:
	virtual int test() 
	{
		return 1;
	}

	static A* factory() 
	{
		return new A;
	}
	virtual void addref() 
	{
		refCount++;
	}
	virtual void release() 
	{
		refCount--; 
		if( refCount == 0 ) 
			delete this;
	}
	virtual A& assign(const A &other)
	{
		return *this;
	}
protected:
	A() {refCount = 1;}
	virtual ~A() {}
	int refCount;
};

// Class B is the sub class, derived from A
class B : public A
{
public:
	virtual int test() 
	{
		return 2;
	}

	static B* factory() 
	{
		return new B;
	}
	static A* castToA(B*b) 
	{
		A *a = dynamic_cast<A*>(b);
		return a;
	}
	static B* AcastToB(A*a) 
	{
		B *b = dynamic_cast<B*>(a);
		return b;
	}
protected:
	B() : A() {}
	~B() {}
};

bool Test2();

bool Test()
{
	bool fail = Test2();
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

	// Two forms of casts: value cast and ref cast
	// A value cast actually constructs a new object
	// A ref cast will only reinterpret a handle, without actually constructing any object

	// Should be possible to tell AngelScript if it may use the behaviour implicitly or not
	// Since care must be taken with implicit casts, it is not allowed by default,
	// i.e. asBEHAVE_VALUE_CAST and asBEHAVE_VALUE_CAST_IMPLICIT or
	//      asBEHAVE_REF_CAST and asBEHAVE_REF_CAST_IMPLICIT

	//----------------------------------------------------------------------------
	// VALUE_CAST

	// TODO: (Test) Cast from primitive to object is an object constructor/factory
	// TODO: (Test) Cast from object to object can be either object behaviour or object constructor/factory, 
	// depending on which object registers the cast

	// TODO: (Implement) It shall be possible to register cast operators as explicit casts. The constructor/factory 
	// is by default an explicit cast, but shall be possible to register as implicit cast.

	// TODO: (Implement) Type constructors should be made explicit cast only, or perhaps not permit casts at all

	// TODO: (Test) When compiling operators with non-primitives, the compiler should first look for 
	// compatible registered operator behaviours. If not found, the compiler should see if 
	// there is any cast behaviour that allow conversion of the type to a primitive type.

	// Test 1
	// A class can be implicitly cast to a primitive, if registered the VALUE_CAST behaviour
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->RegisterGlobalFunction("void assert( bool )", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("type", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Type_construct0), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Type_construct1), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_IMPLICIT_VALUE_CAST, "int f()", asFUNCTION(Type_castInt), asCALL_GENERIC); assert( r >= 0 );

	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "type t(5); \n"
		                         "int a = t; \n"             // conversion to primitive in assignment
								 "assert( a == 5 ); \n"
								 "assert( a + t == 10 ); \n" // conversion to primitive with math operation
								 "a -= t; \n"                // conversion to primitive with math operation
								 "assert( a == 0 ); \n"
								 "assert( t == int(5) ); \n" // conversion to primitive with comparison 
								 "type b(t); \n"             // conversion to primitive with parameter
								 "assert( 32 == (1 << t) ); \n"   // conversion to primitive with bitwise operation 
	                             "assert( (int(5) & t) == 5 ); \n" // conversion to primitive with bitwise operation
								 , 0, ctx);
	if( r != 0 )
	{
		if( r == 3 )
			PrintException(ctx);
		fail = true;
	}
	if( ctx ) ctx->Release();

	// Test 2
	// A class won't be converted to primitive if there is no obvious target type
	// ex: t << 1 - It is not known what type t should be converted to
	// ex: t + t - It is not known what type t should be converted to
	// ex: t < t - It is not known what type t should be converted to
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "type t(5); t << 1; ");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : Illegal operation on 'type&'\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	bout.buffer = "";
	r = ExecuteString(engine, "type t(5); t + t; ");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : No matching operator that takes the types 'type&' and 'type&' found\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	bout.buffer = "";
	r = ExecuteString(engine, "type t(5); t < t; ");
	if( r >= 0 ) fail = true;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : No matching operator that takes the types 'type&' and 'type&' found\n" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}

	// Test3
	// If an object has a cast to more than one matching primitive type, the cast to the 
	// closest matching type will be used, i.e. Obj has cast to int and to float. A type of 
	// int8 is requested, so the cast to int is used
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = ExecuteString(engine, "type t(2); assert( (1.0 / t) == (1.0 / 2.0) );");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	// Test4
	// It shall not be possible to register a cast behaviour from an object to a boolean type
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	r = engine->RegisterObjectType("type", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("type", asBEHAVE_IMPLICIT_VALUE_CAST, "bool f()", asFUNCTION(Type_castInt), asCALL_GENERIC); 
	if( r != asNOT_SUPPORTED )
	{
		fail = true;
	}

	engine->Release();

	// Test5
	// Exclicit value cast
	// TODO: This should work for MAX_PORTABILITY as well
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		r = engine->RegisterGlobalFunction("void assert( bool )", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("type", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Type_construct0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("type", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(Type_construct1), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("type", asBEHAVE_VALUE_CAST, "int f()", asFUNCTION(Type_castInt), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("type", "bool opEquals(const type &in) const", asFUNCTION(Type_equal), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
		r = engine->RegisterObjectProperty("type", "int v", 0);

		// explicit cast to int is allowed
		r = ExecuteString(engine, "type t; t.v = 5; int a = int(t); assert(a == 5);"); 
		if( r < 0 )
			fail = true;

		// as cast to int is allowed, AngelScript also allows cast to float (using cast to int then implicit cast to int)
		r = ExecuteString(engine, "type t; t.v = 5; float a = float(t); assert(a == 5.0f);");
		if( r < 0 )
			fail = true;

		// implicit cast to int is not allowed
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		r = ExecuteString(engine, "type t; int a = t;");
		if( r >= 0 )
			fail = true;
		if( bout.buffer != "ExecuteString (1, 17) : Error   : Can't implicitly convert from 'type&' to 'int'.\n" )
		{
			printf(bout.buffer.c_str());
			fail = true;
		}
/*
		// Having an implicit constructor with an int param makes it possible to compare the type with int
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		r = ExecuteString(engine, "type t(5); assert( t == 5 );");
		if( r < 0 )
			fail = true;
*/
		engine->Release();
	}

	// Test6
	// Must be possible to return a const string variable from a function returning a non-const string
	// string registered as reference type
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptString(engine);

		const char *script = "string test() { const string s = 'hello'; return s; }";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		int r = mod->Build();
		if( r < 0 )
		{
			fail = true;
		}

		engine->Release();
	}

	// Test7
	// Must be possible to return a const string variable from a function returning a non-const string
	// string registered as value type
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);

		const char *script = "string test() { const string s = 'hello'; return s; }";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		int r = mod->Build();
		if( r < 0 )
		{
			fail = true;
		}

		engine->Release();
	}

	//-----------------------------------------------------------------
	// REFERENCE_CAST

	// TODO: This should work for MAX_PORTABILITY as well
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{

		// It must be possible to cast an object handle to another object handle, without 
		// losing the reference to the original object. This is what will allow applications
		// to register inheritance for registered types. This should be a special 
		// behaviour, i.e. REF_CAST. 
		
		// How to provide a cast from a base class to a derived class?
		// The base class may not know about the derived class, so it must
		// be the derived class that registers the behaviour. 
		
		// How to provide interface functionalities to registered types? I.e. a class implements 
		// various interfaces, and a handle to one of the interfaces may be converted to a handle
		// of another interface that is implemented by the class.

		// TODO: Can't register casts from primitive to primitive

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		// Class A is the base class
		engine->RegisterObjectType("A", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("A", asBEHAVE_FACTORY, "A@f()", asFUNCTION(A::factory), asCALL_CDECL); 
		engine->RegisterObjectBehaviour("A", asBEHAVE_RELEASE, "void f()", asMETHOD(A, release), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("A", asBEHAVE_ADDREF, "void f()", asMETHOD(A, addref), asCALL_THISCALL);
		engine->RegisterObjectMethod("A", "A& opAssign(const A &in)", asMETHOD(A, assign), asCALL_THISCALL);
		engine->RegisterObjectMethod("A", "int test()", asMETHOD(A, test), asCALL_THISCALL);

		// Class B inherits from class A
		engine->RegisterObjectType("B", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("B", asBEHAVE_FACTORY, "B@f()", asFUNCTION(B::factory), asCALL_CDECL); 
		engine->RegisterObjectBehaviour("B", asBEHAVE_RELEASE, "void f()", asMETHOD(B, release), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("B", asBEHAVE_ADDREF, "void f()", asMETHOD(B, addref), asCALL_THISCALL);
		engine->RegisterObjectMethod("B", "int test()", asMETHOD(B, test), asCALL_THISCALL);
		
		// Test the classes to make sure they work
		r = ExecuteString(engine, "A a; assert(a.test() == 1); B b; assert(b.test() == 2);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// It should be possible to register a REF_CAST to allow implicit cast
		// Test IMPLICIT_REF_CAST from subclass to baseclass
		r = engine->RegisterObjectBehaviour("B", asBEHAVE_IMPLICIT_REF_CAST, "A@+ f()", asFUNCTION(B::castToA), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = ExecuteString(engine, "B b; A@ a = b; assert(a.test() == 2);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// Test explicit cast with registered IMPLICIT_REF_CAST
		r = ExecuteString(engine, "B b; A@ a = cast<A>(b); assert(a.test() == 2);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// It should be possible to assign a value of type B 
		// to and variable of type A due to the implicit ref cast
		r = ExecuteString(engine, "A a; B b; a = b;");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// Test REF_CAST from baseclass to subclass
		r = engine->RegisterObjectBehaviour("A", asBEHAVE_REF_CAST, "B@+ f()", asFUNCTION(B::AcastToB), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = ExecuteString(engine, "B b; A@ a = cast<A>(b); B@ _b = cast<B>(a); assert(_b.test() == 2);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// Test REF_CAST from baseclass to subclass, where the cast is invalid
		r = ExecuteString(engine, "A a; B@ b = cast<B>(a); assert(@b == null);");
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// Doing a ref cast on a null pointer must not throw an exception
		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "A@ a;\n B@ b;\n @b = cast<B>(a);\n @a = @b;\n", 0, ctx);
		if( r != asEXECUTION_FINISHED )
			fail = true;
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		ctx->Release();

		// Various situations with ref casts on null pointers
		const char* script =
			"A@ global_a; \n"
			"B@ global_b; \n"
			"B@ global_b_nulled = null; \n"
			"B@ ret_null() { return null; } \n"
			"class C { B@ b; } \n"
			"void testFunc() { \n"
			"	A@ local_a = A(); \n"
			"	B@ local_b = B(); \n"
			"	@local_a = @local_b; \n"
			"	@global_a = @local_b; \n"
			"	@local_a = cast<A>(global_b); \n"
			"	@local_a = @global_b; \n"
			"	@local_a = @global_b_nulled; \n"
			"   @local_a = @ret_null(); \n"
			"   C c; \n"
			"   @local_a = @c.b; \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			fail = true;

		r = ExecuteString(engine, "testFunc();", mod);
		if( r != asEXECUTION_FINISHED )
			fail = true;

		// TODO: This requires implicit value cast
		// Test passing a value of B to a function expecting its base class
		// the compiler will automatically create a copy
/*		const char *script = 
			"void func(A a) {assert(a.test() == 1);}\n";
		r = mod->AddScriptSection(0, "script", script, strlen(script));
		r = mod->Build(0);
		if( r < 0 )
			fail = true;
		r = ExecuteString(engine, "B b; func(b)");
		if( r < 0 )
			fail = true;
*/
		// TODO: A handle to A can not be implicitly cast to a handle to B since it was registered as explicit REF_CAST
		// TODO: It shouldn't be possible to cast away constness

		engine->Release();
	}

	// Success
 	return fail;
}

struct Simple {
};

struct Complex {
};

void implicit(asIScriptGeneric * gen) {
}

bool Test2()
{
	bool fail = false;
	COutStream out;
	int r;

	asIScriptEngine * engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	r = engine->RegisterObjectType("simple", sizeof(Simple), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert(r >= 0);
	r = engine->RegisterObjectType("complex", sizeof(Complex), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("complex", asBEHAVE_IMPLICIT_VALUE_CAST, "int f()", asFUNCTION(implicit), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("complex", asBEHAVE_IMPLICIT_VALUE_CAST, "double f()", asFUNCTION(implicit), asCALL_GENERIC);
	r = engine->RegisterObjectBehaviour("complex", asBEHAVE_IMPLICIT_VALUE_CAST, "simple f()", asFUNCTION(implicit), asCALL_GENERIC);

	const char script[] =
	"void main() {\n"
	"  int i;\n"
	"  double d;\n"
	"  simple s;\n"
	"  complex c;\n"
	"  i = c;\n"
	"  d = c;\n"
	"  s = c;\n"
	"}";
	asIScriptModule * mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script, sizeof(script) - 1); assert(r >= 0);
	r = mod->Build(); 
	if( r < 0 )
		fail = true;
	else
	{
		int func_id = mod->GetFunctionIdByDecl("void main()"); assert(func_id >= 0);
		asIScriptContext * ctx = engine->CreateContext();

		r = ctx->Prepare(func_id); assert(r >= 0);
		r = ctx->Execute(); assert(r >= 0);

		ctx->Release();
	}

	// It must be possible to cast using an explicit construct cast
	r = ExecuteString(engine, "complex c; simple s = simple(c);");
	if( r < 0 )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}


} // namespace

