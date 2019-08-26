/**

\page doc_datatypes Data types

Note that the host application may add types specific to that application, refer to the application's manual for more information.

 - \subpage doc_datatypes_primitives 
 - \subpage doc_datatypes_obj
 - \subpage doc_datatypes_funcptr
 - \subpage doc_datatypes_auto

\see \ref doc_script_stdlib 



 
 
\page doc_datatypes_primitives Primitives

\section void void

<code>void</code> is not really a data type, more like lack of data type. It can only be used to tell the compiler that a function doesn't return any data.



\section bool bool

<code>bool</code> is a boolean type with only two
possible values: <code>true</code> or
<code>false</code>. The keywords
<code>true</code> and
<code>false</code> are constants of type
<code>bool</code> that can be used as such in expressions.


\section int Integer numbers

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100><b>type</b></td><td width=200><b>min value</b></td><td><b>max value</b></td></tr>
<tr><td><code>int8  </code></td><td>                      -128</td><td>                       127</td></tr>
<tr><td><code>int16 </code></td><td>                   -32,768</td><td>                    32,767</td></tr>
<tr><td><code>int   </code></td><td>            -2,147,483,648</td><td>             2,147,483,647</td></tr>
<tr><td><code>int64 </code></td><td>-9,223,372,036,854,775,808</td><td> 9,223,372,036,854,775,807</td></tr>
<tr><td><code>uint8 </code></td><td>                         0</td><td>                       255</td></tr>
<tr><td><code>uint16</code></td><td>                         0</td><td>                    65,535</td></tr>
<tr><td><code>uint  </code></td><td>                         0</td><td>             4,294,967,295</td></tr>
<tr><td><code>uint64</code></td><td>                         0</td><td>18,446,744,073,709,551,615</td></tr>
</table>

As the scripting engine has been optimized for 32 bit datatypes, using the smaller variants is only recommended for accessing application specified variables. For local variables it is better to use the 32 bit variant.

<code>int32</code> is an alias for <code>int</code>, and <code>uint32</code> is an alias for <code>uint</code>.




\section real Real numbers

<table border=0 cellspacing=0 cellpadding=0>
<tr><td width=100><b>type</b></td><td width=230><b>range of values</b></td><td width=230><b>smallest positive value</b></td> <td><b>maximum digits</b></td></tr>
<tr><td><code>float </code></td> <td>+/- 3.402823466e+38      </td> <td>1.175494351e-38      </td> <td>6 </td></tr>
<tr><td><code>double</code></td> <td>+/- 1.79769313486231e+308</td> <td>2.22507385850720e-308</td> <td>15</td></tr>
</table>

\note These numbers assume the platform uses the IEEE 754 to represent floating point numbers in the CPU

Rounding errors may occur if more digits than the maximum number of digits are used.

<b>Curiousity</b>: Real numbers may also have the additional values of positive and negative 0 or 
infinite, and NaN (Not-a-Number). For <code>float</code> NaN is represented by the 32 bit data word 0x7fc00000.






\page doc_datatypes_obj Objects and handles

\section objects Objects

There are two forms of objects, reference types and value types. 

Value types behave much like the primitive types, in that they are allocated on the stack and deallocated 
when the variable goes out of scope. Only the application can register these types, so you need to check
with the application's documentation for more information about the registered types.

Reference types are allocated on the memory heap, and may outlive the initial variable that allocates
them if another reference to the instance is kept. All \ref doc_script_class "script declared classes" are reference types. 
\ref doc_global_interface "Interfaces" are a special form of reference types, that cannot be instantiated, but can be used to access
the objects that implement the interfaces without knowing exactly what type of object it is.

<pre>
  obj o;      // An object is instantiated
  o = obj();  // A temporary instance is created whose 
              // value is assigned to the variable
</pre>



\section handles Object handles

Object handles are a special type that can be used to hold references to other objects. When calling methods or 
accessing properties on a variable that is an object handle you will be accessing the actual object that the 
handle references, just as if it was an alias. Note that unless initialized with the handle of an object, the 
handle is <code>null</code>.

<pre>
  obj o;
  obj@ a;           // a is initialized to null
  obj@ b = \@o;      // b holds a reference to o

  b.ModifyMe();     // The method modifies the original object

  if( a is null )   // Verify if the object points to an object
  {
    \@a = \@b;        // Make a hold a reference to the same object as b
  }
</pre>

Not all types allow a handle to be taken. Neither of the primitive types can have handles, and there may exist some object types that do not allow handles. Which objects allow handles or not, are up to the application that registers them.

Object handle and array type modifiers can be combined to form handles to arrays, or arrays of handles, etc.

\see \ref doc_script_handle









\page doc_datatypes_funcptr Function handles

A function handle is a data type that can be dynamically set to point to a global function that has
a matching function signature as that defined by the variable declaration. Function handles are commonly
used for callbacks, i.e. where a piece of code must be able to call back to some code based on some 
conditions, but the code that needs to be called is not known at compile time.

To use function handles it is first necessary to \ref doc_global_funcdef "define the function signature" 
that will be used at the global scope or as a member of a class. Once that is done the variables can be 
declared using that definition.

Here's an example that shows the syntax for using function handles

<pre>
  // Define a function signature for the function handle
  funcdef bool CALLBACK(int, int);

  // An example function that shows how to use this
  void main()
  {
    // Declare a function handle, and set it 
    // to point to the myCompare function.
    CALLBACK \@func = \@myCompare;

    // The function handle can be compared with the 'is' operator
    if( func is null )
    {
      print("The function handle is null\n");
      return;
    }

    // Call the function through the handle, just as if it was a normal function
    if( func(1, 2) )
    {
      print("The function returned true\n");
    }
    else
    {
      print("The function returned false\n");
    }
  }

  // This function matches the CALLBACK definition, since it has 
  // the same return type and parameter types.
  bool myCompare(int a, int b)
  {
    return a > b;
  }
</pre>

\section doc_datatypes_delegate Delegates

It is also possible to take function handles to class methods, but in this case the class method
must be bound to the object instance that will be used for the call. To do this binding is called
creating a delegate, and is done by performing a construct call for the declared function definition
passing the class method as the argument.

<pre>
  class A
  {
    bool Cmp(int a, int b)
    {
       count++;
       return a > b;
    }
    int count = 0;
  }
  
  void main()
  {
    A a;
    
    // Create the delegate for the A::Cmp class method
    CALLBACK \@func = CALLBACK(a.Cmp);
    
    // Call the delegate normally as if it was a global function
    if( func(1,2) )
    {
      print("The function returned true\n");
    }
    else
    {
      print("The function returned false\n");
    }
    
    printf("The number of comparisons performed is "+a.count+"\n");
  }
</pre>









\page doc_datatypes_auto Auto declarations

It is possible to use 'auto' as the data type of an assignment-style variable declaration.

The appropriate type for the variable(s) will be automatically determined.

<pre>
  auto i = 18;         // i will be an integer
  auto f = 18 + 5.f;   // the type of f resolves to float
  auto o = getLongObjectTypeNameById(id); // avoids redundancy for long type names
</pre>


Auto can be qualified with const to force a constant value:

<pre>
  const auto i = 2;  // i will be typed as 'const int'
</pre>


For types that support handles, auto will always become a handle as it is more efficient
than doing a value assignment. If the variable must not be a handle for some reason, then 
do not use the 'auto' keyword.

<pre>
  auto  a = getObject();  // auto is typed 'obj@'
  auto@ b = getObject();  // this is still allowed if you want to be more explicit
</pre>


Auto handles can not be used to declare class members, since their resolution is dependent on the constructor.

*/
