//
// Tests constant properties to see if they can be overwritten
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestConstProperty
{

#define TESTNAME "TestConstProperty"

class CVec3
{
public:
	CVec3() {}
	CVec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	float x,y,z;
};

class CObj
{
public:
	CVec3 simplevec;
	CVec3 constvec;

	CObj() {}
	~CObj() {}
};

CVec3 vec3add(const CVec3& v1, const CVec3& v2)
{
	return CVec3(v1.x+v2.x,v1.y+v2.y,v1.z+v2.z);
}

static const char *script =
"void Init()          \n"
"{                    \n"
"  CObj someObj;      \n"
"  CVec3 someVec;     \n"
"  someVec = someObj.simplevec + someObj.constvec; \n"
"  someVec = vec3add(someObj.simplevec,someObj.constvec); \n"
"}                    \n";

static const char *script2 =
//"Obj1 myObj1;         \n"
//"Obj2 myObj2;         \n"
"float myFloat;       \n"
"                     \n"
"void Init()          \n"
"{                    \n"
//"  g_Obj1 = myObj1;   \n"
//"  g_Obj2 = myObj2;   \n"
"  g_Float = myFloat; \n"
"}                    \n";


bool Test()
{
	bool fail = false;

	// TEST 1
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("CVec3", sizeof(CVec3), asOBJ_CLASS_C);
	engine->RegisterObjectProperty("CVec3", "float x", offsetof(CVec3,x));
	engine->RegisterObjectProperty("CVec3", "float y", offsetof(CVec3,y));
	engine->RegisterObjectProperty("CVec3", "float z", offsetof(CVec3,z));

	engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "CVec3 f(const CVec3 &in, const CVec3 &in)", asFUNCTION(vec3add), asCALL_CDECL);

	engine->RegisterGlobalFunction("CVec3 vec3add(const CVec3 &in, const CVec3 &in)", asFUNCTION(vec3add), asCALL_CDECL);

	engine->RegisterObjectType("CObj", sizeof(CObj), asOBJ_CLASS_CD);
	engine->RegisterObjectProperty("CObj", "CVec3 simplevec", offsetof(CObj,simplevec));
	engine->RegisterObjectProperty("CObj", "const CVec3 constvec", offsetof(CObj,constvec));

	CBufferedOutStream out;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script, strlen(script), 0);
	engine->Build(0);

	if( !out.buffer.empty() )
	{
		printf("%s: Failed to pass argument as 'const type &in'\n%s", TESTNAME, out.buffer.c_str());
		fail = true;
	}

	engine->Release();

	// TEST 2
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Obj1", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectProperty("Obj1", "int val", 0);
	engine->RegisterObjectBehaviour("Obj1", asBEHAVE_ASSIGNMENT, "Obj1 &f(Obj1 &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterObjectType("Obj2", sizeof(int), asOBJ_PRIMITIVE);
	engine->RegisterObjectProperty("Obj2", "int val", 0);

//	int constantProperty1 = 0;
//	engine->RegisterGlobalProperty("const Obj1 g_Obj1", &constantProperty1);

//	int constantProperty2 = 0;
//	engine->RegisterGlobalProperty("const Obj2 g_Obj2", &constantProperty2);

	float constantFloat = 0;
	engine->RegisterGlobalProperty("const float g_Float", &constantFloat);

	out.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	engine->Build(0);

	if( out.buffer != "TestConstProperty (3, 1) : Info    : Compiling void Init()\n"
		              "TestConstProperty (5, 11) : Error   : Reference is read-only\n" )
	{
		printf("%s: Failed to detect all properties as constant\n%s", TESTNAME, out.buffer.c_str());
		fail = true;
	}

	engine->Release();


	// Success
	return fail;
}

} // namespace

