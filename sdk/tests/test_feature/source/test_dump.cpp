#include "utils.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace TestDump
{

void DumpModule(asIScriptModule *mod);

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

	const char *script = 
		"void Test() {} \n"
		"class A : I { void i(float) {} void a(int) {} float f; } \n"
		"class B : A { B(int) {} } \n"
		"interface I { void i(float); } \n"
		"float a; \n"
		"I@ i; \n"
		"enum E { eval = 0, eval2 = 2 } \n"
		"E e; \n"
		"typedef float real; \n"
		"real pi = 3.14f; \n"
		"import void ImpFunc() from \"mod\"; \n";

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		fail = true;

	DumpModule(mod);

	// Save/Restore the bytecode and then test again for the loaded bytecode
	CBytecodeStream stream;
	mod->SaveByteCode(&stream);

	mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
	mod->LoadByteCode(&stream);

	DumpModule(mod);

	engine->Release();

	return fail;
}

void DumpModule(asIScriptModule *mod)
{
	int c, n;
	asIScriptEngine *engine = mod->GetEngine();
	stringstream s;

	// Enumerate global functions
	c = mod->GetFunctionCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *func = mod->GetFunctionDescriptorByIndex(n);
		s << "func: " << func->GetDeclaration() << endl;
	}

	// Enumerate object types
	c = mod->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		asIObjectType *objType = mod->GetObjectTypeByIndex(n);

		if( objType->GetSize() ) 
		{
			string inheritance;
			if( objType->GetBaseType() )
				inheritance += objType->GetBaseType()->GetName();

			for( int i = 0; i < objType->GetInterfaceCount(); i++ )
			{
				if( inheritance.length() )
					inheritance += ", ";
				inheritance += objType->GetInterface(i)->GetName();
			}

			s << "type: class " << objType->GetName() << " : " << inheritance << endl;
		}
		else
		{
			s << "type: interface " << objType->GetName() << endl;
		}

		// Show factory functions
		for( int f = 0; f < objType->GetFactoryCount(); f++ )
		{
			s << " " << engine->GetFunctionDescriptorById(objType->GetFactoryIdByIndex(f))->GetDeclaration() << endl;
		}

		// Show methods
		for( int m = 0; m < objType->GetMethodCount(); m++ )
		{
			s << " " << objType->GetMethodDescriptorByIndex(m)->GetDeclaration() << endl;
		}

		// Show properties
		for( int p = 0; p < objType->GetPropertyCount(); p++ )
		{
			s << " " << engine->GetTypeDeclaration(objType->GetPropertyTypeId(p)) << " " << objType->GetPropertyName(p) << endl;
		}
	}

	// Enumerate global variables
	c = mod->GetGlobalVarCount();
	for( n = 0; n < c; n++ )
	{
		s << "global: " << mod->GetGlobalVarDeclaration(n) << endl;
	}

	// Enumerate enums
	c = mod->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int eid = mod->GetEnumTypeIdByIndex(n);

		s << "enum: " << engine->GetTypeDeclaration(eid) << endl;

		// List enum values
		for( int e = 0; e < mod->GetEnumValueCount(eid); e++ )
		{
			int value;
			const char *name = mod->GetEnumValueByIndex(eid, e, &value);
			s << " " << name << " = " << value << endl;
		}
	}

	// Enumerate type defs
	c = mod->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int tid;
		const char *name = mod->GetTypedefByIndex(n, &tid);

		s << "typedef: " << name << " => " << engine->GetTypeDeclaration(tid) << endl;
	}

	// Enumerate imported functions
	c = mod->GetImportedFunctionCount();
	for( n = 0; n < c; n++ )
	{
		s << "import: " << mod->GetImportedFunctionDeclaration(n) << " from \"" << mod->GetImportedFunctionSourceModule(n) << "\"" << endl;
	}

	if( s.str() != "func: void Test()\n"
	               "type: class A : I\n"
	               " A@ A()\n"
	               " void A::i(float)\n"
	               " void A::a(int)\n"
	               " float f\n"
	               "type: class B : A, I\n"
	               " B@ B()\n"
	               " B@ B(int)\n"
	               " void A::i(float)\n"
	               " void A::a(int)\n"
	               " float f\n"
	               "type: interface I\n"
	               " void I::i(float)\n"
	               "global: float a\n"
	               "global: I@ i\n"
	               "global: E e\n"
	               "global: float pi\n"
	               "enum: E\n"
	               " eval = 0\n"
	               " eval2 = 2\n"
	               "typedef: real => float\n"
	               "import: void ImpFunc() from \"mod\"\n" )
	{
		cout << s.str() << endl;
		cout << "Failed to get the expected result when dumping the module" << endl << endl;
	}
}


} // namespace

