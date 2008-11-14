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

static const char *script2 =
"uint8 gb;              \n"
"uint16 gw;             \n"
"void Test()            \n"
"{                      \n"
"  gb = ReturnByte(1);  \n"
"  Assert(gb == 1);     \n"
"  gb = ReturnByte(0);  \n"
"  Assert(gb == 0);     \n"
"  gw = ReturnWord(1);  \n"
"  Assert(gw == 1);     \n"
"  gw = ReturnWord(0);  \n"
"  Assert(gw == 0);     \n"
"}                      \n";  

// uint8 ReturnByte(uint8)
void ReturnByte(asIScriptGeneric *gen)
{
	asBYTE b = *(asBYTE*)gen->GetAddressOfArg(0);
	// Return a full dword, even though AngelScript should only use a byte
#ifdef __BIG_ENDIAN__
	if( b )
		*(asDWORD*)gen->GetReturnPointer() = 0x00000000 | (int(b)<<24);
	else
		*(asDWORD*)gen->GetReturnPointer() = 0x00FFFFFF | (int(b)<<24);
#else
	if( b )
		*(asDWORD*)gen->GetReturnPointer() = 0x00000000 | b;
	else
		*(asDWORD*)gen->GetReturnPointer() = 0xFFFFFF00 | b;
#endif
}

// uint16 ReturnWord(uint16)
void ReturnWord(asIScriptGeneric *gen)
{
	asWORD w = *(asWORD*)gen->GetAddressOfArg(0);
	// Return a full dword, even though AngelScript should only use a word
#ifdef __BIG_ENDIAN__
	if( w )
		*(asDWORD*)gen->GetReturnPointer() = 0x00000000 | (int(w)<<16);
	else
		*(asDWORD*)gen->GetReturnPointer() = 0X0000FFFF | (int(w)<<16);
#else
	if( w )
		*(asDWORD*)gen->GetReturnPointer() = 0x00000000 | w;
	else
		*(asDWORD*)gen->GetReturnPointer() = 0XFFFF0000 | w;
#endif
}


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

	r = engine->ExecuteString(0, "uint8 newmask = 0XFE; Assert( (newmask & mask0) == 0 );");
	if( r != asEXECUTION_FINISHED ) fail = true;

	r = engine->ExecuteString(0, "uint8 b = 0xFF; b &= ~mask4; BitsTest(b);");
	if( r != asEXECUTION_FINISHED ) fail = true;


	engine->RegisterGlobalFunction("uint8 ReturnByte(uint8)", asFUNCTION(ReturnByte), asCALL_GENERIC);
	engine->RegisterGlobalFunction("uint16 ReturnWord(uint16)", asFUNCTION(ReturnWord), asCALL_GENERIC);
	engine->AddScriptSection(0, "script", script2, strlen(script2));
	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);
	r = engine->Build(0);
	if( r < 0 ) 
		fail = true;
	r = engine->ExecuteString(0, "Test()");
	if( r != asEXECUTION_FINISHED )
		fail = true;

	// bitwise operators should maintain signed/unsigned type of left hand operand
	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = engine->ExecuteString(0, "int a = 0, b = 0; bool c = (a < (b>>1));");
	if( r < 0 )
		fail = true;
	r = engine->ExecuteString(0, "uint a = 0, b = 0; bool c = (a < (b>>1));");
	if( r < 0 )
		fail = true;
	if( bout.buffer != "" )
	{
		printf(bout.buffer.c_str());
		fail = true;
	}
	
	engine->Release();

	// Success
	return fail;
}

} // namespace

