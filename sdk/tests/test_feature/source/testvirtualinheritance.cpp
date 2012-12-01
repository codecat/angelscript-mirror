//
// This test was designed to test the functionality of methods 
// from classes with virtual inheritance
//
// Author: Andreas Jönsson
//

#include "utils.h"

static const char * const TESTNAME = "TestVirtualInheritance";

static std::string output2;

class CVBase1
{
public:
	CVBase1() {me1 = "CVBase1";}
	virtual void CallMe1() 
	{
		output2 += me1; 
		output2 += ": "; 
		output2 += "CVBase1::CallMe1()\n";
	}
	const char *me1;
};

class CVBase2
{
public:
	CVBase2() {me2 = "CVBase2";}
	virtual void CallMe2() 
	{
		output2 += me2; 
		output2 += ": "; 
		output2 += "CVBase2::CallMe2()\n";
	}
	const char *me2;
};

#ifdef _MSC_VER
// This part forces the compiler to use a generic method pointer for CDerivedVirtual methods
class CDerivedVirtual;
static const int CDerivedVirtual_ptrsize = sizeof(void (CDerivedVirtual::*)());
#endif

class CDerivedVirtual : virtual public CVBase1, virtual public CVBase2
{
public:
	CDerivedVirtual() : CVBase1(), CVBase2() {}
};

// This one is registered without the forward declaration, which means MSVC will choose the 
// method pointer that doesn't include all the information needed. Still AngelScript should
// be able to tell that it is for a class with virtual inheritance and return an error
class CDerivedVirtual2 : virtual public CVBase1, virtual public CVBase2
{
public:
	CDerivedVirtual2() : CVBase1(), CVBase2() {}
};


bool TestVirtualInheritance()
{
#ifdef __GNUC__
	printf("%s: GNUC: AngelScript cannot detect virtual inheritance thus this test doesn't apply\n", TESTNAME);
	return false;
#else

	bool fail = false;
	int r;
	
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	// Register the derived class that has been forward declared
	r = engine->RegisterObjectType("class1", 0, asOBJ_REF);
	r = engine->RegisterObjectMethod("class1", "void CallMe1()", asMETHOD(CDerivedVirtual, CallMe1), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		TEST_FAILED;
	}

	r = engine->RegisterObjectMethod("class1", "void CallMe2()", asMETHOD(CDerivedVirtual, CallMe2), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		TEST_FAILED;
	}

	// Register the derived class that has not been forward declared
	r = engine->RegisterObjectType("class2", 0, asOBJ_REF);
	r = engine->RegisterObjectMethod("class2", "void CallMe1()", asMETHOD(CDerivedVirtual2, CallMe1), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		TEST_FAILED;
	}

	r = engine->RegisterObjectMethod("class2", "void CallMe2()", asMETHOD(CDerivedVirtual2, CallMe2), asCALL_THISCALL);
	if( r != asNOT_SUPPORTED )
	{
		printf("%s: Registering virtual methods shouldn't be supported.\n", TESTNAME);
		TEST_FAILED;
	}


	// Calling methods for classes with virtual inheritance is not supported so we don't try it
	
	engine->Release();

	return fail;
#endif
}
