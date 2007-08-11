#include <assert.h>
#include <math.h>
#include <string.h>
#include "scriptmath.h"

BEGIN_AS_NAMESPACE

// The modf function doesn't seem very intuitive, so I'm writing this 
// function that simply returns the fractional part of the float value
float fraction(float v)
{
	float intPart;
	return modff(v, &intPart);
}

void RegisterScriptMath_Native(asIScriptEngine *engine)
{
	int r;

	// Trigonometric functions
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tan(float)", asFUNCTION(tanf), asCALL_CDECL); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("float acos(float)", asFUNCTION(acosf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float asin(float)", asFUNCTION(asinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan(float)", asFUNCTION(atanf), asCALL_CDECL); assert( r >= 0 );

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
	r = engine->RegisterGlobalFunction("float fraction(float)", asFUNCTION(fraction), asCALL_CDECL); assert( r >= 0 );

	// Don't register modf because AngelScript already supports the % operator
}

// This macro creates simple generic wrappers for functions of type 'float func(float)'
#define GENERICff(x) \
void x##_generic(asIScriptGeneric *gen) \
{ \
	float f = *(float*)gen->GetArgPointer(0); \
	*(float*)gen->GetReturnPointer() = x(f); \
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
GENERICff(fraction)

void powf_generic(asIScriptGeneric *gen)
{
	float f1 = *(float*)gen->GetArgPointer(0);
	float f2 = *(float*)gen->GetArgPointer(1);
	*(float*)gen->GetReturnPointer() = powf(f1, f2);
}

void RegisterScriptMath_Generic(asIScriptEngine *engine)
{
	int r;

	// Trigonometric functions
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float tan(float)", asFUNCTION(tanf_generic), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("float acos(float)", asFUNCTION(acosf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float asin(float)", asFUNCTION(asinf_generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float atan(float)", asFUNCTION(atanf_generic), asCALL_GENERIC); assert( r >= 0 );

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
	r = engine->RegisterGlobalFunction("float fraction(float)", asFUNCTION(fraction_generic), asCALL_GENERIC); assert( r >= 0 );

	// Don't register modf because AngelScript already supports the % operator
}

void RegisterScriptMath(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptMath_Generic(engine);
	else
		RegisterScriptMath_Native(engine);
}

END_AS_NAMESPACE


