#include <iostream>  // cout
#include <angelscript.h>
#ifdef _MSC_VER
	#include <crtdbg.h>  // debugging routines
#endif

#include "gamemgr.h"
#include "scriptmgr.h"

using namespace std;

CScriptMgr *scriptMgr = 0;
CGameMgr   *gameMgr   = 0;

int main(int argc, char **argv)
{
#ifdef _MSC_VER
	// Detect memory leaks
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);

	// Use _CrtSetBreakAlloc(n) to find a specific memory leak
#endif

	int r;

	// Initialize the game manager
	gameMgr = new CGameMgr();

	// Initialize the script manager
	scriptMgr = new CScriptMgr();
	r = scriptMgr->Init();
	if( r < 0 )
	{
		delete scriptMgr;
		delete gameMgr;
		return r;
	}

	// Start a new game
	r = gameMgr->StartGame();
	if( r >= 0 )
	{
		// Let the game manager handle the game loop
		gameMgr->Run();
	}
	
	// Uninitialize the game manager
	if( gameMgr )
	{
		delete gameMgr;
		gameMgr = 0;
	}

	// Uninitialize the script manager
	if( scriptMgr )
	{
		delete scriptMgr;
		scriptMgr = 0;
	}

	return r;
}



