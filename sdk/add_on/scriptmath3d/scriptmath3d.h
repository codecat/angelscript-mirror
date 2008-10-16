#ifndef SCRIPTMATH3D_H
#define SCRIPTMATH3D_H

#include <angelscript.h>

BEGIN_AS_NAMESPACE

struct Vector3
{
	Vector3();
	Vector3(const Vector3 &other);
	Vector3(float x, float y, float z);

	Vector3 &operator=(const Vector3 &other);
	Vector3 &operator+=(const Vector3 &other);
	Vector3 &operator-=(const Vector3 &other);
	Vector3 &operator*=(float scalar);
	Vector3 &operator/=(float scalar);

	float length() const;

	float x;
	float y;
	float z;
};

bool operator==(const Vector3 &a, const Vector3 &b);
bool operator!=(const Vector3 &a, const Vector3 &b);
Vector3 operator+(const Vector3 &a, const Vector3 &b);
Vector3 operator-(const Vector3 &a, const Vector3 &b);
Vector3 operator*(float s, const Vector3 &v);
Vector3 operator*(const Vector3 &v, float s);
Vector3 operator/(const Vector3 &v, float s);

// This function will determine the configuration of the engine
// and use one of the two functions below to register the string type
void RegisterScriptMath3D(asIScriptEngine *engine);

// Call this function to register the math functions using native calling conventions
void RegisterScriptMath3D_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptMath3D_Native(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
