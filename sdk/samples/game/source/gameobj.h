#ifndef GAMEOBJ_H
#define GAMEOBJ_H

#include <string>
#include <angelscript.h>

class CGameObj
{
public:
	CGameObj(char dispChar, int x, int y);
	int AddRef();
	int Release();

	// This method is used by the application 
	// when the object should be destroyed
	void DestroyAndRelease();

	// This event handler is called by the game manager each frame
	void OnThink();

	bool Move(int dx, int dy);
	void Send(asIScriptObject *msg, CGameObj *other);
	void Kill();

	// The script shouldn't be allowed to update the position directly 
	// so we won't provide direct access to the position
	int GetX() const;
	int GetY() const;

	std::string name;
	char displayCharacter;
	bool isDead;
	asIScriptObject *controller;
	int x, y;

protected:
	~CGameObj();
	int refCount;
};

#endif