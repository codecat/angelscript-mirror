#include "debugger.h"
#include <iostream>  // cout
#include <sstream> // stringstream

using namespace std;

CDebugger::CDebugger()
{
	m_action = CONTINUE;
}

CDebugger::~CDebugger()
{
}

string CDebugger::ToString(void *value, asUINT typeId)
{
	stringstream s;
	if( typeId == asTYPEID_VOID )
		return "<void>";
	else if( typeId == asTYPEID_BOOL )
		return *(bool*)value ? "true" : "false";
	else if( typeId == asTYPEID_INT8 )
		s << *(signed char*)value;
	else if( typeId == asTYPEID_INT16 )
		s << *(signed short*)value;
	else if( typeId == asTYPEID_INT32 )
		s << *(signed int*)value;
	else if( typeId == asTYPEID_INT64 )
		s << *(asINT64*)value;
	else if( typeId == asTYPEID_UINT8 )
		s << *(unsigned char*)value;
	else if( typeId == asTYPEID_UINT16 )
		s << *(unsigned short*)value;
	else if( typeId == asTYPEID_UINT32 )
		s << *(unsigned int*)value;
	else if( typeId == asTYPEID_UINT64 )
		s << *(asQWORD*)value;
	else if( typeId == asTYPEID_FLOAT )
		s << *(float*)value;
	else if( typeId == asTYPEID_DOUBLE )
		s << *(double*)value;
	else
		s << "{" << value << "}";

	// TODO: Should expand enums to the enum name
	// TODO: Should expand script classes to show values of members

	return s.str();
}

void CDebugger::LineCallback(asIScriptContext *ctx)
{
	if( m_action == CONTINUE )
	{
		if( !CheckBreakPoint(ctx) )
			return;
	}
	else if( m_action == STEP_OVER )
	{
		if( ctx->GetCallstackSize() > m_currentStackLevel )
		{
			if( !CheckBreakPoint(ctx) )
				return;
		}
	}
	else if( m_action == STEP_OUT )
	{
		if( ctx->GetCallstackSize() >= m_currentStackLevel )
		{
			if( !CheckBreakPoint(ctx) )
				return;
		}
	}
	else if( m_action == STEP_INTO )
	{
		// Always break
	}

	cout << ctx->GetFunction()->GetDeclaration() << ":" << ctx->GetLineNumber() << endl;

	TakeCommands(ctx);
}

bool CDebugger::CheckBreakPoint(asIScriptContext *ctx)
{
	// TODO: Should cache the break points in a function by checking which possible break points
	//       can be hit when entering a function. If there are no break points in the current function
	//       then there is no need to check every line.

	// TODO: consider just filename, not the full path
	// TODO: do case-less comparison
	const char *file = 0;
	int lineNbr = ctx->GetLineNumber(0, 0, &file);

	for( size_t n = 0; n < breakPoints.size(); n++ )
	{
		if( breakPoints[n].lineNbr == lineNbr &&
			breakPoints[n].file == file )
		{
			cout << "Reached break point " << n << " in file '" << file << "' at line " << lineNbr << endl;
			return true;
		}
	}

	return false;
}

void CDebugger::TakeCommands(asIScriptContext *ctx)
{
	for(;;)
	{
		char buf[512];

		cout << "[dbg]> ";
		cin.getline(buf, 512);

		if( InterpretCommand(string(buf), ctx) )
			break;
	}
}

