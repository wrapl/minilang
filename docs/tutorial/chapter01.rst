Chapter 1
=========

Introduction
------------

Minilang is a dynamically typed interpreted scripting language, designed to be embedded into larger applications in order to allow application users to run their own code in a safe and controlled manner. This can be to customize or extend the application, or as the main feature of the application (e.g. a serverless development platform). Minilang code can also be run directly by the standalone interpreter, which can used to create scripts or desktop applications.

Here's some sample *Minilang* code.

.. tryit::

	2
	3.14
	"Hello world!"

	[1, 2, 3]
	{"a" is 1, "b" is 2, "c" is 3}

Values
------

Nil
...

Minilang has a dedicated value, :mini:`nil`, for representing the absence of any other value. Variables and parameters that are not assigned any other value will have a value of :mini:`nil`. :mini:`nil` is also returned from operations and methods which do not generate a result or generate a "negative" result such as accessing a missing key in a map, indexing outside the range of a list and comparison operations that are not satisfied.

.. tryit::

	nil
	{"a" is 1, "b" is 2, "c" is 3}["d"]
	[1, 2, 3][4]
	1 > 2

Numbers
.......

Numeric values (integers and real numbers) follow the usual syntax.

.. tryit::

	2
	1000
	-50
	3.14159
	1.23e45

Minilang provides the usual infix and prefix arithmetic operators.

.. note::

	Infix operations in Minilang do not have any precedence rules, i.e. all infix operations are evaluated from left to right, except that operations in parentheses are evaluated first.

.. tryit::

	2 + 3
	4 * 7

	2 + 3 * 4 :> evaluates as (2 + 3) * 4 = 20
	2 + (3 * 4) :> evalutes as 2 + (3 * 4) = 14

	1 / 2
	3 ^ 3
	

Strings
.......

Strings values are written between double quotes, :mini:`"..."`. Non-printable characters can be written using *escape sequences*, ``\`` followed by one or more characters.

.. tryit::

	"Hello world!"
	"This is a tab\tand a new line\n"
	"Unicode characters can be written directly: ☺"
	"or with escape sequences: \u263A"

Minilang provides operators and methods for working with strings.

.. note::

	In Minilang, string indexing (as well as indexing lists, arrays, etc) begins at :mini:`1`. In addition, :mini:`-1` refers to the last index, with :mini:`0` referring to just beyond the last index, useful for slicing strings and lists.

.. tryit::

	"Hello " + "world!"
	"Hello world!"[1, 6]
	"Hello world!"[7, -1]
	"Hello world!"[12, 0]

	"Hello world!":upper
	"Hello world!":lower

	"Hello world!":find("world")
	"Hello world!":find("Goodbye")

Variables
---------

There are 4 ways to declare variables in Minilang using different keywords, :mini:`let`, :mini:`var`, :mini:`def` and :mini:`ref`. The last 2 of these are not commonly used so will be explained in later chapters. In general, the same identifier can not be used to declare more than one variable within the same block / scope, but can be used in nested blocks / scopes. As a special exception to this rule, interactive environments, including the embedded code blocks in this tutorial, often allow the same identifier to be redeclared, overwriting the previous declaration.  

:mini:`let` declarations
........................

A variable declared with :mini:`let Name := <expression>` has the value of :mini:`<expression>` for the rest of the block / scope. Such variables can not be reassigned. This is the preferred way to declare variables in Minilang.

.. note::

	If the result of :mini:`<expression>` has mutable contents like a list, map, etc, the contents can still be modified even if :mini:`let` is used. Is it only the variable that can not be reassigned. 

.. tryit::

	let X := 1 + 2
	X
	X + 3
	X := 4

.. note::

	The error message in the last example, ``TypeError: <integer> is not assignable``, is due to the left-hand side of the assignment, :mini:`X`, evaluating directly to :mini:`3`, which can not be assigned a new value. The full details of how references and assignments work in Minilang will be explained in later chapters.

:mini:`var` declarations
........................

If it necessary to modified the value of a variable itself over its lifetime, the variable must be declared using :mini:`var Name := <expression>`. In this case, :mini:`<expression>` can be omitted, in which case the value :mini:`nil` is used instead.

.. tryit::

	var Y := 1 + 2
	Y
	Y + 3
	Y := 4
	Y + 3

	var Z
	Z
	Z := 10
	Z

Blocks
------
 
Every construct in Minilang is an expression, that is they evaluate to a value. This includes :mini:`if`-expressions, :mini:`switch`-expressions and even :mini:`for`-expressions and :mini:`loop`-expressions. These will be covered individually later, in general these expressions use blocks of code in their bodies / branches. A block of code in Minilang is simply a number of declarations and expressions. After evaluating each expression in the block, the result of the last expression is returned as the value of the block.

If required, a block can be introduced whenever a single expression is expected by using :mini:`do ... end`.

.. tryit::

	print(do
		let X := "Hello"
		X + " world!"
	end)

Expressions and declarations within a block can also be separated by semi-colons :mini:`;`. The example above can be rewritten as:

.. tryit::

	print(do let X := "Hello"; X + " world!" end)

Error handling
..............

When an error occurs in Minilang code, an :mini:`error` value is created. Execution then jumps to the nearest surrounding error handler, passing this error value. If there is no error handler within the current function, the error is returned to the calling function which then jumps to the nearest surrounding error handler and so on. When run from the command line, an unhandled error will cause Minilang to terminate, displaying information about the error in the console. Interactive sessions such as the sample code blocks in this tutorial will display the error but still allow more code to be evaluated.

.. tryit::

	1 + "a"
	
	1 / 0

