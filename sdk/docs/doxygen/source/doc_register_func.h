/**

\page doc_register_func Registering a function

This article aims to explain the way functions are registered with AngelScript, and some of the 
differences between C++ and AngelScript that the developer needs to be aware of in order to be 
successful in registering the application interface that the scripts will use. The principles
learned here are used in several locations, such as \ref asIScriptEngine::RegisterGlobalFunction 
"RegisterGlobalFunction", \ref asIScriptEngine::RegisterObjectMethod "RegisterObjectMethod", \ref
asIScriptEngine::RegisterObjectBehaviour "RegisterObjectBehaviour", \ref asIScriptEngine::RegisterGlobalBehaviour 
"RegisterGlobalBehaviour", etc.

\section doc_register_func_1 How to get the address of the application function or method

The macros \ref asFUNCTION, \ref asFUNCTIONPR, \ref asMETHOD, and \ref asMETHODPR 
have been implemented to facilitate the task of getting the function pointer and 
passing them on to the script engine.

The asFUNCTION takes the function name as the parameter, which works for all global 
functions that do not have any overloads. If you use overloads, i.e. multiple functions
with the same name but with different parameters, then you need to use asFUNCTIONPR instead.
This macro takes as parameter the function name, parameter list, and return type, so that
the C++ compiler can resolve exactly which overloaded function to take the address of.

\code
// Global function
void globalFunc();
r = engine->RegisterGlobalFunction("void globalFunc()", asFUNCTION(globalFunc), asCALL_CDECL); assert( r >= 0 );

// Overloaded global functions
void globalFunc2(int);
void globalFunc2(float);
r = engine->RegisterGlobalFunction("void globalFunc2(int)", asFUNCTIONPR(globalFunc2, (int), void), asCALL_CDECL); assert( r >= 0 );
\endcode

The same goes for asMETHOD and asMETHODPR. The difference between these and asFUNCTION/asFUNCTIONPR
is that the former take the class name as well as parameter.

\code
class Object
{
  // Class method
  void method();
  
  // Overloaded assignment operator
  Object &operator=(const Object &);
  Object &operator=(int);
};

// Registering the class method
r = engine->RegisterObjectMethod("object", "void method()", asMETHOD(Object,method), asCALL_THISCALL); assert( r >= 0 );

// Registering the assignment behaviours
r = engine->RegisterObjectBehaviour("object", asBEHAVE_ASSIGNMENT, "object &f(const object &in)", asMETHODPR(Object, operator=, (const Object&), Object&), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("object", asBEHAVE_ASSIGNMENT, "object &f(int)", asMETHODPR(Object, operator=, (int), Object&), asCALL_THISCALL); assert( r >= 0 );
\endcode




\section doc_register_func_2 Calling convention

AngelScript accepts most common calling conventions that C++ uses, i.e. cdecl, stdcall, and thiscall. There is also a 
generic calling convention that can be used for example when native calling conventions are not supported on the target platform.

All functions and behaviours must be registered with the \ref asCALL_CDECL, \ref asCALL_STDCALL, \ref asCALL_THISCALL, or 
\ref asCALL_GENERIC flags to tell AngelScript which calling convention the application function uses. The special conventions 
\ref asCALL_CDECL_OBJLAST and \ref asCALL_CDECL_OBJFIRST can also be used wherever asCALL_THISCALL is accepted, in order to 
simulate a class method through a global function. If the incorrect calling convention is given on the registration you'll very 
likely see the application crash with a stack corruption whenever the script engine calls the function.

cdecl is the default calling convention for all global functions in C++ programs, so if in doubt try with asCALL_CDECL first. 
The calling convention only differs from cdecl if the function is explicitly declared to use a different convention, or if you've
set the compiler options to default to another convention.

For class methods there is only the thiscall convention, except when the method is static, as those methods are in truth global
functions in the class namespace.

\see \ref doc_generic




\section doc_register_func_3 A little on type differences

AngelScript supports most of the same types that C++ has, but there are differences that you'll need to know when registering
functions, methods, and behaviours.

All primitive types in C++ have a corresponding type in AngelScript, though sometimes with a slightly different name, i.e.
<code>char</code> in C++ is <code>int8</code> in AngelScript. You can see a list of all types and their match in respective 
language \ref doc_as_vs_cpp_types "here".

Pointers do not exist in AngelScript in the same way as in C++, so you need to decide on how they should be passed. For this you
have two options, either as reference, or as an \ref doc_obj_handle "object handle". Most common uses of pointers in parameters
can be represented with either references or object handles in AngelScript, for the few uses where it cannot be done a wrapper 
function must be written to simplify the function interface to a form that AngelScript can understand.

Parameter references in AngelScript have an additional restriction over the C++ references, and that is that you must specify
the intended direction of the value that the reference points to, i.e. whether it is an input value, output value, or if the value
is both input and output. This is done by adding the keywords <code>in</code>, <code>out</code>, or <code>inout</code> after the 
& character. If no keyword is given AngelScript assumes <code>inout</code>. Value types can only use <code>in</code> and <code>out</code>,
as AngelScript cannot guarantee the safety of the references otherwise. 

Object handles are reference counted pointers to objects, so when using these you need to pay attention to the reference counter,
e.g. whenever you receive an object handle from AngelScript, you must make sure to decrease the reference when you're done with it. 
Similarly whenever you pass an object handle to AngelScript you must make sure that reference is accounted for, so that AngelScript
doesn't destroy the object too early. If your application functions are not already prepared to work like this, you can most of the 
time tell AngelScript to handle the reference counting for you by using the auto-handles, <code>\@+</code>.

Strings are a bit complicated as C++ doesn't have one standard way of dealing with them. Because of that AngelScript also doesn't
impose a specific string type on the applications. Instead the application needs to register the string type it wants to use, and
then the string parameters needs to be registered accordingly. AngelScript comes with a standard add-on for a string type, which 
for the most part is compatible with std::string, except that it is reference counted.

\see \ref doc_obj_handle, \ref doc_datatypes, \ref doc_addon_string

*/