bool CDebugger::InterpretCommand(string &cmd, asIScriptContext *ctx)
{
	if( cmd.length() == 0 ) return true;

	switch( cmd[0] )
	{
	case 'c':
		m_action = CONTINUE;
		break;

	case 's':
		m_action = STEP_INTO;
		break;

	case 'n':
		m_action = STEP_OVER;
		m_currentStackLevel = ctx->GetCallstackSize();
		break;

	case 'o':
		m_action = STEP_OUT;
		m_currentStackLevel = ctx->GetCallstackSize();
		break;

	case 'b':
		{
			// Set break point
			// TODO: When no : is given, assume it is a function name
			//       As many functions can have the same name, set the break point for the first function entered with that name
			size_t div = cmd.find(':'); 
			if( div != string::npos && div > 2 )
			{
				string file = cmd.substr(2, div-2);
				string line = cmd.substr(div+1);

				int nbr = atoi(line.c_str());

				AddBreakPoint(file, nbr);
			}
			else
			{
				cout << "Incorrect format for setting break point, expected:" << endl;
				cout << "b <file name>:<line number>" << endl;
			}
		}
		// take more commands
		return false;

	case 'r':
		{
			// Remove break point
			if( cmd.length() > 2 )
			{
				string br = cmd.substr(2);
				if( br == "all" )
				{
					breakPoints.clear();
					cout << "All break points have been removed" << endl;
				}
				else
				{
					int nbr = atoi(br.c_str());
					if( nbr >= 0 && nbr < (int)breakPoints.size() )
						breakPoints.erase(breakPoints.begin()+nbr);
					ListBreakPoints();
				}
			}
			else
			{
				cout << "Incorrect format for removing break points, expected:" << endl;
				cout << "r <all|number of break point>" << endl;
			}
		}
		// take more commands
		return false;

	case 'l':
		{
			// List something
			size_t p = cmd.find_first_not_of(" \t", 1);
			if( p != string::npos )
			{
				if( cmd[p] == 'b' )
				{
					ListBreakPoints();
				}
				else if( cmd[p] == 'v' )
				{
					ListLocalVariables(ctx);
				}
				else if( cmd[p] == 'g' )
				{
					ListGlobalVariables(ctx);
				}
				else
				{
					cout << "Unknown list option, expected one of:" << endl;
					cout << "b - breakpoints" << endl;
					cout << "v - local variables" << endl;
					cout << "g - global variables" << endl;
				}
			}
			else 
			{
				cout << "Incorrect format for list, expected:" << endl;
				cout << "l <list option>" << endl;
			}
		}
		// take more commands
		return false;

	case 'h':
		PrintHelp();
		// take more commands
		return false;

	case 'e':
		// TODO: Implement this
		// Evaluate some expression
		// take more commands
		return false;

	case 'w':
		// Where am I?
		PrintCallstack(ctx);
		// take more commands
		return false;

	case 'a':
		// abort the execution
		ctx->Abort();
		break;

	default:
		cout << "Unknown command" << endl;
		// take more commandsc
		return false;
	}

	// Continue execution
	return true;
}

void CDebugger::ListBreakPoints()
{
	// List all break points
	for( size_t b = 0; b < breakPoints.size(); b++ )
		cout << b << " - " << breakPoints[b].file << ":" << breakPoints[b].lineNbr << endl;
}

void CDebugger::ListLocalVariables(asIScriptContext *ctx)
{
	asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	for( int n = 0; n < func->GetVarCount(); n++ )
	{
		// TODO: Should only list the variables visible at the current position
		cout << func->GetVarDecl(n) << " = " << ToString(ctx->GetAddressOfVar(n), ctx->GetVarTypeId(n)) << endl;
	}
}

void CDebugger::ListGlobalVariables(asIScriptContext *ctx)
{
	// Determine the current module from the function
	asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	asIScriptModule *mod = ctx->GetEngine()->GetModule(func->GetModuleName(), asGM_ONLY_IF_EXISTS);
	if( !mod ) return;

	for( int n = 0; n < mod->GetGlobalVarCount(); n++ )
	{
		int typeId;
		mod->GetGlobalVar(n, 0, &typeId);
		cout << mod->GetGlobalVarDeclaration(n) << " = " << ToString(mod->GetAddressOfGlobalVar(n), typeId) << endl;
	}
}

void CDebugger::PrintCallstack(asIScriptContext *ctx)
{
	for( asUINT n = 0; n < ctx->GetCallstackSize(); n++ )
	{
		cout << ctx->GetFunction(n)->GetDeclaration() << ":" << ctx->GetLineNumber(n) << endl;
	}
}

void CDebugger::AddBreakPoint(std::string &file, int lineNbr)
{
	// TODO: Store just file name, not entire path
	// TODO: Verify that there actually is any byte code on that line, otherwise the breakpoint will never be reached

	cout << "Setting break point in file '" << file << "' at line " << lineNbr << endl;

	BreakPoint bp(file, lineNbr);
	breakPoints.push_back(bp);
}

void CDebugger::PrintHelp()
{
	cout << "c - Continue" << endl;
	cout << "s - Step into" << endl;
	cout << "n - Next step" << endl;
	cout << "o - Step out" << endl;
	cout << "b - Set break point" << endl;
	cout << "l - List various things" << endl;
	cout << "r - Remove break point" << endl;
	cout << "w - Where am I?" << endl;
	cout << "a - Abort execution" << endl;
	cout << "h - Print this help text" << endl;
}