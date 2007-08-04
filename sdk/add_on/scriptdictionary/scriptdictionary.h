#ifndef SCRIPTDICTIONARY_H
#define SCRIPTDICTIONARY_H

// The dictionary class relies on the script string object, thus the script
// string type must be registered with the engine before registering the
// dictionary type

#include <angelscript.h>
#include <string>

#ifdef _MSC_VER
// Turn off annoying warnings about truncated symbol names
#pragma warning (disable:4786)
#endif

#include <map>

BEGIN_AS_NAMESPACE

class CScriptDictionary
{
public:
    CScriptDictionary(asIScriptEngine *engine);
    void AddRef();
    void Release();

    void Set(std::string &key, void *value, int typeId);
    void Get(std::string &key, void *value, int typeId);
    bool Exists(std::string &key);
    void Delete(std::string &key);
    void DeleteAll();

protected:

	// The structure for holding the values
    struct v
    {
        union
        {
            asQWORD valueInt;
            double  valueFlt;
            void   *valueObj;
        };
        int   typeId;
    };
    
	// We don't want anyone to call the destructor directly, it should be called through the Release method
	virtual ~CScriptDictionary();

	// Don't allow assignment
    CScriptDictionary &operator =(const CScriptDictionary &other);

	// Helper methods
    void FreeValue(v &value);

	// Our properties
    asIScriptEngine *engine;
    int refCount;
    std::map<std::string, v> dict;
};

// This function will determine the configuration of the engine
// and use one of the two functions below to register the dictionary object
void RegisterScriptDictionary(asIScriptEngine *engine);

// Call this function to register the math functions
// using native calling conventions
void RegisterScriptDictionary_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptDictionary_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
