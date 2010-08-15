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
		"const float aConst = 3.141592f; \n"
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
	engine->RegisterGlobalProperty("const float myConst", &f);
	engine->RegisterGlobalFunction("void func(int &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->BeginConfigGroup("test");
	engine->RegisterGlobalFunction("void func2()", asFUNCTION(0), asCALL_GENERIC);
	engine->EndConfigGroup();

	engine->RegisterEnum("myEnum");
	engine->RegisterEnumValue("myEnum", "value1", 1);
	engine->RegisterEnumValue("myEnum", "value2", 2);

	engine->RegisterFuncdef("void Callback(int a, int b)");

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		fail = true;

	DumpModule(mod);

	// Save/Restore the bytecode and then test again for the loaded bytecode
	CBytecodeStream stream(__FILE__"1");
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

		s << objType->GetName();

		const char *group = objType->GetConfigGroup();
		s << " group: " << (group ? group : "<null>") << endl;
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
			s << " beh(" << beh << ") " << engine->GetFunctionDescriptorById(bid)->GetDeclaration(false) << endl;
		}
	}

	// Show methods
	for( int m = 0; m < objType->GetMethodCount(); m++ )
	{
		s << " " << objType->GetMethodDescriptorByIndex(m)->GetDeclaration(false) << endl;
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
		int eid;
		const char *ename = mod->GetEnumByIndex(n, &eid);

		s << "enum: " << ename << endl;

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
		bool isConst;
		const char *group;
		engine->GetGlobalPropertyByIndex(n, &name, &typeId, &isConst, &group);
		s << "reg prop: ";
		if( isConst ) 
			s << "const ";
		s << engine->GetTypeDeclaration(typeId) << " " << name;
		s << " group: " << (group ? group : "<null>") << endl;
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
		const char *group = engine->GetFunctionDescriptorById(funcId)->GetConfigGroup();
		s << "reg func: " << engine->GetFunctionDescriptorById(funcId)->GetDeclaration() << 
			" group: " << (group ? group : "<null>") << endl;
	}

	// Enumerate registered enums
	c = engine->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int eid;
		const char *ename = engine->GetEnumByIndex(n, &eid);

		s << "reg enum: " << ename << endl;

		// List enum values
		for( int e = 0; e < engine->GetEnumValueCount(eid); e++ )
		{
			int value;
			const char *name = engine->GetEnumValueByIndex(eid, e, &value);
			s << " " << name << " = " << value << endl;
		}
	}

	// Enumerate registered func defs
	c = engine->GetFuncdefCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *funcdef = engine->GetFuncdefByIndex(n);

		s << "reg funcdef: " << funcdef->GetDeclaration() << endl;
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
	if( s.str() != 
		"func: void Test()\n"
		"type: class A : I\n"
		" A@ A()\n"
		" void i(float)\n"
		" void a(int)\n"
		" float f\n"
		"type: class B : A, I\n"
		" B@ B()\n"
		" B@ B(int)\n"
		" void i(float)\n"
		" void a(int)\n"
		" float f\n"
		"type: interface I\n"
		" void i(float)\n"
		"global: float a\n"
		"global: const float aConst\n"
		"global: E e\n"
		"global: float pi\n"
		"global: I@ i\n"
		"enum: E\n"
		" eval = 0\n"
		" eval2 = 2\n"
		"typedef: real => float\n"
		"import: void ImpFunc() from \"mod\"\n"
		"-------\n"
		"reg prop: float f group: <null>\n"
		"reg prop: const float myConst group: <null>\n"
		"reg typedef: myFloat => float\n"
		"reg func: void func(int&in) group: <null>\n"
		"reg func: void func2() group: test\n"
		"reg enum: myEnum\n"
		" value1 = 1\n"
		" value2 = 2\n"
		"reg funcdef: void Callback(int, int)\n"
		"string factory: string\n"
		"reg type: val string group: <null>\n"
		" beh(1) void _beh_1_()\n"
		" beh(0) void _beh_0_()\n"
		" beh(10) uint8& _beh_10_(uint)\n"
		" beh(10) const uint8& _beh_10_(uint) const\n"
		" string& opAssign(const string&in)\n"
		" string& opAddAssign(const string&in)\n"
		" bool opEquals(const string&in) const\n"
		" int opCmp(const string&in) const\n"
		" string opAdd(const string&in) const\n"
#if defined(__LP64__) || defined(_WIN64)
		" uint64 length() const\n"
		" void resize(uint64)\n"
#else
		" uint length() const\n"
		" void resize(uint)\n"
#endif
		" string& opAssign(double)\n"
		" string& opAddAssign(double)\n"
		" string opAdd(double) const\n"
		" string opAdd_r(double) const\n"
		" string& opAssign(int)\n"
		" string& opAddAssign(int)\n"
		" string opAdd(int) const\n"
		" string opAdd_r(int) const\n"
		" string& opAssign(uint)\n"
		" string& opAddAssign(uint)\n"
		" string opAdd(uint) const\n"
		" string opAdd_r(uint) const\n" 
		" string& opAssign(bool)\n"
		" string& opAddAssign(bool)\n"
		" string opAdd(bool) const\n"
		" string opAdd_r(bool) const\n" )
	{
		cout << s.str() << endl;
		cout << "Failed to get the expected result when dumping the module" << endl << endl;
	}
}


} // namespace

