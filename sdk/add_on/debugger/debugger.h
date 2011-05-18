#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <angelscript.h>
#include <string>

class CDebugger
{
public:
	CDebugger();
	~CDebugger();

	virtual void LineCallback(asIScriptContext *ctx);
	virtual void TakeCommands(asIScriptContext *ctx);
	virtual bool InterpretCommand(std::string &cmd, asIScriptContext *ctx);
	virtual void PrintHelp();

protected:
	enum DebugAction
	{
		CONTINUE,  // continue until next break point
		STEP_INTO, // stop at next instruction
		STEP_OVER, // stop at next instruction, skipping called functions
		STEP_OUT   // run until returning from current function
	};
	DebugAction m_action;
	asUINT      m_currentStackLevel;
};

#endif