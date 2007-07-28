#include "utils.h"

namespace TestVarType
{

#define TESTNAME "TestVarType"


// AngelScript syntax: void testFuncI(?& in)
// C++ syntax: void testFuncI(void *ref, int typeId)
// void testFuncI(void *ref, int typeId)
void testFuncI(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "int") )
		assert(*(int*)ref == 42);
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "string") )
		assert((*(asCScriptString**)ref)->buffer == "test");
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "string@") )
		assert((*(asCScriptString**)ref)->buffer == "test");
	else
		assert(false);
}

// AngelScript syntax: void testFuncO(?& in)
// C++ syntax: void testFuncO(void *ref, int typeId)
// void testFuncO(void *ref, int typeId)
void testFuncO(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "int") )
		*(int*)ref = 42;
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "string") )
		(*(asCScriptString**)ref)->buffer = "test";
	else if( typeId == gen->GetEngine()->GetTypeIdByDecl(0, "string@") )
		*(asCScriptString**)ref = new asCScriptString("test");
	else
		assert(false);
}

// AngelScript syntax: void testFuncIS(?& in, const string &in)
// C++ syntax: void testFuncIS(void *ref, int typeId, asCScriptString &)
void testFuncIS(asIScriptGeneric *gen)
{
	void *ref = gen->GetArgAddress(0);
	int typeId = gen->GetArgTypeId(0);
	asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(1);

	assert(str->buffer == "test");

	assert(*(int*)ref == 42);
}

// AngelScript syntax: void testFuncSI(const string &in, ?& in)
// C++ syntax: void testFuncSI(asCScriptString &, void *ref, int typeId)
void testFuncSI(asIScriptGeneric *gen)
{
	asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
	void *ref = gen->GetArgAddress(1);
	int typeId = gen->GetArgTypeId(1);

	assert(str->buffer == "test");

	assert(*(int*)ref == 42);
}

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
 	asIScriptEngine *engine = 0;

	// It must not be possible to declare global variables of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script1 = "? globvar;";
	engine->AddScriptSection(0, "script", script1, strlen(script1));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Error   : Unexpected token '?'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare local variables of the var type ?
	const char *script2 = "void func() {? localvar;}";
	engine->AddScriptSection(0, "script", script2, strlen(script2));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
                       "script (1, 14) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to register global properties of the var type ?
	r = engine->RegisterGlobalProperty("? prop", 0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "Property (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must not be possible to register object members of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterObjectType("test", 0, 0); assert( r >= 0 );
	r = engine->RegisterObjectProperty("test", "? prop", 0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "Property (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must not be possible to declare script class members of the var type ?
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script3 = "class c {? member;}";
	engine->AddScriptSection(0, "script", script3, strlen(script3));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 10) : Error   : Expected method or property\n"
		               "script (1, 19) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";
	
	// It must not be possible to declare script functions that take the var type ? as parameter 
	const char *script4 = "void func(?&in a) {}";
	engine->AddScriptSection(0, "script", script4, strlen(script4));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 11) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script functions that return the var type ?
	const char *script5 = "? func() {}";
	engine->AddScriptSection(0, "script", script5, strlen(script5));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Error   : Unexpected token '?'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script class methods that take the var type ? as parameter
	const char *script6 = "class c {void method(?& in a) {}}";
	engine->AddScriptSection(0, "script", script6, strlen(script6));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 22) : Error   : Expected data type\n" 
		               "script (1, 22) : Error   : Expected method or property\n" 
					   "script (1, 33) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare script class methods that return the var type ?
	const char *script7 = "class c {? method() {}}";
	engine->AddScriptSection(0, "script", script7, strlen(script7));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 10) : Error   : Expected method or property\n"
		               "script (1, 23) : Error   : Unexpected token '}'\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare arrays of the var type ?
	const char *script8 = "void func() { ?[] array; }";
	engine->AddScriptSection(0, "script", script8, strlen(script8));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 15) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to declare handles of the var type ?
	const char *script9 = "void func() { ?@ handle; }";
	engine->AddScriptSection(0, "script", script9, strlen(script9));
	r = engine->Build(0);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 15) : Error   : Expected expression value\n" ) fail = true;
	bout.buffer = "";

	// It must not be possible to register functions that return the var type ?
	r = engine->RegisterGlobalFunction("? testFunc()", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r >= 0 ) fail = true;
	if( bout.buffer != "System function (1, 1) : Error   : Expected data type\n" ) fail = true;
	bout.buffer = "";
	engine->Release();

	// It must be possible to register functions that take the var type ? as parameter
	// Only when the expression is explicitly sent as @ should the type id be @
	// const ? & in
	// ? & in
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString(engine);
	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	r = engine->RegisterGlobalFunction("void testFuncI(?& in)", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r < 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncCI(const?&in)", asFUNCTION(testFuncI), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a = 42; testFuncI(a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a = \"test\"; testFuncI(a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string @a = @\"test\"; testFuncI(@a);");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// It must be possible to register with 'out' references
	// ? & out
	r = engine->RegisterGlobalFunction("void testFuncO(?&out)", asFUNCTION(testFuncO), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a; testFuncO(a); assert(a == 42);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string a; testFuncO(a); assert(a == \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "string @a; testFuncO(@a); assert(a == \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// It must be possible to mix normal parameter types with the var type ?
	// e.g. func(const string &in, const ?& in), or func(const ?& in, const string &in)
	r = engine->RegisterGlobalFunction("void testFuncIS(?& in, const string &in)", asFUNCTION(testFuncIS), asCALL_GENERIC);
	if( r < 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncSI(const string &in, ?& in)", asFUNCTION(testFuncSI), asCALL_GENERIC);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "int a = 42; testFuncIS(a, \"test\");");
	if( r != asEXECUTION_FINISHED ) fail = true;
	r = engine->ExecuteString(0, "int a = 42; testFuncSI(\"test\", a);");
	if( r != asEXECUTION_FINISHED ) fail = true;

	// Don't give error on passing reference to const to ?&out
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	const char *script = 
	"class C { string @a; } \n";
	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->Build(0);
	r = engine->ExecuteString(0, "const C c; testFuncO(@c.a);");
	if( r != asEXECUTION_FINISHED ) fail = true;
	if( bout.buffer != "ExecuteString (1, 22) : Warning : Argument cannot be assigned. Output will be discarded.\n" ) fail = true;
	bout.buffer = "";

	// It must be possible to overload functions that take parameters of the var type ?. 
	// The var type function has less priority
	
	// Don't allow ?& with operators yet

	// Don't allow use of ? without being reference

	// It must be possible to use native functions

	// If the user registers a container class that can hold any type, how can we prevent circular references?
	// "class C { var v; } void func() { C c; c.v.store(@c); }"

	// The built-in any type must handle at run time the type validation to make sure it knows how to handle the type that is used

	// It must be possible to register with 'inout' references, in which case only objects that support object handles can be used
	// ? & [inout]
	// const ? & [inout]
/*	r = engine->RegisterGlobalFunction("void testFuncIO(?&)", asFUNCTION(testFunc), asCALL_CDECL);
	if( r < 0 ) fail = true;
	r = engine->RegisterGlobalFunction("void testFuncCIO(const?&)", asFUNCTION(testFunc), asCALL_CDECL);
	if( r < 0 ) fail = true;
*/

	engine->Release();

	return fail;
}

} // namespace

