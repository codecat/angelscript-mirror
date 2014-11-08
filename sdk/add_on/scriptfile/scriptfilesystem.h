#ifndef AS_SCRIPTFILESYSTEM_H
#define AS_SCRIPTFILESYSTEM_H

#ifndef ANGELSCRIPT_H 
// Avoid having to inform include path if header is already include before
#include <angelscript.h>
#endif

#include <string>
#include <stdio.h>

#include "../scriptarray/scriptarray.h"

BEGIN_AS_NAMESPACE

class CScriptFileSystem
{
public:
    CScriptFileSystem();

    void AddRef() const;
    void Release() const;

	// Sets the current path that should be used in other calls when using relative paths
	// It can use relative paths too, so moving up a directory is used by passing in ".."
	bool ChangeCurrentPath(const std::string &path);
	std::string GetCurrentPath() const;

	// Determines the relative path compared to the current path
//	std::string GetRelativePathTo(const std::string &path) const;

//	bool DirectoryExists(const std::string &path) const;
//	bool IsDirectory(const std::string &path) const;

//	bool   FileExists(const std::string &path) const;
//	asUINT GetSizeOfFile(const std::string &path) const;
//	asUINT ComputeCRC32(const std::string &path) const;

	// Returns a list of the matching files
	// Pattern can include ? to match a single character, and * to match multiple characters
	CScriptArray *GetMatchingFiles(const std::string &pattern) const;

protected:
    ~CScriptFileSystem();

    mutable int refCount;
    std::string currentPath;
};

void RegisterScriptFileSystem(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
