#ifndef SCRIPTANY_H
#define SCRIPTANY_H

#include <angelscript.h>

BEGIN_AS_NAMESPACE

class CScriptAny 
{
public:
	CScriptAny(asIScriptEngine *engine);
	CScriptAny(void *ref, int refTypeId, asIScriptEngine *engine);
	virtual ~CScriptAny();

	int AddRef();
	int Release();

	CScriptAny &operator=(CScriptAny&);

	void Store(void *ref, int refTypeId);
	void Store(asINT64 &value);
	void Store(double &value);

	bool Retrieve(void *ref, int refTypeId);
	bool Retrieve(asINT64 &value);
	bool Retrieve(double &value);

	int  GetTypeId();
	int  CopyFrom(CScriptAny *other);

	// GC methods
	int  GetRefCount();
	void SetFlag();
	bool GetFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllHandles(asIScriptEngine *engine);

protected:
	void FreeObject();

	int refCount;
	asIScriptEngine *engine;

	// The structure for holding the values
    struct valueStruct
    {
        union
        {
            asINT64 valueInt;
            double  valueFlt;
            void   *valueObj;
        };
        int   typeId;
    };

	valueStruct value;
};

void RegisterScriptAny(asIScriptEngine *engine);
void RegisterScriptAny_Native(asIScriptEngine *engine);
void RegisterScriptAny_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
