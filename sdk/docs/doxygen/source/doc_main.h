/**

\mainpage Manual

\image html aslogo.png 

AngelScript is an extremely flexible cross-platform scripting library designed to allow applications to extend 
their functionality through external scripts. It has been designed from the beginning to be an easy to use 
component, both for the application programmer and the script writer. 

Efforts have been made to let it call standard C functions and C++ methods with little to no need for proxy functions. 
The application simply registers the functions, objects, and methods that the scripts should be able to work 
with and nothing more has to be done with your code. The same functions used by the application internally can 
also be used by the scripting engine, which eliminates the need to duplicate functionality. 

The scripting language is very similar to the widely known syntax of C++, but without the need to worry about 
pointers and memory leaks. Contrary to most scripting languages that are weakly typed, AngelScript uses the 
common C/C++ datatypes for more efficient communication with the host application. 


\section main_topics Topics

 - \subpage doc_license
 - \subpage doc_start
 - \subpage doc_using
 - \subpage doc_advanced
 - \subpage doc_script
 - \subpage doc_api 
 - \subpage doc_samples
 - \subpage doc_addon








 
\page doc_start Getting started

 - \subpage doc_compile_lib
 - \subpage doc_good_practice
 
\todo Add a page explaining the structure of the engine, modules, contexts, etc
\todo Write a page "Your first script" that will go through the basics of executing a script






\page doc_using Using AngelScript

 - \subpage doc_register_func
 - \subpage doc_register_type
 - \subpage doc_obj_handle
 - \subpage doc_call_script_func
 - \subpage doc_generic
 - \subpage doc_gc

\todo Write a page about registering global properties
 
 
 
 
 
 
\page doc_advanced Advanced topics

 - \subpage doc_adv_var_type
 - \subpage doc_memory
 - \subpage doc_gc_object
 - \subpage doc_debug
 - \subpage doc_adv_c_dll
 - \subpage doc_adv_timeout
 - \subpage doc_adv_multithread
 - \subpage doc_adv_concurrent
 
\todo Add page about dynamic config groups
\todo Add page about pre-compiled bytecode
\todo Add page about custom memory management
\todo Add page about customizations/optimizations
\todo Add page about co-routines
\todo Add page about registering class hierarchies
 
 
 
 
\page doc_script The script language

This is the reference documentation for the AngelScript scripting language.

 - \subpage doc_global
 - \subpage doc_script_statements
 - \subpage doc_expressions
 - \subpage doc_datatypes
 - \subpage doc_operator_precedence
 - \subpage doc_reserved_keywords
 - \subpage doc_as_vs_cpp_types

\todo Add more comparison with C++



\page doc_api The API reference

This is the reference documentation for the AngelScript application programming interface.

 - \subpage doc_api_functions
 - \subpage doc_api_interfaces
 - \subpage doc_api_behaviours



*/
