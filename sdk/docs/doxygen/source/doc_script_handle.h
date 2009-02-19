/**

\page doc_script_handle Object handles


An object handle is a type that can hold a reference to an object. With
object handles it is possible to declare more than one variables that refer
to the same physical object.

Not all types allow object handles to be used. None of the primitive
data types, bool, int, float, etc, can have object handles. Object types
registered by the application may or may not allow object handles, depending 
on how they have been registered.

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
represents an empty handle. This is done using the identity operator, <code>is</code>.

<pre>
  object\@ obj_a, obj_b;
  if( obj_a is obj_b ) {}
  if( obj_a !is null ) {}
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

\todo Show how handles can be cast to other types with ref cast

*/
