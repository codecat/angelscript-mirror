/**

\page doc_datatypes Data types

Note that the host application may add types specific to that application, refer to the application's manual for more information.

 - \ref void
 - \ref bool
 - \ref int
 - \ref uint
 - \ref float
 - \ref double
 - \ref arrays
 - \ref handles
 - \ref strings


\section void void

<code>void</code> is not really a data type, more like lack of data type. It can only be used to tell the compiler that a function doesn't return any data.


\section bool bool

<code>bool</code> is a boolean type with only two
possible values: <code>true</code> or
<code>false</code>. The keywords
<code>true</code> and
<code>false</code> are constants of type
<code>bool</code> that can be used as such in expressions.


\section int int, int8, int16, int64

<code>int64</code> holds integer values in the range -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807.

<code>int</code> holds integer values in the range -2,147,483,648 to 2,147,483,647.

<code>int16</code> holds integer values in the range -32,768 to 32,767.

<code>int8</code> holds integer values in the range -128 to 128.

As the scripting engine has been optimized for 32 bit datatypes, using the smaller variants is only recommended for accessing application specified variables. For local variables it is better to use the 32 bit variant.

<code>int32</code> is an alias for <code>int</code>.


\section uint uint, uint8, uint16, uint64

<code>uint64</code> holds integer values in the range 0 to 18,446,744,073,709,551,615.

<code>uint</code> holds integer values in the range 0 to 4,294,967,295.

<code>uint16</code> holds integer values in the range 0 to 65,535.

<code>uint8</code> holds integer values in the range 0 to 255.

As the scripting engine has been optimized for 32 bit datatypes, using the smaller variants is only recommended for accessing application specified variables. For local variables it is better to use the 32 bit variant.

<code>uint32</code> is an alias for <code>uint</code>.


\section float float

<code>float</code> holds real (or floating point) values in the range -/+3.402823466e38.

The smallest possible positive number that a float can destinguish is: 1.175494351e-38. The maximum number of decimal digits that can be safely used is 6, i.e. if more digits are used they are prone to rounding errors during operations.

<b>Curiousity</b>: Floats may also have the additional values of positive and negative 0 or infinite, and NaN (Not-a-Number). NaN is represented by the 32 bit data word 0x7fc00000.


\section double double

<code>double</code> holds real (or floating point) values in the range -/+1.7976931348623158e+308.

The smallest possible positive number that a double can destinguish is: 2.2250738585072014e-308. The maximum number of decimal digits that can be safely used is 15, i.e. if more digits are used they are prone to rounding errors during operations.

<b>Curiousity</b>: Doubles may also have the additional values of positive and negative 0 or infinite, and NaN (Not-a-Number).






\section arrays Arrays

It is also possible to declare array variables by appending the [] brackets to the type.

When declaring a variable with a type modifier, the type modifier affects the type of all variables in the list.
Example:

<pre class=border>
int[] a, b, c;
</pre>

<code>a</code>, <code>b</code>, and <code>c</code> are now arrays of integers.

When declaring arrays it is possible to define the initial size of the array by passing the length as a parameter to the constructor. The elements can also be individually initialized by specifying an initialization list. Example:

<pre class=border>
int[] a;           // A zero-length array of integers
int[] b(3);        // An array of integers with 3 elements
int[] c = {,3,4,}; // An array of integers with 4 elements, where
                   // the second and third elements are initialized
</pre>

Each element in the array is accessed with the indexing operator. The indices are zero based, i.e the range of valid indices are from 0 to length - 1.

<pre class=border>
a[0] = some_value;
</pre>

An array also have two methods. length() allow you to determine how many elements are in the array, and resize() lets you resize the array.


\section handles Object handles

Object handles are a special type that can be used to hold references to other objects. When calling methods or accessing properties on a variable that is an object handle you will be accessing the actual object that the handle references, just as if it was an alias. Note that unless initialized with the handle of an object, the handle is <code>null</code>.

<pre class=border>
obj o;
obj@ a;           // a is initialized to null
obj@ b = \@o;      // b holds a reference to o

b.ModifyMe();     // The method modifies the original object

if( \@a == null )  // Verify if the object points to an object
{
  \@a = \@b;        // Make a hold a reference to the same object as b
}
</pre>

Not all types allow a handle to be taken. Neither of the primitive types can have handles, and there may exist some object types that do not allow handles. Which objects allow handles or not, are up to the application that registers them.

Object handle and array type modifiers can be combined to form handles to arrays, or arrays of handles, etc.

\section strings Strings

Strings are a special type of data that can be used only if the application
registers support for them. They hold an array of
bytes. The only limit to how large this array can be is the memory available on
the computer.

There are two types of string constants supported in the AngelScript
language, the normal double quoted string, and the documentation strings,
called heredoc strings.

The normal strings are written between double quotation marks (<code>"</code>) or single quotation marks (<code>'</code>)<sup>1</sup>.
Inside the constant strings some escape sequences can be used to write exact
byte values that might not be possible to write in your normal editor.


<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=80 valign=top><b>sequence</b></td>
<td valign=top width=50><b>value</b></td>
<td valign=top><b>description</b></td></tr>

<tr><td width=80 valign=top><code>\\0</code>&nbsp;  </td>
<td valign=top width=50>0</td>
<td valign=top>null character</td></tr>
<tr><td width=80 valign=top><code>\\\\</code>&nbsp;  </td>
<td valign=top width=50>92</td>
<td valign=top>back-slash</td></tr>
<tr><td width=80 valign=top><code>\\'</code>&nbsp;  </td>
<td valign=top width=50>39</td>
<td valign=top>single quotation mark (apostrophe)</td></tr>
<tr><td width=80 valign=top><code>\\"</code>&nbsp;  </td>
<td valign=top width=50>34</td>
<td valign=top>double quotation mark</td></tr>
<tr><td width=80 valign=top><code>\\n</code>&nbsp;  </td>
<td valign=top width=50>10</td>
<td valign=top>new line feed</td></tr>
<tr><td width=80 valign=top><code>\\r</code>&nbsp;  </td>
<td valign=top width=50>13</td>
<td valign=top>carriage return</td></tr>
<tr><td width=80 valign=top><code>\\t</code>&nbsp;  </td>
<td valign=top width=50>9</td>
<td valign=top>tab character</td></tr>
<tr><td width=80 valign=top><code>\\xFF</code>&nbsp;</td>
<td valign=top width=50>0xFF</td>
<td valign=top>FF should be exchanged for the hexadecimal number representing the byte value wanted</td></tr>
</table>


<pre class=border>
string str1 = "This is a string with \"escape sequences\".";
string str2 = 'If single quotes are used then double quotes can be included without "escape sequences".';
</pre>


The heredoc strings are designed for inclusion of large portions of text
without processing of escape sequences. A heredoc string is surrounded by
triple double-quotation marks (<code>"""</code>), and can span multiple lines
of code. If the characters following the start of the string until the first
linebreak only contains white space, it is automatically removed by the
compiler. Likewise if the characters following the last line break until the
end of the string only contains white space this is also remove, including the
linebreak.


<pre class=border>
string str = """
This is some text without "escape sequences". This is some text.
This is some text. This is some text. This is some text. This is
some text. This is some text. This is some text. This is some
text. This is some text. This is some text. This is some text.
This is some text.
""";
</pre>

If more than one string constants are written in sequence with only whitespace or
comments between them the compiler will concatenate them into one constant.

<pre class=border>
string str = "First line.\n"
             "Second line.\n"
             "Third line.\n";
</pre>

<sup>1) The application can change the interpretation of single quoted strings by setting an engine property. If this is done the first character in the single quoted string will be interpreted as a single uint8 value instead of a string literal.</sup>


*/