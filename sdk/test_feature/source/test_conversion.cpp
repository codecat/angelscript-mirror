#include "utils.h"

namespace TestConversion
{

#define TESTNAME "TestConversion"


void TestI8(char) {}
void TestI16(short) {}
void TestInt(long) {}
void TestUI8(unsigned char) {}
void TestUI16(unsigned short) {}
void TestUInt(unsigned long) {}
void TestFloat(float) {}
void TestDouble(double) {}

void TestI8ByRef(char&) {}
void TestI16ByRef(short&) {}
void TestIntByRef(long&) {}
void TestUI8ByRef(unsigned char&) {}
void TestUI16ByRef(unsigned short&) {}
void TestUIntByRef(unsigned long&) {}
void TestFloatByRef(float&) {}
void TestDoubleByRef(double&) {}
/*
void TestC()
{
	char i8 = 0;
	short i16 = 0;
	long i = 0;
	unsigned char ui8 = 0;
	unsigned short ui16 = 0;
	unsigned long ui = 0;
	float f = 0;
	double d = 0;

	i = i;
	i = i8;
	i = i16;
	i = ui;
	i = ui8;
	i = ui16;
	i = f;
	i = d;
	TestInt(i);
	TestInt(i8);
	TestInt(i16);
	TestInt(ui);
	TestInt(ui8);
	TestInt(ui16);
	TestInt(f);
	TestInt(d);

	i8 = i;
	i8 = i8;
	i8 = i16;
	i8 = ui;
	i8 = ui8;
	i8 = ui16;
	i8 = f;
	i8 = d;
	TestI8(i);
	TestI8(i8);
	TestI8(i16);
	TestI8(ui);
	TestI8(ui8);
	TestI8(ui16);
	TestI8(f);
	TestI8(d);
//	TestI8ByRef(i);
//	TestI8ByRef(i8);
//	TestI8ByRef(i16);
//	TestI8ByRef(ui);
//	TestI8ByRef(ui8);
//	TestI8ByRef(ui16);
//	TestI8ByRef(f);
//	TestI8ByRef(d);

	i16 = i;
	i16 = i8;
	i16 = i16;
	i16 = ui;
	i16 = ui8;
	i16 = ui16;
	i16 = f;
	i16 = d;
	TestI16(i);
	TestI16(i8);
	TestI16(i16);
	TestI16(ui);
	TestI16(ui8);
	TestI16(ui16);
	TestI16(f);
	TestI16(d);
//	TestI16ByRef(i);
//	TestI16ByRef(i8);
//	TestI16ByRef(i16);
//	TestI16ByRef(ui);
//	TestI16ByRef(ui8);
//	TestI16ByRef(ui16);
//	TestI16ByRef(f);
//	TestI16ByRef(d);

	ui = i;
	ui = i8;
	ui = i16;
	ui = ui;
	ui = ui8;
	ui = ui16;
	ui = f;
	ui = d;
	TestUInt(i);
	TestUInt(i8);
	TestUInt(i16);
	TestUInt(ui);
	TestUInt(ui8);
	TestUInt(ui16);
	TestUInt(f);
	TestUInt(d);
//	TestUIntByRef(i);
//	TestUIntByRef(i8);
//	TestUIntByRef(i16);
//	TestUIntByRef(ui);
//	TestUIntByRef(ui8);
//	TestUIntByRef(ui16);
//	TestUIntByRef(f);
//	TestUIntByRef(d);

	ui8 = i;
	ui8 = i8;
	ui8 = i16;
	ui8 = ui;
	ui8 = ui8;
	ui8 = ui16;
	ui8 = f;
	ui8 = d;
	TestUI8(i);
	TestUI8(i8);
	TestUI8(i16);
	TestUI8(ui);
	TestUI8(ui8);
	TestUI8(ui16);
	TestUI8(f);
	TestUI8(d);
//	TestUI8ByRef(i);
//	TestUI8ByRef(i8);
//	TestUI8ByRef(i16);
//	TestUI8ByRef(ui);
//	TestUI8ByRef(ui8);
//	TestUI8ByRef(ui16);
//	TestUI8ByRef(f);
//	TestUI8ByRef(d);

	ui16 = i;
	ui16 = i8;
	ui16 = i16;
	ui16 = ui;
	ui16 = ui8;
	ui16 = ui16;
	ui16 = f;
	ui16 = d;
	TestUI16(i);
	TestUI16(i8);
	TestUI16(i16);
	TestUI16(ui);
	TestUI16(ui8);
	TestUI16(ui16);
	TestUI16(f);
	TestUI16(d);
//	TestUI16ByRef(i);
//	TestUI16ByRef(i8);
//	TestUI16ByRef(i16);
//	TestUI16ByRef(ui);
//	TestUI16ByRef(ui8);
//	TestUI16ByRef(ui16);
//	TestUI16ByRef(f);
//	TestUI16ByRef(d);

	f = i;
	f = i8;
	f = i16;
	f = ui;
	f = ui8;
	f = ui16;
	f = f;
	f = d;
	TestFloat(i);
	TestFloat(i8);
	TestFloat(i16);
	TestFloat(ui);
	TestFloat(ui8);
	TestFloat(ui16);
	TestFloat(f);
	TestFloat(d);
//	TestFloatByRef(i);
//	TestFloatByRef(i8);
//	TestFloatByRef(i16);
//	TestFloatByRef(ui);
//	TestFloatByRef(ui8);
//	TestFloatByRef(ui16);
//	TestFloatByRef(f);
//	TestFloatByRef(d);

	d = i;
	d = i8;
	d = i16;
	d = ui;
	d = ui8;
	d = ui16;
	d = f;
	d = d;
	TestDouble(i);
	TestDouble(i8);
	TestDouble(i16);
	TestDouble(ui);
	TestDouble(ui8);
	TestDouble(ui16);
	TestDouble(f);
	TestDouble(d);
//	TestDoubleByRef(i);
//	TestDoubleByRef(i8);
//	TestDoubleByRef(i16);
//	TestDoubleByRef(ui);
//	TestDoubleByRef(ui8);
//	TestDoubleByRef(ui16);
//	TestDoubleByRef(f);
//	TestDoubleByRef(d);
}
*/

static const char *script =
"void TestScript()                  \n"
"{                                  \n"
"  double a = 1.2345;               \n"
"  TestSFloat(a);                   \n"
"  float b = 1.2345f;               \n"
"  TestSDouble(b);                  \n"
"}                                  \n"
"void TestSFloat(float a)           \n"
"{                                  \n"
"  Assert(a == 1.2345f);            \n"
"}                                  \n"
"void TestSDouble(double a)         \n"
"{                                  \n"
"  Assert(a == double(1.2345f));    \n"
"}                                  \n";


static void Assert(bool expr)
{
	if( !expr )
	{
		printf("Assert failed\n");
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			asIScriptEngine *engine = ctx->GetEngine();
			printf("func: %s\n", engine->GetFunctionDeclaration(ctx->GetCurrentFunction()));
			printf("line: %d\n", ctx->GetCurrentLineNumber());
			ctx->SetException("Assert failed");
		}
	}
}

