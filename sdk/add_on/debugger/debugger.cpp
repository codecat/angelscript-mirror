#include "debugger.h"
#include <iostream>  // cout
#include <sstream> // stringstream

using namespace std;

CDebugger::CDebugger()
{
	m_action = CONTINUE;
	m_lastStackLevel = 0;
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
#if defined(_MSC_VER) && _MSC_VER <= 1200
		s << "{...}"; // MSVC6 doesn't like the operator for 64bit integer
#else
		s << *(asINT64*)value;
#endif
	else if( typeId == asTYPEID_UINT8 )
		s << *(unsigned char*)value;
	else if( typeId == asTYPEID_UINT16 )
		s << *(unsigned short*)value;
	else if( typeId == asTYPEID_UINT32 )
		s << *(unsigned int*)value;
	else if( typeId == asTYPEID_UINT64 )
#if defined(_MSC_VER) && _MSC_VER <= 1200
		s << "{...}"; // MSVC6 doesn't like the operator for 64bit integer
#else
		s << *(asQWORD*)value;
#endif
	else if( typeId == asTYPEID_FLOAT )
		s << *(float*)value;
	else if( typeId == asTYPEID_DOUBLE )
		s << *(double*)value;
	else
		s << "{" << value << "}";

	// TODO: Should expand enums to the enum name
	// TODO: Should expand script classes to show values of members
	// TODO: Value types can have their properties expanded by default

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
		if( ctx->GetCallstackSize() > m_lastCommandAtStackLevel )
		{
			if( !CheckBreakPoint(ctx) )
				return;
		}
	}
	else if( m_action == STEP_OUT )
	{
		if( ctx->GetCallstackSize() >= m_lastCommandAtStackLevel )
		{
			if( !CheckBreakPoint(ctx) )
				return;
		}
	}
	else if( m_action == STEP_INTO )
	{
		CheckBreakPoint(ctx);

		// Always break, but we call the check break point anyway 
		// to tell user when break point has been reached
	}

	stringstream s;
	s << ctx->GetFunction()->GetDeclaration() << ":" << ctx->GetLineNumber() << endl;
	Output(s.str());

	TakeCommands(ctx);
}

bool CDebugger::CheckBreakPoint(asIScriptContext *ctx)
{
	// TODO: Should cache the break points in a function by checking which possible break points
	//       can be hit when entering a function. If there are no break points in the current function
	//       then there is no need to check every line.

	// TODO: A break point can be placed on a line where there is no code. In this case  
	//       the first line with code after it should be taken as the break point. It's 
	//       necessary to know where the function is first declared.

	if( m_lastStackLevel < ctx->GetCallstackSize() )
	{
		// We've moved into a new function, so we need to check for a breakpoint at entering the function
		for( size_t n = 0; n < breakPoints.size(); n++ )
		{
			if( breakPoints[n].func )
			{
				if( breakPoints[n].name == ctx->GetFunction()->GetName() )
				{
					stringstream s;
					s << "Entering function '" << breakPoints[n].name << "'. Transforming it into break point" << endl;
					Output(s.str());

					// Transform the function breakpoint into a file breakpoint
					const char *file = 0;
					int lineNbr = ctx->GetLineNumber(0, 0, &file);
					breakPoints[n].name    = file;
					breakPoints[n].lineNbr = lineNbr;
					breakPoints[n].func    = false;
				}
			}
		}
	}
	m_lastStackLevel = ctx->GetCallstackSize();

	const char *tmp = 0;
	int lineNbr = ctx->GetLineNumber(0, 0, &tmp);

	// Consider just filename, not the full path
	string file = tmp;
	size_t r = file.find_last_of("\\/");
	if( r != string::npos )
		file = file.substr(r+1);

	for( size_t n = 0; n < breakPoints.size(); n++ )
	{
		// TODO: do case-less comparison
		if( !breakPoints[n].func &&
			breakPoints[n].lineNbr == lineNbr &&
			breakPoints[n].name == file )
		{
			stringstream s;
			s << "Reached break point " << n << " in file '" << file << "' at line " << lineNbr << endl;
			Output(s.str());
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

		Output("[dbg]> ");
		cin.getline(buf, 512);

		if( InterpretCommand(string(buf), ctx) )
			break;
	}
}

bool CDebugger::InterpretCommand(const string &cmd, asIScriptContext *ctx)
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
		m_lastCommandAtStackLevel = ctx->GetCallstackSize();
		break;

	case 'o':
		m_action = STEP_OUT;
		m_lastCommandAtStackLevel = ctx->GetCallstackSize();
		break;

	case 'b':
		{
			// Set break point
			size_t div = cmd.find(':'); 
			if( div != string::npos && div > 2 )
			{
				string file = cmd.substr(2, div-2);
				string line = cmd.substr(div+1);

				int nbr = atoi(line.c_str());

				AddFileBreakPoint(file, nbr);
			}
			else if( div == string::npos && (div = cmd.find_first_not_of(" \t", 1)) != string::npos )
			{
				string func = cmd.substr(div);

				AddFuncBreakPoint(func);
			}
			else
			{
				Output("Incorrect format for setting break point, expected one of:\n"
				       "b <file name>:<line number>\n"
				       "b <function name>\n");
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
					Output("All break points have been removed\n");
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
				Output("Incorrect format for removing break points, expected:\n"
				       "r <all|number of break point>\n");
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
				else if( cmd[p] == 's' )
				{
					ListStatistics(ctx);
				}
				else
				{
					Output("Unknown list option, expected one of:\n"
					       "b - breakpoints\n"
					       "v - local variables\n"
					       "g - global variables\n"
						   "s - statistics\n");
				}
			}
			else 
			{
				Output("Incorrect format for list, expected:\n"
				       "l <list option>\n");
			}
		}
		// take more commands
		return false;

	case 'h':
		PrintHelp();
		// take more commands
		return false;

	case 'p':
		// TODO: Implement this
		// Print a value (this is simpler than evaluate)
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
		Output("Unknown command\n");
		// take more commandsc
		return false;
	}

	// Continue execution
	return true;
}

