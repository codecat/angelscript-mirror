/**

\page doc_script_func Functions

Global functions provide the mean to implement routines that should operate on some input and produce a result. 
The functions themselves do not keep any memory, though they can update global variables, or memory passed by 
reference.

The function is always declared together with the function body. There is no need to declare function prototypes,
as the function is globally visible regardless where it has been declared.

<pre>
  // A simple function declaration
  int AFunction(int a, int b)
  {
    // Implement the function logic here
    return a + b;
  }
</pre>

The type of the return value should be specified before the name of the function. If the function doesn't return anything
the type should be defined as 'void'. After the function name, the list of parameters is specified between parenthesis. 
Each parameter is defined by its type and name.

\section doc_script_func_ref References

The parameters and return value can be of any type that can also be used for variable declarations. In addition, it is also
possible to make a function take a value by reference, i.e. rather than a copy of the original value, the parameter will refer
to the value.

Parameter references are used mainly for two purposes; a means of providing additional output from the function, or as a more performatic 
way of passing values.

In AngelScript it is necessary to specify the intention of the parameter reference, i.e. if it is meant as input, output, or both. 
This is necessary in order for the compiler to prepare the reference in a way that it cannot be invalidated du to some action during
the processing of the function.

Input references are written as &in. As the reference is meant as input only, the actual value it refers to normally is a copy of the 
original so the function doesn't accidentally modify the original value. These are not commonly used, as they provide little benefit
over passing arguments by value. Only in some circumstances can the performance be improved, especially if the parameter is declared as 
const too.

Output references are written as &out. These references are meant to allow the function to return additional values. When going in to 
the function, the reference will point to an uninitialized value. After the function returns, the value assigned by the function will be 
copied to the destination determined by the caller.

When the reference is meant as both input and output, it is declared as &inout, or just &. In this case the reference will point to the
actual value. Only \ref doc_datatypes_obj "reference types", i.e. that can have handles to them, are allowed to be passed as inout references. This is because 
in order to guarantee that the reference will stay valid during the entire execution of the function, the value must be located in the 
memory heap.

<pre>
void Function(const int &in a, int &out b, Object &c)
{
  // Assigning an output value to the output reference
  b = a;
  
  // The object is an inout reference and refers to the real object
  c.DoSomething();
}
</pre>

\todo Explain returning references


\section doc_script_func_overload Function overloading

\todo Explain how function overloading works, and how the compiler chooses which function to call



\section doc_script_func_defarg Default arguments

\todo Explain how default arguments works








*/
