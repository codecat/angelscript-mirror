/**

\page doc_serialization Serialization

To serialize scripts the application needs to be able to iterate through all variables, and objects that have any relation to the 
script and store their content in a way that would allow it to be restored at a later time. For simple values this is trivial, but 
for objects and handles it becomes more complex as it is necessary to keep track of references between objects and internal structure 
of both classes defined by scripts and application registered types.

Iterating through the variables and members of objects is already described in the article about \ref doc_adv_reflection "reflection", 
so this article will focus on the storing and restoring of values.

Look at the source code for the \ref doc_addon_serializer add-on for a sample implementation of serialization.

\see \ref doc_adv_dynamic_build_hot

\section doc_serialization_modules Serialization of modules

To serialize the script modules you'll need to enumerate all the global variables and \ref doc_serialization_vars "serialize each of them".

When deserializing the module, first compile it normally either from \ref doc_compile_script "source script" or 
by \ref doc_adv_precompile "loading a pre-compiled byte code", except you first need to turn off initialization of 
global variables by \ref asIScriptEngine::SetEngineProperty "turning off the engine property asEP_INIT_GLOBAL_VARS_AFTER_BUILD".

\section doc_serialization_vars Serialization of global variables

To serialize a global variable you'll need the name and namespace to use as the key for the variable, and then the type id and address 
of the variable. You get these with the methods \ref asIScriptModule::GetGlobalVar and \ref asIScriptModule::GetAddressOfGlobalVar. If 
the type id is for a primitive type, then the value can be stored as is. If it is a handle or reference you'll need to serialize the 
reference itself and the object it refers to. If the type id is an object type then \ref doc_serialization_objects "serialize the object" 
and its content.

To deserialize the global variable, you'll use the name and namespace to look it up, and then use GetAddressOfGlobalVar to get the 
address of the memory that needs to be restored with the serialized content.

\section doc_serialization_objects Serialization of objects

\todo Refer to reflection for serialization. Mention CreateUnitializedScriptObject for deserialization.

\section doc_serialization_contexts Serialization of contexts

\todo Give overview of the steps to serialize contexts.




*/
