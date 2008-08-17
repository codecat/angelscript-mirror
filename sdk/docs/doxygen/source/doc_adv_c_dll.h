/**

\page doc_adv_c_dll Compiling an ANSI C binary compatible DLL of AngelScript

AngelScript is written with C++, and unfortunately C++ compilers were
never standardized on the binary level, so code compiled by one C++ compiler
is usually not compatible with code compiled from another C++ compiler.

Fortunately there is a way to make a binary compatible DLL of AngelScript.
Because of the large base of ANSI C resources readily available all C++ 
compilers make sure to produce compatible C code. Even other languages, such
as Delphi and D, are able to use an ANSI C compatible dll. AngelScript has a 
prepared C compatible interface that can be used for this purpose.

To compile the dll and expose the C interface some preprocessor flags has 
to be turned on: <code>AS_C_INTERFACE</code> and <code>ANGELSCRIPT_EXPORT</code>. 
Obviously it will not be possible to register C++ class methods through this 
interface, so it might be useful to remove that part from the engine by 
defining the flag <code>AS_NO_CLASS_METHODS</code>.

These flags and others are documented in the as_config.h header file.






*/
