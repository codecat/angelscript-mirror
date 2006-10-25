#ifdef _MSC_VER
#pragma warning(disable:4786) // disable warnings about truncated symbol names
#endif

#include <iostream>  // cout
#include <assert.h>  // assert()
#include <conio.h>   // kbhit(), getch()
#include <windows.h> // timeGetTime()
#include <set>
#include <angelscript.h>

#include "../../../add_on/scriptstring/scriptstring.h"

using namespace std;


// This class is a helper for loading scripts, and support the #include directive
class ScriptLoader
{
public:
	ScriptLoader(asIScriptEngine *engine);

	int LoadScript(const char *file);
	int ProcessInclude(string &script);

	asIScriptEngine *engine;

	set<string> includedScripts;
};


// Function prototypes
int  RunApplication();
void ConfigureEngine(asIScriptEngine *engine);
int  CompileScript(asIScriptEngine *engine);
void PrintString(string &str);
void PrintString_Generic(asIScriptGeneric *gen);
void timeGetTime_Generic(asIScriptGeneric *gen);
void LineCallback(asIScriptContext *ctx, DWORD *timeOut);

int main(int argc, char **argv)
{
	RunApplication();

	// Wait until the user presses a key
	cout << endl << "Press any key to quit." << endl;
	while(!getch());

	return 0;
}

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}


int RunApplication()
{
	int r;

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will write any compiler messages to the callback.
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine(engine);
	
	// Compile the script code
	r = CompileScript(engine);
	if( r < 0 )
	{
		engine->Release();
		return -1;
	}

	// Create a context that will execute the script.
	asIScriptContext *ctx = engine->CreateContext();
	if( ctx == 0 ) 
	{
		cout << "Failed to create the context." << endl;
		engine->Release();
		return -1;
	}

	// We don't want to allow the script to hang the application, e.g. with an
	// infinite loop, so we'll use the line callback function to set a timeout
	// that will abort the script after a certain time. Before executing the 
	// script the timeOut variable will be set to the time when the script must 
	// stop executing. 
	DWORD timeOut;
	r = ctx->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);
	if( r < 0 )
	{
		cout << "Failed to set the line callback function." << endl;
		ctx->Release();
		engine->Release();
		return -1;
	}

	// Find the function id for the function we want to execute.
	int funcId = engine->GetFunctionIDByDecl(0, "void main()");
	if( funcId < 0 )
	{
		cout << "The function 'void main()' was not found." << endl;
		ctx->Release();
		engine->Release();
		return -1;
	}

	// Prepare the script context with the function we wish to execute. Prepare()
	// must be called on the context before each new script function that will be
	// executed. Note, that if you intend to execute the same function several 
	// times, it might be a good idea to store the function id returned by 
	// GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
	r = ctx->Prepare(funcId);
	if( r < 0 ) 
	{
		cout << "Failed to prepare the context." << endl;
		ctx->Release();
		engine->Release();
		return -1;
	}

	// Set the timeout before executing the function. Give the function 1 sec
	// to return before we'll abort it.
	timeOut = timeGetTime() + 1000;

	// Execute the function
	cout << "Executing the script." << endl;
	cout << "---" << endl;
	r = ctx->Execute();
	cout << "---" << endl;
	if( r != asEXECUTION_FINISHED )
	{
		// The execution didn't finish as we had planned. Determine why.
		if( r == asEXECUTION_ABORTED )
			cout << "The script was aborted before it could finish. Probably it timed out." << endl;
		else if( r == asEXECUTION_EXCEPTION )
		{
			cout << "The script ended with an exception." << endl;

			// Write some information about the script exception
			int funcID = ctx->GetExceptionFunction();
			cout << "func: " << engine->GetFunctionDeclaration(funcID) << endl;
			cout << "modl: " << engine->GetFunctionModule(funcID) << endl;
			cout << "sect: " << engine->GetFunctionSection(funcID) << endl;
			cout << "line: " << ctx->GetExceptionLineNumber() << endl;
			cout << "desc: " << ctx->GetExceptionString() << endl;
		}
		else
			cout << "The script ended for some unforeseen reason (" << r << ")." << endl;
	}
	else
	{
		cout << "The script finished successfully." << endl;
	}

	// We must release the contexts when no longer using them
	ctx->Release();

	// Release the engine
	engine->Release();

	return 0;
}

void ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// Register the script string type
	// Look at the implementation for this function for more information  
	// on how to register a custom string type, and other object types.
	// The implementation is in "/add_on/scriptstring/scriptstring.cpp"
	RegisterScriptString(engine);

	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Register the functions that the scripts will be allowed to use.
		// Note how the return code is validated with an assert(). This helps
		// us discover where a problem occurs, and doesn't pollute the code
		// with a lot of if's. If an error occurs in release mode it will
		// be caught when a script is being built, so it is not necessary
		// to do the verification here as well.
		r = engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
	}
	else
	{
		// Notice how the registration is almost identical to the above. 
		r = engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC); assert( r >= 0 );
	}


	// It is possible to register the functions, properties, and types in 
	// configuration groups as well. When compiling the scripts it then
	// be defined which configuration groups should be available for that
	// script. If necessary a configuration group can also be removed from
	// the engine, so that the engine configuration could be changed 
	// without having to recompile all the scripts.
}

