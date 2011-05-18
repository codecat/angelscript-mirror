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
		// Check for break points
		// TODO: Implement this
		return;
	}
	else if( m_action == STEP_OVER )
	{
		if( ctx->GetCallstackSize() > m_currentStackLevel )
		{
			// Check for break points
			// TODO: Implement this
			return;
		}
	}
	else if( m_action == STEP_OUT )
	{
		if( ctx->GetCallstackSize() >= m_currentStackLevel )
		{
			// Check for break points
			// TODO: Implement this
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

void CDebugger::TakeCommands(asIScriptContext *ctx)
{
	for(;;)
	{
		string cmd;
		cout << "[dbg]> ";
		cin >> cmd;

		if( InterpretCommand(cmd, ctx) )
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
		// Set break point
		// take more commands
		return false;

	case 'r':
		// Remove break point
		// take more commands
		return false;

	case 'l':
		// List something
		// take more commands
		return false;

	case 'h':
	case '?':
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

void CDebugger::PrintHelp()
{
	cout << "c - Continue" << endl;
	cout << "s - Step into" << endl;
	cout << "n - Step over" << endl;
	cout << "o - Step out" << endl;
	cout << "h - Print this help text" << endl;
}