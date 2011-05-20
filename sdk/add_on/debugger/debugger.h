#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <angelscript.h>
#include <string>
#include <vector>

class CDebugger
{
public:
	CDebugger();
	virtual ~CDebugger();

	virtual void LineCallback(asIScriptContext *ctx);
	virtual void TakeCommands(asIScriptContext *ctx);
	virtual bool InterpretCommand(std::string &cmd, asIScriptContext *ctx);
	virtual void PrintHelp();
	virtual void AddBreakPoint(std::string &file, int lineNbr);
	virtual bool CheckBreakPoint(asIScriptContext *ctx);

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

	struct BreakPoint
	{
		BreakPoint(std::string f, int n) : file(f), lineNbr(n) {}
		std::string file;
		int         lineNbr;
	};
	std::vector<BreakPoint> breakPoints;
};

#endif