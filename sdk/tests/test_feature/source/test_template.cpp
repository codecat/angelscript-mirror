#include "utils.h"

using namespace std;

namespace TestTemplate
{

class MyTmpl
{
public:
	MyTmpl(asIObjectType *t) 
	{
		refCount = 1;
		type = t;

		type->AddRef();
	}

	~MyTmpl()
	{
		if( type ) 
			type->Release();
	}

	void AddRef()
	{
		refCount++;
	}

	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}

	string GetNameOfType()
	{
		if( !type ) return "";

		string name = type->GetName();
		name += "<";
		name += type->GetEngine()->GetTypeDeclaration(type->GetSubTypeId());
		name += ">";

		return name;
	}

	MyTmpl &Assign(const MyTmpl &other)
	{
		return *this;
	}

	void SetVal(void *val)
	{
	}

	void *GetVal()
	{
		return 0;
	}

	asIObjectType *type;
	int refCount;

	int length;
};

MyTmpl *MyTmpl_factory(asIObjectType *type)
{
	return new MyTmpl(type);
}

// A template specialization
class MyTmpl_float
{
public:
	MyTmpl_float()
	{
		refCount = 1;
	}
	~MyTmpl_float()
	{
	}
	void AddRef()
	{
		refCount++;
	}
	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}
	string GetNameOfType()
	{
		return "MyTmpl<float>";
	}

	MyTmpl_float &Assign(const MyTmpl_float &other)
	{
		return *this;
	}

	void SetVal(float &val)
	{
	}

	float &GetVal()
	{
		return val;
	}

	int refCount;
	float val;
};

MyTmpl_float *MyTmpl_float_factory()
{
	return new MyTmpl_float();
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("Skipped due to max portability\n");
		return false;
	}

	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
	if( r == asNOT_SUPPORTED )
	{
		printf("Skipping template test because it is not yet supported\n");
		engine->Release();
		return false;	
	}

	if( r < 0 )
		fail = true;

	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTION(MyTmpl_factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

	// Must be possible to register properties for templates, but not of the template subtype
	r = engine->RegisterObjectProperty("MyTmpl<T>", "int length", offsetof(MyTmpl,length)); assert( r >= 0 );

	// Add method to return the type of the template instance as a string
	r = engine->RegisterObjectMethod("MyTmpl<T>", "string GetNameOfType()", asMETHOD(MyTmpl, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );

	// Add method that take and return the template type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "MyTmpl<T> &Assign(const MyTmpl<T> &in)", asMETHOD(MyTmpl, Assign), asCALL_THISCALL); assert( r >= 0 );

	// Add methods that take and return the template sub type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "const T &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(const T& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	// Test that it is possible to instanciate the template type for different sub types
	r = ExecuteString(engine, "MyTmpl<int> i;    \n"
								 "MyTmpl<string> s; \n"
								 "assert( i.GetNameOfType() == 'MyTmpl<int>' ); \n"
								 "assert( s.GetNameOfType() == 'MyTmpl<string>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that the assignment works
	r = ExecuteString(engine, "MyTmpl<int> i1, i2; \n"
		                         "i1.Assign(i2);      \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// Test that the template sub type works
	r = ExecuteString(engine, "MyTmpl<int> i; \n"
		                         "i.SetVal(0); \n"
								 "i.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	// It should be possible to register specializations of the template type
	r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF); assert( r >= 0 );
	// The specialization's factory doesn't take the hidden asIObjectType parameter
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_FACTORY, "MyTmpl<float> @f()", asFUNCTION(MyTmpl_float_factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl_float, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl_float, Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "string GetNameOfType()", asMETHOD(MyTmpl_float, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "MyTmpl<float> &Assign(const MyTmpl<float> &in)", asMETHOD(MyTmpl_float, Assign), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "const float &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "void SetVal(const float& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "assert( f.GetNameOfType() == 'MyTmpl<float>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "MyTmpl<float> f1, f2; \n"
		                         "f1.Assign(f2);        \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "f.SetVal(0); \n"
								 "f.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	
	// TODO: Test behaviours that take and return the template sub type
	// TODO: Test behaviours that take and return the proper template instance type

	// TODO: Even though the template doesn't accept a value subtype, it must still be possible to register a template specialization for the subtype

	// TODO: Must be possible to allow use of initialization lists

	// TODO: Must allow the subtype to be another template type, e.g. array<array<int>>

	// TODO: Must allow multiple subtypes, e.g. map<string,int>



	engine->Release();

	// Test that a proper error occurs if the instance of a template causes invalid data types, e.g. int@
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTION(MyTmpl_factory), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method makes it impossible to instanciate the template for primitive types
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T@)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		r = ExecuteString(engine, "MyTmpl<int> t;");
		if( r >= 0 )
		{
			fail = true;
		}

		if( bout.buffer != "ExecuteString (1, 8) : Error   : Can't instanciate template 'MyTmpl' with subtype 'int'\n" )
		{
			printf("%s", bout.buffer.c_str());
			fail = true;
		}

		engine->Release();
	}

	// Test that a template registered to take subtype by value cannot be instanciated for reference types
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTION(MyTmpl_factory), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method makes it impossible to instanciate the template for reference types
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		r = ExecuteString(engine, "MyTmpl<string> t;");
		if( r >= 0 )
		{
			fail = true;
		}

		if( bout.buffer != "ExecuteString (1, 8) : Error   : Can't instanciate template 'MyTmpl' with subtype 'string'\n" )
		{
			printf("%s", bout.buffer.c_str());
			fail = true;
		}

		engine->Release();
	}


	// The factory behaviour for a template class must have a hidden reference as first parameter (which receives the asIObjectType)
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f()", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			fail = true;
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int)", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			fail = true;
		engine->Release();
	}

	// Must not allow registering properties with the template subtype 
	// TODO: Must be possible to use getters/setters to register properties of the template subtype
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectProperty("MyTmpl<T>", "T a", 0); 
		if( r != asINVALID_DECLARATION )
		{
			fail = true;
		}
		engine->Release();
	}

	// Must not be possible to register specialization before the template type
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF);
		if( r != asINVALID_NAME )
		{
			fail = true;
		}
		engine->Release();
	}

 	return fail;
}

} // namespace

