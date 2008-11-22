#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

#include <angelscript.h>
#include <string>
#include <map>

BEGIN_AS_NAMESPACE

class CScriptBuilder
{
public:
	int BuildScriptFromMemory(asIScriptEngine *engine, const char *module, const char *script);

	const char *GetMetadataStringForType(int typeId);
	const char *GetMetadataStringForFunc(int funcId);
	const char *GetMetadataStringForVar(int varIdx);

protected:
	int SkipStatementBlock(int pos);
	int ExtractMetadataString(int pos, std::string &outMetadata);
	int ExtractDeclaration(int pos, std::string &outDeclaration, int &outType);

	asIScriptEngine *engine;
	std::string modifiedScript;

	std::map<int, std::string> typeMetadataMap;
	std::map<int, std::string> funcMetadataMap;
	std::map<int, std::string> varMetadataMap;
};

END_AS_NAMESPACE

#endif
