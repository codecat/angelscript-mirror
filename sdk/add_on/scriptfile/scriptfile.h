//
// asCScriptFile
//
// This class encapsulates a FILE pointer in a reference counted class for
// use within AngelScript.
//
// It requires the asCScriptString add-on to work.
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

	// mode = "r" -> open the file for reading
    int Open(const std::string &filename, const std::string &mode);
    int Close();
    int GetSize();

    // Reading
    CScriptString *ReadString(unsigned int length);

	// TODO: Add the following functions
/*  bool             IsEOF();
    asCScriptString *ReadStringUntilOneOf(const std::string &chars);
    asINT8           ReadInt8();
    asINT16          ReadInt16();
    asINT32          ReadInt32();
    asINT64          ReadInt64();
    float            ReadFloat();
    double           ReadDouble();*/

    // Writing
/*  int WriteString(const std::string &str);
    int WriteInt8(asINT8 i);
    int WriteInt16(asINT16 i);
    int WriteInt32(asINT32 i);
    int WriteInt64(asINT64 i);
    int WriteFloat(float f);
    int WriteDouble(double d);

    // Cursor
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
