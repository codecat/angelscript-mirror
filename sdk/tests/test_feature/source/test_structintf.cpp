#include "utils.h"
#include "../../../add_on/scriptany/scriptany.h"

namespace TestStructIntf
{

static const char * const TESTNAME = "TestStructIntf";

// Normal structure
static const char *script1 =
"struct MyStruct              \n"
"{                            \n"
"   float a;                  \n"
"   string b;                 \n"
"   string @c;                \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"   MyStruct s;               \n"
"   s.a = 3.141592f;          \n"
"   s.b = \"test\";           \n"
"   @s.c = \"test2\";         \n"
"   g_any.store(@s);          \n"
"}                            \n";



CScriptAny *any = 0;

bool Test()
{
	bool fail = false;
	int r;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	any = (CScriptAny*)engine->CreateScriptObject(engine->GetTypeIdByDecl("any"));
	engine->RegisterGlobalProperty("any g_any", any);

	COutStream out;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 ) fail = true;

	// Try retrieving the type Id for the structure
	int typeId = mod->GetTypeIdByDecl("MyStruct");
	if( typeId < 0 )
	{
		printf("%s: Failed to retrieve the type id for the script struct\n", TESTNAME);
		fail = true;
	}

	r = ExecuteString(engine, "Test()", mod);
	if( r != asEXECUTION_FINISHED ) 
		fail = true;
	else
	{		
		asIScriptObject *s = 0;
		typeId = any->GetTypeId();
		any->Retrieve(&s, typeId);

		if( (typeId & asTYPEID_MASK_OBJECT) != asTYPEID_SCRIPTOBJECT )
			fail = true;

		if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct@") )
			fail = true;

		typeId = s->GetTypeId();
		if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct") )
			fail = true;

		if( s->GetPropertyCount() != 3 )
			fail = true;

		if( strcmp(s->GetPropertyName(0), "a") )
			fail = true;

		if( s->GetPropertyTypeId(0) != engine->GetTypeIdByDecl("float") )
			fail = true;

		if( *(float*)s->GetAddressOfProperty(0) != 3.141592f )
			fail = true;

		if( strcmp(s->GetPropertyName(1), "b") )
			fail = true;

		if( s->GetPropertyTypeId(1) != engine->GetTypeIdByDecl("string") )
			fail = true;

		if( ((CScriptString*)s->GetAddressOfProperty(1))->buffer != "test" )
			fail = true;

		if( strcmp(s->GetPropertyName(2), "c") )
			fail = true;

		if( s->GetPropertyTypeId(2) != engine->GetTypeIdByDecl("string@") )
			fail = true;

		if( (*(CScriptString**)s->GetAddressOfProperty(2))->buffer != "test2" )
			fail = true;

		if( s )
			s->Release();
	}

	if( any )
		any->Release();

	// The type id is valid for as long as the type exists
	if( strcmp(engine->GetTypeDeclaration(typeId), "MyStruct") )
		fail = true;

	// Make sure the type is not used anywhere
	engine->DiscardModule(0);
	engine->GarbageCollect();

	// The type id is no longer valid
	if( engine->GetTypeDeclaration(typeId) != 0 )
		fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace

