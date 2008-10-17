/**

\page doc_register_type Registering an object type


The are two principal paths to take when registering a new type, either the
type is a reference type that is located in dynamic memory, or the type is a
value type that is located on the stack. Complex types are usually registered
as reference types, while simple types that are meant to be used as primitives
are registered as value types. A reference type support object handles (unless restricted by application), but
cannot be passed by value to application registered functions, a value type
doesn't support handles and can be passed by value to application registered
functions.

 - \subpage doc_register_ref_type
 - \subpage doc_register_val_type
 - \subpage doc_reg_opbeh
 - \subpage doc_reg_objmeth
 - \subpage doc_reg_objprop


\page doc_register_ref_type Registering a reference type

 - \ref doc_reg_basicref
 - \ref doc_reg_noinst
 - \ref doc_reg_single
 - \ref doc_reg_scoped

\see \ref doc_gc_object

\section doc_reg_basicref Registering a basic reference type

The basic reference type should be registered with the following behaviours:
asBEHAVE_FACTORY, asBEHAVE_ADDREF, and asBEHAVE_RELEASE. If it is desired that
assignments should be allowed for the type the asBEHAVE_ASSIGNMENT behaviour
must be registered as well. Other behaviours, such as math operators,
comparisons, etc may be registered as needed.

\code
// Registering the reference type
r = engine->RegisterObjectType("ref", 0, asOBJ_REF); assert( r >= 0 );
\endcode

\see The \ref doc_addon_string "string" add-on for an example of a reference type


\subsection doc_reg_basicref_1 Factory function

The factory function is the one that AngelScript will use to instanciate
objects of this type when a variable is declared. It is responsible for
allocating and initializing the object memory.

The default factory function doesn't take any parameters and should return
an object handle for the new object. Make sure the object's reference counter
is accounting for the reference being returned by the factory function, so
that the object is properly released when all references to it are removed.

\code
CRef::CRef()
{
    // Let the constructor initialize the reference counter to 1
    refCount = 1;
}

CRef *Ref_Factory()
{
    // The class constructor is initializing the reference counter to 1
    return new CRef();
}

// Registering the factory behaviour
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_FACTORY, "ref@ f()", asFUNCTION(Ref_Factory), asCALL_CDECL); assert( r >= 0 );
\endcode

You may also register factory functions that take parameters, which may
then be used when initializing the object.

The factory function must be registered as a global function, but can be
implemented as a static class method, common global function, or a global
function following the generic calling convention.

\subsection doc_reg_basicref_2 Addref and release behaviours

\code
void CRef::Addref()
{
    // Increase the reference counter
    refCount++;
}

void CRef::Release()
{
    // Decrease ref count and delete if it reaches 0
    if( --refCount == 0 )
        delete this;
}

// Registering the addref/release behaviours
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ADDREF, "void f()", asMETHOD(CRef,AddRef), asCALL_THISCALL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_RELEASE, "void f()", asMETHOD(CRef,Release), asCALL_THISCALL); assert( r >= 0 );
\endcode

\subsection doc_reg_basicref_3 Assignment behaviour

\code
CRef &CRef::operator =(const CRef &other)
{
    // Copy everything from the other class, except the reference counter
}

// Registering the assignment behaviour
r = engine->RegisterObjectBehaviour("ref", asBEHAVE_ASSIGNMENT, "ref &f(const &in)", asMETHOD(CRef,operator=), asCALL_THISCALL); assert( r >= 0 );
\endcode

The assignment behaviour can be overloaded with other types if that is
desired, that way the script writer doesn't have to manually convert the
expressions before assigning the values to the type.










\section doc_reg_noinst Registering an uninstanciable reference type

Sometimes it may be useful to register types that cannot be instanciated by
the scripts, yet can be interacted with. You can do this by registering the
type as a normal reference type, but omit the registration of the factory
behaviour. You can later register global properties, or functions that allow the
scripts to access objects created by the application via object handles.

This would be used when the application has a limited number of objects
available and doesn't want to create new ones. For example singletons, or
pooled objects.






\section doc_reg_single Registering a single-reference type

A variant of the uninstanciable reference types is the single-reference
type. This is a type that have only 1 reference accessing it, i.e. the script
cannot store any extra references to the object during execution. The script
is forced to use the reference it receives from the application at the moment
the application passes it on to the script.

The reference can be passed to the script through a property, either global
or a class member, or it can be returned from an application registered
function or class method.

\code
// Registering the type so that it cannot be instanciated
// by the script, nor allow scripts to store references to the type
r = engine->RegisterObjectType("single", 0, asOBJ_REF | asOBJ_NOHANDLE); assert( r >= 0 );
\endcode

This sort of type is most useful when you want to have complete control over
references to an object, for example so that the application can destroy and
recreate objects of the type without having to worry about potential references
held by scripts. This allows the application to control when a script has access
to an object and it's members.




\section doc_reg_scoped Registering a scoped type

Some C++ value types have special requirements for the memory where they
are located, e.g. specific alignment needs, or memory pooling. Since
AngelScript doesn't provide that much control over where and how value types
are allocated, they must be registered as reference types. In this case you'd
register the type as a scoped reference type.

A scoped reference type will have the life time controlled by the scope of
the variable that instanciate it, i.e. as soon as the variable goes out of
scope the instance is destroyed. This means that the type doesn't permit
handles to be taken for the type.

A scoped reference type requires two behaviours to be registered, the
factory and the release behaviour. The addref behaviour is not permitted.

Since no handles can be taken for the object type, there is no need to keep
track of the number of references held to the object. This means that the
release behaviour should simply destroy and deallocate the object as soon as
it's called.

\code
scoped *Scoped_Factory()
{
  return new scoped;
}

void Scoped_Release(scoped *s)
{
  if( s ) delete s;
}

// Registering a scoped reference type
r = engine->RegisterObjectType("scoped", 0, asOBJ_REF | asOBJ_SCOPED); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_FACTORY, "scoped @f()", asFUNCTION(Scoped_Factory), asCALL_CDECL); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("scoped", asBEHAVE_RELEASE, "void f()", asFUNCTION(Scoped_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode






\page doc_register_val_type Registering a value type

When registering a value type, the size of the type must be given so that AngelScript knows how much space is needed for it.
If the type doesn't require any special treatment, i.e. doesn't contain any pointers or other resource references that must be
maintained, then the type can be registered with the flag asOBJ_POD. In this case AngelScript doesn't require the default
constructor, assignment behaviour, or destructor as it will be able to automatically handle these cases the same way it handles
built-in primitives.

If the type will be passed to and from the application by value using native calling conventions, it is important to inform
AngelScript of its real type in C++, otherwise AngelScript won't be able to determine exactly how C++ is treating the type in
a parameter or return value. There are a few different flags for this:

<table border=0 cellspacing=0 cellpadding=0>
<tr><td>asOBJ_APP_CLASS             &nbsp; </td><td>The C++ type is a class, struct, or union</td></tr>
<tr><td>asOBJ_APP_CLASS_CONSTRUCTOR &nbsp; </td><td>The C++ type has a defined constructor</td></tr>
<tr><td>asOBJ_APP_CLASS_DESTRUCTOR  &nbsp; </td><td>The C++ type has a defined destructor</td></tr>
<tr><td>asOBJ_APP_CLASS_ASSIGNMENT  &nbsp; </td><td>The C++ type has a defined assignment operator</td></tr>
<tr><td>asOBJ_APP_PRIMITIVE         &nbsp; </td><td>The C++ type is a C++ primitive, but not a float or double</td></tr>
<tr><td>asOBJ_APP_FLOAT             &nbsp; </td><td>The C++ type is a float or double</td></tr>
</table>


\code
// Register a primitive type, that doesn't need any special management of the content
r = engine->RegisterObjectType("pod", sizeof(pod), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );

// Register a class that must be properly initialized and uninitialized
r = engine->RegisterObjectType("val", sizeof(val), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >= 0 );
\endcode

\see The \ref doc_addon_math3d "vector3" add-on for an example of a value type


\section doc_reg_val_1 Constructor and destructor

If a constructor or destructor is needed they shall be registered the following way:

\code
void Constructor(void *memory)
{
  // Initialize the pre-allocated memory by calling the
  // object constructor with the placement-new operator
  new(memory) Object();
}

void Destructor(void *memory)
{
  // Uninitialize the memory by calling the object destructor
  ((Object*)memory)->~Object();
}

// Register the behaviours
r = engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Constructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
r = engine->RegisterObjectBehaviour("val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Destructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode

The assignment behaviour is registered the same way as for reference types.





\page doc_reg_opbeh Registering operator behaviours

You can register operator behaviours for your types as well. By doing this
you'll allow the script to work with the types in expressions, just like the
built-in types.

There two forms of operator behaviours, either object behaviours or global
behaviours. An object behaviour is implemented as a class method, and a global
behaviour is implemented as a global function.

\code
// Registering an object behaviour
int &MyClass::operator[] (int index)
{
  return internal_array[index];
}

r = engine->RegisterObjectBehaviour("mytype", asBEHAVE_INDEX, "int &f(int)", asMETHOD(MyClass,operator[]), asCALL_THISCALL); assert( r >= 0 );

// Registering a global behaviour
MyClass operator+(const MyClass &a, const MyClass &b)
{
  MyClass res = a + b;
  return res;
}

r = engine->RegisterGlobalBehaviour(asBEHAVE_ADD, "mytype f(const mytype &in, const mytype &in)", asFUNCTIONPR(operator+, (const MyClass &, const MyClass &), MyClass), asCALL_CDECL); assert( r >= 0 );
\endcode

You can find a complete list of behaviours \ref doc_api_behaviours "here".

\page doc_reg_objmeth Registering object methods

Class methods are registered with the RegisterObjectMethod call.

\code
// Register a class method
void MyClass::ClassMethod()
{
  // Do something
}

r = engine->RegisterObjectMethod("mytype", "void ClassMethod()", asMETHOD(MyClass,ClassMethod), asCALL_THISCALL); assert( r >= 0 );
\endcode

It is also possible to register a global function that takes a pointer to
the object as a class method. This can be used to extend the functionality of
a class when accessed via AngelScript, without actually changing the C++
implementation of the class.

\code
// Register a global function as an object method
void MyClass_MethodWrapper(MyClass *obj)
{
  // Access the object
  obj->DoSomething();
}

r = engine->RegisterObjectMethod("mytype", "void MethodWrapper()", asFUNCTION(MyClass_MethodWrapper), asCALL_CDECL_OBJLAST); assert( r >= 0 );
\endcode

\page doc_reg_objprop Registering object properties

Class member variables can be registered so that they can be directly
accessed by the script without the need for any method calls.

\code
struct MyStruct
{
  int a;
};

r = engine->RegisterObjectProperty("mytype", "int a", offsetof(MyStruct,a)); assert( r >= 0 );
\endcode

offsetof() is a macro declared in stddef.h header file.







*/
