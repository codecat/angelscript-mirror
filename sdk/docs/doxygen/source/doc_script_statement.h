/**

\page doc_statements Statements

	<ul>
	<li>\ref block
	<li>\ref variable
	<li>\ref if
	<li>\ref while
	<li>\ref break
	<li>\ref return
	<li>\ref expression
	</ul>


\section block Statement blocks

<pre>
{
   STATEMENTS
}
</pre>

A statement block is a collection of statements.




\section variable Variable declarations

<pre>
<i>TYPE</i> VarName = <i>EXPR</i>, VarName2 = <i>EXPR</i>;
</pre>

TYPE is exchanged for the data type. The EXPR expression must evaluate to the 
same data type (or one that can be implicitly converted to the variable type). 
The initialization expressions are optional. Any number of 
variables can be declared on the same line separated with commas.

Variables can be declared as <code>const</code>. In these cases the value of the 
variable cannot be changed after initialization.

Variables not initialized cannot be assumed to be set to zero, although object 
variables usually are since the engine's memory management depends on it.

Variables must be declared before they are used within the statement block, 
or any sub blocks. When the code exits the statement block where the variable 
was declared the variable is no longer valid.



\section if Conditions: if / if-else / switch-case

<pre>
if( <i>BOOL_EXP</i> ) <i>STATEMENT</i>

if( <i>BOOL_EXP</i> ) <i>STATEMENT</i> else <i>STATEMENT</i>
</pre>

BOOL_EXP can be exchanged for any expression that evaluates to a boolean 
data type. STATEMENT is either a one-line statement or a statement block.

<pre>
switch( <i>INT_EXP</i> )
{
case <i>INT_CONST</i>:
  <i>STATEMENT</i>

default:
  <i>STATEMENT</i>
}
</pre>

If you have an integer (signed or unsigned) expression that have many 
different outcomes that should lead to different code, a switch case is often 
the best choice for implementing the condition. It is much faster than a 
series of ifs, especially if all of the case values are close in numbers.

Each case should be terminated with a break statement unless you want the 
code to continue with the next case.

The case value can be a const variable that was initialized with a constant
expression. If the constant variable was initialized with an expression that 
cannot be determined at compile time, it cannot be used in the case values.




\section while Loops: while / do-while / for

<pre>
while( <i>BOOL_EXP</i> ) <i>STATEMENT</i>

do <i>STATEMENT</i> while( <i>BOOL_EXP</i> );

for( <i>INIT</i> ; <i>BOOL_EXP</i> ; <i>NEXT</i> ) <i>STATEMENT</i>
</pre>

BOOL_EXP can be exchanged for any expression that evaluates
to a boolean data type. STATEMENT is either a one-line statement or a statement block. INIT
can be a variable declaration, an expression, or even blank. If it is a declaration the variable
goes out of scope as the for loop ends. NEXT is an expression that is executed after STATEMENT
and before BOOL_EXP. It can also be blank. If BOOL_EXP for the for loop evaluates
to true or is blank the loop continues.






\section break Loop control: break / continue

<pre>
break;

continue;
</pre>

<code>break</code> terminates the smallest enclosing loop statement or switch statement. 
<code>continue</code> jumps to the next iteration of the smallest enclosing loop statement.





\section return Return statement

<pre>
return <i>EXPR</i>;
</pre>

Any function with a return type other than <code>void</code> must be finished with a 
<code>return</code> statement where EXPR is an expression that evaluates to the same 
data type as the function. Functions declared as <code>void</code> can have 
<code>return</code> statements without any expression to terminate early.




\section expression Expression statement

<pre><i>EXPR</i>;</pre>

Any \ref doc_expressions "expression" may be placed alone on a line as a statement. This will 
normally be used for variable assignments or function calls that don't return any value of importance.


*/