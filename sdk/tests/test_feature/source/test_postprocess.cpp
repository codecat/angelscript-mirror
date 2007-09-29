#include "utils.h"

namespace TestPostProcess
{

#define TESTNAME "TestPostProcess"


static const char *script = 
"void pickOrDrop(ClientData @client, int slot)  \n"
"{                                              \n"
"    Actor@ player;                             \n"
"    @player=@client.getActor(0);               \n"
"    if(player==null)                           \n"
"        return;//no player actor yet           \n"
"                                               \n"
"    bool useSecondary;                         \n"
"    int itemInSlot=0;                          \n"
"    if(itemInSlot>=0){                         \n"
"        useSecondary=true;                     \n"
"    }                                          \n"
"    else{                                      \n"
"        Actor@ p2;                             \n"
"        @p2=@client.getActor(0);               \n"
"        if(p2!=null){                          \n"
"            return;                            \n"
"        }                                      \n"
"    }                                          \n"
"}                                              \n";

bool Test()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	int r;
	r = engine->RegisterObjectType("ClientData", 4, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ClientData", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);  assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ClientData", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("Actor", 4, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Actor", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);  assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Actor", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectMethod("ClientData", "Actor @getActor(int)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );


	engine->AddScriptSection(0, "script", script, strlen(script), 0);
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	engine->Release();

	return fail;
}

}


