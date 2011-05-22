#include "debugger.h"
#include <iostream>  // cout

using namespace std;

CDebugger::CDebugger()
{
	m_action = CONTINUE;
}

CDebugger::~CDebugger()
{
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

	cout << "line: " << ctx->GetLineNumber() << endl;

	TakeCommands(ctx);
}

bool CDebugger::CheckBreakPoint(asIScriptContext *ctx)
{
	// TODO: consider just filename, not the full path
	// TODO: do case less comparison
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
					breakPoints.clear();
				else
				{
					int nbr = atoi(br.c_str());
					if( nbr >= 0 && nbr < (int)breakPoints.size() )
						breakPoints.erase(breakPoints.begin()+nbr);
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
					// List all break points
					for( size_t b = 0; b < breakPoints.size(); b++ )
						cout << b << " - " << breakPoints[b].file << ":" << breakPoints[b].lineNbr << endl;
				}
				else
				{
					cout << "Unknown list option, expected one of:" << endl;
					cout << "b - breakpoints" << endl;
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

	case 'p':
		// print some value
		// take more commands
		return false;

	default:
		cout << "Unknown command" << endl;
		// take more commandsc
		return false;
	}

	// Continue execution
	return true;
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
	cout << "n - Step over" << endl;
	cout << "o - Step out" << endl;
	cout << "b - Set break point" << endl;
	cout << "h - Print this help text" << endl;
}