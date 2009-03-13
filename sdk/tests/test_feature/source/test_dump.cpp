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

	RegisterStdString(engine);

	float f;
	engine->RegisterTypedef("myFloat", "float");
	engine->RegisterGlobalProperty("myFloat f", &f);
	engine->RegisterGlobalFunction("void func(int &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterEnum("myEnum");
	engine->RegisterEnumValue("myEnum", "value1", 1);
	engine->RegisterEnumValue("myEnum", "value2", 2);

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

void DumpObjectType(stringstream &s, asIObjectType *objType)
{
	asIScriptEngine *engine = objType->GetEngine();

	if( objType->GetFlags() & asOBJ_SCRIPT_OBJECT )
	{
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
	}
	else
	{
		s << "reg type: ";
		if( objType->GetFlags() & asOBJ_REF )
			s << "ref ";
		else
			s << "val ";

		s << objType->GetName() << endl;
	}

	// Show factory functions
	for( int f = 0; f < objType->GetFactoryCount(); f++ )
	{
		s << " " << engine->GetFunctionDescriptorById(objType->GetFactoryIdByIndex(f))->GetDeclaration() << endl;
	}

	if( !( objType->GetFlags() & asOBJ_SCRIPT_OBJECT ) )
	{
		// Show behaviours
		for( int b = 0; b < objType->GetBehaviourCount(); b++ )
		{
			asEBehaviours beh;
			int bid = objType->GetBehaviourByIndex(b, &beh);
			s << " beh(" << beh << ") " << engine->GetFunctionDescriptorById(bid)->GetDeclaration() << endl;
		}
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
		DumpObjectType(s, mod->GetObjectTypeByIndex(n));
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

	s << "-------" << endl;

	// Enumerate registered global properties
	c = engine->GetGlobalPropertyCount();
	for( n = 0; n < c; n++ )
	{
		const char *name;
		int typeId;
		engine->GetGlobalPropertyByIndex(n, &name, &typeId);
		s << "reg prop: " << engine->GetTypeDeclaration(typeId) << " " << name << endl;
	}

	// Enumerate registered typedefs
	c = engine->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		const char *name = engine->GetTypedefByIndex(n, &typeId);
		s << "reg typedef: " << name << " => " << engine->GetTypeDeclaration(typeId) << endl;
	}

	// Enumerate registered global functions
	c = engine->GetGlobalFunctionCount();
	for( n = 0; n < c; n++ )
	{
		int funcId = engine->GetGlobalFunctionIdByIndex(n);
		s << "reg func: " << engine->GetFunctionDescriptorById(funcId)->GetDeclaration() << endl;
	}

	// Enumerate registered enums
	c = engine->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int eid = engine->GetEnumTypeIdByIndex(n);

		s << "reg enum: " << engine->GetTypeDeclaration(eid) << endl;

		// List enum values
		for( int e = 0; e < engine->GetEnumValueCount(eid); e++ )
		{
			int value;
			const char *name = engine->GetEnumValueByIndex(eid, e, &value);
			s << " " << name << " = " << value << endl;
		}
	}

	// Get the string factory return type
	int typeId = engine->GetStringFactoryReturnTypeId();
	s << "string factory: " << engine->GetTypeDeclaration(typeId) << endl;

	// Enumerate registered types
	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		DumpObjectType(s, engine->GetObjectTypeByIndex(n));
	}

	//--------------------------------
	// Validate the dump
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
	               "import: void ImpFunc() from \"mod\"\n"
	               "-------\n"
	               "reg prop: float f\n" 
	               "reg typedef: myFloat => float\n" 
				   "reg func: void func(int&in)\n" 
				   "reg enum: myEnum\n"
				   " value1 = 1\n"
				   " value2 = 2\n"
				   "string factory: string\n"
				   "reg type: val string\n"
				   " beh(1) void string::?()\n"
				   " beh(0) void string::?()\n"
				   " beh(9) string& string::?(const string&in)\n"
				   " beh(10) string& string::?(const string&in)\n"
				   " beh(7) uint8& string::?(uint)\n"
				   " beh(7) const uint8& string::?(uint) const\n"
				   " beh(9) string& string::?(double)\n"
				   " beh(10) string& string::?(double)\n"
				   " beh(9) string& string::?(int)\n"
				   " beh(10) string& string::?(int)\n"
				   " beh(9) string& string::?(uint)\n"
				   " beh(10) string& string::?(uint)\n"
				   " uint string::length() const\n" )
	{
		cout << s.str() << endl;
		cout << "Failed to get the expected result when dumping the module" << endl << endl;
	}
}


} // namespace

