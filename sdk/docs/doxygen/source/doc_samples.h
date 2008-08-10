/**

\page samples Samples


This page gives a brief description of the samples that you'll find in the /sdk/samples/ folder.

 - \ref tutorial
 - \ref concurrent
 - \ref console
 - \ref corout
 - \ref events
 - \ref incl



\section tutorial Tutorial

This sample was written with the intention of explaining the basics of 
AngelScript, that is, how to configure the engine, load and compile a script, 
and finally execute a script function with parameters and return value.


 - LineCallback() function which aborts execution when the time is up
 - Strings
 - Registered global functions
 - Script function parameters and return value
 - Retrieving information about script exceptions
 - asIScriptGeneric for when the library doesn't support native calling convention






\section concurrent Concurrent scripts

This sample shows how to execute two or more long running scripts in 
parallel. The scripts voluntarily hand over the control to the next script in 
the queue by calling the function Sleep().


 - Multiple scripts running in parallel
 - Sleep()
 - Strings
 - Registered global functions




\section console Console

This sample implements a simple console, which lets the user type in 
commands and also evaluate simple script statements.

 - ExecuteString()
 - Strings
 - Registered global functions
 - Registered global properties
 - Special function _grab() with overloads to receive and print resulting value from script statements



\section corout Co-routines

This sample shows how co-routines can be implemented with AngelScript. Co-routines are
threads that can be created from the scripts, and that work together by voluntarily passing control
to each other by calling Yield().

 - Co-routines created from the scripts with variable parameter structure.
 - Strings
 - Registered global functions
 - Handling the variable argument type
 - Passing arguments to script functions


\section events Events

This sample has the script engine execute a long running script. The script execution is regularly interrupted by the application so that keyboard events can be processed, which execute another short script before resuming the execution of the main script. The event handling scripts change the state of the long running script.

 - LineCallback() function which suspends execution when the time is up
 - Strings
 - Registered global functions
 - Scripted event handlers



\section incl Implementing the include directive

This sample shows how to implement a very simple preprocessor to add support for the #%include directive, which allow the script writer to reuse common script code. The preprocessor simply adds the included scripts as multiple script sections, which is ok as AngelScript is able to resolve global declarations independently of their order. The preprocessor also makes sure that a script file is only included once, so the script writer doesn't have to take extra care to avoid multiple includes or even complicated circular includes.

 - LineCallback() functions which aborts execution when the time is up
 - Processing the #%include directive
 - Circular #%includes are resolved automatically



*/