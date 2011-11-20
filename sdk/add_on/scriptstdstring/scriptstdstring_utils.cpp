#include <assert.h>
#include "scriptstdstring.h"
#include "../scriptarray/scriptarray.h"
#include <stdio.h>
#include <string.h>

using namespace std;

BEGIN_AS_NAMESPACE


// This function takes an input string and splits it into parts by looking
// for a specified delimiter. Example:
//
// string str = "A|B||D";
// array<string>@ array = str.split("|");
//
// The resulting array has the following elements:
//
// {"A", "B", "", "D"}
//
// AngelScript signature:
// array<string>@ string::split(const string &in delim) const
static void StringSplit_Generic(asIScriptGeneric *gen)
{
    // Obtain a pointer to the engine
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();

    // TODO: This should only be done once
	// TODO: This assumes that CScriptArray was already registered
	asIObjectType *arrayType = engine->GetObjectTypeById(engine->GetTypeIdByDecl("array<string>"));

    // Create the array object
    CScriptArray *array = new CScriptArray(0, arrayType);

    // Get the arguments
    string *str   = (string*)gen->GetObject();
    string *delim = *(string**)gen->GetAddressOfArg(0);

    // Find the existence of the delimiter in the input string
    int pos = 0, prev = 0, count = 0;
    while( (pos = (int)str->find(*delim, prev)) != (int)string::npos )
    {
        // Add the part to the array
        array->Resize(array->GetSize()+1);
        ((string*)array->At(count))->assign(&(*str)[prev], pos-prev);

        // Find the next part
        count++;
        prev = pos + (int)delim->length();
    }

    // Add the remaining part
    array->Resize(array->GetSize()+1);
    ((string*)array->At(count))->assign(&(*str)[prev]);

    // Return the array by handle
    *(CScriptArray**)gen->GetAddressOfReturnLocation() = array;
}



// This function takes as input an array of string handles as well as a
// delimiter and concatenates the array elements into one delimited string.
// Example:
//
// array<string> array = {"A", "B", "", "D"};
// string str = join(array, "|");
//
// The resulting string is:
//
// "A|B||D"
//
// AngelScript signature:
// string join(const array<string> &in array, const string &in delim)
static void StringJoin_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    CScriptArray  *array = *(CScriptArray**)gen->GetAddressOfArg(0);
    string *delim = *(string**)gen->GetAddressOfArg(1);

    // Create the new string
    string str = "";
	if( array->GetSize() )
	{
		int n;
		for( n = 0; n < (int)array->GetSize() - 1; n++ )
		{
			str += *(string*)array->At(n);
			str += *delim;
		}

		// Add the last part
		str += *(string*)array->At(n);
	}

    // Return the string
    new(gen->GetAddressOfReturnLocation()) string(str);
}


// AngelScript signature:
// string formatInt(int64 val, const string &in options, uint width)
static string formatInt(asINT64 value, const string &options, asUINT width)
{
	bool leftJustify = options.find("l") != -1;
	bool padWithZero = options.find("0") != -1;
	bool alwaysSign  = options.find("+") != -1;
	bool spaceOnSign = options.find(" ") != -1;
	bool hexSmall    = options.find("h") != -1;
	bool hexLarge    = options.find("H") != -1;

	string fmt = "%";
	if( leftJustify ) fmt += "-";
	if( alwaysSign ) fmt += "+";
	if( spaceOnSign ) fmt += " ";
	if( padWithZero ) fmt += "0";

#ifdef __GNUC__
#ifdef _LP64
	fmt += "*l";
#else
	fmt += "*ll";
#endif
#else
	fmt += "*I64";
#endif

	if( hexSmall ) fmt += "x";
	else if( hexLarge ) fmt += "X";
	else fmt += "d";

	string buf;
	buf.resize(width+20);
#if _MSC_VER >= 1400 // MSVC 8.0 / 2005
	sprintf_s(&buf[0], buf.size(), fmt.c_str(), width, value);
#else
	sprintf(&buf[0], fmt.c_str(), width, value);
#endif
	buf.resize(strlen(&buf[0]));
	
	return buf;
}

// AngelScript signature:
// string formatFloat(double val, const string &in options, uint width, uint precision)
static string formatFloat(double value, const string &options, asUINT width, asUINT precision)
{
	bool leftJustify = options.find("l") != -1;
	bool padWithZero = options.find("0") != -1;
	bool alwaysSign  = options.find("+") != -1;
	bool spaceOnSign = options.find(" ") != -1;
	bool expSmall    = options.find("e") != -1;
	bool expLarge    = options.find("E") != -1;

	string fmt = "%";
	if( leftJustify ) fmt += "-";
	if( alwaysSign ) fmt += "+";
	if( spaceOnSign ) fmt += " ";
	if( padWithZero ) fmt += "0";

	fmt += "*.*";

	if( expSmall ) fmt += "e";
	else if( expLarge ) fmt += "E";
	else fmt += "f";

	string buf;
	buf.resize(width+precision+50);
#if _MSC_VER >= 1400 // MSVC 8.0 / 2005
	sprintf_s(&buf[0], buf.size(), fmt.c_str(), width, precision, value);
#else
	sprintf(&buf[0], fmt.c_str(), width, precision, value);
#endif
	buf.resize(strlen(&buf[0]));
	
	return buf;
}


// This is where the utility functions are registered.
// The string type must have been registered first.
void RegisterStdStringUtils(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", asFUNCTION(StringSplit_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string join(const array<string> &in, const string &in)", asFUNCTION(StringJoin_Generic), asCALL_GENERIC); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatInt(int64 val, const string &in options, uint width = 0)", asFUNCTION(formatInt), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("string formatFloat(double val, const string &in options, uint width = 0, uint precision = 0)", asFUNCTION(formatFloat), asCALL_CDECL); assert(r >= 0);
	// TODO: implement parseInt
	// TODO: implement parseFloat
}

END_AS_NAMESPACE