void CDebugger::ListBreakPoints()
{
	// List all break points
	stringstream s;
	for( size_t b = 0; b < breakPoints.size(); b++ )
		if( breakPoints[b].func )
			s << b << " - " << breakPoints[b].name << endl;
		else
			s << b << " - " << breakPoints[b].name << ":" << breakPoints[b].lineNbr << endl;
	Output(s.str());
}

void CDebugger::ListLocalVariables(asIScriptContext *ctx)
{
	asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	stringstream s;
	for( asUINT n = 0; n < func->GetVarCount(); n++ )
	{
		if( ctx->IsVarInScope(n) )
			s << func->GetVarDecl(n) << " = " << ToString(ctx->GetAddressOfVar(n), ctx->GetVarTypeId(n)) << endl;
	}
	Output(s.str());
}

void CDebugger::ListGlobalVariables(asIScriptContext *ctx)
{
	// Determine the current module from the function
	asIScriptFunction *func = ctx->GetFunction();
	if( !func ) return;

	asIScriptModule *mod = ctx->GetEngine()->GetModule(func->GetModuleName(), asGM_ONLY_IF_EXISTS);
	if( !mod ) return;

	stringstream s;
	for( int n = 0; n < mod->GetGlobalVarCount(); n++ )
	{
		int typeId;
		mod->GetGlobalVar(n, 0, &typeId);
		s << mod->GetGlobalVarDeclaration(n) << " = " << ToString(mod->GetAddressOfGlobalVar(n), typeId) << endl;
	}
	Output(s.str());
}

void CDebugger::ListStatistics(asIScriptContext *ctx)
{
	asIScriptEngine *engine = ctx->GetEngine();
	
	asUINT gcCurrSize, gcTotalDestr, gcTotalDet;
	engine->GetGCStatistics(&gcCurrSize, &gcTotalDestr, &gcTotalDet);

	stringstream s;
	s << "Garbage collector: " << endl;
	s << " current size:    " << gcCurrSize << endl;
	s << " total destroyed: " << gcTotalDestr << endl;
	s << " total detected:  " << gcTotalDet << endl;

	Output(s.str());
}

void CDebugger::PrintCallstack(asIScriptContext *ctx)
{
	stringstream s;
	for( asUINT n = 0; n < ctx->GetCallstackSize(); n++ )
		s << ctx->GetFunction(n)->GetDeclaration() << ":" << ctx->GetLineNumber(n) << endl;
	Output(s.str());
}

void CDebugger::AddFuncBreakPoint(const string &func)
{
	// Trim the function name
	size_t b = func.find_first_not_of(" \t");
	size_t e = func.find_last_not_of(" \t");
	string actual = func.substr(b, e != string::npos ? e-b+1 : string::npos);

	stringstream s;
	s << "Adding deferred break point for function '" << actual << "'" << endl;
	Output(s.str());

	BreakPoint bp(actual, 0, true);
	breakPoints.push_back(bp);
}

void CDebugger::AddFileBreakPoint(const string &file, int lineNbr)
{
	// Store just file name, not entire path
	size_t r = file.find_last_of("\\/");
	string actual;
	if( r != string::npos )
		actual = file.substr(r+1);
	else
		actual = file;

	// Trim the file name
	size_t b = actual.find_first_not_of(" \t");
	size_t e = actual.find_last_not_of(" \t");
	actual = actual.substr(b, e != string::npos ? e-b+1 : string::npos);

	stringstream s;
	s << "Setting break point in file '" << actual << "' at line " << lineNbr << endl;
	Output(s.str());

	BreakPoint bp(actual, lineNbr, false);
	breakPoints.push_back(bp);
}

void CDebugger::PrintHelp()
{
	Output("c - Continue\n"
	       "s - Step into\n"
	       "n - Next step\n"
	       "o - Step out\n"
	       "b - Set break point\n"
	       "l - List various things\n"
	       "r - Remove break point\n"
	       "w - Where am I?\n"
	       "a - Abort execution\n"
	       "h - Print this help text\n");
}

void CDebugger::Output(const string &str)
{
	// By default we just output to stdout
	cout << str;
}