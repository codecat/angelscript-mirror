/**

\page doc_understanding_as Understanding AngelScript

In order to successfully use AngelScript it is important to 
understand some differences between AngelScript and C++. While
the library has been written so that it can be embedded within
the application with as little change to the application functions
and classes as possible, there are some care that must be taken
when passing objects between AngelScript and C++.

 - \subpage doc_versions
 - \subpage doc_module
 - \subpage doc_as_vs_cpp_types
 - \subpage doc_obj_handle
 - \subpage doc_memory
 - \subpage doc_typeid

\todo Add article on the calling conventions used by AngelScript

\todo Add article on sandboxing





\page doc_typeid Structure of the typeid

The typeid used in several functions of the AngelScript interface is a 32bit signed integer.
Negative values are not valid typeids, and functions returning a typeid can thus use negative
values to \ref asERetCodes "indicate errors".

The value contained in the typeid is composed to two parts, the lower bits indicated by \ref asTYPEID_MASK_SEQNBR
is a sequence that will increment with each new type registered by the application or declared in scripts. The
higher bits form a bit mask indicating the category of the type. The bits in the bit mask have 
the following meaning:

 - bit \ref asTYPEID_APPOBJECT : The type is an application registered object
 - bit \ref asTYPEID_SCRIPTOBJECT : The type is a script declared type, i.e. object can be cast to \ref asIScriptObject
 - bit \ref asTYPEID_TEMPLATE : The type is a template, and cannot be instantiated
 - bit \ref asTYPEID_HANDLETOCONST : The type is a handle to a const object, i.e. the referenced object must not be modified
 - bit \ref asTYPEID_OBJHANDLE : The type is a object handle, i.e. the value must be dereferenced to get the address of the actual object
 - bit 32: The typeid is invalid, and the number indicates an error from \ref asERetCodes

To identify if a typeid is to a primitive check that the bits in \ref asTYPEID_MASK_OBJECT are zero. This will be true
for all built-in primitive types and for enums.

The built-in primitive types have pre-defined ids as seen in the enum \ref asETypeIdFlags, and can be directly compared 
for their value, e.g. the typeid for a 32bit signed integer is \ref asTYPEID_INT32.

To get further information about the type, e.g. the exact object type, then use \ref asIScriptEngine::GetTypeInfoById which
will return a \ref asITypeInfo that can then be used to get all details about the type.




*/
