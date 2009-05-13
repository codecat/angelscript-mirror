#include "utils.h"

namespace TestTemplate
{

class MyTmpl
{
public:
	MyTmpl(asIObjectType *t) 
	{
		refCount = 1;
		type = t;

		// TODO: Must be possible to secure the reference to the asIObjectType
		// type->AddRef();
	}

	~MyTmpl()
	{
		// TODO: Must be possible to secure the reference to the asIObjectType
		// if( type ) 
		//	type->Release();
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

	asIObjectType *type;
	int refCount;
};

MyTmpl *MyTmpl_factory(asIObjectType *type)
{
	return new MyTmpl(type);
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
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

	// TODO: Test that the factory must have a hidden reference as first parameter (which receives the asIObjectType 
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTION(MyTmpl_factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

	// TODO: Test that it is possible to instanciate the template type for different sub types
	r = engine->ExecuteString(0, "MyTmpl<int> i");
	if( r < 0 )
		fail = true;

	// TODO: Add method to return the type of the template instance as a string

	// TODO: It should be possible to register specializations of the template type
	// TODO: The specialization's factory doesn't take the hidden asIObjectType parameter
	// TODO: The specialization must implement all the methods/behaviours of the template type
	// TODO: Must not be possible to register specialization before the template type

	// TODO: Test that a proper error occurs if the instance of a template causes invalid data types, e.g. int@

	// TODO: Test bytecode serialization with template instances

	engine->Release();

 	return fail;
}

} // namespace

