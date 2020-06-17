Types
=====

*Minilang* provides a number of built-in types.

Nil
---

The special built in value :mini:`nil` denotes the absence of any other value. Every variable has the value :mini:`nil` before they are assigned any other value. Likewise, function parameters default to :mini:`nil` if a function is called with too few arguments.

.. note::

   *Minilang* has no boolean values, such as ``true`` or ``false``. Instead, conditional and looping statements treat :mini:`nil` as false and *any* other value as true.  

.. code-block:: mini

   -- Nil
   nil
   
   if nil then
      print("This will not be seen!")
   end
   
   if 0 then
      print("This will be seen!")
   end

.. _comparisons:

In *Minilang*, comparison operators such as :mini:`=`, :mini:`>=`, etc, all return the second argument if the comparison is true, and :mini:`nil` if it is not.

.. code-block:: mini

   1 < 2 -- returns 2
   1 > 2 -- returns nil 

Numbers
-------

Numbers in *Minilang* are either integers (whole numbers) or reals (decimals / floating point numbers).

Integers can be written in standard decimal notation. Reals can be written in standard decimal notation, with either ``e`` or ``E`` to denote an exponent in scientific notation. If a number contains either a decimal point ``.`` or an exponent, then it will be read as a real number, otherwise it will be read as an integer.

They support the standard arithmetic operations, comparison operations and conversion to or from strings.

.. code-block:: mini

   -- Integers
   10
   127
   -1
   
   --Reals
   1.234
   10.
   0.78e-12

.. code-block:: mini

   -- Arithmetic
   1 + 1 -- 2
   2 - 1.5 -- 0.5
   2 * 3 -- 6
   4 / 2 -- 2
   3 / 2 -- 1.5
   3 // 2 -- 1
   
   -- Comparison
   1 < 2 -- 2
   1 <= 2 -- 2
   1 = 1.0 -- 1.0
   1 > 1 -- nil
   1 >= 1 -- 1
   1 != 1 -- nil

Strings
-------

Strings can be written in two ways:

Regular strings are written between double quotes ``"``, and contain regular characters. Special characters such as line breaks, tabs or ANSI escape sequences can be written using an escape sequence ``\n``, ``\t``, etc.

Complex strings are written between single quotes ``'`` and can contain the same characters and escape sequences as regular strings. In addition, they can contain embedded expressions between braces ``{`` and ``}``. At runtime, the expressions in braces are evaluated and converted to strings. To include a left brace ``{`` in a complex string, escape it  ``\{``.

.. code-block:: mini

   -- Regular strings
   "Hello world!"
   "This has a new line\n", "\t"
   
   -- Complex strings
   'The value of x is \'{x}\''
   'L:length = {L:length}\n'

:mini:`String[I]`
   Returns the *I*-th character of *String* as a string of length 1.

:mini:`String[I, J]`
   Returns the sub-string of *String* starting with the *I*-th character up to but excluding the *J*-th character. Negative indices are taken from the end of *String*. If either *I* or *J* it outside the range of *String*, or *I* > *J* then :mini:`nil` is returned.

Regular Expressions
-------------------

Regular expressions can be written as ``r"expression"``, where *expression* is a POSIX compatible regular expression.

.. code-block:: mini

   -- Regular expressions
   r"[0-9]+/[0-9]+/[0-9]+"

Lists
-----

Lists are extendable ordered collections of values, and are created using square brackets, ``[`` and ``]``. A list can contain any value, including other lists, maps, etc.

:mini:`List[I]`
   Returns an assignable reference to the *I*-th element of *List*. If *I* is negative, then the indexing is done from the end *List*, where ``-1`` is the last element. If *I* is outside the bounds of *List*, then :mini:`nil` is returned.
   
:mini:`List[I, J]`
   Returns the sub-list of *List* starting with the *I*-th element up to but excluding the *J*-th element. Negative indices are taken from the end of *List*. If either *I* or *J* it outside the range of *List*, or *I* > *J* then :mini:`nil` is returned.
   
:mini:`List:put(X1, X2, ...)`
   Puts *X1*, *X2*, ... onto the end of *List* and returns *List*.
   
:mini:`List:push(X1, X2, ...)`
   Puts *X1*, *X2*, ... onto the beginning of *List* and returns *List*.
   
:mini:`List:pull`
   Removes and returns the last element of *List*, or :mini:`nil` if *List* is empty.
   
:mini:`List:pop`
   Removes and returns the first element of *List*, or :mini:`nil` if *List* is empty.
   
:mini:`List1 + List2`
   Returns the concatenation of *List1* and *List2*.
   
:mini:`List:length`
   Returns the number of elements in *List*.
   
:mini:`List:string`
   Returns *List* converted to a string.
   
:mini:`List:join(Sep)`
   Returns *List* converted to a list with *Sep* between each element.

:mini:`for Value in List do ... end`
   Iterates through the values in *List*.

:mini:`for Index, Value in List do ... end`
   Iterates through the indices and values in *List*. 

Maps
----

Maps are extendable collections of key-value pairs, which can be indexed by its keys. Maps are created using braces ``{`` and ``}``. Keys can be of any immutable type supporting equality testings (typically numbers and strings), and different types of keys can be mixed in the same map. Each key can only be associated with one value, although values can be any type, including lists, other maps, etc.

:mini:`Map[Key]`
   Returns an assignable reference to the value associated with *Key* in *Map*. If *Key* is not currently in *Map* then returns :mini:`nil`. Assigning to :mini:`Map[Key]` will overwrite the associated value or insert a new association if *Key* is not already in *Map*.
   
 
