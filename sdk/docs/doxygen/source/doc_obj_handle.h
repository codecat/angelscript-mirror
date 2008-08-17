/**

\page doc_obj_handle Object handles


In AngelScript an object handle is a reference counted pointer to an object. In the scripts 
they are used to pass the objects around by reference instead of by value.

\section doc_obj_handle_1 In the scripts

An object handle is a type that can hold a reference to an object. With
object handles it is possible to declare more than one variables that refer
to the same physical object.

Not all types allow object handles to be used. None of the primitive
data types, bool, int, float, etc, can have object handles. Object types
registered by the application may or may not allow object handles, verify
the applications documentation.

An object handle is declared by appending the @ symbol to the data type.

<pre>
object\@ obj_h;
</pre>

This code declares the object handle obj and initializes it to null, i.e.
it doesn't hold a reference to any object.

In expressions variables declared as object handles are used the exact
same way as normal objects. But you should be aware that object handles are
not guaranteed to actually reference an object, and if you try to access the
contents of an object in a handle that is null an exception will be raised.

<pre>
object obj;
object\@ obj_h;
obj.Method();
obj_h.Method();
</pre>

Operators like = or any other operator registered for the object type work
on the actual object that the handle references. These will also throw an
exception if the handle is empty.

<pre>
object obj;
object\@ obj_h;
obj_h = obj;
</pre>

When you need to make an operation on the actual handle, you should prepend
the expression with the \@ symbol. Setting the object handle to point to an
object is for example done like this:

<pre>
object obj;
object\@ obj_h;
\@obj_h = \@obj;
</pre>

An object handle can be compared against another object handle
(of the same type) to verify if they are pointing to the same object or not.
It can also be compared against null, which is a special keyword that
represents an empty handle.

<pre>
object\@ obj_a, obj_b;
if( \@obj_a == \@obj_b ) {}
if( \@obj_a == null ) {}
</pre>

An object's life time is normally for the duration of the scope the
variable was declared in. But if a handle outside the scope is set to
reference the object, the object will live on until all object handles are
released.

<pre>
object\@ obj_h;
{
  object obj;
  \@obj_h = \@obj;

  // The object would normally die when the block ends,
  // but the handle is still holding a reference to it
}

// The object still lives on in obj_h ...
obj_h.Method();

// ... until the reference is explicitly released
// or the object handle goes out of scope
\@obj_h = null;
</pre>

\section doc_obj_handle_2 Behaviours to support object handles

For an application registered object to support object handles, it must register the 
behaviours asBEHAVE_ADDREF and asBEHAVE_RELEASE. To initialize the reference counter 
properly it is also necessary to register the asBEHAVE_CONSTRUCT and asBEHAVE_ASSIGNMENT 
behaviours.

The object's constructor behaviour must initialize the reference counter to 1, otherwise 
the object may never be released.

\code
void obj_constructor(obj *o)
{
  new(o) obj();

  // In case the class' constructor doesn't initialize
  // the reference counter to 1 then we must do that here
}
\endcode

The object's destructor behaviour will never be used, as the object's release behaviour 
is responsible for destroying the object when the reference counter reaches 0.


\code
void obj::Release()
{
  if( --refCount == 0 )
    delete this;
}
\endcode


The object's assignment operator must be registered and should not copy the reference 
counter. If the assignment operator is not registered then AngelScript will do a default 
byte-for-byte copy of the object, including the reference counter. This will corrupt 
the value of the reference counter of the left hand object.


\section doc_obj_handle_3 Managing the reference counter in functions

Whenever the object handle is passed by value from the application to the script engine, 
or vice versa its reference should be accounted for. This means that the application must 
release any object handles it receives as parameters when it no longer needs them, it also 
means that the application must increase the reference counter for any object handle being 
returned to the script engine.

A function that creates an object and returns it to the script engine might look like this:

\code
// Registered as "obj@ CreateObject()"
obj *CreateObject()
{
  // The constructor already initializes the ref count to 1
  return new obj();
}
\endcode

A function that receives an object handle from the script and stores it in a global variable might look like this:

\code
// Registered as "void StoreObject(obj@)"
obj *o = 0;
void StoreObject(obj *newO)
{
  // Release the old object handle
  if( o ) o->Release();

  // Store the new object handle
  o = newO;
}
\endcode

A function that retrieves a previously stored object handle might look like this:

\code
// Registered as "obj@ RetrieveObject()"
obj *RetrieveObject()
{
  // Increase the reference counter to account for the returned handle
  if( o ) o->AddRef();

  // It is ok to return null, incase there is no previous handle stored
  return o;
}
\endcode

A function that receives an object handle in the parameter, but doesn't store it looks like this:

\code
// Registered as "void DoSomething(obj@)"
void DoSomething(obj *o)
{
  // When finished with the object it must be released
  if( o ) o->Release();
}
\endcode

\section doc_obj_handle_4 Auto handles can make it easier

The application can use auto handles (\@+) to alleviate some of the work of managing the reference counter. 
When registering the function or method with AngelScript, add a plus sign to the object handles that 
AngelScript should automatically manage. For parameters AngelScript will then release the reference after 
the function returns, and for the return value AngelScript will increase the reference on the returned 
pointer. The reference for the returned value is increased before the parameters are released, so it is 
possible to have the function return one of the parameters.

\code
// Registered as "obj@+ ChooseObj(obj@+, obj@+)"
obj *ChooseObj(obj *a, obj *b)
{
  // Because of the auto handles AngelScript will
  // automatically manage the reference counters
  return some_condition ? a : b;
}
\endcode

However, it is not recommended to use this feature unless you can't change the functions you want 
to register to properly handle the reference counters. When using the auto handles, AngelScript 
needs to process all of the handles which causes an extra overhead when calling application registered functions.



*/
