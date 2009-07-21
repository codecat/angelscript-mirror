#include <assert.h>
#include <math.h>
#include <string.h>
#include "scriptmath.h"

BEGIN_AS_NAMESPACE

// Determine whether the float version should be registered, or the double version
#ifndef AS_USE_FLOAT
#if !defined(_WIN32_WCE) // WinCE doesn't have the float versions of the math functions
#define AS_USE_FLOAT 1
#endif
#endif

// The modf function doesn't seem very intuitive, so I'm writing this 
// function that simply returns the fractional part of the float value
#if AS_USE_FLOAT
float fractionf(float v)
{
	float intPart;
	return modff(v, &intPart);
}
#else
double fraction(double v)
{
	double intPart;
	return modf(v, &intPart);
}
#endif

void RegisterScriptMath_Native(asIScriptEngine *engine)
{
	int r;

#if AS_USE_FLOAT
	// Trigonometric functions
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tan(float)", asFUNCTION(tanf), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("float acos(float)", asFUNCTION(acosf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float asin(float)", asFUNCTION(asinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan(float)", asFUNCTION(atanf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan2(float,float)", asFUNCTION(atan2f), asCALL_CDECL); assert( r >= 0 );

	// Hyberbolic functions
	r = engine->RegisterGlobalFunction("float cosh(float)", asFUNCTION(coshf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sinh(float)", asFUNCTION(sinhf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tanh(float)", asFUNCTION(tanhf), asCALL_CDECL); assert( r >= 0 );

	// Exponential and logarithmic functions
	r = engine->RegisterGlobalFunction("float log(float)", asFUNCTION(logf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float log10(float)", asFUNCTION(log10f), asCALL_CDECL); assert( r >= 0 );

	// Power functions
	r = engine->RegisterGlobalFunction("float pow(float, float)", asFUNCTION(powf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sqrt(float)", asFUNCTION(sqrtf), asCALL_CDECL); assert( r >= 0 );

	// Nearest integer, absolute value, and remainder functions
	r = engine->RegisterGlobalFunction("float ceil(float)", asFUNCTION(ceilf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float abs(float)", asFUNCTION(fabsf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float floor(float)", asFUNCTION(floorf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float fraction(float)", asFUNCTIONPR(fractionf, (float), float), asCALL_CDECL); assert( r >= 0 );

	// Don't register modf because AngelScript already supports the % operator
#else
	// double versions of the same
	r = engine->RegisterGlobalFunction("double cos(double)", asFUNCTION(cos), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sin(double)", asFUNCTION(sin), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double tan(double)", asFUNCTION(tan), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double acos(double)", asFUNCTION(acos), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double asin(double)", asFUNCTION(asin), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double atan(double)", asFUNCTION(atan), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double atan2(double,double)", asFUNCTION(atan2), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double cosh(double)", asFUNCTION(cosh), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sinh(double)", asFUNCTION(sinh), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double tanh(double)", asFUNCTION(tanh), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double log(double)", asFUNCTION(log), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double log10(double)", asFUNCTION(log10), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double pow(double, double)", asFUNCTION(pow), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sqrt(double)", asFUNCTION(sqrt), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double ceil(double)", asFUNCTION(ceil), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double abs(double)", asFUNCTION(fabs), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double floor(double)", asFUNCTION(floor), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double fraction(double)", asFUNCTIONPR(fraction, (double), double), asCALL_CDECL); assert( r >= 0 );
#endif
}

#if AS_USE_FLOAT
// This macro creates simple generic wrappers for functions of type 'float func(float)'
#define GENERICff(x) \
void x##_generic(asIScriptGeneric *gen) \
{ \
	float f = *(float*)gen->GetAddressOfArg(0); \
	*(float*)gen->GetAddressOfReturnLocation() = x(f); \
}

GENERICff(cosf)
GENERICff(sinf)
GENERICff(tanf)
GENERICff(acosf)
GENERICff(asinf)
GENERICff(atanf)
GENERICff(coshf)
GENERICff(sinhf)
GENERICff(tanhf)
GENERICff(logf)
GENERICff(log10f)
GENERICff(sqrtf)
GENERICff(ceilf)
GENERICff(fabsf)
GENERICff(floorf)
GENERICff(fractionf)

void powf_generic(asIScriptGeneric *gen)
{
	float f1 = *(float*)gen->GetAddressOfArg(0);
	float f2 = *(float*)gen->GetAddressOfArg(1);
	*(float*)gen->GetAddressOfReturnLocation() = powf(f1, f2);
}
void atan2f_generic(asIScriptGeneric *gen)
{
	float f1 = *(float*)gen->GetAddressOfArg(0);
	float f2 = *(float*)gen->GetAddressOfArg(1);
	*(float*)gen->GetAddressOfReturnLocation() = atan2f(f1, f2);
}

#else
// This macro creates simple generic wrappers for functions of type 'double func(double)'
#define GENERICdd(x) \
void x##_generic(asIScriptGeneric *gen) \
{ \
	double f = *(double*)gen->GetAddressOfArg(0); \
	*(double*)gen->GetAddressOfReturnLocation() = x(f); \
}

GENERICdd(cos)
GENERICdd(sin)
GENERICdd(tan)
GENERICdd(acos)
GENERICdd(asin)
GENERICdd(atan)
GENERICdd(cosh)
GENERICdd(sinh)
GENERICdd(tanh)
GENERICdd(log)
GENERICdd(log10)
GENERICdd(sqrt)
GENERICdd(ceil)
GENERICdd(fabs)
GENERICdd(floor)
GENERICdd(fraction)

void pow_generic(asIScriptGeneric *gen)
{
	double f1 = *(double*)gen->GetAddressOfArg(0);
	double f2 = *(double*)gen->GetAddressOfArg(1);
	*(double*)gen->GetAddressOfReturnLocation() = pow(f1, f2);
}
void atan2_generic(asIScriptGeneric *gen)
{
	double f1 = *(double*)gen->GetAddressOfArg(0);
	double f2 = *(double*)gen->GetAddressOfArg(1);
	*(double*)gen->GetAddressOfReturnLocation() = atan2(f1, f2);
}
#endif
void RegisterScriptMath_Generic(asIScriptEngine *engine)
{
	int r;

#if AS_USE_FLOAT
	// Trigonometric functions
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tan(float)", asFUNCTION(tanf_generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("float acos(float)", asFUNCTION(acosf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float asin(float)", asFUNCTION(asinf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan(float)", asFUNCTION(atanf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan2(float,float)", asFUNCTION(atan2f_generic), asCALL_GENERIC); assert( r >= 0 );

	// Hyberbolic functions
	r = engine->RegisterGlobalFunction("float cosh(float)", asFUNCTION(coshf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sinh(float)", asFUNCTION(sinhf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tanh(float)", asFUNCTION(tanhf_generic), asCALL_GENERIC); assert( r >= 0 );

	// Exponential and logarithmic functions
	r = engine->RegisterGlobalFunction("float log(float)", asFUNCTION(logf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float log10(float)", asFUNCTION(log10f_generic), asCALL_GENERIC); assert( r >= 0 );

	// Power functions
	r = engine->RegisterGlobalFunction("float pow(float, float)", asFUNCTION(powf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sqrt(float)", asFUNCTION(sqrtf_generic), asCALL_GENERIC); assert( r >= 0 );

	// Nearest integer, absolute value, and remainder functions
	r = engine->RegisterGlobalFunction("float ceil(float)", asFUNCTION(ceilf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float abs(float)", asFUNCTION(fabsf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float floor(float)", asFUNCTION(floorf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float fraction(float)", asFUNCTION(fractionf_generic), asCALL_GENERIC); assert( r >= 0 );

	// Don't register modf because AngelScript already supports the % operator
#else
	// double versions of the same
	r = engine->RegisterGlobalFunction("double cos(double)", asFUNCTION(cos_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sin(double)", asFUNCTION(sin_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double tan(double)", asFUNCTION(tan_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double acos(double)", asFUNCTION(acos_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double asin(double)", asFUNCTION(asin_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double atan(double)", asFUNCTION(atan_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double atan2(double,double)", asFUNCTION(atan2_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double cosh(double)", asFUNCTION(cosh_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sinh(double)", asFUNCTION(sinh_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double tanh(double)", asFUNCTION(tanh_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double log(double)", asFUNCTION(log_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double log10(double)", asFUNCTION(log10_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double pow(double, double)", asFUNCTION(pow_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double sqrt(double)", asFUNCTION(sqrt_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double ceil(double)", asFUNCTION(ceil_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double abs(double)", asFUNCTION(fabs_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double floor(double)", asFUNCTION(floor_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("double fraction(double)", asFUNCTION(fraction_generic), asCALL_GENERIC); assert( r >= 0 );
#endif
}

void RegisterScriptMath(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptMath_Generic(engine);
	else
		RegisterScriptMath_Native(engine);
}

END_AS_NAMESPACE


