#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

#include <angelscript.h>

#if defined(_MSC_VER) && _MSC_VER <= 1200 
// disable the annoying warnings on MSVC 6
#pragma warning (disable:4786)
#endif

#include <string>
#include <map>
#include <set>
#include <vector>

BEGIN_AS_NAMESPACE

struct SMetadataDecl;

// Helper class for loading and pre-processing script files to 
// support include directives and metadata declarations
class CScriptBuilder
{
public:
	// Load and build a script file from disk
	int BuildScriptFromFile(asIScriptEngine *engine, 
		                    const char      *module, 
							const char      *filename);

	// Build a script file from a memory buffer
	int BuildScriptFromMemory(asIScriptEngine *engine, 
		                      const char      *module, 
							  const char      *script, 
							  const char      *sectionname = "");

	// Add a pre-processor define for conditional compilation
	void DefineWord(const char *word);

	// Get metadata declared for class types and interfaces
	const char *GetMetadataStringForType(int typeId);

	// Get metadata declared for functions
	const char *GetMetadataStringForFunc(int funcId);

	// Get metadata declared for global variables
	const char *GetMetadataStringForVar(int varIdx);

protected:
	void ClearAll();
	int  Build();
	int  ProcessScriptSection(const char *script, const char *sectionname);
	int  LoadScriptSection(const char *filename);

	int  SkipStatementBlock(int pos);
	int  ExtractMetadataString(int pos, std::string &outMetadata);
	int  ExtractDeclaration(int pos, std::string &outDeclaration, int &outType);
	int  ExcludeCode(int start);

	asIScriptEngine           *engine;
	asIScriptModule           *module;
	std::string                modifiedScript;
	std::vector<SMetadataDecl> foundDeclarations;

	std::map<int, std::string> typeMetadataMap;
	std::map<int, std::string> funcMetadataMap;
	std::map<int, std::string> varMetadataMap;

	std::set<std::string>      includedScripts;

	std::set<std::string>      definedWords;
};

// Temporary structure for storing metadata and declaration
struct SMetadataDecl
{
	SMetadataDecl(std::string m, std::string d, int t) : metadata(m), declaration(d), type(t) {}
	std::string metadata;
	std::string declaration;
	int         type;
};

END_AS_NAMESPACE

#endif
