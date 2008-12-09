#include "scriptfile.h"
#include "../scriptstring/scriptstring.h"
#include <new>
#include <assert.h>
#include <string>
#include <string.h>
#include <stdio.h>

using namespace std;

BEGIN_AS_NAMESPACE

CScriptFile *ScriptFile_Factory()
{
    return new CScriptFile();
}

void RegisterScriptFile(asIScriptEngine *engine)
{
    int r;

    r = engine->RegisterObjectType("file", 0, asOBJ_REF); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_FACTORY, "file @f()", asFUNCTION(ScriptFile_Factory), asCALL_CDECL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptFile,AddRef), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptFile,Release), asCALL_THISCALL); assert( r >= 0 );

    r = engine->RegisterObjectMethod("file", "int open(const string &in, const string &in)", asMETHOD(CScriptFile,Open), asCALL_THISCALL); assert( r >= 0 );
    r = engine->RegisterObjectMethod("file", "int close()", asMETHOD(CScriptFile,Close), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("file", "int getSize()", asMETHOD(CScriptFile,GetSize), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("file", "string @readString(uint)", asMETHOD(CScriptFile,ReadString), asCALL_THISCALL); assert( r >= 0 );
}

CScriptFile::CScriptFile()
{
    refCount = 1;
    file = 0;
}

CScriptFile::~CScriptFile()
{
    Close();
}

void CScriptFile::AddRef()
{
    ++refCount;
}

void CScriptFile::Release()
{
    if( --refCount == 0 )
        delete this;
}

int CScriptFile::Open(const std::string &filename, const std::string &mode)
{
    // Close the previously opened file handle
    if( file )
        Close();

    // Validate the mode (currently we only permit reading)
	string m;
    if( mode != "r" )
        return -1;
	else
		m = "r";
	
#ifdef WIN32
	// By default windows translates "\r\n" to "\n", but we want to read the file as-is.
	m += "b";
#endif

    // Open the file
    file = fopen(filename.c_str(), m.c_str());
    if( file == 0 )
        return -1;

    return 0;
}

int CScriptFile::Close()
{
    if( file == 0 )
        return -1;

    fclose(file);
    file = 0;

    return 0;
}

int CScriptFile::GetSize()
{
	if( file == 0 )
		return -1;

	int pos = ftell(file);
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, pos, SEEK_SET);

	return size;
}

CScriptString *CScriptFile::ReadString(unsigned int length)
{
	if( file == 0 )
		return 0;

	// Read the string
	string buf;
	buf.resize(length);
	int size = (int)fread(&buf[0], 1, length, file); 
	buf.resize(size);

	// Create the string object that will be returned
	CScriptString *str = new CScriptString();
	str->buffer.swap(buf);

	return str;
}


END_AS_NAMESPACE
