#include <stdio.h>
#include <conio.h>
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

bool TestCreateEngine();
bool TestExecute();
bool TestExecute1Arg();
bool TestExecute2Args();
bool TestExecute4Args();
bool TestExecute4Argsf();
bool TestExecute32Args();
bool TestExecuteMixedArgs();
bool TestExecute32MixedArgs();
bool TestExecuteThis32MixedArgs();
bool TestReturn();
bool TestReturnF();
bool TestReturnD();
bool TestTempVar();
bool TestExecuteScript();
bool Test2Modules();
bool TestStdcall4Args();
bool TestInt64();
bool TestModuleRef();
bool TestEnumGlobVar();
bool TestGlobalVar();
bool TestBStr();
bool TestBStr2();
bool TestSwitch();
bool TestNegateOperator();
bool TestException();
bool TestCDecl_Class();
bool TestCDecl_ClassA();
bool TestCDecl_ClassC();
bool TestCDecl_ClassD();
bool TestNotComplexThisCall();
bool TestNotComplexStdcall();
bool TestReturnWithCDeclObjFirst();
bool TestStdString();
bool TestLongToken();
bool TestVirtualMethod();
bool TestMultipleInheritance();
bool TestVirtualInheritance();
bool TestStack();
bool TestExecuteString();
bool TestCondition();
bool TestFuncOverload();
bool TestNeverVisited();
bool TestNested();
bool TestConstructor();
bool TestOptimize();
bool TestNotInitialized();
bool TestVector3();

namespace TestSuspend         { bool Test(); }
namespace TestConstProperty   { bool Test(); }
namespace TestOutput          { bool Test(); }
namespace TestImport          { bool Test(); }
namespace Test2Func           { bool Test(); }
namespace TestDiscard         { bool Test(); }
namespace TestCircularImport  { bool Test(); }
namespace TestMultiAssign     { bool Test(); }
namespace TestSaveLoad        { bool Test(); }
namespace TestConstructor2    { bool Test(); }
namespace TestScriptCall      { bool Test(); }
namespace TestArray           { bool Test(); }
namespace TestStdVector       { bool Test(); }
namespace TestArrayObject     { bool Test(); }
namespace TestPointer         { bool Test(); }
namespace TestConversion      { bool Test(); }
namespace TestObject          { bool Test(); }
namespace TestExceptionMemory { bool Test(); }
namespace TestArgRef          { bool Test(); }
namespace TestRefArgument     { bool Test(); }

void DetectMemoryLeaks()
{
#if defined(_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif
}

int main(int argc, char **argv)
{
	DetectMemoryLeaks();



	if( TestRefArgument::Test()       ) goto failed;
	if( TestSaveLoad::Test()          ) goto failed;
	if( TestImport::Test()            ) goto failed;
	if( TestConversion::Test()        ) goto failed;
	if( TestStdString()               ) goto failed;
	if( TestObject::Test()            ) goto failed;
	if( TestArray::Test()             ) goto failed;
	if( TestStdVector::Test()         ) goto failed;
	if( TestArgRef::Test()            ) goto failed;
	if( TestConstructor2::Test()      ) goto failed;
	if( TestFuncOverload()            ) goto failed;
	if( TestArrayObject::Test()       ) goto failed;
	if( TestNested()                  ) goto failed;
	if( TestCondition()               ) goto failed;
	if( TestExceptionMemory::Test()   ) goto failed;
	if( TestSwitch()                  ) goto failed;
	if( TestSuspend::Test()           ) goto failed;
	if( TestConstructor()             ) goto failed;
	if( TestVector3()                 ) goto failed;
	if( TestInt64()                   ) goto failed;
	if( TestGlobalVar()               ) goto failed;
	if( TestNegateOperator()          ) goto failed;
	if( TestReturnWithCDeclObjFirst() ) goto failed;
	if( TestCDecl_Class()             ) goto failed;
	if( TestNotComplexStdcall()       ) goto failed;
	if( TestCDecl_ClassA()            ) goto failed;
	if( TestCDecl_ClassC()            ) goto failed;
	if( TestCDecl_ClassD()            ) goto failed;
	if( TestNotComplexThisCall()      ) goto failed;
	if( TestVirtualMethod()           ) goto failed;
	if( TestMultipleInheritance()     ) goto failed;
	if( TestExecuteString()           ) goto failed;
	if( TestMultiAssign::Test()       ) goto failed;
	if( TestExecuteThis32MixedArgs()  ) goto failed;
	if( TestException()               ) goto failed;
	if( TestOptimize()                ) goto failed;
	if( TestStack()                   ) goto failed;
	if( TestNotInitialized()          ) goto failed;
	if( TestCreateEngine()            ) goto failed;
	if( TestExecute()                 ) goto failed;
	if( TestExecute1Arg()             ) goto failed;
	if( TestExecute2Args()            ) goto failed;
	if( TestExecute4Args()            ) goto failed;
	if( TestExecute4Argsf()           ) goto failed;
	if( TestExecute32Args()           ) goto failed;
	if( TestExecuteMixedArgs()        ) goto failed;
	if( TestExecute32MixedArgs()      ) goto failed;
	if( TestStdcall4Args()            ) goto failed;
	if( TestReturn()                  ) goto failed;
	if( TestReturnF()                 ) goto failed;
	if( TestReturnD()                 ) goto failed;
	if( TestTempVar()                 ) goto failed;
	if( Test2Modules()                ) goto failed;
	if( TestModuleRef()               ) goto failed;
	if( TestEnumGlobVar()             ) goto failed;
	if( TestBStr()                    ) goto failed;
	if( TestBStr2()                   ) goto failed;
	if( TestLongToken()               ) goto failed;
	if( TestVirtualInheritance()      ) goto failed;
	if( TestOutput::Test()            ) goto failed;
	if( Test2Func::Test()             ) goto failed;
	if( TestDiscard::Test()           ) goto failed;
	if( TestCircularImport::Test()    ) goto failed;
	if( TestNeverVisited()            ) goto failed;
	if( TestConstProperty::Test()     ) goto failed;
	if( TestExecuteScript()           ) goto failed;
	if( TestPointer::Test()           ) goto failed;

	printf("--------------------------------------------\n");
	printf("All of the tests passed with success.\n\n");
	printf("Press any key to quit.\n");
	while(!getch());
	return 0;

failed:
	printf("--------------------------------------------\n");
	printf("One of the tests failed, see details above.\n\n");
	printf("Press any key to quit.\n");
	while(!getch());
	return 0;
}
