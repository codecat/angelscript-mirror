#include "utils.h"
#include "../../../add_on/scriptmath3d/scriptmath3d.h"

#define TESTNAME "TestVector3"

static char *script =
"vector3 TestVector3()  \n"
"{                      \n"
"  vector3 v;           \n"
"  v.x=1;               \n"
"  v.y=2;               \n"
"  v.z=3;               \n"
"  return v;            \n"
"}                      \n"
"vector3 TestVector3Val(vector3 v)  \n"
"{                                  \n"
"  return v;                        \n"
"}                                  \n"
"void TestVector3Ref(vector3 &out v)\n"
"{                                  \n"
"  v.x=1;                           \n"
"  v.y=2;                           \n"
"  v.z=3;                           \n"
"}                                  \n";

bool TestVector3()
{
	bool fail = false;
	COutStream out;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptMath3D(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	Vector3 v;
	engine->RegisterGlobalProperty("vector3 v", &v);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script);
	r = mod->Build();
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		fail = true;
	}
	else
	{
		// Internal return
		r = engine->ExecuteString(0, "v = TestVector3();");
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			fail = true;
		}
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		// Manual return
		v.x = 0; v.y = 0; v.z = 0;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionIdByDecl("vector3 TestVector3()"));

		ctx->Execute();
		Vector3 *ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 1 || ret->y != 2 || ret->z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(mod->GetFunctionIdByDecl("vector3 TestVector3Val(vector3)"));
		v.x = 3; v.y = 2; v.z = 1;
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 3 || ret->y != 2 || ret->z != 1 )
		{
			printf("%s: Failed to pass Vector3 by val\n", TESTNAME);
			fail = true;
		}

		ctx->Prepare(mod->GetFunctionIdByDecl("void TestVector3Ref(vector3 &out)"));
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to pass Vector3 by ref\n", TESTNAME);
			fail = true;
		}

		ctx->Release();
	}

	// Assignment of temporary object
	r = engine->ExecuteString(0, "vector3 v; float x = (v = vector3(10.0f,7,8)).x; assert( x > 9.9999f && x < 10.0001f );");
	if( r != asEXECUTION_FINISHED )
	{
		fail = true;
	}

	engine->Release();

	return fail;
}