int CompileScript(asIScriptEngine *engine)
{
	int r;
	ScriptLoader loader(engine);

	// Load the script from disk, and process #include directives
	r = loader.LoadScript("script.as");
	if( r < 0 ) return r;
	
	// Compile the script. If there are any compiler messages they will
	// be written to the message stream that we set right after creating the 
	// script engine. If there are no errors, and no warnings, nothing will
	// be written to the stream.
	r = engine->Build(0);
	if( r < 0 )
	{
		cout << "Build() failed" << endl;
		return -1;
	}

	// The engine doesn't keep a copy of the script sections after Build() has
	// returned. So if the script needs to be recompiled, then all the script
	// sections must be added again.

	// If we want to have several scripts executing at different times but 
	// that have no direct relation with each other, then we can compile them
	// into separate script modules. Each module use their own namespace and 
	// scope, so function names, and global variables will not conflict with
	// each other.

	return 0;
}

void LineCallback(asIScriptContext *ctx, DWORD *timeOut)
{
	// If the time out is reached we abort the script
	if( *timeOut < timeGetTime() )
		ctx->Abort();

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later 
	// time, by simply calling Execute() again.
}

// Function implementation with native calling convention
void PrintString(string &str)
{
	cout << str;
}

// Function implementation with generic script interface
void PrintString_Generic(asIScriptGeneric *gen)
{
	string *str = (string*)gen->GetArgAddress(0);
	cout << *str;
}


//---------------------------------------------
// Implementation for the script loader

ScriptLoader::ScriptLoader(asIScriptEngine *engine)
{
	this->engine = engine;
}

int ScriptLoader::LoadScript(const char *file)
{
	int r;

	// TODO: The file name stored in the set should be the fully resolved name because
	// it is possible to name the same file in multiple ways using relative paths.

	// Has the script been loaded already?
	string scriptFile = file;
	if( includedScripts.find(scriptFile) != includedScripts.end() )
	{
		// Already loaded 
		cout << "File '" << scriptFile << "' already included. Ignoring." << endl;
		return 0;
	}

	// Included the script as loaded
	cout << "Loading '" << scriptFile << "'." << endl;
	includedScripts.insert(scriptFile);

	// We will load the script from a file on the disk.
	FILE *f = fopen(file, "rb");
	if( f == 0 )
	{
		cout << "Failed to open the script file 'script.as'." << endl;
		return -1;
	}

	// Determine the size of the file	
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// On Win32 it is possible to do the following instead
	// int len = _filelength(_fileno(f));

	// Read the entire file
	string script;
	script.resize(len);
	int c =	fread(&script[0], len, 1, f);
	fclose(f);

	if( c == 0 ) 
	{
		cout << "Failed to load script file." << endl;
		return -1;
	}

	// Search the script file for #include directives
	r = ProcessInclude(script);
	if( r < 0 ) return r;

	// Add the script sections that will be compiled into executable code.
	// If we want to combine more than one file into the same script, then 
	// we can call AddScriptSection() several times for the same module and
	// the script engine will treat them all as if they were one. The script
	// section name, will allow us to localize any errors in the script code.

	// We'll tell AngelScript to make a copy of the script buffer, as we won't keep it in memory (last param).
	r = engine->AddScriptSection(0, file, &script[0], len, 0, true);
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}

	return 0;
}

int ScriptLoader::ProcessInclude(string &script)
{
	// Go through the script line by line, searching for #include directives
	for( int pos = 0; pos != script.npos && pos < script.length(); )
	{
		// Find the end of the line
		int end = script.find('\n', pos);
		if( end == script.npos ) 
		{
			// End of script
			end = script.length(); 
		}
		else 
		{
			// Don't include the newline character
			--end; 
		}
		
		// Skip white spaces
		int start = script.find_first_not_of(" \t", pos, 2);
		if( start == script.npos || start > end ) start = end;
		
		if( start < end && script[start] == '#' && (end-start) > 10 )
		{
			// Is it an #include directive?
			if( strncmp("#include", &script[start], 8) == 0 )
			{
				start += 8;
				
				// Skip white spaces
				start = script.find_first_not_of(" \t", start, 2);
				if( start == script.npos || start > end ) start = end;
				
				if( script[start] == '\"' )
				{
					++start;
					
					// Find second "
					int stop = script.find('\"', start);
					if( stop != script.npos && stop <= end )
					{
						string filename = script.substr(start, stop-start);
						
						// Load the included script (if not already loaded)
						LoadScript(filename.c_str());
						
						// Comment this line so that the compiler doesn't complain
						script[pos] = '/';
						script[pos+1] = '/';
					}
				}
			}
		}
		
		// Move to the next line
		pos = end + 2;
	}

	return 0;
}