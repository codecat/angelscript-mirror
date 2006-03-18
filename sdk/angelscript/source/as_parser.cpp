/*
   AngelCode Scripting Library
   Copyright (c) 2003-2004 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
	  this software in a product, an acknowledgment in the product 
	  documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_parser.cpp
//
// This class parses the script code and builds a tree for compilation
//



#include "as_config.h"
#include "as_parser.h"
#include "as_tokendef.h"
#include "as_texts.h"

asCParser::asCParser(asCBuilder *builder)
{
	this->builder    = builder;

	script			= 0;
	scriptNode		= 0;
}

asCParser::~asCParser()
{
	Reset();
}

void asCParser::Reset()
{
	errorWhileParsing = false;
	isSyntaxError     = false;

	sourcePos = 0;

	if( scriptNode )
		delete scriptNode;

	scriptNode = 0;

	script = 0;
}

asCScriptNode *asCParser::GetScriptNode()
{
	return scriptNode;
}

int asCParser::ParseScript(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = ParseScript();

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParseFunctionDefinition(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = ParseFunctionDefinition();

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParseDataType(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new asCScriptNode(snDataType);
		
	scriptNode->AddChildLast(ParseType());
	if( isSyntaxError ) return -1;

	scriptNode->AddChildLast(ParseTypeMod());

	if( errorWhileParsing )
		return -1;

	return 0;
}

int asCParser::ParsePropertyDeclaration(asCScriptCode *script)
{
	Reset();

	this->script = script;

	scriptNode = new asCScriptNode(snDeclaration);

	scriptNode->AddChildLast(ParseType());
	if( isSyntaxError ) return -1;

	scriptNode->AddChildLast(ParseTypeMod());
	if( isSyntaxError ) return -1;

	scriptNode->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return -1;

	return 0;
}

asCScriptNode *asCParser::ParseImport()
{
	asCScriptNode *node = new asCScriptNode(snImport);

	sToken t;
	GetToken(&t);
	if( t.type != ttImport )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttImport)), &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	node->AddChildLast(ParseFunctionDefinition());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttIdentifier )
	{
		Error(ExpectedToken("from"), &t);
		return node;
	}

	asCString str;
	str.Copy(&script->code[t.pos], t.length);
	if( str != "from" )
	{
		Error(ExpectedToken("from"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttStringConstant )
	{
		Error(TXT_EXPECTED_STRING, &t);
		return node;
	}

	asCScriptNode *mod = new asCScriptNode(snConstant);
	node->AddChildLast(mod);

	mod->SetToken(&t);
	mod->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(asGetTokenDefinition(ttEndStatement)), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseFunctionDefinition()
{
	asCScriptNode *node = new asCScriptNode(snFunction);

	node->AddChildLast(ParseType());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	return node;
}

asCScriptNode *asCParser::ParseScript()
{
	asCScriptNode *node = new asCScriptNode(snScript);

	// Determine type of node
	sToken t1;

	for(;;)
	{
		while( !isSyntaxError )
		{
			GetToken(&t1);
			RewindTo(&t1);

			if( IsDataType(t1.type) )
			{
				if( IsGlobalVar() )
					node->AddChildLast(ParseGlobalVar());
				else
					node->AddChildLast(ParseFunction());
			}
			else if( t1.type == ttImport )
				node->AddChildLast(ParseImport());
			else if( t1.type == ttConst )
				node->AddChildLast(ParseGlobalVar());
			else if( t1.type == ttEnd )
				return node;
			else
			{
				asCString str;
				const char *t = asGetTokenDefinition(t1.type);
				if( t == 0 ) t = "<unknown token>";

				str.Format(TXT_UNEXPECTED_TOKEN_s, t);

				Error(str, &t1);
			}
		}

		if( isSyntaxError )
		{
			// Search for either ';' or '{' or end
			GetToken(&t1);
			while( t1.type != ttEndStatement && t1.type != ttEnd &&
				   t1.type != ttStartStatementBlock )
				GetToken(&t1);

			if( t1.type == ttStartStatementBlock )
			{
				// Find the end of the block and skip nested blocks
				int level = 1;
				while( level > 0 )
				{
					GetToken(&t1);
					if( t1.type == ttStartStatementBlock ) level++;
					if( t1.type == ttEndStatementBlock ) level--;
					if( t1.type == ttEnd ) break;
				}
			}

			isSyntaxError = false;
		}
	}
	return 0;
}

bool asCParser::IsGlobalVar()
{
	// Set start point so that we can rewind
	sToken t;
	GetToken(&t);
	RewindTo(&t);
	
	// A global variable can start with a const
	sToken t1;
	GetToken(&t1);
	if( t1.type == ttConst )
		GetToken(&t1);

	if( !IsDataType(t1.type) )
	{
		RewindTo(&t);
		return false;
	}

	sToken t2;
	GetToken(&t2);
	while( t2.type == ttStar )
		GetToken(&t2);

	while( t2.type == ttOpenBracket )
	{
		GetToken(&t2);
		if( t2.type != ttCloseBracket )
			return false;
		GetToken(&t2);
	}

	if( t2.type != ttIdentifier )
	{
		RewindTo(&t);
		return false;
	}

	GetToken(&t2);
	if( t2.type == ttEndStatement || t2.type == ttAssignment || t2.type == ttListSeparator )
	{
		RewindTo(&t);
		return true;
	}
	if( t2.type == ttOpenParanthesis ) 
	{	
		// If the closing paranthesis is followed by a statement 
		// block or end-of-file, then treat it as a function. 
		while( t2.type != ttCloseParanthesis && t2.type != ttEnd )
			GetToken(&t2);

		if( t2.type == ttEnd ) 
			return false;
		else
		{
			GetToken(&t1);
			RewindTo(&t);
			if( t1.type == ttStartStatementBlock || t1.type == ttEnd )
				return false;
		}

		RewindTo(&t);

		return true;
	}

	RewindTo(&t);
	return false;
}

asCScriptNode *asCParser::ParseFunction()
{
	asCScriptNode *node = new asCScriptNode(snFunction);

	node->AddChildLast(ParseType());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseTypeMod());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseParameterList());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseStatementBlock());

	return node;
}

asCScriptNode *asCParser::ParseGlobalVar()
{
	asCScriptNode *node = new asCScriptNode(snGlobalVar);

	// Parse data type
	node->AddChildLast(ParseType());
	if( isSyntaxError ) return node;

	sToken t;

	for(;;)
	{
		node->AddChildLast(ParseTypeMod());
		if( isSyntaxError ) return node;

		// Parse identifier
		node->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		// If next token is assignment, parse expression
		GetToken(&t);
		if( t.type == ttAssignment )
		{
			node->AddChildLast(ParseAssignment());
			if( isSyntaxError ) return node;
		}
		else if( t.type == ttOpenParanthesis ) 
		{
			RewindTo(&t);
			node->AddChildLast(ParseArgList());
			if( isSyntaxError ) return node;
		}
		else
			RewindTo(&t);

		// continue if list separator, else terminate with end statement
		GetToken(&t);
		if( t.type == ttListSeparator )
			continue;
		else if( t.type == ttEndStatement )
		{
			node->UpdateSourcePos(t.pos, t.length);

			return node;
		}
		else
		{
			Error(ExpectedTokens(",", ";"), &t);
			return node;
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseTypeMod()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	// Parse pointer type
	sToken t;
	GetToken(&t);
	RewindTo(&t);
	while( t.type == ttStar )
	{
		node->AddChildLast(ParseToken(ttStar));
		if( isSyntaxError ) return node;

		GetToken(&t);
		RewindTo(&t);
	}

	// Parse []
	GetToken(&t);
	RewindTo(&t);
	while( t.type == ttOpenBracket )
	{
		node->AddChildLast(ParseToken(ttOpenBracket));
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttCloseBracket )
		{
			Error(ExpectedToken("]"), &t);
			return node;
		}

		GetToken(&t);
		RewindTo(&t);
	}

	// Parse possibly byref token
	GetToken(&t);
	RewindTo(&t);
	if( t.type == ttAmp )
	{
		node->AddChildLast(ParseToken(ttAmp));
		if( isSyntaxError ) return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseType()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	RewindTo(&t1);
	if( t1.type == ttConst )
	{
		node->AddChildLast(ParseToken(ttConst));
		if( isSyntaxError ) return node;
	}

	node->AddChildLast(ParseDataType());

	return node;
}

asCScriptNode *asCParser::ParseToken(int token)
{
	asCScriptNode *node = new asCScriptNode(snUndefined);

	sToken t1;

	GetToken(&t1);
	if( t1.type != token )
	{
		Error(ExpectedToken(asGetTokenDefinition(token)), &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseDataType()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	if( !IsDataType(t1.type) )
	{
		Error(TXT_EXPECTED_DATA_TYPE, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseRealType()
{
	asCScriptNode *node = new asCScriptNode(snDataType);

	sToken t1;

	GetToken(&t1);
	if( !IsRealType(t1.type) )
	{
		Error(TXT_EXPECTED_DATA_TYPE, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseIdentifier()
{
	asCScriptNode *node = new asCScriptNode(snIdentifier);

	sToken t1;

	GetToken(&t1);
	if( t1.type != ttIdentifier )
	{
		Error(TXT_EXPECTED_IDENTIFIER, &t1);
		return node;
	}

	node->SetToken(&t1);
	node->UpdateSourcePos(t1.pos, t1.length);

	return node;
}

asCScriptNode *asCParser::ParseParameterList()
{
	asCScriptNode *node = new asCScriptNode(snParameterList);

	sToken t1;
	GetToken(&t1);
	if( t1.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type == ttCloseParanthesis )
	{
		node->UpdateSourcePos(t1.pos, t1.length);

		// Statement block is finished
		return node;
	}
	else
	{
		RewindTo(&t1);

		for(;;)
		{
			// Parse data type
			node->AddChildLast(ParseType());
			if( isSyntaxError ) return node;

			node->AddChildLast(ParseTypeMod());
			if( isSyntaxError ) return node;

			// Parse identifier
			GetToken(&t1);
			if( t1.type == ttIdentifier )
			{
				RewindTo(&t1);

				node->AddChildLast(ParseIdentifier());
				if( isSyntaxError ) return node;

				GetToken(&t1);
			}

			// Check if list continues
			if( t1.type == ttCloseParanthesis )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				return node;
			}
			else if( t1.type == ttListSeparator )
				continue;
			else
			{
				Error(ExpectedTokens(")", ","), &t1);
				return node;
			}
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprValue()
{
	asCScriptNode *node = new asCScriptNode(snExprValue);

	sToken t1, t2;
	GetToken(&t1);
	GetToken(&t2);
	RewindTo(&t1);

	if( t1.type == ttIdentifier )
	{
		if( t2.type == ttOpenParanthesis )
			node->AddChildLast(ParseFunctionCall());
		else
			node->AddChildLast(ParseIdentifier());
	}
	else if( IsConstant(t1.type) )
		node->AddChildLast(ParseConstant());
	else if( t1.type == ttOpenParanthesis )
	{
		GetToken(&t1);
		node->UpdateSourcePos(t1.pos, t1.length);

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t1);
		if( t1.type != ttCloseParanthesis )
			Error(ExpectedToken(")"), &t1);

		node->UpdateSourcePos(t1.pos, t1.length);
	}
	else if( IsRealType(t1.type) )
		node->AddChildLast(ParseConversion());
	else
		Error(TXT_EXPECTED_EXPRESSION_VALUE, &t1);

	return node;
}

asCScriptNode *asCParser::ParseConversion()
{
	asCScriptNode *node = new asCScriptNode(snConversion);

	node->AddChildLast(ParseType());
	if( isSyntaxError ) return node;

	sToken t;
	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")"), &t);
		return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseConstant()
{
	asCScriptNode *node = new asCScriptNode(snConstant);

	sToken t;
	GetToken(&t);
	if( !IsConstant(t.type) )
	{
		Error(TXT_EXPECTED_CONSTANT, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	// We want to gather a list of string constants to concatenate as children
	if( t.type == ttStringConstant )
		RewindTo(&t);

	while( t.type == ttStringConstant )
	{
		node->AddChildLast(ParseStringConstant());

		GetToken(&t);
		RewindTo(&t);
	}

	return node;
}

asCScriptNode *asCParser::ParseStringConstant()
{
	asCScriptNode *node = new asCScriptNode(snConstant);

	sToken t;
	GetToken(&t);
	if( t.type != ttStringConstant )
	{
		Error(TXT_EXPECTED_STRING, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseFunctionCall()
{
	asCScriptNode *node = new asCScriptNode(snFunctionCall);

	node->AddChildLast(ParseIdentifier());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseArgList());

	return node;
}

asCScriptNode *asCParser::ParseArgList()
{
	asCScriptNode *node = new asCScriptNode(snArgList);

	sToken t1;
	GetToken(&t1);
	if( t1.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	GetToken(&t1);
	if( t1.type == ttCloseParanthesis )
	{
		node->UpdateSourcePos(t1.pos, t1.length);

		// Statement block is finished
		return node;
	}
	else
	{
		RewindTo(&t1);

		for(;;)
		{
			node->AddChildLast(ParseAssignment());
			if( isSyntaxError ) return node;

			// Check if list continues
			GetToken(&t1);
			if( t1.type == ttCloseParanthesis )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				return node;
			}
			else if( t1.type == ttListSeparator )
				continue;
			else
			{
				Error(ExpectedTokens(")", ","), &t1);
				return node;
			}
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseStatementBlock()
{
	asCScriptNode *node = new asCScriptNode(snStatementBlock);

	sToken t1;

	GetToken(&t1);
	if( t1.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{"), &t1);
		return node;
	}

	node->UpdateSourcePos(t1.pos, t1.length);

	for(;;)
	{
		while( !isSyntaxError )
		{
			GetToken(&t1);
			if( t1.type == ttEndStatementBlock )
			{
				node->UpdateSourcePos(t1.pos, t1.length);

				// Statement block is finished
				return node;
			}
			else
			{
				RewindTo(&t1);

				if( IsDeclaration() )
					node->AddChildLast(ParseDeclaration());
				else
					node->AddChildLast(ParseStatement());
			}
		}

		if( isSyntaxError )
		{
			// Search for either ';', '{', '}', or end
			GetToken(&t1);
			while( t1.type != ttEndStatement && t1.type != ttEnd &&
				   t1.type != ttStartStatementBlock && t1.type != ttEndStatementBlock )
			{
				GetToken(&t1);
			}

			// Skip this statement block
			if( t1.type == ttStartStatementBlock )
			{
				// Find the end of the block and skip nested blocks
				int level = 1;
				while( level > 0 )
				{
					GetToken(&t1);
					if( t1.type == ttStartStatementBlock ) level++;
					if( t1.type == ttEndStatementBlock ) level--;
					if( t1.type == ttEnd ) break;
				}
			}
			else if( t1.type == ttEndStatementBlock )
			{
				RewindTo(&t1);
			}
			else if( t1.type == ttEnd )
			{
				Error(TXT_UNEXPECTED_END_OF_FILE, &t1);
				return node;
			}

			isSyntaxError = false;
		}
	}
	return 0;
}

bool asCParser::IsDeclaration()
{
	sToken t1, t2, t3;

	GetToken(&t1);
	GetToken(&t2);
	GetToken(&t3);
	RewindTo(&t1);

	if( t1.type == ttConst )
		return true;

	if( !IsDataType(t1.type) )
		return false;

	if( t2.type == ttStar || t2.type == ttIdentifier )
		return true;

	if( t2.type == ttOpenBracket && t3.type == ttCloseBracket )
		return true;

	return false;
}

asCScriptNode *asCParser::ParseDeclaration()
{
	asCScriptNode *node = new asCScriptNode(snDeclaration);

	// Parse data type
	node->AddChildLast(ParseType());
	if( isSyntaxError ) return node;

	sToken t;

	for(;;)
	{
		node->AddChildLast(ParseTypeMod());
		if( isSyntaxError ) return node;

		// Parse identifier
		node->AddChildLast(ParseIdentifier());
		if( isSyntaxError ) return node;

		// If next token is assignment, parse expression
		GetToken(&t);
		if( t.type == ttOpenParanthesis )
		{
			RewindTo(&t);
			node->AddChildLast(ParseArgList());
			if( isSyntaxError ) return node;
		}
		else if( t.type == ttAssignment )
		{
			node->AddChildLast(ParseAssignment());
			if( isSyntaxError ) return node;
		}
		else
			RewindTo(&t);

		// continue if list separator, else terminate with end statement
		GetToken(&t);
		if( t.type == ttListSeparator )
			continue;
		else if( t.type == ttEndStatement )
		{
			node->UpdateSourcePos(t.pos, t.length);

			return node;
		}
		else
		{
			Error(ExpectedTokens(",", ";"), &t);
			return node;
		}
	}
	return 0;
}

asCScriptNode *asCParser::ParseStatement()
{
	sToken t1;

	GetToken(&t1);
	RewindTo(&t1);

	if( t1.type == ttIf )
		return ParseIf();
	else if( t1.type == ttFor )
		return ParseFor();
	else if( t1.type == ttWhile )
		return ParseWhile();
	else if( t1.type == ttReturn )
		return ParseReturn();
	else if( t1.type == ttStartStatementBlock )
		return ParseStatementBlock();
	else if( t1.type == ttBreak )
		return ParseBreak();
	else if( t1.type == ttContinue )
		return ParseContinue();
	else if( t1.type == ttDo )
		return ParseDoWhile();
	else if( t1.type == ttSwitch )
		return ParseSwitch();
	else
		return ParseExpressionStatement();
}

asCScriptNode *asCParser::ParseExpressionStatement()
{
	asCScriptNode *node = new asCScriptNode(snExpressionStatement);

	sToken t;
	GetToken(&t);
	if( t.type == ttEndStatement )
	{
		node->UpdateSourcePos(t.pos, t.length);

		return node;
	}

	RewindTo(&t);

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseSwitch()
{
	asCScriptNode *node = new asCScriptNode(snSwitch);

	sToken t;
	GetToken(&t);
	if( t.type != ttSwitch )
	{
		Error(ExpectedToken("switch"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")"), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttStartStatementBlock )
	{
		Error(ExpectedToken("{"), &t);
		return node;
	}
	
	while( !isSyntaxError )
	{
		GetToken(&t);
		
		if( t.type == ttEndStatementBlock || t.type == ttDefault)
			break;

		RewindTo(&t);

		if( t.type != ttCase )
		{
			Error(ExpectedToken("case"), &t);
			return node;
		}

		node->AddChildLast(ParseCase());
		if( isSyntaxError ) return node;
	}

	if( t.type == ttDefault)
	{
		RewindTo(&t);

		node->AddChildLast(ParseCase());
		if( isSyntaxError ) return node;

		GetToken(&t);
	}

	if( t.type != ttEndStatementBlock )
	{
		Error(ExpectedToken("}"), &t);
		return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseCase()
{
	asCScriptNode *node = new asCScriptNode(snCase);

	sToken t;
	GetToken(&t);
	if( t.type != ttCase && t.type != ttDefault )
	{
		Error(ExpectedTokens("case", "default"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	if(t.type == ttCase)
	{
		node->AddChildLast(ParseExpression());
	}

	GetToken(&t);
	if( t.type != ttColon )
	{
		Error(ExpectedToken(":"), &t);
		return node;
	}

	// Parse statements until we find either of }, case, default, and break
	GetToken(&t);
	RewindTo(&t);
	while( t.type != ttCase && 
		   t.type != ttDefault && 
		   t.type != ttEndStatementBlock && 
		   t.type != ttBreak )
	{

		node->AddChildLast(ParseStatement());
		if( isSyntaxError ) return node;

		GetToken(&t);
		RewindTo(&t);
	}

	// If the case was ended with a break statement, add it to the node
	if( t.type == ttBreak )
		node->AddChildLast(ParseBreak());

	return node;
}

asCScriptNode *asCParser::ParseIf()
{
	asCScriptNode *node = new asCScriptNode(snIf);

	sToken t;
	GetToken(&t);
	if( t.type != ttIf )
	{
		Error(ExpectedToken("if"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")"), &t);
		return node;
	}

	node->AddChildLast(ParseStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttElse )
	{
		// No else statement return already
		RewindTo(&t);
		return node;
	}

	node->AddChildLast(ParseStatement());

	return node;
}

asCScriptNode *asCParser::ParseFor()
{
	asCScriptNode *node = new asCScriptNode(snFor);

	sToken t;
	GetToken(&t);
	if( t.type != ttFor )
	{
		Error(ExpectedToken("for"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	if( IsDeclaration() )
		node->AddChildLast(ParseDeclaration());
	else
		node->AddChildLast(ParseExpressionStatement());
	if( isSyntaxError ) return node;

	node->AddChildLast(ParseExpressionStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		RewindTo(&t);

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttCloseParanthesis )
		{
			Error(ExpectedToken(")"), &t);
			return node;
		}
	}

	node->AddChildLast(ParseStatement());
	
	return node;
}




	
asCScriptNode *asCParser::ParseWhile()
{
	asCScriptNode *node = new asCScriptNode(snWhile);

	sToken t;
	GetToken(&t);
	if( t.type != ttWhile )
	{
		Error(ExpectedToken("while"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")"), &t);
		return node;
	}

	node->AddChildLast(ParseStatement());

	return node;
}

asCScriptNode *asCParser::ParseDoWhile()
{
	asCScriptNode *node = new asCScriptNode(snDoWhile);

	sToken t;
	GetToken(&t);
	if( t.type != ttDo )
	{
		Error(ExpectedToken("do"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	node->AddChildLast(ParseStatement());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttWhile )
	{
		Error(ExpectedToken("while"), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttOpenParanthesis )
	{
		Error(ExpectedToken("("), &t);
		return node;
	}

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttCloseParanthesis )
	{
		Error(ExpectedToken(")"), &t);
		return node;
	}

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";"), &t);
		return node;
	}
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseReturn()
{
	asCScriptNode *node = new asCScriptNode(snReturn);

	sToken t;
	GetToken(&t);
	if( t.type != ttReturn )
	{
		Error(ExpectedToken("return"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type == ttEndStatement )
	{
		node->UpdateSourcePos(t.pos, t.length);
		return node;
	}

	RewindTo(&t);

	node->AddChildLast(ParseAssignment());
	if( isSyntaxError ) return node;

	GetToken(&t);
	if( t.type != ttEndStatement )
	{
		Error(ExpectedToken(";"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseBreak()
{
	asCScriptNode *node = new asCScriptNode(snBreak);

	sToken t;
	GetToken(&t);
	if( t.type != ttBreak )
	{
		Error(ExpectedToken("break"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
		Error(ExpectedToken(";"), &t);

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseContinue()
{
	asCScriptNode *node = new asCScriptNode(snContinue);

	sToken t;
	GetToken(&t);
	if( t.type != ttContinue )
	{
		Error(ExpectedToken("continue"), &t);
		return node;
	}

	node->UpdateSourcePos(t.pos, t.length);

	GetToken(&t);
	if( t.type != ttEndStatement )
		Error(ExpectedToken(";"), &t);

	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseAssignment()
{
	asCScriptNode *node = new asCScriptNode(snAssignment);

	node->AddChildLast(ParseCondition());
	if( isSyntaxError ) return node;

	sToken t;
	GetToken(&t);
	RewindTo(&t);

	if( IsAssignOperator(t.type) )
	{
		node->AddChildLast(ParseAssignOperator());
		if( isSyntaxError ) return node;

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;
	}

	return node;
}

asCScriptNode *asCParser::ParseCondition()
{
	asCScriptNode *node = new asCScriptNode(snCondition);

	node->AddChildLast(ParseExpression());
	if( isSyntaxError ) return node;

	sToken t;
	GetToken(&t);
	if( t.type == ttQuestion )
	{
		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;

		GetToken(&t);
		if( t.type != ttColon )
		{
			Error(ExpectedToken(":"), &t);
			return node;
		}

		node->AddChildLast(ParseAssignment());
		if( isSyntaxError ) return node;
	}
	else
		RewindTo(&t);

	return node;
}

asCScriptNode *asCParser::ParseExpression()
{
	asCScriptNode *node = new asCScriptNode(snExpression);

	node->AddChildLast(ParseExprTerm());
	if( isSyntaxError ) return node;

	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);

		if( !IsOperator(t.type) )
			return node;

		node->AddChildLast(ParseExprOperator());
		if( isSyntaxError ) return node;

		node->AddChildLast(ParseExprTerm());
		if( isSyntaxError ) return node;
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprTerm()
{
	asCScriptNode *node = new asCScriptNode(snExprTerm);

	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);
		if( !IsPreOperator(t.type) )
			break;

		node->AddChildLast(ParseExprPreOp());
		if( isSyntaxError ) return node;
	}

	node->AddChildLast(ParseExprValue());
	if( isSyntaxError ) return node;

	
	for(;;)
	{
		sToken t;
		GetToken(&t);
		RewindTo(&t);
		if( !IsPostOperator(t.type) )
			return node;

		node->AddChildLast(ParseExprPostOp());
		if( isSyntaxError ) return node;
	}
	return 0;
}

asCScriptNode *asCParser::ParseExprPreOp()
{
	asCScriptNode *node = new asCScriptNode(snExprPreOp);

	sToken t;
	GetToken(&t);
	if( !IsPreOperator(t.type) )
	{
		Error(TXT_EXPECTED_PRE_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseExprPostOp()
{
	asCScriptNode *node = new asCScriptNode(snExprPostOp);

	sToken t;
	GetToken(&t);
	if( !IsPostOperator(t.type) )
	{
		Error(TXT_EXPECTED_POST_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	if( t.type == ttDot || t.type == ttArrow )
	{
		sToken t1, t2;
		GetToken(&t1);
		GetToken(&t2);
		RewindTo(&t1);
		if( t2.type == ttOpenParanthesis )
			node->AddChildLast(ParseFunctionCall());
		else
			node->AddChildLast(ParseIdentifier());
	}
	else if( t.type == ttOpenBracket )
	{
		node->AddChildLast(ParseAssignment());

		GetToken(&t);
		if( t.type != ttCloseBracket )
		{
			ExpectedToken("]");
			return node;
		}

		node->UpdateSourcePos(t.pos, t.length);
	}

	return node;
}

asCScriptNode *asCParser::ParseExprOperator()
{
	asCScriptNode *node = new asCScriptNode(snExprOperator);

	sToken t;
	GetToken(&t);
	if( !IsOperator(t.type) )
	{
		Error(TXT_EXPECTED_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

asCScriptNode *asCParser::ParseAssignOperator()
{
	asCScriptNode *node = new asCScriptNode(snExprOperator);

	sToken t;
	GetToken(&t);
	if( !IsAssignOperator(t.type) )
	{
		Error(TXT_EXPECTED_OPERATOR, &t);
		return node;
	}

	node->SetToken(&t);
	node->UpdateSourcePos(t.pos, t.length);

	return node;
}

void asCParser::GetToken(sToken *token)
{
	int sourceLength = script->code.GetLength();

	do
	{
		if( sourcePos >= sourceLength )
		{
			token->type = ttEnd;
			token->length = 0;
		}
		else
			token->type = tokenizer.GetToken(&script->code[sourcePos], sourceLength - sourcePos, &token->length);

		token->pos = sourcePos;

		// Update state
		sourcePos += token->length;
	}
	// Filter out whitespace and comments
	while( token->type == ttWhiteSpace || 
	       token->type == ttOnelineComment ||
		   token->type == ttMultilineComment );
}

void asCParser::RewindTo(const sToken *token)
{
	sourcePos = token->pos;
}

void asCParser::Error(const char *text, sToken *token)
{
	RewindTo(token);

	isSyntaxError     = true;
	errorWhileParsing = true;

	int row, col;
	script->ConvertPosToRowCol(token->pos, &row, &col);

	if( builder )
		builder->WriteError(script->name, text, row, col);
}

bool asCParser::IsRealType(int tokenType)
{
	if( tokenType == ttVoid ||
		tokenType == ttInt ||
		tokenType == ttInt8 ||
		tokenType == ttInt16 ||
		tokenType == ttUInt ||
		tokenType == ttUInt8 ||
		tokenType == ttUInt16 ||
		tokenType == ttFloat ||
		tokenType == ttBool ||
		tokenType == ttBits ||
		tokenType == ttBits8 ||
		tokenType == ttBits16 ||
		tokenType == ttDouble )
		return true;

	return false;
}


bool asCParser::IsDataType(int tokenType)
{
	if( tokenType == ttIdentifier ||
		IsRealType(tokenType) )
		return true;

	return false;
}

bool asCParser::IsOperator(int tokenType)
{
	if( tokenType == ttPlus ||
		tokenType == ttMinus ||
		tokenType == ttStar ||
		tokenType == ttSlash ||
		tokenType == ttPercent ||
		tokenType == ttAnd ||
		tokenType == ttOr ||
		tokenType == ttXor ||
		tokenType == ttEqual ||
		tokenType == ttNotEqual ||
		tokenType == ttLessThan ||
		tokenType == ttLessThanOrEqual ||
		tokenType == ttGreaterThan ||
		tokenType == ttGreaterThanOrEqual ||
		tokenType == ttAmp ||
		tokenType == ttBitOr ||
		tokenType == ttBitXor ||
		tokenType == ttBitShiftLeft ||
		tokenType == ttBitShiftRight ||
		tokenType == ttBitShiftRightArith )
		return true;

	return false;
}

bool asCParser::IsAssignOperator(int tokenType)
{
	if( tokenType == ttAssignment ||
		tokenType == ttAddAssign ||
		tokenType == ttSubAssign ||
		tokenType == ttMulAssign ||
		tokenType == ttDivAssign ||
		tokenType == ttModAssign ||
		tokenType == ttAndAssign ||
		tokenType == ttOrAssign ||
		tokenType == ttXorAssign ||
		tokenType == ttShiftLeftAssign ||
		tokenType == ttShiftRightLAssign ||
		tokenType == ttShiftRightAAssign )
		return true;

	return false;
}

bool asCParser::IsPreOperator(int tokenType)
{
	if( tokenType == ttMinus ||
		tokenType == ttPlus ||
		tokenType == ttNot ||
		tokenType == ttInc ||
		tokenType == ttDec ||
		tokenType == ttBitNot )
		return true;
	return false;
}

bool asCParser::IsPostOperator(int tokenType)
{
	if( tokenType == ttInc ||
		tokenType == ttDec ||
		tokenType == ttDot ||
		tokenType == ttArrow ||
		tokenType == ttOpenBracket )
		return true;
	return false;
}

bool asCParser::IsConstant(int tokenType)
{
	if( tokenType == ttIntConstant ||
		tokenType == ttFloatConstant ||
		tokenType == ttDoubleConstant ||
		tokenType == ttStringConstant ||
		tokenType == ttTrue ||
		tokenType == ttFalse ||
		tokenType == ttBitsConstant )
		return true;

	return false;
}

asCString asCParser::ExpectedToken(const char *token)
{
	asCString str;

	str.Format(TXT_EXPECTED_s, token);

	return str;
}

asCString asCParser::ExpectedTokens(const char *t1, const char *t2)
{
	asCString str;

	str.Format(TXT_EXPECTED_s_OR_s, t1, t2);

	return str;
}
