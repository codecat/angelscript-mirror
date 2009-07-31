/**

\page doc_adv_template Registering template types

A template type in AngelScript works similarly to how templates work in C++. The scripts 
will be able to instanciate different forms of the template type by specifying which subtype 
that should be used. The methods for the instance will then be adapted to this subtype, so
that the correct handling of parameters and return types will be applied.

The implementation of the template type is however not a C++ template, instead it must 
be implemented as a generic class that can determine what to do dynamically at runtime based
on the subtype for which it was instanciated. This is obviously a lot less efficient than
having specific implementations for each type, and for that reason AngelScript permits the
application to register a template specialization where the extra performance is needed. 

This gives the best of both worlds, performance where the subtype is known before hand, and 
support for all other types that cannot be pre-determined.

\section doc_adv_template_1 Registering the template type

The template registration is registered similarly to normal \ref doc_reg_basicref "reference types",
with a few differences. The name of the type is formed by the name of the template type plus
the name of the subtype with angle brackets. The type flag asOBJ_TEMPLATE must also be used.

\code
  // Register the template type
  r = engine->RegisterObjectType("myTemplate<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); assert( r >= 0 );
\endcode

The template type doesn't have to be \ref doc_gc_object "garbage collected", but since you may not know 
which subtypes it will be instanciated for, it is usually best to implement that support.

When registering the behaviours, methods, and properties for the template type the type is identified
with the name and subtype within angle brackets, but without the class token, e.g. <tt>myTemplate&lt;T&gt;</tt>.
The sub type is identified by just the name of the subtype as it was declared in the call to RegisterObjectType.

The factory behaviour for the template type is also different. In order for the implementation to know
which subtype it is instanciated for the factory receives the \ref asIObjectType of the template instance
as a hidden first parameter. When registering the factory this hidden parameter is reflected in the declaration,
for example as <tt>int &amp;in</tt>.

\code
  // Register the factory behaviour
  r = engine->RegisterObjectBehaviour("myTemplate<T>", asBEHAVE_FACTORY, "myTemplate<T>@ f(int&in)", asFUNCTIONPR(myTemplateFactory, (asIObjectType*), myTemplate*), asCALL_CDECL); assert( r >= 0 );
\endcode

Remember that since the subtype must be determined dynamically at runtime, it is not possible to declare
functions to receive the subtype by value, nor to return it by value. Instead you'll have to design the
methods and behaviours to take the type by reference. It is possible to use object handles, but then
the script engine won't be able to instanciate the template type for primitives and other values types.




\see \ref doc_addon_array

\section doc_adv_template_2 Template specializations

When registering a template specialization you override the template instance that AngelScript would normally
do when compiling a declaration with the template type. This allow the application to register a completely
different object with its own implementation for template specializations. Obviously it is recommended that
the template specialization is registered so that to the script writer it is transparent, i.e. try to avoid
having different method names or behaviours for the template type and template specializations.

With the exception of the type name, a template specialization is registered exactly like a \ref doc_register_type "normal type". 

\code
  // Register a template specialization for the float subtype
  r = engine->RegisterObjectType("myTemplate<float>", 0, asOBJ_REF); assert( r >= 0 );
  
  // Register the factory (there are no hidden parameters for specializations)
  r = engine->RegisterObjectBehaviour("myTemplate<float>", asBEHAVE_FACTORY, "myTemplate<float>@ f()", asFUNCTION(myTemplateFloatFactory, (), myTemplateFloat*), asCALL_CDECL); assert( r >= 0 );
\endcode





\section doc_adv_template_3 Current limitations

 - Template types are currently limited to reference types only, i.e. it must be registered
   with a factory and addref and release behaviours.
   
 - Only one template subtype can be used at the moment.
 
 - The template subtype isn't validated at the moment the script is compiled, thus it necessary
   to perform runtime checks, and possibly raise script exceptions if the subtype cannot be
   handled by the type.


*/
