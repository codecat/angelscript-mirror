//
// Tests of asCALL_THISCALL_OBJLAST and asCALL_THISCALL_OBJFIRST.
//
// Check if returned value when register with this calling conventions are the expected.
//
// Test author: Jordi Oliveras Rovira
//

#include "utils.h"

namespace TestThisCallMethod_ConfigErrors
{
	static const char * const TESTNAME = "TestThisCallMethod_ConfigErrors";

	class TestType
	{
	};

	// Class to use this methods with new calling convention
	class MethodsClass
	{
		public:
			void TestMethod(void* /*thisPtr*/) {  }
	} test;

	// Value type behaviour auxiliar funtions
	namespace Value
	{
		void Constructor(void *) { }

		void Destructor(void *) { }

		void CopyConstructor(void *, void *) { }

		void ListConstructor(void *, void *) { }
	}

	// Reference type behaviour auxiliar funtions
	namespace Refer
	{
		TestType* Factory() { return NULL; }

		void AddRef(void *) { }

		void Release(void *) { }

		TestType* ListFactory(void *, void *) { return NULL; }
	}

	// Check if the returned value is the expected error value
#define CHECK(expected, call) if (expected != (call)) TEST_FAILED;

	// Check if no error
#define CHECK_OK(call) if ((call) < 0) TEST_FAILED;

	bool Test()
	{
		RET_ON_MAX_PORT

		bool fail = false;
		// COutStream out;

		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		// engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		// Value type behaviours checks
		{
			engine->RegisterObjectType("Val", sizeof(TestType), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CD);

			// Constructor behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Value::Constructor), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Value::Constructor), asCALL_THISCALL_OBJLAST));

			// Destructor behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Value::Destructor), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Value::Destructor), asCALL_THISCALL_OBJLAST));

			// Copy constructor behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Value::CopyConstructor), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Value::CopyConstructor), asCALL_THISCALL_OBJLAST));

			// List constructor behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_LIST_CONSTRUCT, "void f(int &in) { float, float }", asFUNCTION(Value::ListConstructor), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Val", asBEHAVE_LIST_CONSTRUCT, "void f(int &in) { float, float }", asFUNCTION(Value::ListConstructor), asCALL_THISCALL_OBJLAST));
		}

		{
			engine->RegisterObjectType("Ref", 0, asOBJ_REF);

			// Factory behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_FACTORY, "Ref@ f()", asFUNCTION(Refer::Factory), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_FACTORY, "Ref@ f()", asFUNCTION(Refer::Factory), asCALL_THISCALL_OBJLAST));

			// Add ref behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(Refer::AddRef), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_ADDREF, "void f()", asFUNCTION(Refer::AddRef), asCALL_THISCALL_OBJLAST));

			// Release behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(Refer::Release), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_RELEASE, "void f()", asFUNCTION(Refer::Release), asCALL_THISCALL_OBJLAST));

			// List factory behaviour
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_LIST_FACTORY, "Ref@ f(int &in) { float, float }",
				asFUNCTION(Value::ListConstructor), asCALL_THISCALL_OBJFIRST));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectBehaviour("Ref", asBEHAVE_LIST_FACTORY, "Ref@ f(int &in) { float, float }",
				asFUNCTION(Value::ListConstructor), asCALL_THISCALL_OBJLAST));
		}

		if (strstr(asGetLibraryOptions(), "THISCALL_METHOD_NO_IMPLEMENTED"))
		{
			// Not suport the new calling conventions
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectMethod("Val", "void Method1()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, &test));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectMethod("Val", "void Method2()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, &test));

			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Val", "void Method3()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, NULL));
			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Val", "void Method4()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, NULL));

			CHECK(asNOT_SUPPORTED, engine->RegisterObjectMethod("Ref", "void Method1()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, &test));
			CHECK(asNOT_SUPPORTED, engine->RegisterObjectMethod("Ref", "void Method2()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, &test));

			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Ref", "void Method3()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, NULL));
			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Ref", "void Method4()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, NULL));
		}
		else
		{
			CHECK_OK(engine->RegisterObjectMethod("Val", "void Method1()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, &test));
			CHECK_OK(engine->RegisterObjectMethod("Val", "void Method2()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, &test));

			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Val", "void Method3()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, NULL));
			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Val", "void Method4()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, NULL));

			CHECK_OK(engine->RegisterObjectMethod("Ref", "void Method1()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, &test));
			CHECK_OK(engine->RegisterObjectMethod("Ref", "void Method2()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, &test));

			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Ref", "void Method3()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJFIRST, NULL));
			CHECK(asINVALID_ARG, engine->RegisterObjectMethod("Ref", "void Method4()", asMETHOD(MethodsClass, TestMethod), asCALL_THISCALL_OBJLAST, NULL));
		}

		engine->Release();

		return fail;
	}
}
