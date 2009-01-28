//
// CScriptFile
//
// This class encapsulates a FILE pointer in a reference counted class for
// use within AngelScript.
//
// It requires the CScriptString add-on to work.
//

#ifndef SCRIPTFILE_H
#define SCRIPTFILE_H

#include <angelscript.h>
#include <string>

BEGIN_AS_NAMESPACE

class CScriptString;

class CScriptFile
{
public:
    CScriptFile();

    void AddRef();
    void Release();

	// TODO: Implement the "w" and "a" modes
	// mode = "r" -> open the file for reading
    int  Open(const std::string &filename, const std::string &mode);
    int  Close();
    int  GetSize();
	bool IsEOF();

    // Reading
    CScriptString *ReadString(unsigned int length);
	CScriptString *ReadLine();

	// TODO: Add the following functions

    // Writing
/*  int WriteString(const std::string &str);

    // Cursor
	int GetPos();
    int SetPos(int pos);
    int MovePos(int delta); */

protected:
    ~CScriptFile();

    int   refCount;
    FILE *file;
};

// This function will determine the configuration of the engine
// and use one of the two functions below to register the file type
void RegisterScriptFile(asIScriptEngine *engine);

// Call this function to register the file type
// using native calling conventions
void RegisterScriptFile_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptFile_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
