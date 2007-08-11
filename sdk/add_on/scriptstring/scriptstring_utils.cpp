#include <assert.h>
#include "scriptstring.h"



// This function returns a string containing the substring of the input string
// determined by the starting index and count of characters.
//
// AngelScript signature:
// string@ substring(const string &in str, int start, int count)
void StringSubString_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    int start = *(int*)gen->GetArgPointer(1);
    int count = *(int*)gen->GetArgPointer(2);

    // Create the substring
    asCScriptString *sub = new asCScriptString();
    sub->buffer = str->buffer.substr(start,count);

    // Return the substring
    *(asCScriptString**)gen->GetReturnPointer() = sub;
}



// This function returns the index of the first position where the substring
// exists in the input string. If the substring doesn't exist in the input
// string -1 is returned.
//
// AngelScript signature:
// int findFirst(const string &in str, const string &in sub, int start)
void StringFindFirst_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *sub = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.find(sub->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
// TODO: Angelscript should permit default parameters
void StringFindFirst0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *sub = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.find(sub->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}



// This function returns the index of the last position where the substring
// exists in the input string. If the substring doesn't exist in the input
// string -1 is returned.
//
// AngelScript signature:
// int findLast(const string &in str, const string &in sub, int start)
void StringFindLast_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *sub = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.rfind(sub->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
void StringFindLast0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *sub = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.rfind(sub->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}



// This function returns the index of the first character that is in
// the specified set of characters. If no such character is found -1 is
// returned.
//
// AngelScript signature:
// int findFirstOf(const string &in str, const string &in chars, int start)
void StringFindFirstOf_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.find_first_of(chars->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
void StringFindFirstOf0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.find_first_of(chars->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}


// This function returns the index of the first character that is not in
// the specified set of characters. If no such character is found -1 is
// returned.
//
// AngelScript signature:
// int findFirstNotOf(const string &in str, const string &in chars, int start)
void StringFindFirstNotOf_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.find_first_not_of(chars->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
void StringFindFirstNotOf0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.find_first_not_of(chars->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}



// This function returns the index of the last character that is in
// the specified set of characters. If no such character is found -1 is
// returned.
//
// AngelScript signature:
// int findLastOf(const string &in str, const string &in chars, int start)
void StringFindLastOf_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.find_last_of(chars->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
void StringFindLastOf0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.find_last_of(chars->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}



// This function returns the index of the last character that is not in
// the specified set of characters. If no such character is found -1 is
// returned.
//
// AngelScript signature:
// int findLastNotOf(const string &in str, const string &in chars, int start)
void StringFindLastNotOf_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);
    int start = *(int*)gen->GetArgPointer(2);

    // Find the substring
    int loc = (int)str->buffer.find_last_not_of(chars->buffer, start);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}
void StringFindLastNotOf0_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *chars = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the substring
    int loc = (int)str->buffer.find_last_not_of(chars->buffer);

    // Return the result
    *(int*)gen->GetReturnPointer() = loc;
}



// This function takes an input string and splits it into parts by looking
// for a specified delimiter. Example:
//
// string str = "A|B||D";
// string@[]@ array = split(str, "|");
//
// The resulting array has the following elements:
//
// {"A", "B", "", "D"}
//
// AngelScript signature:
// string@[]@ split(const string &in str, const string &in delim)
void StringSplit_Generic(asIScriptGeneric *gen)
{
    // Obtain a pointer to the engine
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();

    // TODO: This should only be done once
    int stringArrayType = engine->GetTypeIdByDecl(0, "string@[]");

    // Create the array object
    asIScriptArray *array = (asIScriptArray*)engine->CreateScriptObject(stringArrayType);

    // Get the arguments
    asCScriptString *str = *(asCScriptString**)gen->GetArgPointer(0);
    asCScriptString *delim = *(asCScriptString**)gen->GetArgPointer(1);

    // Find the existence of the delimiter in the input string
    int pos = 0, prev = 0, count = 0;
    while( (pos = (int)str->buffer.find(delim->buffer, prev)) != (int)std::string::npos )
    {
        // Add the part to the array
        asCScriptString *part = new asCScriptString();
        part->buffer.assign(&str->buffer[prev], pos-prev);
        array->Resize(array->GetElementCount()+1);
        *(asCScriptString**)array->GetElementPointer(count) = part;

        // Find the next part
        count++;
        prev = pos + (int)delim->buffer.length();
    }

    // Add the remaining part
    asCScriptString *part = new asCScriptString();
    part->buffer.assign(&str->buffer[prev]);
    array->Resize(array->GetElementCount()+1);
    *(asCScriptString**)array->GetElementPointer(count) = part;

    // Return the array by handle
    *(asIScriptArray**)gen->GetReturnPointer() = array;
}



// This function takes as input an array of string handles as well as a
// delimiter and concatenates the array elements into one delimited string.
// Example:
//
// string@[] array = {"A", "B", "", "D"};
// string str = join(array, "|");
//
// The resulting string is:
//
// "A|B||D"
//
// AngelScript signature:
// string@ join(const string@[] &in array, const string &in delim)
void StringJoin_Generic(asIScriptGeneric *gen)
{
    // Get the arguments
    asIScriptArray *array = *(asIScriptArray**)gen->GetArgPointer(0);
    asCScriptString *delim = *(asCScriptString**)gen->GetArgPointer(1);

    // Create the new string
    asCScriptString *str = new asCScriptString();
    int n;
    for( n = 0; n < (int)array->GetElementCount() - 1; n++ )
    {
        asCScriptString *part = *(asCScriptString**)array->GetElementPointer(n);
        str->buffer += part->buffer;
        str->buffer += delim->buffer;
    }

    // Add the last part
    asCScriptString *part = *(asCScriptString**)array->GetElementPointer(n);
    str->buffer += part->buffer;

    // Return the string
    *(asCScriptString**)gen->GetReturnPointer() = str;
}



// This is where the utility functions are registered.
// The string type must have been registered first.
void RegisterScriptStringUtils(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterGlobalFunction("string@ substring(const string &in, int, int)", asFUNCTION(StringSubString_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirst(const string &in, const string &in)", asFUNCTION(StringFindFirst0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirst(const string &in, const string &in, int)", asFUNCTION(StringFindFirst_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLast(const string &in, const string &in)", asFUNCTION(StringFindLast0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLast(const string &in, const string &in, int)", asFUNCTION(StringFindLast_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirstOf(const string &in, const string &in)", asFUNCTION(StringFindFirstOf0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirstOf(const string &in, const string &in, int)", asFUNCTION(StringFindFirstOf_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirstNotOf(const string &in, const string &in)", asFUNCTION(StringFindFirstNotOf0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findFirstNotOf(const string &in, const string &in, int)", asFUNCTION(StringFindFirstNotOf_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLastOf(const string &in, const string &in)", asFUNCTION(StringFindLastOf0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLastOf(const string &in, const string &in, int)", asFUNCTION(StringFindLastOf_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLastNotOf(const string &in, const string &in)", asFUNCTION(StringFindLastNotOf0_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("int findLastNotOf(const string &in, const string &in, int)", asFUNCTION(StringFindLastNotOf_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string@[]@ split(const string &in, const string &in)", asFUNCTION(StringSplit_Generic), asCALL_GENERIC); assert(r >= 0);
    r = engine->RegisterGlobalFunction("string@ join(const string@[] &in, const string &in)", asFUNCTION(StringJoin_Generic), asCALL_GENERIC); assert(r >= 0);
}
