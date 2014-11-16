#include "scriptfilesystem.h"

#if defined(_WIN32)
#include <direct.h> // _getcwd
#include <Windows.h> // FindFirstFile, GetFileAttributes
#else
#include <unistd.h> // getcwd
#endif
#include <assert.h> // assert

using namespace std;

BEGIN_AS_NAMESPACE

CScriptFileSystem *ScriptFileSystem_Factory()
{
	return new CScriptFileSystem();
}

void RegisterScriptFileSystem_Native(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectType("filesystem", 0, asOBJ_REF); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("filesystem", asBEHAVE_FACTORY, "filesystem @f()", asFUNCTION(ScriptFileSystem_Factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("filesystem", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptFileSystem,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("filesystem", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptFileSystem,Release), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("filesystem", "bool changeCurrentPath(const string &in)", asMETHOD(CScriptFileSystem, ChangeCurrentPath), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("filesystem", "string getCurrentPath() const", asMETHOD(CScriptFileSystem, GetCurrentPath), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("filesystem", "array<string> @getMatchingDirs(const string &in)", asMETHOD(CScriptFileSystem, GetMatchingDirs), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("filesystem", "array<string> @getMatchingFiles(const string &in)", asMETHOD(CScriptFileSystem, GetMatchingFiles), asCALL_THISCALL); assert( r >= 0 );
}

void RegisterScriptFileSystem(asIScriptEngine *engine)
{
//	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
//		RegisterScriptFileSystem_Generic(engine);
//	else
		RegisterScriptFileSystem_Native(engine);
}

CScriptFileSystem::CScriptFileSystem()
{
	refCount = 1;

	// Gets the application's current working directory as the starting point
	char buffer[1000];
#if defined(_WIN32)
	currentPath = _getcwd(buffer, 1000);
#else
	currentPath = getcwd(buffer, 1000);
#endif
}

CScriptFileSystem::~CScriptFileSystem()
{
}

void CScriptFileSystem::AddRef() const
{
	asAtomicInc(refCount);
}

void CScriptFileSystem::Release() const
{
	if( asAtomicDec(refCount) == 0 )
		delete this;
}

CScriptArray *CScriptFileSystem::GetMatchingFiles(const string &pattern) const
{
	// Obtain a pointer to the engine
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();

	// TODO: This should only be done once
	// TODO: This assumes that CScriptArray was already registered
	asIObjectType *arrayType = engine->GetObjectTypeByDecl("array<string>");

	// Create the array object
	CScriptArray *array = CScriptArray::Create(arrayType);

	// pattern can include wildcards * and ?
	string searchPattern = currentPath + "/" + pattern;

#if defined(_WIN32)
	// Windows uses UTF16 so it is necessary to convert the string
	wchar_t bufUTF16[10000];
	MultiByteToWideChar(CP_UTF8, 0, searchPattern.c_str(), -1, bufUTF16, 10000);

	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(bufUTF16, &ffd);
	if( INVALID_HANDLE_VALUE == hFind ) 
		return array;
	
	do
	{
		// Skip directories
		if( (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			continue;

		// Convert the file name back to UTF8
		char bufUTF8[10000];
		WideCharToMultiByte(CP_UTF8, 0, ffd.cFileName, -1, bufUTF8, 10000, 0, 0);
		
		// Add the file to the array
		array->Resize(array->GetSize()+1);
		((string*)(array->At(array->GetSize()-1)))->assign(bufUTF8);
	}
	while( FindNextFileW(hFind, &ffd) != 0 );

	FindClose(hFind);
#else
	// TODO: implement this for Linux etc
#endif

	return array;
}

CScriptArray *CScriptFileSystem::GetMatchingDirs(const string &pattern) const
{
	// Obtain a pointer to the engine
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();

	// TODO: This should only be done once
	// TODO: This assumes that CScriptArray was already registered
	asIObjectType *arrayType = engine->GetObjectTypeByDecl("array<string>");

	// Create the array object
	CScriptArray *array = CScriptArray::Create(arrayType);

	// pattern can include wildcards * and ?
	string searchPattern = currentPath + "/" + pattern;

#if defined(_WIN32)
	// Windows uses UTF16 so it is necessary to convert the string
	wchar_t bufUTF16[10000];
	MultiByteToWideChar(CP_UTF8, 0, searchPattern.c_str(), -1, bufUTF16, 10000);
	
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = FindFirstFileW(bufUTF16, &ffd);
	if( INVALID_HANDLE_VALUE == hFind ) 
		return array;
	
	do
	{
		// Skip files
		if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			continue;

		// Convert the file name back to UTF8
		char bufUTF8[10000];
		WideCharToMultiByte(CP_UTF8, 0, ffd.cFileName, -1, bufUTF8, 10000, 0, 0);

		if( strcmp(bufUTF8, ".") == 0 || strcmp(bufUTF8, "..") == 0 )
			continue;
		
		// Add the dir to the array
		array->Resize(array->GetSize()+1);
		((string*)(array->At(array->GetSize()-1)))->assign(bufUTF8);
	}
	while( FindNextFileW(hFind, &ffd) != 0 );

	FindClose(hFind);
#else
	// TODO: Implement this for Linux etc
#endif

	return array;
}

bool CScriptFileSystem::ChangeCurrentPath(const string &path)
{
	if( path.find(":") != string::npos || path.find("/") == 0 || path.find("\\") == 0 )
		currentPath = path;
	else
		currentPath += "/" + path;

	// Remove trailing slashes from the path
	while( currentPath.length() && (currentPath.back() == '/' || currentPath.back() == '\\') )
		currentPath.resize(currentPath.length()-1);

#if defined(_WIN32)
	// Windows uses UTF16 so it is necessary to convert the string
	wchar_t bufUTF16[10000];
	MultiByteToWideChar(CP_UTF8, 0, currentPath.c_str(), -1, bufUTF16, 10000);

	// Check if the path exists
	DWORD attrib = GetFileAttributesW(bufUTF16);
	if( attrib == INVALID_FILE_ATTRIBUTES ||
		!(attrib & FILE_ATTRIBUTE_DIRECTORY) )
		return false;
#else
	// TODO: Implement this for Linux etc
#endif
		
	return true;
}

string CScriptFileSystem::GetCurrentPath() const
{
	return currentPath;
}


END_AS_NAMESPACE
