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
	v valStruct = {0,0};
	if( typeId & asTYPEID_OBJHANDLE )
	{
		// We're receiving a reference to the handle, so we need to dereference it
		valStruct.valueObj = *(void**)value;
		valStruct.typeId = typeId;
		engine->AddRefScriptObject(valStruct.valueObj, typeId);
	}
	else if( typeId & asTYPEID_MASK_OBJECT )
	{
		// Create a copy of the object
		// We need to dereference the reference
		valStruct.valueObj = engine->CreateScriptObjectCopy(*(void**)value, typeId);
		valStruct.typeId = typeId;
	}
	else
	{
		// TODO:
		// Copy the primitive value
	}

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

// Returns true if the value was successfully retrieved
bool CScriptDictionary::Get(string &key, void *value, int typeId)
{
    map<string, v>::iterator it;
    it = dict.find(key);
    if( it != dict.end() )
    {
        // Return the value
		if( typeId & asTYPEID_OBJHANDLE )
		{
			// TODO: The engine should tell me if the handle is compatible with the type or not
			// Verify if the wanted handle is compatible with the stored object
			bool isCompatible = false;
			if( it->second.typeId == typeId )
				isCompatible = true;
			// Allow obj@ to be copied to const obj@
			else if( ((it->second.typeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR)) == (typeId & (asTYPEID_OBJHANDLE | asTYPEID_MASK_OBJECT | asTYPEID_MASK_SEQNBR))) && // Handle to the same object type
	 				((typeId & (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) == (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) )			   // Handle to const object
				isCompatible = true;

			// TODO:
			// Allow retrieving a handle for an object that was stored as non-handle

			// Add a reference to the handle, then return it
			if( isCompatible )
			{
				engine->AddRefScriptObject(it->second.valueObj, it->second.typeId);
				*(void**)value = it->second.valueObj;

				return true;
			}
		}
		else if( typeId & asTYPEID_MASK_OBJECT )
		{
			// Verify that the copy can be made
			bool isCompatible = false;
			if( it->second.typeId == typeId )
				isCompatible = true;

			// Copy the object into the given reference
			if( isCompatible )
			{
				engine->CopyScriptObject(*(void**)value, it->second.valueObj, typeId);

				return true;
			}
		}
		else
		{
			// Copy the primitive value
		}
    }

    // AngelScript has already initialized the value with a default value,
    // so we don't have to do anything if we don't find the element

	return false;
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
	if( value.typeId & asTYPEID_MASK_OBJECT )
	{
		// Let the engine release the object
		engine->ReleaseScriptObject(value.valueObj, value.typeId);
		value.valueObj = 0;
		value.typeId = 0;
	}

    // For primitives, there's nothing to do
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
    void *ref = *(void**)gen->GetArgPointer(1);
    int typeId = gen->GetArgTypeId(1);
    dict->Set(*key, ref, typeId);
}

void ScriptDictionaryGet_Generic(asIScriptGeneric *gen)
{
    CScriptDictionary *dict = (CScriptDictionary*)gen->GetObject();
    string *key = *(string**)gen->GetArgPointer(0);
    void *ref = *(void**)gen->GetArgPointer(1);
    int typeId = gen->GetArgTypeId(1);
    *(bool*)gen->GetReturnPointer() = dict->Get(*key, ref, typeId);
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
    r = engine->RegisterObjectMethod("dictionary", "bool get(const string &in, ?&out)", asFUNCTION(ScriptDictionaryGet_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "bool exists(const string &in)", asFUNCTION(ScriptDictionaryExists_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void delete(const string &in)", asFUNCTION(ScriptDictionaryDelete_Generic), asCALL_GENERIC); assert( r >= 0 );
    r = engine->RegisterObjectMethod("dictionary", "void deleteAll()", asFUNCTION(ScriptDictionaryDeleteAll_Generic), asCALL_GENERIC); assert( r >= 0 );
}

END_AS_NAMESPACE


