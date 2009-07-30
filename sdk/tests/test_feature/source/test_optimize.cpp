#include "utils.h"
using namespace std;

static const char * const TESTNAME = "TestOptimize";

// TODO: asIScriptFunction should have a method for querying the size of the bytecode. I can 
//       use that to automate some of the regressions tests for bytecode optimization. With it
//       I won't accidentally increase the size of the produced bytecode with compiler changes.

static const char *script1 =
"void TestOptimizeAdd()        \n"
"{                             \n"
"  int a = 43, b = 13;         \n"
"  g_i0 = a + b;               \n"
"  g_i1 = a + 13;              \n"
"  g_i2 = 43 + b;              \n"
"  g_i3 = 43 + 13;             \n"
"  g_i4 = a;                   \n"
"  g_i4 += b;                  \n"
"  g_i5 = a;                   \n"
"  g_i5 += 13;                 \n"
"}                             \n"
"void TestOptimizeSub()        \n"
"{                             \n"
"  int a = 43, b = 13;         \n"
"  g_i0 = a - b;               \n"
"  g_i1 = a - 13;              \n"
"  g_i2 = 43 - b;              \n"
"  g_i3 = 43 - 13;             \n"
"  g_i4 = a;                   \n"
"  g_i4 -= b;                  \n"
"  g_i5 = a;                   \n"
"  g_i5 -= 13;                 \n"
"}                             \n"
"void TestOptimizeMul()        \n"
"{                             \n"
"  int a = 43, b = 13;         \n"
"  g_i0 = a * b;               \n"
"  g_i1 = a * 13;              \n"
"  g_i2 = 43 * b;              \n"
"  g_i3 = 43 * 13;             \n"
"  g_i4 = a;                   \n"
"  g_i4 *= b;                  \n"
"  g_i5 = a;                   \n"
"  g_i5 *= 13;                 \n"
"}                             \n"
"void TestOptimizeDiv()        \n"
"{                             \n"
"  int a = 43, b = 13;         \n"
"  g_i0 = a / b;               \n"
"  g_i1 = a / 13;              \n"
"  g_i2 = 43 / b;              \n"
"  g_i3 = 43 / 13;             \n"
"  g_i4 = a;                   \n"
"  g_i4 /= b;                  \n"
"  g_i5 = a;                   \n"
"  g_i5 /= 13;                 \n"
"}                             \n"
"void TestOptimizeMod()        \n"
"{                             \n"
"  int a = 43, b = 13;         \n"
"  g_i0 = a % b;               \n"
"  g_i1 = a % 13;              \n"
"  g_i2 = 43 % b;              \n"
"  g_i3 = 43 % 13;             \n"
"  g_i4 = a;                   \n"
"  g_i4 %= b;                  \n"
"  g_i5 = a;                   \n"
"  g_i5 %= 13;                 \n"
"}                             \n"
"void TestOptimizeAdd64()      \n"
"{                             \n"
"  int64 a = 43, b = 13;       \n"
"  g_i64_0 = a + b;            \n"
"  g_i64_1 = a + 13;           \n"
"  g_i64_2 = 43 + b;           \n"
"  g_i64_3 = 43 + 13;          \n"
"  g_i64_4 = a;                \n"
"  g_i64_4 += b;               \n"
"  g_i64_5 = a;                \n"
"  g_i64_5 += 13;              \n"
"}                             \n"
"void TestOptimizeSub64()      \n"
"{                             \n"
"  int64 a = 43, b = 13;       \n"
"  g_i64_0 = a - b;            \n"
"  g_i64_1 = a - 13;           \n"
"  g_i64_2 = 43 - b;           \n"
"  g_i64_3 = 43 - 13;          \n"
"  g_i64_4 = a;                \n"
"  g_i64_4 -= b;               \n"
"  g_i64_5 = a;                \n"
"  g_i64_5 -= 13;              \n"
"}                             \n"
"void TestOptimizeMul64()      \n"
"{                             \n"
"  int64 a = 43, b = 13;       \n"
"  g_i64_0 = a * b;            \n"
"  g_i64_1 = a * 13;           \n"
"  g_i64_2 = 43 * b;           \n"
"  g_i64_3 = 43 * 13;          \n"
"  g_i64_4 = a;                \n"
"  g_i64_4 *= b;               \n"
"  g_i64_5 = a;                \n"
"  g_i64_5 *= 13;              \n"
"}                             \n"
"void TestOptimizeDiv64()      \n"
"{                             \n"
"  int64 a = 43, b = 13;       \n"
"  g_i64_0 = a / b;            \n"
"  g_i64_1 = a / 13;           \n"
"  g_i64_2 = 43 / b;           \n"
"  g_i64_3 = 43 / 13;          \n"
"  g_i64_4 = a;                \n"
"  g_i64_4 /= b;               \n"
"  g_i64_5 = a;                \n"
"  g_i64_5 /= 13;              \n"
"}                             \n"
"void TestOptimizeMod64()      \n"
"{                             \n"
"  int64 a = 43, b = 13;       \n"
"  g_i64_0 = a % b;            \n"
"  g_i64_1 = a % 13;           \n"
"  g_i64_2 = 43 % b;           \n"
"  g_i64_3 = 43 % 13;          \n"
"  g_i64_4 = a;                \n"
"  g_i64_4 %= b;               \n"
"  g_i64_5 = a;                \n"
"  g_i64_5 %= 13;              \n"
"}                             \n"
"void TestOptimizeAddf()       \n"
"{                             \n"
"  float a = 43, b = 13;       \n"
"  g_f0 = a + b;               \n"
"  g_f1 = a + 13;              \n"
"  g_f2 = 43 + b;              \n"
"  g_f3 = 43.0f + 13;          \n"
"  g_f4 = a;                   \n"
"  g_f4 += b;                  \n"
"  g_f5 = a;                   \n"
"  g_f5 += 13;                 \n"
"}                             \n"
"void TestOptimizeSubf()       \n"
"{                             \n"
"  float a = 43, b = 13;       \n"
"  g_f0 = a - b;               \n"
"  g_f1 = a - 13;              \n"
"  g_f2 = 43 - b;              \n"
"  g_f3 = 43.0f - 13;          \n"
"  g_f4 = a;                   \n"
"  g_f4 -= b;                  \n"
"  g_f5 = a;                   \n"
"  g_f5 -= 13;                 \n"
"}                             \n"
"void TestOptimizeMulf()       \n"
"{                             \n"
"  float a = 43, b = 13;       \n"
"  g_f0 = a * b;               \n"
"  g_f1 = a * 13;              \n"
"  g_f2 = 43 * b;              \n"
"  g_f3 = 43.0f * 13;          \n"
"  g_f4 = a;                   \n"
"  g_f4 *= b;                  \n"
"  g_f5 = a;                   \n"
"  g_f5 *= 13;                 \n"
"}                             \n"
"void TestOptimizeDivf()       \n"
"{                             \n"
"  float a = 43, b = 13;       \n"
"  g_f0 = a / b;               \n"
"  g_f1 = a / 13;              \n"
"  g_f2 = 43 / b;              \n"
"  g_f3 = 43.0f / 13.0f;       \n"
"  g_f4 = a;                   \n"
"  g_f4 /= b;                  \n"
"  g_f5 = a;                   \n"
"  g_f5 /= 13;                 \n"
"}                             \n"
"void TestOptimizeModf()       \n"
"{                             \n"
"  float a = 43, b = 13;       \n"
"  g_f0 = a % b;               \n"
"  g_f1 = a % 13;              \n"
"  g_f2 = 43 % b;              \n"
"  g_f3 = 43.0f % 13;          \n"
"  g_f4 = a;                   \n"
"  g_f4 %= b;                  \n"
"  g_f5 = a;                   \n"
"  g_f5 %= 13;                 \n"
"}                             \n"
"void TestOptimizeAddd()       \n"
"{                             \n"
"  double a = 43.0, b = 13.0;  \n"
"  g_d0 = a + b;               \n"
"  g_d1 = a + 13.0;            \n"
"  g_d2 = 43.0 + b;            \n"
"  g_d3 = 43.0 + 13.0;         \n"
"  g_d4 = a;                   \n"
"  g_d4 += b;                  \n"
"  g_d5 = a;                   \n"
"  g_d5 += 13.0;               \n"
"}                             \n"
"void TestOptimizeSubd()       \n"
"{                             \n"
"  double a = 43.0, b = 13.0;  \n"
"  g_d0 = a - b;               \n"
"  g_d1 = a - 13.0;            \n"
"  g_d2 = 43.0 - b;            \n"
"  g_d3 = 43.0 - 13.0;         \n"
"  g_d4 = a;                   \n"
"  g_d4 -= b;                  \n"
"  g_d5 = a;                   \n"
"  g_d5 -= 13.0;               \n"
"}                             \n"
"void TestOptimizeMuld()       \n"
"{                             \n"
"  double a = 43.0, b = 13.0;  \n"
"  g_d0 = a * b;               \n"
"  g_d1 = a * 13.0;            \n"
"  g_d2 = 43.0 * b;            \n"
"  g_d3 = 43.0 * 13.0;         \n"
"  g_d4 = a;                   \n"
"  g_d4 *= b;                  \n"
"  g_d5 = a;                   \n"
"  g_d5 *= 13.0;               \n"
"}                             \n"
"void TestOptimizeDivd()       \n"
"{                             \n"
"  double a = 43.0, b = 13.0;  \n"
"  g_d0 = a / b;               \n"
"  g_d1 = a / 13.0;            \n"
"  g_d2 = 43.0 / b;            \n"
"  g_d3 = 43.0 / 13.0;         \n"
"  g_d4 = a;                   \n"
"  g_d4 /= b;                  \n"
"  g_d5 = a;                   \n"
"  g_d5 /= 13.0;               \n"
"}                             \n"
"void TestOptimizeModd()       \n"
"{                             \n"
"  double a = 43.0, b = 13.0;  \n"
"  g_d0 = a % b;               \n"
"  g_d1 = a % 13.0;            \n"
"  g_d2 = 43.0 % b;            \n"
"  g_d3 = 43.0 % 13.0;         \n"
"  g_d4 = a;                   \n"
"  g_d4 %= b;                  \n"
"  g_d5 = a;                   \n"
"  g_d5 %= 13.0;               \n"
"}                             \n"
"void TestOptimizeAnd()        \n"
"{                             \n"
"  uint a = 0xF3, b = 0x17;    \n"
"  g_b0 = a & b;               \n"
"  g_b1 = a & 0x17;            \n"
"  g_b2 = 0xF3 & b;            \n"
"  g_b3 = 0xF3 & 0x17;         \n"
"  g_b4 = a;                   \n"
"  g_b4 &= b;                  \n"
"  g_b5 = a;                   \n"
"  g_b5 &= 0x17;               \n"
"}                             \n"
"void TestOptimizeOr()         \n"
"{                             \n"
"  uint a = 0xF3, b = 0x17;    \n"
"  g_b0 = a | b;               \n"
"  g_b1 = a | 0x17;            \n"
"  g_b2 = 0xF3 | b;            \n"
"  g_b3 = 0xF3 | 0x17;         \n"
"  g_b4 = a;                   \n"
"  g_b4 |= b;                  \n"
"  g_b5 = a;                   \n"
"  g_b5 |= 0x17;               \n"
"}                             \n"
"void TestOptimizeXor()        \n"
"{                             \n"
"  uint a = 0xF3, b = 0x17;    \n"
"  g_b0 = a ^ b;               \n"
"  g_b1 = a ^ 0x17;            \n"
"  g_b2 = 0xF3 ^ b;            \n"
"  g_b3 = 0xF3 ^ 0x17;         \n"
"  g_b4 = a;                   \n"
"  g_b4 ^= b;                  \n"
"  g_b5 = a;                   \n"
"  g_b5 ^= 0x17;               \n"
"}                             \n"
"void TestOptimizeSLL()        \n"
"{                             \n"
"  uint a = 0xF3;              \n"
"  uint b = 3;                 \n"
"  g_b0 = a << b;              \n"
"  g_b1 = a << 3;              \n"
"  g_b2 = 0xF3 << b;           \n"
"  g_b3 = 0xF3 << 3;           \n"
"  g_b4 = a;                   \n"
"  g_b4 <<= b;                 \n"
"  g_b5 = a;                   \n"
"  g_b5 <<= 3;                 \n"
"}                             \n"
"void TestOptimizeSRA()        \n"
"{                             \n"
"  uint a = 0xF3;              \n"
"  uint b = 3;                 \n"
"  g_b0 = a >>> b;             \n"
"  g_b1 = a >>> 3;             \n"
"  g_b2 = 0xF3 >>> b;          \n"
"  g_b3 = 0xF3 >>> 3;          \n"
"  g_b4 = a;                   \n"
"  g_b4 >>>= b;                \n"
"  g_b5 = a;                   \n"
"  g_b5 >>>= 3;                \n"
"}                             \n"
"void TestOptimizeSRL()        \n"
"{                             \n"
"  uint a = 0xF3;              \n"
"  uint b = 3;                 \n"
"  g_b0 = a >> b;              \n"
"  g_b1 = a >> 3;              \n"
"  g_b2 = 0xF3 >> b;           \n"
"  g_b3 = 0xF3 >> 3;           \n"
"  g_b4 = a;                   \n"
"  g_b4 >>= b;                 \n"
"  g_b5 = a;                   \n"
"  g_b5 >>= 3;                 \n"
"}                             \n"
"void TestOptimizeAnd64()      \n"
"{                             \n"
"  uint64 a = 0xF3, b = 0x17;  \n"
"  g_b64_0 = a & b;            \n"
"  g_b64_1 = a & 0x17;         \n"
"  g_b64_2 = 0xF3 & b;         \n"
"  g_b64_3 = 0xF3 & 0x17;      \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 &= b;               \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 &= 0x17;            \n"
"}                             \n"
"void TestOptimizeOr64()       \n"
"{                             \n"
"  uint64 a = 0xF3, b = 0x17;  \n"
"  g_b64_0 = a | b;            \n"
"  g_b64_1 = a | 0x17;         \n"
"  g_b64_2 = 0xF3 | b;         \n"
"  g_b64_3 = 0xF3 | 0x17;      \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 |= b;               \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 |= 0x17;            \n"
"}                             \n"
"void TestOptimizeXor64()      \n"
"{                             \n"
"  uint64 a = 0xF3, b = 0x17;  \n"
"  g_b64_0 = a ^ b;            \n"
"  g_b64_1 = a ^ 0x17;         \n"
"  g_b64_2 = 0xF3 ^ b;         \n"
"  g_b64_3 = 0xF3 ^ 0x17;      \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 ^= b;               \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 ^= 0x17;            \n"
"}                             \n"
"void TestOptimizeSLL64()      \n"
"{                             \n"
"  uint64 a = 0xF3;            \n"
"  uint64 b = 3;               \n"
"  g_b64_0 = a << b;           \n"
"  g_b64_1 = a << 3;           \n"
"  g_b64_2 = 0xF3 << b;        \n"
"  g_b64_3 = 0xF3 << 3;        \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 <<= b;              \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 <<= 3;              \n"
"}                             \n"
"void TestOptimizeSRA64()      \n"
"{                             \n"
"  uint64 a = 0xF3;            \n"
"  uint64 b = 3;               \n"
"  g_b64_0 = a >>> b;          \n"
"  g_b64_1 = a >>> 3;          \n"
"  g_b64_2 = 0xF3 >>> b;       \n"
"  g_b64_3 = 0xF3 >>> 3;       \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 >>>= b;             \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 >>>= 3;             \n"
"}                             \n"
"void TestOptimizeSRL64()      \n"
"{                             \n"
"  uint64 a = 0xF3;            \n"
"  uint64 b = 3;               \n"
"  g_b64_0 = a >> b;           \n"
"  g_b64_1 = a >> 3;           \n"
"  g_b64_2 = 0xF3 >> b;        \n"
"  g_b64_3 = 0xF3 >> 3;        \n"
"  g_b64_4 = a;                \n"
"  g_b64_4 >>= b;              \n"
"  g_b64_5 = a;                \n"
"  g_b64_5 >>= 3;              \n"
"}                             \n";


