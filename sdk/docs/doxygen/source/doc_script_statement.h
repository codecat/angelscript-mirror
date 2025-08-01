/**

\page doc_script_statements Statements

 - \ref variable
 - \ref expression
 - \ref if
 - \ref while
 - \ref break
 - \ref return
 - \ref block
 - \ref try
 - \ref using_ns


\section variable Variable declarations

<pre>
  int var = 0, var2 = 10;
  object\@ handle, handle2;
  const float pi = 3.141592f;
  object obj(23), obj2 = object(23);
  array<int> arr, arr2 = {1,2,3};
</pre>

Variables must be declared before they are used within the statement block, 
or any sub blocks. When the code exits the statement block where the variable 
was declared the variable is no longer valid.

A variable can be declared with or without an initial expression. If it 
is declared with an initial expression it, the expression must evaluate
to a type compatible with the variable type.

Any number of variables can be declared on the same line separated with commas, where
all variables then get the same type.

Variables can be declared as <code>const</code>. In these cases the value of the 
variable cannot be changed after initialization.

Variables of primitive types that are declared without an initial value, will have 
a random value. Variables of complex types, such as handles and object are initialized
with a default value. For handles this is <code>null</code>, for objects this is 
what is defined by the object's default constructor.

\see \ref doc_datatypes_auto




\section expression Expression statement

<pre>
  a = b;  // a variable assignment
  func(); // a function call
</pre>

Any \ref doc_expressions "expression" may be placed alone on a line as a statement. This will 
normally be used for variable assignments or function calls that don't return any value of importance.

All expression statements must end with a <code>;</code>. 







\section if Conditions: if / if-else / switch-case

<pre>
  if( condition ) 
  {
    // Do something if condition is true
  }

  if( value < 10 ) 
  {
    // Do something if value is less than 10
  }
  else
  {
    // Do something else if value is greater than or equal to 10
  }
</pre>

If statements are used to decide whether to execute a part of the logic
or not depending on a certain condition. The conditional expression must
always evaluate to <code>true</code> or <code>false</code>. 

It's possible to chain several <code>if-else</code> statements, in which case
each condition will be evaluated sequencially until one is found to be <code>true</code>.

<pre>
  switch( value )
  {
  case 0:
    // Do something if value equals 0, then leave
    break;

  case 2:
  case constant_value:
    // This will be executed if value equals 2 or the constant_value
    break;

  default:
    // This will be executed if value doesn't equal any of the cases
  }
</pre>

If you have an integer (signed or unsigned) expression that have many 
different outcomes that should lead to different code, a switch case is often 
the best choice for implementing the condition. It is much faster than a 
series of ifs, especially if all of the case values are close in numbers.

Each case should be terminated with a break statement unless you want the 
code to continue with the next case.

The case value can be a constant variable that was initialized with a constant
expression. If the constant variable was initialized with an expression that 
cannot be determined at compile time it cannot be used in the case values.




\section while Loops: while / do-while / for / foreach

<pre>
  // Loop, where the condition is checked before the logic is executed
  int i = 0;
  while( i < 10 )
  {
    // Do something
    i++;
  }

  // Loop, where the logic is executed before the condition is checked
  int j = 0;
  do 
  {
    // Do something
    j++;
  } while( j < 10 );
</pre>

For both <code>while</code> and <code>do-while</code> the expression that determines
if the loop should continue must evaluate to either true or false. If it evaluates
to true, the loop continues, otherwise it stops and the code will continue with the next
statement immediately following the loop.

<pre>
  // More compact loop, where condition is checked before the logic is executed
  for( int n = 0; n < 10; n++ ) 
  {
    // Do something
  }
</pre>

The <code>for</code> loop is a more compact form of a <code>while</code> loop. The 
first part of the statement (until the first <code>;</code>) is executed only once, 
before the loop starts. Here it is possible to declare a variable that will be visible
only within the loop statement. The second part is the condition that must be satisfied
for the loop to be executed. A blank expression here will always evaluate to true. The last 
part is executed after the logic within the loop, e.g. used to increment an iteration variable. 

Multiple variables can be declared in the <code>for</code> loop, separated by <code>,</code>. 
Likewise, multiple increment expressions can be used in the last part by separating them with <code>,</code>.

<pre>
  // A foreach loop iterates over each element in a container object
  dictionary dict = {...};
  int count = 0;
  int sum = 0;
  foreach( auto val, auto key : dict )
  {
     count++;
     sum += int(val);
     dict[key] = 0;
  }
  double average = double(sum)/count;
</pre>

A <code>foreach</code> is a special kind of loop for container objects, where the loop will iterate over each element in the container.

The type and number of values that can be used for the container depends on the type of the container. Each container type that supports 
<code>foreach</code> loops will have a set of methods to allow the compiler to build the logic for iterating over the elements.

If the container is modified while still within the <code>foreach</code> loop, e.g. an element is removed or added, the behavior is undefined.

\see \ref doc_script_class_foreach_ops 





\section break Loop control: break / continue

<pre>
  for(;;) // endless loop
  {
    // Do something 

    // End the loop when condition is true
    if( condition )
      break;
  }
</pre>

<code>break</code> terminates the smallest enclosing loop statement or switch statement.

<pre>
  for(int n = 0; n < 10; n++ )
  {
    if( n == 5 )
      continue;

    // Do something for all values from 0 to 9, except for the value 5
  }
</pre>

<code>continue</code> jumps to the next iteration of the smallest enclosing loop statement.





\section return Return statement

<pre>
  float valueOfPI()
  {
    return 3.141592f; // return a value 
  }
</pre>

Any function with a return type other than <code>void</code> must be finished with a 
<code>return</code> statement where expression evaluates to the same 
data type as the function return type. Functions declared as <code>void</code> can have 
<code>return</code> statements without any expression to terminate early.








\section block Statement blocks

<pre>
  {
    int a; 
    float b;

    {
      float a; // Override the declaration of the outer variable
               // but only within the scope of this block.

      // variables from outer blocks are still visible
      b = a;
    }
  
    // a now refers to the integer variable again
  }
</pre>

A statement block is a collection of statements. Each statement block has its own scope of
visibility, so variables declared within a statement block are not visible outside the block.



\section try Try-catch blocks

<pre>
 {
   try
   {
     DoSomethingThatMightThrowException();
  
     // This is not executed if an exception was thrown
   }
   catch
   {
     // This is executed if an exception was thrown
   }
  }
</pre>

A try-catch block can be used if you're executing some code that might throw an exception 
and you want to catch that exception and continue with the exception rather than just abort 
the script.

Exceptions can occur for various reasons, some examples include, accessing null pointers in 
uninitialized handles, division by zero, or exceptions raised from application registered functions.
In some cases exceptions are intentionally raised by the script to interrupt some execution.

\see \ref doc_script_stdlib_exception


\section using_ns Using namespace

It is possible to declare 'using namespace' within a statement block. When this is done, all
subsequent statements within that block will also search for symbols in the given namespace.

<pre>
  namespace test
  {
    void func() {}
  }
  void main()
  {
    test::func(); // This call must explicitly inform the namespace
    {
      using namespace test;
      func(); // This call will implicitly search the namespace
    }
    test::func(); // This is after the statement block and must again explicitly inform the namespace
  }
</pre>

\see \ref doc_global_using_ns "Global 'using namespace"

*/
