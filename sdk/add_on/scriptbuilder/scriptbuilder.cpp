#include "scriptbuilder.h"
#include <vector>
using namespace std;

BEGIN_AS_NAMESPACE

// Temporary structure for storing metadata and declaration
struct SMetadataDecl
{
	SMetadataDecl(string m, string d, int t) : metadata(m), declaration(d), type(t) {}
	string metadata;
	string declaration;
	int    type;
};

int CScriptBuilder::BuildScriptFromMemory(asIScriptEngine *engine, const char *module, const char *script)
{
	// Perform a superficial parsing of the script first to store the metadata
	modifiedScript = script;
	this->engine = engine;

	vector<SMetadataDecl> foundDeclarations;

	// Preallocate memory
	std::string metadata, declaration;
	metadata.reserve(500);
	declaration.reserve(100);

	int pos = 0;
	while( pos < (int)modifiedScript.size() )
	{
		int len;
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( t == asTC_KEYWORD )
		{
			// Is this the start of metadata?
			if( modifiedScript[pos] == '[' )
			{
				// Get the metadata string
				pos = ExtractMetadataString(pos, metadata);

				// Determine what this metadata is for
				int type;
				pos = ExtractDeclaration(pos, declaration, type);
				
				// Store away the declaration in a map for lookup after the build has completed
				if( type > 0 )
				{
					SMetadataDecl decl(metadata, declaration, type);
					foundDeclarations.push_back(decl);
				}
			}
			// Don't search for metadata within statement blocks
			else if( modifiedScript[pos] == '{' )
				pos = SkipStatementBlock(pos);
			else
				pos += len;
		}
		else
			pos += len;
	}

	// Build the actual script
	engine->SetEngineProperty(asEP_COPY_SCRIPT_SECTIONS, 0);
	engine->AddScriptSection(module, "script", modifiedScript.c_str(), modifiedScript.size());
	int r = engine->Build(module);
	if( r < 0 )
		return r;

	// After the script has been built, the metadata strings should be 
	// stored for later lookup by function id, type id, and variable index
	for( int n = 0; n < (int)foundDeclarations.size(); n++ )
	{
		SMetadataDecl *decl = &foundDeclarations[n];
		if( decl->type == 1 )
		{
			// Find the type id
			int typeId = engine->GetTypeIdByDecl(module, decl->declaration.c_str());
			if( typeId >= 0 )
				typeMetadataMap.insert(map<int, string>::value_type(typeId, decl->metadata));
		}
		else if( decl->type == 2 )
		{
			// Find the function id
			int funcId = engine->GetFunctionIDByDecl(module, decl->declaration.c_str());
			if( funcId >= 0 )
				funcMetadataMap.insert(map<int, string>::value_type(funcId, decl->metadata));
		}
		else if( decl->type == 3 )
		{
			// Find the global variable index
			int varIdx = engine->GetGlobalVarIndexByDecl(module, decl->declaration.c_str());
			if( varIdx >= 0 )
				varMetadataMap.insert(map<int, string>::value_type(varIdx, decl->metadata));
		}
	}

	return 0;
}

int CScriptBuilder::ExtractMetadataString(int pos, string &metadata)
{
	metadata = "";

	// Overwrite the metadata with space characters to allow compilation
	modifiedScript[pos] = ' ';

	// Skip opening brackets
	pos += 1;

	int level = 1;
	int len;
	while( level > 0 && pos < (int)modifiedScript.size() )
	{
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
		if( t == asTC_KEYWORD )
		{
			if( modifiedScript[pos] == '[' )
				level++;
			else if( modifiedScript[pos] == ']' )
				level--;
		}

		// Copy the metadata to our buffer
		if( level > 0 )
			metadata.append(&modifiedScript[pos], len);

		// Overwrite the metadata with space characters to allow compilation
		if( t != asTC_WHITESPACE )
			memset(&modifiedScript[pos], ' ', len);

		pos += len;
	}

	return pos;
}

int CScriptBuilder::ExtractDeclaration(int pos, string &declaration, int &type)
{
	declaration = "";
	type = 0;

	int start = pos;

	std::string token;
	int len = 0;
	asETokenClass t = asTC_WHITESPACE;

	// Skip white spaces and comments
	do
	{
		pos += len;
		t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
	} while ( t == asTC_WHITESPACE || t == asTC_COMMENT );

	// We're expecting, either a class, interface, function, or variable declaration
	if( t == asTC_KEYWORD || t == asTC_IDENTIFIER )
	{
		token.assign(&modifiedScript[pos], len);
		if( token == "interface" || token == "class" )
		{
			// Skip white spaces and comments
			do
			{
				pos += len;
				t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
			} while ( t == asTC_WHITESPACE || t == asTC_COMMENT );

			if( t == asTC_IDENTIFIER )
			{
				type = 1;
				declaration.assign(&modifiedScript[pos], len);
				pos += len;
				return pos;
			}
		}
		else
		{
			// For function declarations, store everything up to the start of the statement block

			// For variable declaration store everything up until the first parenthesis, assignment, or end statement.

			// We'll only know if the declaration is a variable or function declaration when we see the statement block, or absense of a statement block.
			int varLength = 0;
			declaration.append(&modifiedScript[pos], len);
			pos += len;
			for(; pos < (int)modifiedScript.size();)
			{
				t = engine->ParseToken(&modifiedScript[pos], modifiedScript.size() - pos, &len);
				if( t == asTC_KEYWORD )
				{
					token.assign(&modifiedScript[pos], len);
					if( token == "{" )
					{
						// We've found the end of a function signature
						type = 2;
						return pos;
					}
					if( token == "=" || token == ";" )
					{
						// We've found the end of a variable declaration.
						if( varLength != 0 )
							declaration.resize(varLength);
						type = 3;
						return pos;
					}
					else if( token == "(" && varLength == 0 )
					{
						// This is the first parenthesis we encounter. If the parenthesis isn't followed 
						// by a statement block, then this is a variable declaration, in which case we 
						// should only store the type and name of the variable, not the initialization parameters.
						varLength = (int)declaration.size();
					}
				}

				declaration.append(&modifiedScript[pos], len);
				pos += len;
			}
		}
	}

	return start;
}

int CScriptBuilder::SkipStatementBlock(int pos)
{
	// Skip opening brackets
	pos += 1;

	// Find the end of the statement block
	int level = 1;
	int len;
	while( level > 0 && pos < (int)modifiedScript.size() )
	{
		asETokenClass t = engine->ParseToken(&modifiedScript[pos], 0, &len);
		if( t == asTC_KEYWORD )
		{
			if( modifiedScript[pos] == '{' )
				level++;
			else if( modifiedScript[pos] == '}' )
				level--;
		}

		pos += len;
	}

	return pos;
}

const char *CScriptBuilder::GetMetadataStringForType(int typeId)
{
	map<int,string>::iterator it = typeMetadataMap.find(typeId);
	if( it != typeMetadataMap.end() )
		return it->second.c_str();

	return "";
}

const char *CScriptBuilder::GetMetadataStringForFunc(int funcId)
{
	map<int,string>::iterator it = funcMetadataMap.find(funcId);
	if( it != funcMetadataMap.end() )
		return it->second.c_str();

	return "";
}

const char *CScriptBuilder::GetMetadataStringForVar(int varIdx)
{
	map<int,string>::iterator it = varMetadataMap.find(varIdx);
	if( it != varMetadataMap.end() )
		return it->second.c_str();

	return "";
}

END_AS_NAMESPACE