static asINT64 g_i64[6];
static int g_i[6];
static float g_f[6];
static double g_d[6];
static asUINT g_b[6];
static asQWORD g_b64[6];

bool TestOptimize()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterGlobalProperty("int g_i0", &g_i[0]);
	engine->RegisterGlobalProperty("int g_i1", &g_i[1]);
	engine->RegisterGlobalProperty("int g_i2", &g_i[2]);
	engine->RegisterGlobalProperty("int g_i3", &g_i[3]);
	engine->RegisterGlobalProperty("int g_i4", &g_i[4]);
	engine->RegisterGlobalProperty("int g_i5", &g_i[5]);
	engine->RegisterGlobalProperty("int64 g_i64_0", &g_i64[0]);
	engine->RegisterGlobalProperty("int64 g_i64_1", &g_i64[1]);
	engine->RegisterGlobalProperty("int64 g_i64_2", &g_i64[2]);
	engine->RegisterGlobalProperty("int64 g_i64_3", &g_i64[3]);
	engine->RegisterGlobalProperty("int64 g_i64_4", &g_i64[4]);
	engine->RegisterGlobalProperty("int64 g_i64_5", &g_i64[5]);
	engine->RegisterGlobalProperty("float g_f0", &g_f[0]);
	engine->RegisterGlobalProperty("float g_f1", &g_f[1]);
	engine->RegisterGlobalProperty("float g_f2", &g_f[2]);
	engine->RegisterGlobalProperty("float g_f3", &g_f[3]);
	engine->RegisterGlobalProperty("float g_f4", &g_f[4]);
	engine->RegisterGlobalProperty("float g_f5", &g_f[5]);
	engine->RegisterGlobalProperty("double g_d0", &g_d[0]);
	engine->RegisterGlobalProperty("double g_d1", &g_d[1]);
	engine->RegisterGlobalProperty("double g_d2", &g_d[2]);
	engine->RegisterGlobalProperty("double g_d3", &g_d[3]);
	engine->RegisterGlobalProperty("double g_d4", &g_d[4]);
	engine->RegisterGlobalProperty("double g_d5", &g_d[5]);
	engine->RegisterGlobalProperty("uint g_b0", &g_b[0]);
	engine->RegisterGlobalProperty("uint g_b1", &g_b[1]);
	engine->RegisterGlobalProperty("uint g_b2", &g_b[2]);
	engine->RegisterGlobalProperty("uint g_b3", &g_b[3]);
	engine->RegisterGlobalProperty("uint g_b4", &g_b[4]);
	engine->RegisterGlobalProperty("uint g_b5", &g_b[5]);
	engine->RegisterGlobalProperty("uint64 g_b64_0", &g_b64[0]);
	engine->RegisterGlobalProperty("uint64 g_b64_1", &g_b64[1]);
	engine->RegisterGlobalProperty("uint64 g_b64_2", &g_b64[2]);
	engine->RegisterGlobalProperty("uint64 g_b64_3", &g_b64[3]);
	engine->RegisterGlobalProperty("uint64 g_b64_4", &g_b64[4]);
	engine->RegisterGlobalProperty("uint64 g_b64_5", &g_b64[5]);


	COutStream out;	

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod->Build();

	int n;
	engine->ExecuteString(0, "TestOptimizeAdd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i[n] != 56 )
		{
			printf("%s: Optimized add failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSub()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i[n] != 30 )
		{
			printf("%s: Optimized sub failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMul()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i[n] != 559 )
		{
			printf("%s: Optimized mul failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeDiv()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i[n] != 3 )
		{
			printf("%s: Optimized div failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMod()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i[n] != 4 )
		{
			printf("%s: Optimized mod failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "g_i0 = 13; g_i1 = -g_i0; g_i2 = -13;"); if( g_i[1] != -13 || g_i[2] != -13 ) { printf("%s: negi failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_i0 = 0; g_i1 = g_i0++; g_i2 = ++g_i0;"); if( g_i[0] != 2 || g_i[1] != 0 || g_i[2] != 2 ) { printf("%s: inci failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_i0 = 0; g_i1 = g_i0--; g_i2 = --g_i0;"); if( g_i[0] != -2 || g_i[1] != 0 || g_i[2] != -2 ) { printf("%s: deci failed\n", TESTNAME); }


	engine->ExecuteString(0, "TestOptimizeAdd64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i64[n] != 56 )
		{
			printf("%s: Optimized add64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSub64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i64[n] != 30 )
		{
			printf("%s: Optimized sub64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMul64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i64[n] != 559 )
		{
			printf("%s: Optimized mul64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeDiv64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i64[n] != 3 )
		{
			printf("%s: Optimized div64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMod64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_i64[n] != 4 )
		{
			printf("%s: Optimized mod64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "g_i64_0 = 13; g_i64_1 = -g_i64_0; g_i64_2 = -13;"); if( g_i64[1] != -13 || g_i64[2] != -13 ) { printf("%s: negi64 failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_i64_0 = 0; g_i64_1 = g_i64_0++; g_i64_2 = ++g_i64_0;"); if( g_i64[0] != 2 || g_i64[1] != 0 || g_i64[2] != 2 ) { printf("%s: inci64 failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_i64_0 = 0; g_i64_1 = g_i64_0--; g_i64_2 = --g_i64_0;"); if( g_i64[0] != -2 || g_i64[1] != 0 || g_i64[2] != -2 ) { printf("%s: deci64 failed\n", TESTNAME); }



	engine->ExecuteString(0, "TestOptimizeAddf()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_f[n] != 56 )
		{
			printf("%s: Optimized addf failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSubf()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_f[n] != 30 )
		{
			printf("%s: Optimized subf failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMulf()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_f[n] != 559 )
		{
			printf("%s: Optimized mulf failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeDivf()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_f[n] != 3.30769230769230748f )
		{
			printf("%s: Optimized divf failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeModf()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_f[n] != 4 )
		{
			printf("%s: Optimized modf failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "g_f0 = 13; g_f1 = -g_f0; g_f2 = -13;"); if( g_f[1] != -13 || g_f[2] != -13 ) { printf("%s: negf failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_f0 = 0; g_f1 = g_f0++; g_f2 = ++g_f0;"); if( g_f[0] != 2 || g_f[1] != 0 || g_f[2] != 2 ) { printf("%s: incf failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_f0 = 0; g_f1 = g_f0--; g_f2 = --g_f0;"); if( g_f[0] != -2 || g_f[1] != 0 || g_f[2] != -2 ) { printf("%s: decf failed\n", TESTNAME); }

	engine->ExecuteString(0, "TestOptimizeAddd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_d[n] != 56 )
		{
			printf("%s: Optimized addd failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSubd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_d[n] != 30 )
		{
			printf("%s: Optimized subd failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeMuld()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_d[n] != 559 )
		{
			printf("%s: Optimized muld failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeDivd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_d[n] != 3.30769230769230748 )
		{
			printf("%s: Optimized divd failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeModd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_d[n] != 4 )
		{
			printf("%s: Optimized modd failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "g_d0 = 13; g_d1 = -g_d0; g_d2 = -13;"); if( g_d[1] != -13 || g_d[2] != -13 ) { printf("%s: negd failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_d0 = 0; g_d1 = g_d0++; g_d2 = ++g_d0;"); if( g_d[0] != 2 || g_d[1] != 0 || g_d[2] != 2 ) { printf("%s: incd failed\n", TESTNAME); }
	engine->ExecuteString(0, "g_d0 = 0; g_d1 = g_d0--; g_d2 = --g_d0;"); if( g_d[0] != -2 || g_d[1] != 0 || g_d[2] != -2 ) { printf("%s: decd failed\n", TESTNAME); }

	engine->ExecuteString(0, "TestOptimizeAnd()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3 & 0x17) )
		{
			printf("%s: Optimized and failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeOr()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3 | 0x17) )
		{
			printf("%s: Optimized or failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeXor()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3 ^ 0x17) )
		{
			printf("%s: Optimized xor failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSLL()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3 << 3) )
		{
			printf("%s: Optimized sll failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSRL()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3u >> 3) )
		{
			printf("%s: Optimized srl failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSRA()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b[n] != (0xF3 >> 3) )
		{
			printf("%s: Optimized sra failed\n", TESTNAME);
			break;
		}
	}
	
	engine->ExecuteString(0, "g_b0 = 0xF3; g_b1 = ~g_b0; g_b2 = ~0xF3;"); if( g_b[1] != ~0xF3 || g_b[2] != ~0xF3 ) { printf("%s: bnot failed\n", TESTNAME); }


	engine->ExecuteString(0, "TestOptimizeAnd64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3 & 0x17) )
		{
			printf("%s: Optimized and64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeOr64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3 | 0x17) )
		{
			printf("%s: Optimized or64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeXor64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3 ^ 0x17) )
		{
			printf("%s: Optimized xor64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSLL64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3 << 3) )
		{
			printf("%s: Optimized sll64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSRL64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3u >> 3) )
		{
			printf("%s: Optimized srl64 failed\n", TESTNAME);
			break;
		}
	}

	engine->ExecuteString(0, "TestOptimizeSRA64()");
	for( n = 0; n < 6; ++ n )
	{
		if( g_b64[n] != (0xF3 >> 3) )
		{
			printf("%s: Optimized sra64 failed\n", TESTNAME);
			break;
		}
	}
	
	engine->ExecuteString(0, "g_b64_0 = 0xF3; g_b64_1 = ~g_b64_0; g_b64_2 = ~uint64(0xF3);"); if( g_b64[1] != ~I64(0xF3) || g_b64[2] != ~I64(0xF3) ) { printf("%s: bnot64 failed\n", TESTNAME); }



	int r;
	r = engine->ExecuteString(0, "bool b = false; if( !b ) {}");
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: !b failed\n", TESTNAME);
		fail = true;
	}
		
	r = engine->ExecuteString(0, "bool b = false; b = not b;");
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: !b failed\n", TESTNAME);
		fail = true;
	}

	r = engine->ExecuteString(0, "uint tmp = 50; uint x = tmp + 0x50;");
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: uint failed\n", TESTNAME);
		fail = true;
	}

	bool boolValue;
	engine->RegisterGlobalProperty("bool boolValue", &boolValue);
	r = engine->ExecuteString(0, "bool a = false, b = false; boolValue = a xor b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = false; boolValue = a xor b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = true; boolValue = a xor b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = false, b = true; boolValue = a xor b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;

	r = engine->ExecuteString(0, "bool a = false, b = false; boolValue = a and b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = false; boolValue = a and b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = true; boolValue = a and b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = false, b = true; boolValue = a and b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;

	r = engine->ExecuteString(0, "bool a = false, b = false; boolValue = a or b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = false; boolValue = a or b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = true; boolValue = a or b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = false, b = true; boolValue = a or b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;

	r = engine->ExecuteString(0, "bool a = false, b = false; boolValue = a == b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = false; boolValue = a == b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = true; boolValue = a == b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = false, b = true; boolValue = a == b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;

	r = engine->ExecuteString(0, "bool a = false, b = false; boolValue = a != b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = false; boolValue = a != b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;
	r = engine->ExecuteString(0, "bool a = true, b = true; boolValue = a != b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != false ) fail = true;
	r = engine->ExecuteString(0, "bool a = false, b = true; boolValue = a != b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;

	r = engine->ExecuteString(0, "bool a = false; bool b = a == false; boolValue = b;"); assert( r == asEXECUTION_FINISHED );
	if( boolValue != true ) fail = true;

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );
	r = engine->ExecuteString(0, "assert(float(5-10) == float(-5));");
	if( r != asEXECUTION_FINISHED ) fail = true;

	engine->Release();

	// Success
	return fail;
}
