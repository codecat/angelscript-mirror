#include <stdio.h>
#if defined(WIN32)
#include <conio.h>
#endif
#if defined(_MSC_VER)
#include <crtdbg.h>
#endif
#include "angelscript.h"

namespace TestBasic        { void Test(double *time); }
namespace TestBasic2       { void Test(double *time); }
namespace TestCall         { void Test(double *time); }
namespace TestCall2        { void Test(double *time); }
namespace TestFib          { void Test(double *time); }
namespace TestInt          { void Test(double *time); }
namespace TestIntf         { void Test(double *time); }
namespace TestMthd         { void Test(double *time); }
namespace TestString       { void Test(double *time); }
namespace TestStringPooled { void Test(double *time); }
namespace TestString2      { void Test(double *time); }
namespace TestThisProp     { void Test(double *time); }
namespace TestVector3      { void Test(double *time); }
namespace TestAssign       { void Test(double *times); }

// Times for 2.23.0
double testTimesOrig[18] = 
{1.25, // Basic
.277,  // Basic2
1.17,  // Call
1.55,  // Call2
1.70,  // Fib
.280,  // Int
1.78,  // Intf
1.56,  // Mthd
1.83,  // String
.910,  // String2
1.14,  // StringPooled
.714,  // ThisProp
.382,  // Vector3
.00459,// Assign.1
.00663,// Assign.2
.00406,// Assign.3
.00551,// Assign.4
.00560 // Assign.5
};

double testTimesBest[18];
double testTimes[18];

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

	printf("Performance test");
#ifdef _DEBUG 
	printf(" (DEBUG)");
#endif
	printf("\n");
	printf("AngelScript %s\n", asGetLibraryVersion()); 

	for( int n = 0; n < 18; n++ )
		testTimesBest[n] = 1000;

	for( int n = 0; n < 3; n++ )
	{
		TestBasic::Test(&testTimes[0]); printf(".");
		TestBasic2::Test(&testTimes[1]); printf(".");
		TestCall::Test(&testTimes[2]); printf(".");
		TestCall2::Test(&testTimes[3]); printf(".");
		TestFib::Test(&testTimes[4]); printf(".");
		TestInt::Test(&testTimes[5]); printf(".");
		TestIntf::Test(&testTimes[6]); printf(".");
		TestMthd::Test(&testTimes[7]); printf(".");
		TestString::Test(&testTimes[8]); printf(".");
		TestString2::Test(&testTimes[9]); printf(".");
		TestStringPooled::Test(&testTimes[10]); printf(".");
		TestThisProp::Test(&testTimes[11]); printf(".");
		TestVector3::Test(&testTimes[12]); printf(".");
		TestAssign::Test(&testTimes[13]); printf(".");

		for( int t = 0; t < 18; t++ )
		{
			if( testTimesBest[t] > testTimes[t] )
				testTimesBest[t] = testTimes[t];
		}

		printf("\n");
	}
	
	printf("Basic          %.3f    %.3f%s\n", testTimesOrig[ 0], testTimesBest[ 0], testTimesBest[ 0] < testTimesOrig[ 0] ? " <+>" : ""); 
	printf("Basic2         %.3f    %.3f%s\n", testTimesOrig[ 1], testTimesBest[ 1], testTimesBest[ 1] < testTimesOrig[ 1] ? " <+>" : ""); 
	printf("Call           %.3f    %.3f%s\n", testTimesOrig[ 2], testTimesBest[ 2], testTimesBest[ 2] < testTimesOrig[ 2] ? " <+>" : ""); 
	printf("Call2          %.3f    %.3f%s\n", testTimesOrig[ 3], testTimesBest[ 3], testTimesBest[ 3] < testTimesOrig[ 3] ? " <+>" : ""); 
	printf("Fib            %.3f    %.3f%s\n", testTimesOrig[ 4], testTimesBest[ 4], testTimesBest[ 4] < testTimesOrig[ 4] ? " <+>" : ""); 
	printf("Int            %.3f    %.3f%s\n", testTimesOrig[ 5], testTimesBest[ 5], testTimesBest[ 5] < testTimesOrig[ 5] ? " <+>" : ""); 
	printf("Intf           %.3f    %.3f%s\n", testTimesOrig[ 6], testTimesBest[ 6], testTimesBest[ 6] < testTimesOrig[ 6] ? " <+>" : ""); 
	printf("Mthd           %.3f    %.3f%s\n", testTimesOrig[ 7], testTimesBest[ 7], testTimesBest[ 7] < testTimesOrig[ 7] ? " <+>" : ""); 
	printf("String         %.3f    %.3f%s\n", testTimesOrig[ 8], testTimesBest[ 8], testTimesBest[ 8] < testTimesOrig[ 8] ? " <+>" : ""); 
	printf("String2        %.3f    %.3f%s\n", testTimesOrig[ 9], testTimesBest[ 9], testTimesBest[ 9] < testTimesOrig[ 9] ? " <+>" : ""); 
	printf("StringPooled   %.3f    %.3f%s\n", testTimesOrig[10], testTimesBest[10], testTimesBest[10] < testTimesOrig[10] ? " <+>" : ""); 
	printf("ThisProp       %.3f    %.3f%s\n", testTimesOrig[11], testTimesBest[11], testTimesBest[11] < testTimesOrig[11] ? " <+>" : ""); 
	printf("Vector3        %.3f    %.3f%s\n", testTimesOrig[12], testTimesBest[12], testTimesBest[12] < testTimesOrig[12] ? " <+>" : ""); 
	printf("Assign.1       %.5f  %.5f%s\n",   testTimesOrig[13], testTimesBest[13], testTimesBest[13] < testTimesOrig[13] ? " <+>" : ""); 
	printf("Assign.2       %.5f  %.5f%s\n",   testTimesOrig[14], testTimesBest[14], testTimesBest[14] < testTimesOrig[14] ? " <+>" : ""); 
	printf("Assign.3       %.5f  %.5f%s\n",   testTimesOrig[15], testTimesBest[15], testTimesBest[15] < testTimesOrig[15] ? " <+>" : ""); 
	printf("Assign.4       %.5f  %.5f%s\n",   testTimesOrig[16], testTimesBest[16], testTimesBest[16] < testTimesOrig[16] ? " <+>" : ""); 
	printf("Assign.5       %.5f  %.5f%s\n",   testTimesOrig[17], testTimesBest[17], testTimesBest[17] < testTimesOrig[17] ? " <+>" : ""); 

	printf("--------------------------------------------\n");
	printf("Press any key to quit.\n");
#if defined(WIN32)
	while(!getch());
#endif
	return 0;
}
