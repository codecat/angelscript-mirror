#include <assert.h>
#include "stdstring.h"
using namespace std;

static string StringFactory(asUINT length, const char *s)
{
	return string(s);
}

static void ConstructString(string *thisPointer)
{
	new(thisPointer) string();
}

static void DestructString(string *thisPointer)
{
	thisPointer->~string();
}

void RegisterStdString(asIScriptEngine *engine)
{
	int r;

	// Register the bstr type
	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_CLASS_CDA); assert( r >= 0 );

	// Register the bstr factory
	r = engine->RegisterStringFactory("string", asFUNCTION(StringFactory), asCALL_CDECL); assert( r >= 0 );

	// Register the object operator overloads
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                  asFUNCTION(ConstructString), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                  asFUNCTION(DestructString),  asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ASSIGNMENT, "string &f(const string &)", asMETHODPR(string, operator =, (const string&), string&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_ADD_ASSIGN, "string &f(const string &)", asMETHODPR(string, operator+=, (const string&), string&), asCALL_THISCALL); assert( r >= 0 );
	
	// Register the global operator overloads
	r = engine->RegisterGlobalBehaviour(asBEHAVE_EQUAL,       "bool f(const string &, const string &)", asFUNCTIONPR(operator==, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_NOTEQUAL,    "bool f(const string &, const string &)", asFUNCTIONPR(operator!=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_LEQUAL,      "bool f(const string &, const string &)", asFUNCTIONPR(operator<=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_GEQUAL,      "bool f(const string &, const string &)", asFUNCTIONPR(operator>=, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_LESSTHAN,    "bool f(const string &, const string &)", asFUNCTIONPR(operator <, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_GREATERTHAN, "bool f(const string &, const string &)", asFUNCTIONPR(operator >, (const string &, const string &), bool), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD,         "string f(const string &, const string &)", asFUNCTIONPR(operator +, (const string &, const string &), string), asCALL_CDECL); assert( r >= 0 );

	// Register the object methods
	r = engine->RegisterObjectMethod("string", "uint length()", asMETHOD(string,size), asCALL_THISCALL); assert( r >= 0 );
}


