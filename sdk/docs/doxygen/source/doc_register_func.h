/**

\page doc_register_func Registering a function

\todo introduction

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

\todo type differences

*/
