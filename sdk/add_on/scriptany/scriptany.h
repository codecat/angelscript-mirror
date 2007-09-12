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
	int  Retrieve(void *ref, int refTypeId);
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
	int valueTypeId;
	void *value;
};

void RegisterScriptAny(asIScriptEngine *engine);
void RegisterScriptAny_Native(asIScriptEngine *engine);
void RegisterScriptAny_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