bool Test()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_CDECL);

	COutStream out;

	float f = 0;
	double d = 0;
	asUINT ui = 0;
	int i = 0;
	char i8 = 0;
	short i16 = 0;
	unsigned char ui8 = 0;
	unsigned short ui16 = 0;
	asBYTE b8 = 0;
	asWORD b16 = 0;
	asDWORD b = 0;
	engine->RegisterGlobalProperty("float f", &f);
	engine->RegisterGlobalProperty("double d", &d);
	engine->RegisterGlobalProperty("uint ui", &ui);
	engine->RegisterGlobalProperty("uint8 ui8", &ui8);
	engine->RegisterGlobalProperty("uint16 ui16", &ui16);
	engine->RegisterGlobalProperty("int i", &i);
	engine->RegisterGlobalProperty("int8 i8", &i8);
	engine->RegisterGlobalProperty("int16 i16", &i16);
	engine->RegisterGlobalProperty("bits b", &b);
	engine->RegisterGlobalProperty("bits8 b8", &b8);
	engine->RegisterGlobalProperty("bits16 b16", &b16);

	engine->RegisterGlobalFunction("void TestDouble(double)", asFUNCTION(TestDouble), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestFloat(float)", asFUNCTION(TestFloat), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestInt(int)", asFUNCTION(TestInt), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestI16(int16)", asFUNCTION(TestI16), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestI8(int8)", asFUNCTION(TestI8), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUInt(uint)", asFUNCTION(TestUInt), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUI16(uint16)", asFUNCTION(TestUI16), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUI8(uint8)", asFUNCTION(TestUI8), asCALL_CDECL);

	engine->RegisterGlobalFunction("void TestDoubleByRef(double &in)", asFUNCTION(TestDoubleByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestFloatByRef(float &in)", asFUNCTION(TestFloatByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestIntByRef(int &in)", asFUNCTION(TestIntByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestI16ByRef(int16 &in)", asFUNCTION(TestI16ByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestI8ByRef(int8 &in)", asFUNCTION(TestI8ByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUIntByRef(uint &in)", asFUNCTION(TestUIntByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUI16ByRef(uint16 &in)", asFUNCTION(TestUI16ByRef), asCALL_CDECL);
	engine->RegisterGlobalFunction("void TestUI8ByRef(uint8 &in)", asFUNCTION(TestUI8ByRef), asCALL_CDECL);

	d    = 12.3;  engine->ExecuteString(0, "d = double(d);", &out);    if( d  !=   12.3  ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "d = double(f);", &out);    if( d  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "d = double(ui);", &out);   if( d  !=  123.0  ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "d = double(ui8);", &out);  if( d  !=  123.0  ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "d = double(ui16);", &out); if( d  !=  123.0  ) fail = true;
	i    = -123;  engine->ExecuteString(0, "d = double(i);", &out);    if( d  != -123.0  ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "d = double(i8);", &out);   if( d  != -123.0  ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "d = double(i16);", &out);   if( d  != -123.0  ) fail = true;

	d    = 12.3;  engine->ExecuteString(0, "d = d;", &out);    if( d  !=   12.3  ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "d = f;", &out);    if( d  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "d = ui;", &out);   if( d  !=  123.0  ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "d = ui8;", &out);  if( d  !=  123.0  ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "d = ui16;", &out); if( d  !=  123.0  ) fail = true;
	i    = -123;  engine->ExecuteString(0, "d = i;", &out);    if( d  != -123.0  ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "d = i8;", &out);   if( d  != -123.0  ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "d = i16;", &out);  if( d  != -123.0  ) fail = true;

	d    = 12.3;  engine->ExecuteString(0, "f = float(d);", &out);     if( f  !=   12.3f ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "f = float(f);", &out);     if( f  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "f = float(ui);", &out);    if( f  !=  123.0f ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "f = float(ui8);", &out);   if( f  !=  123.0f ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "f = float(ui16);", &out);  if( f  !=  123.0f ) fail = true;
	i    = -123;  engine->ExecuteString(0, "f = float(i);", &out);     if( f  != -123.0f ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "f = float(i8);", &out);    if( f  != -123.0f ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "f = float(i16);", &out);   if( f  != -123.0f ) fail = true;

	d    = 12.3;  engine->ExecuteString(0, "f = d;", &out);     if( f  !=   12.3f ) fail = true; 
	f    = 12.3f; engine->ExecuteString(0, "f = f;", &out);     if( f  !=   12.3f ) fail = true; 
	ui   = 123;   engine->ExecuteString(0, "f = ui;", &out);    if( f  !=  123.0f ) fail = true;
	ui8  = 123;   engine->ExecuteString(0, "f = ui8;", &out);   if( f  !=  123.0f ) fail = true;
	ui16 = 123;   engine->ExecuteString(0, "f = ui16;", &out);  if( f  !=  123.0f ) fail = true;
	i    = -123;  engine->ExecuteString(0, "f = i;", &out);     if( f  != -123.0f ) fail = true;
	i8   = -123;  engine->ExecuteString(0, "f = i8;", &out);    if( f  != -123.0f ) fail = true;
	i16  = -123;  engine->ExecuteString(0, "f = i16;", &out);   if( f  != -123.0f ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i = int(d);", &out);      if( i  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i = int(f);", &out);      if( i  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i = int(ui);", &out);     if( i  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i = int(ui8);", &out);    if( i  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i = int(ui16);", &out);   if( i  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i = int(i);", &out);      if( i  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i = int(i8);", &out);     if( i  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i = int(i16);", &out);    if( i  != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i = d;", &out);      if( i  !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i = f;", &out);      if( i  != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i = ui;", &out);     if( i  !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i = ui8;", &out);    if( i  !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i = ui16;", &out);   if( i  !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i = i;", &out);      if( i  != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i = i8;", &out);     if( i  != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i = i16;", &out);    if( i  != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i8 = int8(d);", &out);     if( i8 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i8 = int8(f);", &out);     if( i8 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i8 = int8(ui);", &out);    if( i8 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i8 = int8(ui8);", &out);   if( i8 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i8 = int8(ui16);", &out);  if( i8 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i8 = int8(i);", &out);     if( i8 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i8 = int8(i8);", &out);    if( i8 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i8 = int8(i16);", &out);   if( i8 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i8 = d;", &out);     if( i8 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i8 = f;", &out);     if( i8 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i8 = ui;", &out);    if( i8 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i8 = ui8;", &out);   if( i8 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i8 = ui16;", &out);  if( i8 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i8 = i;", &out);     if( i8 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i8 = i8;", &out);    if( i8 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i8 = i16;", &out);   if( i8 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i16 = int16(d);", &out);    if( i16 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i16 = int16(f);", &out);    if( i16 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i16 = int16(ui);", &out);   if( i16 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i16 = int16(ui8);", &out);  if( i16 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i16 = int16(ui16);", &out); if( i16 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i16 = int16(i);", &out);    if( i16 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i16 = int16(i8);", &out);   if( i16 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i16 = int16(i16);", &out);  if( i16 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "i16 = d;", &out);    if( i16 !=   12 ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "i16 = f;", &out);    if( i16 != - 12 ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "i16 = ui;", &out);   if( i16 !=  123 ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "i16 = ui8;", &out);  if( i16 !=  123 ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "i16 = ui16;", &out); if( i16 !=  123 ) fail = true;
	i    = -123;   engine->ExecuteString(0, "i16 = i;", &out);    if( i16 != -123 ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "i16 = i8;", &out);   if( i16 != -123 ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "i16 = i16;", &out);  if( i16 != -123 ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui = uint(d);", &out);    if( ui != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui = uint(f);", &out);    if( ui != asUINT(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui = uint(ui);", &out);   if( ui !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui = uint(ui8);", &out);  if( ui !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui = uint(ui16);", &out); if( ui !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui = uint(i);", &out);    if( ui != asUINT(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui = uint(i8);", &out);   if( ui != asUINT(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui = uint(i16);", &out);  if( ui != asUINT(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui = d;", &out);    if( ui != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui = f;", &out);    if( ui != asUINT(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui = ui;", &out);   if( ui !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui = ui8;", &out);  if( ui !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui = ui16;", &out); if( ui !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui = i;", &out);    if( ui != asUINT(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui = i8;", &out);   if( ui != asUINT(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui = i16;", &out);  if( ui != asUINT(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui8 = uint8(d);", &out);    if( ui8 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui8 = uint8(f);", &out);    if( ui8 != asBYTE(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui8 = uint8(ui);", &out);   if( ui8 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui8 = uint8(ui8);", &out);  if( ui8 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui8 = uint8(ui16);", &out); if( ui8 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui8 = uint8(i);", &out);    if( ui8 != asBYTE(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui8 = uint8(i8);", &out);   if( ui8 != asBYTE(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui8 = uint8(i16);", &out);  if( ui8 != asBYTE(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui8 = d;", &out);    if( ui8 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui8 = f;", &out);    if( ui8 != asBYTE(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui8 = ui;", &out);   if( ui8 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui8 = ui8;", &out);  if( ui8 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui8 = ui16;", &out); if( ui8 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui8 = i;", &out);    if( ui8 != asBYTE(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui8 = i8;", &out);   if( ui8 != asBYTE(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui8 = i16;", &out);  if( ui8 != asBYTE(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui16 = uint16(d);", &out);    if( ui16 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui16 = uint16(f);", &out);    if( ui16 != asWORD(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui16 = uint16(ui);", &out);   if( ui16 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui16 = uint16(ui8);", &out);  if( ui16 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui16 = uint16(ui16);", &out); if( ui16 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui16 = uint16(i);", &out);    if( ui16 != asWORD(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui16 = uint16(i8);", &out);   if( ui16 != asWORD(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui16 = uint16(i16);", &out);  if( ui16 != asWORD(-123) ) fail = true;

	d    = 12.3;   engine->ExecuteString(0, "ui16 = d;", &out);    if( ui16 != 12           ) fail = true; 
	f    = -12.3f; engine->ExecuteString(0, "ui16 = f;", &out);    if( ui16 != asWORD(-12)  ) fail = true;
	ui   = 123;    engine->ExecuteString(0, "ui16 = ui;", &out);   if( ui16 !=  123         ) fail = true;
	ui8  = 123;    engine->ExecuteString(0, "ui16 = ui8;", &out);  if( ui16 !=  123         ) fail = true;
	ui16 = 123;    engine->ExecuteString(0, "ui16 = ui16;", &out); if( ui16 !=  123         ) fail = true;
	i    = -123;   engine->ExecuteString(0, "ui16 = i;", &out);    if( ui16 != asWORD(-123) ) fail = true;
	i8   = -123;   engine->ExecuteString(0, "ui16 = i8;", &out);   if( ui16 != asWORD(-123) ) fail = true;
	i16  = -123;   engine->ExecuteString(0, "ui16 = i16;", &out);  if( ui16 != asWORD(-123) ) fail = true;

	engine->ExecuteString(0, "TestDouble(d); TestFloat(d); TestInt(d); TestI16(d); TestI8(d); TestUInt(d); TestUI16(d); TestUI8(d);", &out);
	engine->ExecuteString(0, "TestDouble(f); TestFloat(f); TestInt(f); TestI16(f); TestI8(f); TestUInt(f); TestUI16(f); TestUI8(f);", &out);
	engine->ExecuteString(0, "TestDouble(ui); TestFloat(ui); TestInt(ui); TestI16(ui); TestI8(ui); TestUInt(ui); TestUI16(ui); TestUI8(ui);", &out);
	engine->ExecuteString(0, "TestDouble(ui8); TestFloat(ui8); TestInt(ui8); TestI16(ui8); TestI8(ui8); TestUInt(ui8); TestUI16(ui8); TestUI8(ui8);", &out);
	engine->ExecuteString(0, "TestDouble(ui16); TestFloat(ui16); TestInt(ui16); TestI16(ui16); TestI8(ui16); TestUInt(ui16); TestUI16(ui16); TestUI8(ui16);", &out);
	engine->ExecuteString(0, "TestDouble(i); TestFloat(i); TestInt(i); TestI16(i); TestI8(i); TestUInt(i); TestUI16(i); TestUI8(i);", &out);
	engine->ExecuteString(0, "TestDouble(i8); TestFloat(i8); TestInt(i8); TestI16(i8); TestI8(i8); TestUInt(i8); TestUI16(i8); TestUI8(i8);", &out);
	engine->ExecuteString(0, "TestDouble(i16); TestFloat(i16); TestInt(i16); TestI16(i16); TestI8(i16); TestUInt(i16); TestUI16(i16); TestUI8(i16);", &out);

	d = 0; i8 = -22; engine->ExecuteString(0, "d = d + i8"); if( d != -22 ) fail = true;

	engine->ExecuteString(0, "int[] a(1); a[0] == 1", &out);
	engine->ExecuteString(0, "b + i", &out);

	f = 0; engine->ExecuteString(0, "f = float(0x3f800000)", &out); if( f != 1 ) fail = true;

	CBufferedOutStream bout;
	engine->ExecuteString(0, "i == ui", &bout); 
	if( bout.buffer != "ExecuteString (1, 3) : Warning : Signed/Unsigned mismatch\n" )
		fail = true;

	bout.buffer = "";

	int r;
	r = engine->ExecuteString(0, "TestDoubleByRef(d); TestFloatByRef(d); TestIntByRef(d); TestI16ByRef(d); TestI8ByRef(d); TestUIntByRef(d); TestUI16ByRef(d); TestUI8ByRef(d);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(f); TestFloatByRef(f); TestIntByRef(f); TestI16ByRef(f); TestI8ByRef(f); TestUIntByRef(f); TestUI16ByRef(f); TestUI8ByRef(f);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui); TestFloatByRef(ui); TestIntByRef(ui); TestI16ByRef(ui); TestI8ByRef(ui); TestUIntByRef(ui); TestUI16ByRef(ui); TestUI8ByRef(ui);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui8); TestFloatByRef(ui8); TestIntByRef(ui8); TestI16ByRef(ui8); TestI8ByRef(ui8); TestUIntByRef(ui8); TestUI16ByRef(ui8); TestUI8ByRef(ui8);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(ui16); TestFloatByRef(ui16); TestIntByRef(ui16); TestI16ByRef(ui16); TestI8ByRef(ui16); TestUIntByRef(ui16); TestUI16ByRef(ui16); TestUI8ByRef(ui16);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i); TestFloatByRef(i); TestIntByRef(i); TestI16ByRef(i); TestI8ByRef(i); TestUIntByRef(i); TestUI16ByRef(i); TestUI8ByRef(i);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i8); TestFloatByRef(i8); TestIntByRef(i8); TestI16ByRef(i8); TestI8ByRef(i8); TestUIntByRef(i8); TestUI16ByRef(i8); TestUI8ByRef(i8);", &bout); if( r >= 0 ) fail = true;
	r = engine->ExecuteString(0, "TestDoubleByRef(i16); TestFloatByRef(i16); TestIntByRef(i16); TestI16ByRef(i16); TestI8ByRef(i16); TestUIntByRef(i16); TestUI16ByRef(i16); TestUI8ByRef(i16);", &bout); if( r >= 0 ) fail = true;

	
	engine->AddScriptSection(0, "script", script, strlen(script));
	engine->Build(0, &out);

	// This test is to make sure that the float is in fact converted to a double
	engine->ExecuteString(0, "TestScript();", &out);

	engine->Release();

	if( fail )
		printf("%s: failed\n", TESTNAME);

	// Success
	return fail;
}

} // namespace

