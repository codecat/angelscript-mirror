#include "utils.h"

namespace TestBits
{

#define TESTNAME "TestBits"

static const char *script = "\n\
const uint8	mask0=1;         \n\
const uint8	mask1=1<<1;      \n\
const uint8	mask2=1<<2;      \n\
const uint8	mask3=1<<3;      \n\
const uint8	mask4=1<<4;      \n\
const uint8	mask5=1<<5;      \n\
                             \n\
void BitsTest(uint8 b)       \n\
{                            \n\
  Assert((b&mask4) == 0);    \n\
}                            \n";

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "uint8 newmask = 0xFF, mask = 0x15; Assert( (newmask & ~mask) == 0xEA );");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "uint8 newmask = 0xFF; newmask = newmask & (~mask2) & (~mask3) & (~mask5); Assert( newmask == 0xD3 );");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "uint8 newmask = 0xFE; Assert( (newmask & mask0) == 0 );");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "uint8 b = 0xFF; b &= ~mask4; BitsTest(b);");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	// Success
	return fail;
}

} // namespace

