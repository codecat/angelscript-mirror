/**

\page doc_addon Add-ons

This page gives a brief description of the add-ons that you'll find in the /sdk/add_on/ folder.

 - \subpage doc_addon_any
 - \subpage doc_addon_string
 - \subpage doc_addon_dict
 - \subpage doc_addon_file
 - \subpage doc_addon_math
 - \subpage doc_addon_math3d
 - \subpage doc_addon_build
 - \subpage doc_addon_clib



\page doc_addon_any any object

<b>Path:</b> /sdk/add_on/scriptany/

The <code>any</code> type is a generic container that can hold any value. It is a reference type.

The type is registered with <code>RegisterScriptAny(asIScriptEngine*);</code>.

In the scripts it can be used as follows:

<pre>
  int value;
  obj object;
  obj \@handle;
  any a,b,c;
  a.store(value);      // store the value
  b.store(\@handle);    // store an object handle
  c.store(object);     // store a copy of the object
  
  a.retrieve(value);   // retrieve the value
  b.retrieve(\@handle); // retrieve the object handle
  c.retrieve(object);  // retrieve a copy of the object
</pre>

In C++ the type can be used as follows:

\code
CScriptAny *myAny;
int typeId = engine->GetTypeIdByDecl(0, "string@");
asCScriptString *str = new asCScriptString("hello world");
myAny->Store((void*)&str, typeId);
myAny->Retrieve((void*)&str, typeId);
\endcode

\todo Expand the documentation for script any



\page doc_addon_string string object 

<b>Path:</b> /sdk/add_on/scriptstring/

This add-on registers a string type that is in most situations compatible with the 
std::string, except that it uses reference counting. This means that if you have an
application function that takes a std::string by reference, you can register it 
with AngelScript to take a script string by reference. This works because the asCScriptString
wraps the std::string type, with the std::string type at the first byte of the asCScriptString
object.

Register the type with <code>RegisterScriptString(asIScriptEngine*)</code>. Register the 
utility functions with <code>RegisterScriptStringUtils(asIScriptEngine*)</code>.

Utility functions:

 - string@    substring(const string &in, int, int)
 - int        findFirst(const string &in, const string &in)
 - int        findFirst(const string &in, const string &in, int)
 - int        findLast(const string &in, const string &in)
 - int        findLast(const string &in, const string &in, int)
 - int        findFirstOf(const string &in, const string &in)
 - int        findFirstOf(const string &in, const string &in, int) 
 - int        findFirstNotOf(const string &in, const string &in)
 - int        findFirstNotOf(const string &in, const string &in, int)
 - int        findLastOf(const string &in, const string &in)
 - int        findLastOf(const string &in, const string &in, int)
 - int        findLastNotOf(const string &in, const string &in)
 - int        findLastNotOf(const string &in, const string &in, int)
 - string@[]@ split(const string &in, const string &in)
 - string@    join(const string@[] &in, const string &in)
 
\todo Expand the documentation for script string




\page doc_addon_dict dictionary object 

<b>Path:</b> /sdk/add_on/scriptdictionary/

The dictionary object maps string values to values or objects of other types. 

Register with <code>RegisterScriptDictionary(asIScriptEngine*)</code>.

Script example:

<pre>
  dictionary dict;
  obj object;
  obj \@handle;
  
  dict.set("one", 1);
  dict.set("object", object);
  dict.set("handle", \@handle);
  
  if( dict.exists("one") )
  {
    bool found = dict.get("handle", \@handle);
    if( found )
    {
      dict.delete("object");
    }
  }
  
  dict.deleteAll();
</pre>

\todo Expand the documentation for the dictionary object



\page doc_addon_file file object 

<b>Path:</b> /sdk/add_on/scriptfile/

This is currently only a basic foundation for a file object that will allow scripts to read and write files.

Register with <code>RegisterScriptFile(asIScriptEngine*)</code>.

Script example:

<pre>
  file f;
  // Open the file in 'read' mode
  if( f.open("file.txt", "r") >= 0 ) 
  {
      // Read the whole file into the string buffer
      string \@str = \@f.readString(f.getSize()); 
      f.close();
  }
</pre>




\page doc_addon_math math functions

<b>Path:</b> /sdk/add_on/scriptmath/

This add-on registers the math functions from the standard C runtime library with the script 
engine. Use <code>RegisterScriptMath(asIScriptEngine*)</code> to perform the registration.

The following functions are registered:

  - float cos(float)
  - float sin(float)
  - float tan(float)
  - float acos(float)
  - float asin(float)
  - float atan(float)
  - float cosh(float)
  - float sinh(float)
  - float tanh(float)
  - float log(float)
  - float log10(float)
  - float pow(float, float)
  - float sqrt(float)
  - float ceil(float)
  - float abs(float)
  - float floor(float)
  - float fraction(float)
 
 
 
 
\page doc_addon_math3d 3D math functions

<b>Path:</b> /sdk/add_on/scriptmath3d/

This add-on registers some value types and functions that permit the scripts to perform 
3D mathematical operations. Use <code>RegisterScriptMath3D(asIScriptEngine*)</code> to
perform the registration.

Currently the only thing registered is the <code>vector3</code> type, representing a 3D vector, with
basic math operators, such as add, subtract, scalar multiply, equality comparison, etc.






\page doc_addon_build Script builder helper

<b>Path:</b> /sdk/add_on/scriptbuilder/

This class is a helper class for loading and building scripts, with a basic pre-processor 
that supports include directives and metadata declarations.

\section doc_addon_build_1 Public interface

\code
class CScriptBuilder
{
public:
  // Load and build a script file from disk
  int BuildScriptFromFile(asIScriptEngine *engine, 
                          const char      *module, 
                          const char      *filename);

  // Build a script file from a memory buffer
  int BuildScriptFromMemory(asIScriptEngine *engine, 
                            const char      *module, 
                            const char      *script, 
                            const char      *sectionname = "");

  // Get metadata declared for class types and interfaces
  const char *GetMetadataStringForType(int typeId);

  // Get metadata declared for functions
  const char *GetMetadataStringForFunc(int funcId);

  // Get metadata declared for global variables
  const char *GetMetadataStringForVar(int varIdx);
};
\endcode

\section doc_addon_build_2 Include directives

Example script with include directive:

<pre>
  \#include "commonfuncs.as"
  
  void main()
  {
    // Call a function from the included file
    CommonFunc();
  }
</pre>


\section doc_addon_build_metadata Metadata in scripts

Metadata can be added before script class, interface, function, and global variable 
declarations. The metadata is removed from the script by the builder class and stored
for post build lookup by the type id, function id, or variable index.

Exactly what the metadata looks like is up to the application. The builder class doesn't
impose any rules, except that the metadata should be added between brackets []. After 
the script has been built the application can obtain the metadata strings and interpret
them as it sees fit.

Example script with metadata:

<pre>
  [factory func = CreateOgre,
   editable: myPosition,
   editable: myStrength [10, 100]]
  class COgre
  {
    vector3 myPosition;
    int     myStrength;
  }
  
  [factory]
  COgre \@CreateOgre()
  {
    return \@COgre();
  }
</pre>

Example usage:

\code
CScriptBuilder builder;
int r = builder.BuildScriptFromMemory(engine, "my module", script);
if( r >= 0 )
{
  // Find global variables that have been marked as editable by user
  int count = engine->GetGlobalVarCount("my module");
  for( int n = 0; n < count; n++ )
  {
    string metadata = builder.GetMetadataStringForVar(n);
    if( metadata == "editable" )
    {
      // Show the global variable in a GUI
      ...
    }
  }
}
\endcode







\page doc_addon_clib C library

<b>Path:</b> /sdk/add_on/clib/

This add-on defines a pure C interface, that can be used in those applications that do not
understand C++ code but do understand C. 

To compile the AngelScript C library, you first need to compile the ordinary AngelScript library 
with the pre-processor word <code>AS_C_LIBRARY</code> defined. Then you compile the AngelScript C library, 
linking with the ordinary AngelScript library.

In the application that will use the AngelScript C library, you'll include the <code>angelscript_c.h</code>
header file, instead of the ordinary <code>%angelscript.h</code> header file. After that you can use the library
much the same way that it's used in C++. 

To find the name of the C functions to call, you normally take the corresponding interface method
and give a prefix according to the following table:

<table border=0 cellspacing=0 cellpadding=0>
<tr><td><b>interface      &nbsp;</b></td><td><b>prefix&nbsp;</b></td></tr>
<tr><td>asIScriptEngine   &nbsp;</td>    <td>asEngine_</td></tr>
<tr><td>asIScriptContext  &nbsp;</td>    <td>asContext_</td></tr>
<tr><td>asIScriptGeneric  &nbsp;</td>    <td>asGeneric_</td></tr>
<tr><td>asIScriptArray    &nbsp;</td>    <td>asArray_</td></tr>
<tr><td>asIObjectType     &nbsp;</td>    <td>asObjectType_</td></tr>
<tr><td>asIScriptFunction &nbsp;</td>    <td>asScriptFunction_</td></tr>
</table>

All interface methods take the interface pointer as the first parameter when in the C function format, the rest
of the parameters are the same as in the C++ interface. There are a few exceptions though, e.g. all parameters that
take an <code>asSFuncPtr</code> take a normal function pointer in the C function format. 

Example:

\code
#include <stdio.h>
#include <assert.h>
#include "angelscript_c.h"

void MessageCallback(asSMessageInfo *msg, void *);
void PrintSomething();

int main(int argc, char **argv)
{
  int r = 0;

  // Create and initialize the script engine
  asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
  r = asEngine_SetMessageCallback(engine, (asFUNCTION_t)MessageCallback, 0, asCALL_CDECL); assert( r >= 0 );
  r = asEngine_RegisterGlobalFunction(engine, "void print()", (asFUNCTION_t)PrintSomething, asCALL_CDECL); assert( r >= 0 );

  // Execute a simple script
  r = asEngine_ExecuteString(engine, 0, "print()", 0, 0);
  if( r != asEXECUTION_FINISHED )
  {
      printf("Something wen't wrong with the execution\n");
  }
  else
  {
      printf("The script was executed successfully\n");
  }

  // Release the script engine
  asEngine_Release(engine);
  
  return r;
}

void MessageCallback(asSMessageInfo *msg, void *)
{
  const char *msgType = 0;
  if( msg->type == 0 ) msgType = "Error  ";
  if( msg->type == 1 ) msgType = "Warning";
  if( msg->type == 2 ) msgType = "Info   ";

  printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
}

void PrintSomething()
{
  printf("Called from the script\n");
}
\endcode

*/  
