/**

\page doc_global Globals

All global declarations share the same namespace so their names may not
conflict. This includes extended data types and built in functions registered
by the host application. Also, all declarations are visible to all, e.g. a
function to be called do not have to be declared above the function that calls
it.

    <ul>
    <li>\ref function
    <li>\ref variable
    <li>\ref class
    <li>\ref interface
    <li>\ref import
    <li>\ref enums
    <li>\ref typedef
    </ul>


\section function Functions

Global functions are declared normally, just as in C/C++. The function body must be defined, 
i.e. it is not possible to declare prototypes, nor is it necessary as the compiler can resolve 
the function names anyway.

For parameters sent by reference, i.e. with the <code>&amp;</code> modifier it is necessary to 
specify in which direction the value is passed, in, out, or inout, e.g. &amp;out. If no keyword 
is used, the compiler assumes the inout modifier. For parameters marked with in, the value is 
passed in to the function, and for parameters marked with out the value is returned from the function.

Parameters can also be declared as <code>const</code> which prohibits the alteration of their value. It is
good practice to declare variables that will not be changed as <code>const</code>,
because it makes for more readable code and the compiler is also able to take advantage 
of it some times. Especially for const &amp;in the compiler is many times able to avoid a copy of the value.

Note that although functions that return types by references can't be
declared by scripts you may still see functions like these if the host
application defines them. In that case you the returned value may also
be used as the target in assignments.

<pre>
int MyFunction(int a, int b)
{
  return a + b;
}
</pre>

\section variable Variables

Global variables may be declared in the scripts, which will then be shared between 
all contexts accessing the script module.

The global variables may be initialized by simple expressions that do not require 
any functions to be called, i.e. the value can be evaluated at compile time.

Variables declared globally like this are accessible from all functions. The 
value of the variables are initialized at compile time and any changes are 
maintained between calls. If a global variable holds a memory resource, e.g. 
a string, its memory is released when the module is discarded or the script engine is reset.

<pre>
int MyValue = 0;
const bits Flag1 = 0x01;
</pre>

\section class Classes

In AngelScript the script writer may declare script classes. The syntax is
similar to that of C++, except the public, protected, and private keywords are
not available, and currently there is no support for inheritance. All the class
methods must be declared with their implementation, like in Java.

The default constructor and destructor are not needed, unless specific
logic is wanted. AngelScript will take care of the proper initialization of
members upon construction, and releasing members upon destruction.

With classes the script writer can declare new data types that hold groups
of variables and methods to manipulate them.

<pre>
// The class declaration
class MyClass
{
  // The default constructor
  MyClass()
  {
    this.a = 0;
  }

  // Destructor
  ~MyClass()
  {
  }

  // Another constructor
  MyClass(int a)
  {
    this.a = a;
  }

  // A class method
  void DoSomething()
  {
    this.a *= 2;
  }

  // A class property
  int a;
}
</pre>

Note, that since AngelScript uses automatic memory management, it can be
difficult to know exactly when the destructor is called, so you shouldn't rely
on the destructor being called at a specific moment. AngelScript will also
call the destructor only once, even if the object is resurrected by adding a
reference to it while executing the destructor.

\section interface Interfaces

An interface works like a contract, the classes that implements an interface
are guaranteed to implement the methods declared in the interface. This allows
for the use of polymorphism, in that a function can specify that it wants an
object handle to an object that implements a certain interface. The function
can then call the methods on this interface without having to know what the
exact type of the object that is working with.

<pre>
// The interface declaration
interface MyInterface
{
  void DoSomething();
}

// A class that implements the interface MyInterface
class MyClass : MyInterface
{
  void DoSomething()
  {
    // Do something
  }
}
</pre>

A class can implement multiple interfaces; Simply list all the interfaces
separated by a comma.

\section import Imports

Sometimes it may be useful to load script modules dynamically without having to recompile 
the main script, but still let the modules interact with each other. In that case the script 
may import functions from another module. This declaration is written using the import 
keyword, followed by the function signature, and then specifying which module to import from.

This allows the script to be compiled using these imported functions, without them actually 
being available at compile time. The application can then bind the functions at a later time, 
and even unbind them again.

If a script is calling an imported function that has not yet been bound the script will be 
aborted with a script exception.

<pre>
import void MyFunction(int a, int b) from "Another module";
</pre>

\section enums Enums

Enums are a convenient way of registering a family of integer constants that may be used throughout the script 
as named literals instead of numeric constants. Using enums often help improve the readability of the code, as
the named literal normally explains what the intention is without the reader having to look up what a numeric value
means in the manual.

Even though enums list the valid values, you cannot rely on a variable of the enum type to only contain values
from the declared list. Always have a default action in case the variable holds an unexpected value.

The enum values are declared by listing them in an enum statement. Unless a specific value is given for an enum 
constant it will take the value of the previous constant + 1. The first constant will receive the value 0, 
unless otherwise specified.

<pre>
enum MyEnum
{
    eValue0,
    eValue2 = 2,
    eValue3,
    eValue200 = eValue2 * 100
}
</pre>


\section typedef Typedefs

Typedefs are used to define aliases for other types.

Currently a typedef can only be used to define an alias for primitive types, but a future version will have
more complete support for all kinds of types.

<pre>
typedef float  real32;
typedef double real64;
</pre>



*/