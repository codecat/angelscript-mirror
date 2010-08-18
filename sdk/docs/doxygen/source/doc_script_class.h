/**


\page doc_script_class Script classes

In AngelScript the script writer may declare script classes. The syntax is
similar to that of C++. All the class methods must be declared with their implementation, 
like in Java.

The default constructor and destructor are not needed, unless specific
logic is wanted. AngelScript will take care of the proper initialization of
members upon construction, and releasing members upon destruction, even if not 
manually implemented.

With classes the script writer can declare new data types that hold groups
of variables and methods to manipulate them. The class' properties can be
accessed directly or through \ref doc_script_class_prop "property accessors". 
It is also possible to \ref doc_script_class_ops "overload operators" for the classes.

<pre>
  // The class declaration
  class MyClass
  {
    // The default constructor
    MyClass()
    {
      a = 0;
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
      a *= 2;
    }

    // A class property
    int a;
  }
</pre>

\section doc_script_class_inheritance Inheritance and polymorphism

AngelScript supports single inheritance, where a derived class inherits the 
properties and methods of its base class. Multiple inheritance is not supported,
but polymorphism is supprted by implementing \ref doc_global_interface "interfaces".

All the class methods are virtual, so it is not necessary to specify this manually. 
When a derived class overrides an implementation, it can extend 
the original implementation by specifically calling the base class' method using the
scope resolution operator. When implementing the constructor for a derived class
the constructor for the base class is called using the <code>super</code> keyword. 
If none of the base class' constructors is manually called, the compiler will 
automatically insert a call to the default constructor in the beginning. The base class'
destructor will always be called after the derived class' destructor, so there is no
need to manually do this.

<pre>
  // A derived class
  class MyDerived : MyClass
  {
    // The default constructor
    MyDerived()
    {
      // Calling the non-default constructor of the base class
      super(10);
      
      b = 0;
    }
    
    // Overloading a virtual method
    void DoSomething()
    {
      // Call the base class' implementation
      MyClass::DoSomething();
      
      // Do something more
      b = a;
    }
    
    int b;
  }
</pre>

Note, that since AngelScript uses \ref doc_memory "automatic memory management", it can be
difficult to know exactly when the destructor is called, so you shouldn't rely
on the destructor being called at a specific moment. AngelScript will also
call the destructor only once, even if the object is resurrected by adding a
reference to it while executing the destructor.

\section doc_script_class_private Private class members

Class members can be declared as private if you wish do not intend for them to be accessed 
from outside the public class methods. This can be useful in large programs where you to 
avoid programmer errors where properties or methods are inappropriately used.

<pre>
  // A class with private members
  class MyPrivate
  {
    // The following are public members
    void PublicFunc()
    {
      // The class can access its own private members
      PrivateProp = 0; // OK
      PrivateFunc();   // OK
    }    
 
    int PublicProp;

    // The following are private members
    private void PrivateFunc()
    {
    } 

    private int PrivateProp;
  }

  void GlobalFunc()
  {
    MyPrivate obj;

    // Public members can be accessed normally
    obj.PublicProp = 0;  // OK
    obj.PublicFunc();    // OK

    // Accessing private members will give a compiler error
    obj.PrivateProp = 0; // Error
    obj.PrivateFunc();   // Error
  }
</pre>



*/
