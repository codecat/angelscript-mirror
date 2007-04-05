#include "utils.h"

namespace TestBool
{

#define TESTNAME "TestBool"

static const char *declarations = "  \n\
bool south = false;              \n\
bool north = true;               \n\
bool east = false;              \n\
bool west = true;               \n\
class MyClass                  \n\
{                              \n\
	string myName;             \n\
	float myFloat;             \n\
	bool myBool1;              \n\
	bool myBool2;              \n\
}                              \n\
MyClass[] a(4);                \n\
int maxCnt = 4;				\n\
int cnt = 0;				   \n\
\n";

static const char *script = "  \n\
void addToArray(string _name, float _myFloat, bool _bool1, bool _bool2) \n\
{							   \n\
	if(maxCnt == cnt)		   \n\
		return;				   \n\
	a[cnt].myName = _name;	   \n\
	a[cnt].myFloat = _myFloat; \n\
	a[cnt].myBool1 = _bool1;   \n\
	a[cnt].myBool2 = _bool2;   \n\
	cnt++;					   \n\
}							   \n\
							   \n\
void MyTest()                  \n\
{                              \n\
  MyClass c;                   \n\
  c.myName = \"test\";         \n\
  c.myFloat = 3.14f;           \n\
  c.myBool1 = south;           \n\
  c.myBool2 = south;           \n\
  Assert(c.myBool1 == false);  \n\
  Assert(c.myBool2 == false);  \n\
  c.myBool1 = north;            \n\
  Assert(c.myBool1 == true);   \n\
  Assert(c.myBool2 == false);  \n\
  c.myBool2 = north;            \n\
  Assert(c.myBool1 == true);   \n\
  Assert(c.myBool2 == true);   \n\
  c.myBool1 = south;           \n\
  Assert(c.myBool1 == false);  \n\
  Assert(c.myBool2 == true);   \n\
  Assert(c.myFloat == 3.14f);  \n\
  CFunc(c.myFloat, c.myBool1, c.myBool2, c.myName); \n\
								\n\
  addToArray(c.myName, 3.14f, south, east); \n\
  addToArray(c.myName, 3.14f, north, east); \n\
  addToArray(c.myName, 3.14f, south, west); \n\
  addToArray(c.myName, 3.14f, north, west); \n\
								\n\
  Assert(a[0].myBool1 == false);  \n\
  Assert(a[0].myBool2 == false);  \n\
  Assert(a[1].myBool1 == true);   \n\
  Assert(a[1].myBool2 == false);  \n\
  Assert(a[2].myBool1 == false);  \n\
  Assert(a[2].myBool2 == true);   \n\
  Assert(a[3].myBool1 == true);   \n\
  Assert(a[3].myBool2 == true);   \n\
  CFunc(a[0].myFloat, a[0].myBool1, a[0].myBool2, a[0].myName); \n\
  CFunc(a[1].myFloat, a[1].myBool1, a[1].myBool2, a[1].myName); \n\
  CFunc(a[2].myFloat, a[2].myBool1, a[2].myBool2, a[2].myName); \n\
  CFunc(a[3].myFloat, a[3].myBool1, a[3].myBool2, a[3].myName); \n\
}                              \n";

static const char *script2 =
"bool gFlag = false;\n"
"void Set() {gFlag = true;}\n"
"void DoNothing() {}\n";

void CFunc(float f, int a, int b, const std::string &name)
{
	if( (a & 0xFFFFFF00) || (b & 0xFFFFFF00) )
	{
		printf("Receiving boolean value with scrap in higher bytes. Not sure this is an error.\n");
	}
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


	// TEST 1
	engine->RegisterGlobalFunction("void CFunc(float, bool, bool, const string &in)", asFUNCTION(CFunc), asCALL_CDECL);

	engine->AddScriptSection(0, "decl", declarations, strlen(declarations));
	engine->AddScriptSection(0, "script", script, strlen(script));
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	r = engine->ExecuteString(0, "MyTest()");
	if( r != asEXECUTION_FINISHED ) fail = true;

	
	// TEST 2
	engine->AddScriptSection(0, "script", script2, strlen(script2));
	r = engine->Build(0);
	if( r < 0 ) fail = true;

	int id = engine->GetGlobalVarIDByName(0, "gFlag");
	bool *flag = (bool*)engine->GetGlobalVarPointer(id);
	*(int*)flag = 0xCDCDCDCD;

	engine->ExecuteString(0, "Set()");
	if( *flag != true )
		fail = true;
	engine->ExecuteString(0, "Assert(gFlag == true)");

	engine->ExecuteString(0, "gFlag = false; DoNothing()");
	if( *flag != false )
		fail = false;
	engine->ExecuteString(0, "Assert(gFlag == false)");

	engine->ExecuteString(0, "gFlag = true; DoNothing()");
	if( *flag != true )
		fail = false;
	engine->ExecuteString(0, "Assert(gFlag == true)");

	engine->Release();

	return fail;
}

} // namespace

