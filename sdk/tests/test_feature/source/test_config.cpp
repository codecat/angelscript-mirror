#include "utils.h"

namespace TestConfig
{

#define TESTNAME "TestConfig"

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;
	CBufferedOutStream bout;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->ClearMessageCallback(); // Make sure this works
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	r = engine->RegisterGlobalFunction("void func(mytype)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalFunction("void func(int &)", asFUNCTION(0), asCALL_GENERIC);
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( r >= 0 ) fail = true;
	}
	else
	{
		if( r < 0 ) fail = true;
	}
	
	r = engine->RegisterObjectType("mytype", 0, 0);
	if( r < 0 ) fail = true;

	r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_CONSTRUCT, "void f(othertype)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "type f(type &, int)", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) fail = true;

	r = engine->RegisterGlobalProperty("type a", 0);
	if( r >= 0 ) fail = true;

	r = engine->RegisterObjectMethod("mytype", "void method(int &)", asFUNCTION(0), asCALL_GENERIC);
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( r >= 0 ) fail = true;
	}
	else
	{
		if( r < 0 ) fail = true;
	}

	r = engine->RegisterObjectProperty("mytype", "type a", 0);
	if( r >= 0 ) fail = true;

	r = engine->RegisterStringFactory("type", asFUNCTION(0), asCALL_GENERIC);
	if( r >= 0 ) fail = true;

	// Verify the output messages
	if( !engine->GetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES) )
	{
		if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
						   "System function (1, 15) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n"
						   "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
						   "System function (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "System function (1, 8) : Error   : Identifier 'type' is not a data type\n"
						   "System function (1, 13) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "System function (1, 17) : Error   : Only object types that support object handles can use &inout. Use &in or &out instead\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (1, 1) : Error   : Identifier 'type' is not a data type\n" )
			fail = true;
	}
	else
	{
		if( bout.buffer != "System function (1, 11) : Error   : Identifier 'mytype' is not a data type\n"
						   "System function (1, 8) : Error   : Identifier 'othertype' is not a data type\n"
						   "System function (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "System function (1, 8) : Error   : Identifier 'type' is not a data type\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   "Property (1, 1) : Error   : Identifier 'type' is not a data type\n"
						   " (1, 1) : Error   : Identifier 'type' is not a data type\n")
			fail = true;
	}

	engine->Release();

	// Success
 	return fail;
}

} // namespace

