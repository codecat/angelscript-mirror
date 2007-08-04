#include <assert.h>
#include <string.h>
#include "scriptdictionary.h"

BEGIN_AS_NAMESPACE

using namespace std;

//--------------------------------------------------------------------------
// CScriptDictionary implementation

CScriptDictionary::CScriptDictionary(asIScriptEngine *engine)
{
    // We start with one reference
    refCount = 1;

    // Keep a reference to the engine for as long as we live
    this->engine = engine;
    engine->AddRef();
}

CScriptDictionary::~CScriptDictionary()
{
    // Delete all keys and values
    DeleteAll();

    // Release the engine reference
    if( engine ) engine->Release();
}

void CScriptDictionary::AddRef()
{
    refCount++;
}

void CScriptDictionary::Release()
{
    if( --refCount == 0 )
        delete this;
}

CScriptDictionary &CScriptDictionary::operator =(const CScriptDictionary &other)
{
    // Do nothing

    return *this;
}

void CScriptDictionary::Set(string &key, void *value, int typeId)
{
    v valStruct;
    valStruct.valueObj = 0;
    valStruct.typeId = 0;

    map<string, v>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        FreeValue(it->second);

        // Insert the new value
        it->second = valStruct;
    }
    else
    {
        dict.insert(map<string, v>::value_type(key, valStruct));
    }
}

void CScriptDictionary::Get(string &key, void *value, int typeId)
{
    map<string, v>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        // Return the value
    }

    // AngelScript has already initialized the value with a default value,
    // so we don't have to do anything if we don't find the element
}

bool CScriptDictionary::Exists(string &key)
{
    map<string, v>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        return true;
    }

    return false;
}

void CScriptDictionary::Delete(string &key)
{
    map<string, v>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        FreeValue(it->second);

        dict.erase(it);
    }
}

void CScriptDictionary::DeleteAll()
{
    map<string, v>::iterator it;
    for( it = dict.begin(); it != dict.end(); it++ )
    {
        FreeValue(it->second);
    }

    dict.clear();
}

void CScriptDictionary::FreeValue(v &value)
{
    // If it is a handle or a ref counted object, call release

    // If it is non ref counted object, call the destructor then free the memory
}

//--------------------------------------------------------------------------
// Custom memory management for the class

// This function allocates memory for the string object
static void *ScriptDictionaryAlloc(int)
{
	return new char[sizeof(CScriptDictionary)];
}

// This function deallocates the memory for the string object
static void ScriptDictionaryFree(void *p)
{
	assert( p );
	delete[] (char*)p;
}

//--------------------------------------------------------------------------
// Generic wrappers

void ScriptDictionaryConstruct_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    new(dict) CScriptDictionary(gen->GetEngine());
}

void ScriptDictionaryAddRef_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->AddRef();
}

void ScriptDictionaryRelease_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->Release();
}

void ScriptDictionarySet_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetArgPointer(0);
    void *ref = gen->GetArgPointer(1);
    int typeId = gen->GetArgTypeId(1);
    dict->Set(*key, ref, typeId);
}

void ScriptDictionaryGet_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetArgPointer(0);
    void *ref = gen->GetArgPointer(1);
    int typeId = gen->GetArgTypeId(1);
    dict->Get(*key, ref, typeId);
}

void ScriptDictionaryExists_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetArgPointer(0);
    bool ret = dict->Exists(*key);
    *(bool*)gen->GetReturnPointer() = ret;
}

void ScriptDictionaryDelete_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetArgPointer(0);
    dict->Delete(*key);
}

void ScriptDictionaryDeleteAll_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    dict->DeleteAll();
}

//--------------------------------------------------------------------------
// Register the type

void RegisterScriptDictionary(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptDictionary_Generic(engine);
	else
		RegisterScriptDictionary_Native(engine);
}

void RegisterScriptDictionary_Native(asIScriptEngine *engine)
{
    RegisterScriptDictionary_Generic(engine);
}

void RegisterScriptDictionary_Generic(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterObjectType("dictionary", sizeof(CScriptDictionary), asOBJ_CLASS_CDA); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ScriptDictionaryConstruct_Generic), asCALL_GENERIC); assert( r>= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptDictionaryAddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptDictionaryRelease_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_ALLOC, "dictionary &f(uint)", asFUNCTION(ScriptDictionaryAlloc), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("dictionary", asBEHAVE_FREE, "void f(dictionary &in)", asFUNCTION(ScriptDictionaryFree), asCALL_CDECL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("dictionary", "void set(const string &in, ?&in)", asFUNCTION(ScriptDictionarySet_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void get(const string &in, ?&out)", asFUNCTION(ScriptDictionaryGet_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool exists(const string &in)", asFUNCTION(ScriptDictionaryExists_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void delete(const string &in)", asFUNCTION(ScriptDictionaryDelete_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void deleteAll()", asFUNCTION(ScriptDictionaryDeleteAll_Generic), asCALL_GENERIC); assert( r >= 0 );
}

END_AS_NAMESPACE


