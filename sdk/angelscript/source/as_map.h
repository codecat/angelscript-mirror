/*
   AngelCode Scripting Library
   Copyright (c) 2003-2004 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
	  this software in a product, an acknowledgment in the product 
	  documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


//
// as_map.h
//
// This class is used for mapping a value to another
//


#ifndef AS_MAP_H
#define AS_MAP_H

typedef unsigned int KEY;
typedef unsigned int VALUE;

struct asSMapNode;

class asCMap
{
public:
	asCMap();
	~asCMap();

	int   Insert(KEY key, VALUE value);
	int   GetCount();
	
	KEY   GetKey();
	VALUE GetValue();

	// Returns true as long as cursor is valid

	bool Erase(bool moveNext = true);
	bool IsValidCursor();
	bool MoveTo(KEY key);
	bool MoveFirst();
	bool MoveLast();
	bool MoveNext();
	bool MovePrev();

	// For debugging only

	int CheckIntegrity(asSMapNode *node);

protected:
	void BalanceInsert(asSMapNode *node);
	void BalanceErase(asSMapNode *child, asSMapNode *parent);

	int EraseAll(asSMapNode *node);
	int RotateLeft(asSMapNode *node);
	int RotateRight(asSMapNode *node);

	asSMapNode *root;
	asSMapNode *cursor;

	int count;
};

#endif

