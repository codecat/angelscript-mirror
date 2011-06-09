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
	virtual bool InterpretCommand(const std::string &cmd, asIScriptContext *ctx);
	virtual void PrintHelp();
	virtual void AddFileBreakPoint(const std::string &file, int lineNbr);
	virtual void AddFuncBreakPoint(const std::string &func);
	virtual bool CheckBreakPoint(asIScriptContext *ctx);
	virtual void ListBreakPoints();
	virtual void ListLocalVariables(asIScriptContext *ctx);
	virtual void ListGlobalVariables(asIScriptContext *ctx);
	virtual void ListMemberProperties(asIScriptContext *ctx);
	virtual void ListStatistics(asIScriptContext *ctx);
	virtual void PrintCallstack(asIScriptContext *ctx);
	virtual std::string ToString(void *value, asUINT typeId, bool expandMembers, asIScriptEngine *engine);
	virtual void Output(const std::string &str);
	virtual void PrintValue(const std::string &expr, asIScriptContext *ctx);

protected:
	enum DebugAction
	{
		CONTINUE,  // continue until next break point
		STEP_INTO, // stop at next instruction
		STEP_OVER, // stop at next instruction, skipping called functions
		STEP_OUT   // run until returning from current function
	};
	DebugAction m_action;
	asUINT      m_lastCommandAtStackLevel;
	asUINT      m_lastStackLevel;

	struct BreakPoint
	{
		BreakPoint(std::string f, int n, bool _func) : name(f), lineNbr(n), func(_func) {}
		std::string name;
		int         lineNbr;
		bool        func;
	};
	std::vector<BreakPoint> breakPoints;
};

#endif