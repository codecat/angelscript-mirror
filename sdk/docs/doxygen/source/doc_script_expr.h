/**

\page doc_expressions Expressions

\todo Clean up this page


	<ul>
	<li>\ref assignment
	<li>\ref compound
	<li>\ref function
	<li>\ref conversion
	<li>\ref math
	<li>\ref bits
	<li>\ref logic
	<li>\ref comparison
	<li>\ref increment
	<li>\ref index
	<li>\ref condition
	<li>\ref member
	<li>\ref handle
	</ul>


\section assignment Assignments

<pre><i>LVALUE</i> = <i>EXPR</i></pre>

LVALUE must be an expression that evaluates to a memory location where the 
expression value can be stored, e.g. a variable. An assignment evaluates to 
the same value and type of the data stored. The right hand expression is 
always computed before the left.




\section compound Compound assignments

<pre><i>LVALUE</i> <i>OP</i> <i>EXPR</i></pre>

In this case the expression is combined with the LVALUE using on of the 
available operators: <code>+= -= *= /= %= &= |= ^= &lt;&lt;= &gt;&gt;= &gt;&gt;&gt;=</code>



\section function Function call

<pre><i>FUNCTION</i>(<i>ARGS</i>)</pre>

Functions can only be called in expressions. Functions with no return value 
(void) can only be called when no data is expected, e.g. alone on a line.



\section conversion Type conversions

<pre class=border>
intf \@a = clss();

// convert the intf handle to a clss handle
clss \@b = cast&lt;clss&gt;(a);
</pre>

Object handles can be converted to other object handles with the cast operator. 
If the cast is valid, i.e. the true object implements the class or interface being 
requested, the operator returns a valid handle. If the cast is not valid, the cast 
returns a null handle.

Primitive types can also be converted using the cast operator, but there is also 
an alternative syntax.

<pre class=border>
int   a = 1;
float b = float(a)/2;
</pre>


In most cases an explicit cast is not necessary for primitive types, however, 
as the compiler is usually able to do an implicit cast to the correct type.

The compiler is also able to use declared object constructors when performing 
implicit conversions. For example, if you have an object type that can be 
constructed with an integer parameter, you will be able to pass integer
expressions to functions that expect that object type, as the compiler will 
automatically construct the object for you. Note, however that this conversion 
cannot be done implicitly if the function expects a reference argument.




\section math Math operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td><td width=100 valign=top><b>description</b></td><td width=80 valign=top><b>left hand</b></td><td width=80 valign=top><b>right hand</b></td><td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>+</code></td> <td width=100 valign=top>unary positive</td>    <td width=80 valign=top>&nbsp;</td>          <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>-</code></td> <td width=100 valign=top>unary negative</td>    <td width=80 valign=top>&nbsp;</td>          <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>+</code></td> <td width=100 valign=top>addition</td>          <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>-</code></td> <td width=100 valign=top>subtraction</td>       <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>*</code></td> <td width=100 valign=top>multiplication</td>    <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>/</code></td> <td width=100 valign=top>division</td>          <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>%</code></td> <td width=100 valign=top>modulos</td>           <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
</table>

Plus and minus can be used as unary operators as well. NUM can be exchanged 
for any numeric type, e.g. <code>int</code> or <code>float</code>. Both terms 
of the dual operations will be implicitly converted to have the same type. The 
result is always the same type as the original terms. One exception is unary 
negative which is not available for <code>uint</code>.




\section bits Bitwise operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td>          <td width=130 valign=top><b>description</b></td>    <td width=80 valign=top><b>left hand</b></td>                         <td width=80 valign=top><b>right hand</b></td>                        <td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>~</code></td>           <td width=130 valign=top>bitwise complement</td>    <td width=80 valign=top>&nbsp;</td>                                   <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&</code></td>           <td width=130 valign=top>bitwise and</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>|</code></td>           <td width=130 valign=top>bitwise or</td>            <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>^</code></td>           <td width=130 valign=top>bitwise xor</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&lt;&lt;</code></td>    <td width=130 valign=top>left shift</td>            <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&gt;&gt;</code></td>    <td width=130 valign=top>right shift</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&gt;&gt;&gt;</code></td><td width=130 valign=top>arithmetic right shift</td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
</table>


All except <code>~</code> are dual operators.





\section logic Logic operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td>                         <td width=130 valign=top><b>description</b></td>  <td width=80 valign=top><b>left hand</b></td>                         <td width=80 valign=top><b>right hand</b></td>                        <td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>not</code></td><td width=130 valign=top>logical not</td>         <td width=80 valign=top>&nbsp;</td>                                   <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>and</code></td><td width=130 valign=top>logical and</td>         <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>or</code></td> <td width=130 valign=top>logical or</td>          <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>xor</code></td><td width=130 valign=top>logical exclusive or</td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
</table>

Boolean operators only evaluate necessary terms. For example in expression 
<code>a and b</code>, <code>b</code> is only evaluated if <code>a</code> is 
<code>true</code>.</p>



\section comparison Comparison operators

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td>   <td width=100 valign=top><b>description</b></td><td width=80 valign=top><b>left hand</b></td><td width=80 valign=top><b>right hand</b></td><td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>==</code></td>   <td width=100 valign=top>equal</td>             <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>!=</code></td>   <td width=100 valign=top>not equal</td>         <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>&lt;</code></td> <td width=100 valign=top>less than</td>         <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>&gt;</code></td> <td width=100 valign=top>greater than</td>      <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>&lt;=</code></td><td width=100 valign=top>less or equal</td>     <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>&gt;=</code></td><td width=100 valign=top>greater or equal</td>  <td width=80 valign=top><i>value</i></td>      <td width=80 valign=top><i>value</i></td>       <td width=80 valign=top><code>bool</code></td></tr>
</table>




\section increment Increment operators

<pre>++ --</pre>

These operators can be placed either before or after an lvalue to 
increment (or decrement) its value either before or after the value 
is used in the expression. The value is always incremented or decremented with 1.



\section index Indexing operator

<pre>[<i>EXPR</i>]</pre>

This operator can only be used if the application has supports it.




\section condition Conditional expression

<pre><i>EXPR</i> ? <i>A</i> : <i>B</i></pre>

If expression is true A is executed, if not then B is executed instead.





\section member Member access

<pre><i>OBJ</i> . <i>MEMBER</i></pre>


OBJ must be an expression resulting in a data type that have members. MEMBER is the name 
of the member to be accessed. This member may or may not be read only.



\section handle Handle-of

<pre>\@ <i>VAR</i></pre>


VAR is an object that allows its handle to be taken. The handle can then be assigned to a 
handle variable of the same type, or compared against another handle, or <code>null</code>.


*/
