#include "utils.h"

namespace TestShark
{

#define TESTNAME "TestShark"

class Point
{
public:
	int iRefCount;
	int x;
	int y;
	Point(void) : iRefCount(1), x(0), y(0) {};
	~Point(void) {};
	inline void Add(Point p) { this->x += p.x; this->y += p.y; };
	inline Point &operator=(const Point &pointOther)
	{
		// leave ref count untouched
		this->x = pointOther.x;
		this->y = pointOther.y;
		return *this;
	}
	inline void AddRef(void) { ++iRefCount; } ;
	inline void Release(void)
	{
		if( --iRefCount == 0 )
		delete this;
	};
};

void Point_Construct(Point *pPoint)
{
	new(pPoint)Point();
}

void Point_Destruct(Point &point)
{
	point.~Point();
}

Point &Point_Assign(Point &point, Point &pointOther)
{
	point = pointOther;
	return point;
}

void Point_Add(Point &point, Point p)
{
	point.Add(p);
}

int *Point_Index(Point &point, int iIndex)
{
	if ((iIndex < 0) || (iIndex > 1))
		asGetActiveContext()->SetException("Point index out of range.");
	if (iIndex == 0)
		return &point.x;
	else
		return &point.y;
}

void Point_AddRef(Point &point)
{
	point.AddRef();
}

void Point_Release(Point &point)
{
	point.Release();
}


static char *script =
"Point AddPoints(Point p1, Point p2) \n"
"{                                   \n"
"Point p3 = p1;                      \n"
"p3.x += p2.x;                       \n"
"p3.y += p2.y;                       \n"
"return p3;                          \n"
"}                                   \n";

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: This test has not been adapted for AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	r = engine->RegisterObjectType("Point", sizeof(Point), asOBJ_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectProperty("Point", "int x", offsetof(Point, x)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("Point", "int y", offsetof(Point, y)); assert( r >= 0 );
	r = engine->RegisterObjectMethod("Point", "void Add(Point)", asFUNCTION(Point_Add), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Point_Construct), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Point_Destruct), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_ASSIGNMENT, "Point &f(Point &in)", asFUNCTION(Point_Assign), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_INDEX, "int &f(int)", asFUNCTION(Point_Index), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_ADDREF, "void f()", asFUNCTION(Point_AddRef), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("Point", asBEHAVE_RELEASE, "void f()", asFUNCTION(Point_Release), asCALL_CDECL_OBJFIRST); assert( r >= 0 );

	COutStream out;
	engine->AddScriptSection(0, TESTNAME, script, strlen(script));
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = engine->Build(0);
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		fail = true;
	}
	else
	{
		// Internal return
		int funcId = engine->GetFunctionIDByName(0, "AddPoints");
		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(funcId);
		Point a, b, c;
		a.x = 1; a.y = 1;
		b.x = 2; b.y = 2;
		ctx->SetArgObject(0, &a);
		ctx->SetArgObject(1, &b);
		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED )
			fail = true;
		Point *ret = (Point*)ctx->GetReturnObject();
		c = *ret;
		ctx->Release();
	}

	engine->Release();

	return fail;
}

} // End namespace