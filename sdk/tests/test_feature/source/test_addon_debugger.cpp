#include "utils.h"
#include "../../../add_on/debugger/debugger.h"

namespace Test_Addon_Debugger
{

class CMyDebugger : public CDebugger
{
public:
	CMyDebugger() : CDebugger() {}

	void Output(const std::string &str)
	{
		// Append output to local buffer instead of the screen
		output += str;
	}

	void TakeCommands(asIScriptContext *ctx)
	{
		// Simulate the user stepping through the execution
		InterpretCommand("s", ctx);
	}

	void LineCallback(asIScriptContext *ctx)
	{
		// Call the command for listing local variables
		InterpretCommand("l v", ctx);

		// Invoke the original line callback
		CDebugger::LineCallback(ctx);
	}

	std::string ToString(void *value, asUINT typeId, bool expandMembers, asIScriptEngine *engine)
	{
		// Interpret the string value
		if( typeId == engine->GetTypeIdByDecl("string") )
		{
			return "\"" + *reinterpret_cast<std::string*>(value) + "\"";
		}

		// Let debugger do the rest
		std::string str = CDebugger::ToString(value, typeId, expandMembers, engine);

		// This here is just so I can automate the test. All the addresses in this test
		// will be exactly the same, so we substitute it with XXXXXXXX to make sure the
		// test will be platform independent.
		if( str.length() > 0 && str[0] == '{' )
		{
			if( lastAddress == "" )
			{
				lastAddress = str;
				return "{XXXXXXXX}";
			}
			else if( str == lastAddress )
				return "{XXXXXXXX}";
			else
				return str;
		}

		return str;
	}

	std::string output;
	std::string lastAddress;
};

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CMyDebugger debug;
 	asIScriptEngine *engine;
	asIScriptModule *mod;
	asIScriptContext *ctx;
	
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);

		const char *script = 
			"void func(int a, const int &in b, string c, const string &in d, type @e, type &f, type @&in g) \n"
			"{ \n"
			"} \n"
			"class type {} \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		ctx->SetLineCallback(asMETHOD(CMyDebugger, LineCallback), &debug, asCALL_THISCALL);

		debug.InterpretCommand("s", ctx);

		r = ExecuteString(engine, "type t; func(1, 2, 'c', 'd', t, t, t)", mod, ctx);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( debug.output != "ExecuteString:1; void ExecuteString()\n"
							"ExecuteString:1; void ExecuteString()\n"
							"script:0; type@ type()\n"
							"script:0; type::type()\n"
							"type t = {XXXXXXXX}\n"
							"ExecuteString:1; void ExecuteString()\n"
							"int a = 1\n"
							"const int& b = 2\n"
							"string c = \"c\"\n"
							"const string& d = \"d\"\n"
							"type@ e = {XXXXXXXX}\n"
							"type& f = {XXXXXXXX}\n"
							"type@& g = {XXXXXXXX}\n"
							"script:3; void func(int, const int&in, string, const string&in, type@, type&inout, type@&in)\n"
							"int a = 1\n"
							"const int& b = 2\n"
							"string c = \"c\"\n"
							"const string& d = \"d\"\n"
							"type@ e = {XXXXXXXX}\n"
							"type& f = {XXXXXXXX}\n"
							"type@& g = {XXXXXXXX}\n"
							"script:3; void func(int, const int&in, string, const string&in, type@, type&inout, type@&in)\n"
							"type t = {XXXXXXXX}\n"
							"ExecuteString:2; void ExecuteString()\n" )
		{
			printf("%s", debug.output.c_str());
			TEST_FAILED;
		}

		ctx->Release();
		
		engine->Release();
	}

	return fail;
}

} // namespace

