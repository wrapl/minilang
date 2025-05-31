Minilang Basics
===============

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

Minilang has a dedicated value, :mini:`nil`, for representing the absence of any other value. Variables and parameters that are not assigned any other value will have a value of :mini:`nil`.

.. tryit::

	nil

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
