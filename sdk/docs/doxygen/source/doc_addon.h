/**

\page doc_addon Add-ons

This page gives a brief description of the add-ons that you'll find in the /sdk/add_on/ folder.

 - \subpage doc_addon_any
 - \subpage doc_addon_string
 - \subpage doc_addon_dict
 - \subpage doc_addon_file
 - \subpage doc_addon_math



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
    
*/  
