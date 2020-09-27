Language
========

*Minilang* has a simple syntax, not too different from *Pascal* (although
probably closer to *Oberon-2*). Keywords are in lower case, statements are
delimited by semicolons ``;``, these can be and are usually omitted at the end
of a line. The following is an example of *Minilang* code showing an
implementation of the Fibonacci numbers.

.. code-block:: mini

   fun fibonacci(N) do
      if N <= 0 then
         error("RangeError", "N must be postive")
      elseif N <= 2 then
         ret 1
      else
         var A := 1, B := 1
         for I in 2 .. (N - 1) do
            var C := A + B
            A := B
            B := C
         end
         ret B
      end
   end
   
   for I in 1 .. 10 do
      print('fibonacci({I}) = {fibonacci(I)}\n')
   end
   
   print('fibonacci({0}) = {fibonacci(0)}\n')

This produces the following output:

.. code-block:: console

   fibonacci(1) = 1
   fibonacci(2) = 1
   fibonacci(3) = 2
   fibonacci(4) = 3
   fibonacci(5) = 5
   fibonacci(6) = 8
   fibonacci(7) = 13
   fibonacci(8) = 21
   fibonacci(9) = 34
   fibonacci(10) = 55
   Error: N must be postive
      test/test12.mini:3
      test/test12.mini:20

Declarations
------------

There are 3 types of declaration in *Minilang*:

#. Variable declarations using :mini:`var Name := Expression`. Variables can
   be reassigned using :mini:`Name :=  Expression`. Here :mini:`:= Expression`
   may be omitted in the declaration in which case initial value of
   :mini:`Name` will be :mini:`nil`.
#. Value declarations using :mini:`let Name := Expression`. Values cannot be
   reassigned and hence :mini:`:= Expression` is required. :mini:`Expression`
   is evaluated once during execution. 
#. Constant declarations using :mini:`def Name := Expression`. Constants
   cannot be reassigned and hence :mini:`:= Expression` is required. Unlike
   values declared with :mini:`let`, constants declared with :mini:`def` are
   evaluated at compile time and inlined into the rest of the code.
   Consequently, :mini:`Expression` can only contain constant values such as
   numbers, strings, function calls as well as references to other constants.
 
All identifiers in *Minilang* are visible within their scope and any nested
scopes, including nested functions, unless they are shadowed by another
declaration.

.. code-block:: mini

   print('Y = {Y}\n') :> Y is nil here
   
   var Y := 1 + 2
   
   print('Y = {Y}\n') :> Y is 3 here
   
   var X
   
   do
      X := 1 :> Sets X in surrounding scope
   end
   
   print('X = {X}\n')
   
   do
      var X :> Shadows declaration of X 
      X := 2 :> Assigns to X in the previous line
      print('X = {X}\n')
   end
   
   print('X = {X}\n')

.. code-block:: console

   Y =
   Y = 3 
   X = 1
   X = 2
   X = 1

For convenience, functions can declared using the following syntax:

.. code-block:: mini

   fun add(X, Y) X + Y

This is equivalent to writing

.. code-block:: mini

   let add := fun(X, Y) X + Y

Functions themselves are described in :ref:`minilang/functions`.

Expressions
-----------

Other than declarations, everything else in *Minilang* is an expression
(something that can be evaluated).

Values
~~~~~~

The simplest expressions are single values. More information on values in
*Minilang* can be found in :doc:`/minilang/types`.

Nil:
   :mini:`nil`.
Integers:
   :mini:`1`, :mini:`-257`. Note that the leading ``-`` is parsed as part of
   a negative number, so that :mini:`2-1` (with no spaces) will be parsed as
   ``2 -1`` (and be invalid syntax) and not ``2 - 1``.
Reals:
   :mini:`1.2`, :mini:`.13`, :mini:`-1.3e5`.
Strings:
   :mini:`"Hello world!\n"`, :mini:`'X = {X}'`. Strings can be written using
   double quotes or single quotes. Strings written with single quotes can have
   embedded expressions (between ``{`` and ``}``) and may span multiple lines.
Regular Expressions:
   :mini:`r".*\.c"`.
Lists:
   :mini:`[1, 2, 3]`, :mini:`["a", 1.23, [nil]]`. The values in a list can be
   of any type including other lists and maps.
Maps:
   :mini:`{"a" is 1, 10 is "string"}`. The keys of a map have to be immutable
   and comparable (e.g. numbers and strings). The values can be of any type.
Tuples:
   :mini:`(1, 2, 3)`, :mini:`("a", 1.23, [nil])`. Like lists, tuples can
   contain values of any type. Tuple differ from lists by being immutable; 
   once constructed the elements of a tuple cannot be modified. 
Booleans:
   :mini:`true` and :mini:`false`.
Methods:
   :mini:`:length`, :mini:`:X`, :mini:`<>`, :mini:`+`, :mini:`:"[]"`. Methods consisting only of the characters ``!``, ``@``, ``#``, ``$``, ``%``, ``^``, ``&``, ``*``, ``-``, ``+``, ``=``, ``|``, ``\\``, ``~``, `````, ``/``, ``?``, ``<``, ``>`` or ``.`` can be written directly without surrounding ``:"`` and ``"``.

.. _minilang/functions:

Functions
~~~~~~~~~

Functions in *Minilang* are first class values. That means they can be passed
to other functions and stored in variables, lists, maps, etc. Functions have
access to variables in their surrounding scope when they were created.

The general syntax of a function is :mini:`fun(Arguments) Body`. Calling a
function is achieved by the traditional syntax :mini:`Function(Arguments)`. 

.. code-block:: mini

   var add := fun(A, B) A + B
   var sub := fun(A, B) A - B
   
   print('add(2, 3) = {add(2, 3)}\n')
   
.. code-block:: console

   add(2, 3) = 5

Note that :mini:`Function` can be a variable containing a function, or any
expression which returns a function.

.. code-block:: mini

   var X := (if nil then add else sub end)(10, 3) :> 7
   
   var f := fun(A) fun(B) A + B
   
   var Y := f(2)(3) :> 5

As a shorthand, the code :mini:`var Name := fun(Arguments) Body` can be written
as :mini:`fun Name(Arguments) Body`. Internally, the two forms are identical.

.. code-block:: mini

   fun add(A, B) A + B

The body of a function can be a block :mini:`do ... end` containing local
variables and other expressions.

When calling a function which expects another function as its last parameter,
the following shorthand can be used:

.. code-block:: mini

   f(1, 2, fun(A, B) do
      ret A + B
   end)

can be written as

.. code-block:: mini

   f(1, 2; A, B) do
      ret A + B
   end

If Expressions
~~~~~~~~~~~~~~

The basic :mini:`if ... then ... else ... end` expression in *Minilang* returns
the value of the selected branch. For example:

.. code-block:: mini

   var X := 1
   print(if X % 2 = 0 then "even" else "odd" end, "\n")

will print ``even``.

Multiple conditions can be included using :mini:`elseif`.

.. code-block:: mini

   for I in 1 .. 100 do
      if I % 3 = 0 and I % 5 = 0 then
         print("fizzbuzz\n")
      elseif I % 3 = 0 then
         print("fizz\n")
      elseif I % 5 = 0 then
         print("buzz\n")
      else
         print(I, "\n")
      end
   end

Loop Expressions
~~~~~~~~~~~~~~~~

*Minilang* provides a simple looping expression, :mini:`loop ... end`. This
keeps evaluating the code inside indefinitely. The expression
:mini:`exit <value>` exits a loop and returns the given value as the value of
the loop. The value can be omitted, in which case the loop evaluates to
:mini:`nil`.

.. code-block:: mini

   var I := 1
   print('Found fizzbuzz at I = {loop
      if I % 3 = 0 and I % 5 = 0 then
         exit I
      end
      I := I + 1
   end}\n')


The keyword :mini:`next` jumps to the start of the next iteration of the loop.

Note that if an expression is passed to :mini:`exit`, it is evaluated outside
the loop. This allows control of nested loops by writing code like
:mini:`exit exit Value` or :mini:`exit next`.

For Expressions
~~~~~~~~~~~~~~~

The for expression, :mini:`for Value in Collection do ... end` is used to
iterate through a collection of values.

.. code-block:: mini

   for X in [1, 2, 3, 4, 5] do
      print('X = {X}\n')
   end

If the collection has a key associated with each value, then a second variable
can be added, :mini:`for Key, Value in Collection do ... end`. When iterating
through a list, the index of each value is used as the key.

.. code-block:: mini

   for Key, Value in {"a" is 1, "b" is 2, "c" is 3} do
      print('{Key} -> {Value}\n')
   end

A for loop is also an expression (like most things in *Minilang*), and can
return a value using :mini:`exit`. Unlike a basic loop expression in
*Minilang*, a for loop can also end when it runs out of values. In this case,
the value of the for loop is :mini:`nil`. An optional :mini:`else` clause can
be added to the for loop to give a different value in this case.

.. code-block:: mini

   var L := [1, 2, 3, 4, 5]
   
   print('Index of 3 is {for I, X in L do if X = 3 then exit I end end}\n')
   print('Index of 6 is {for I, X in L do if X = 6 then exit I end end}\n')
   print('Index of 6 is {for I, X in L do if X = 6 then exit I end else "not found" end}\n')

.. code-block:: console

   Index of 3 is 3
   Index of 6 is
   Index of 6 is not found
   
Generators
..........

For loops are not restricted to using lists and maps. Any value can be used in a
for loop if it can generate a sequence of values (or key / value pairs for the
two variable version).

In order to loop over a range of numbers, *Minilang* has a range type, created
using the :mini:`..` operator.

.. code-block:: mini

   for X in 1 .. 5 do
      print('X = {X}\n')
   end

::

   X = 1
   X = 2
   X = 3
   X = 4
   X = 5

The default step size is :mini:`1` but can be changed using the :mini:`:by`
method.

.. code-block:: mini

   for X in 1 .. 10 by 2 do
      print('X = {X}\n')
   end

::

   X = 1
   X = 3
   X = 5
   X = 7
   X = 9

Methods
~~~~~~~

Internally, *Minilang* treats every value as an object with methods defining
their behaviour. More information can be found in :doc:`/minilang/oop`. Method
names are first class objects in *Minilang*, and can be created using a colon
``:`` followed by one or more alphanumeric characters, or by using two colons
``::`` followed by one or more symbol characters (``!``, ``@``, ``#``, ``$``,
``%``, ``^``, ``&``, ``*``, ``-``, ``+``, ``=``, ``|``, ``\``, ``~``, `````,
``/``, ``?``, ``<``, ``>`` or ``.``). Note that is currently not possible to
mix the two sets of characters in a method name.

.. code-block:: mini

   :put
   ::+

Methods behave as *atoms*, that is two methods with the same characters
internally point to the same object, and are thus identically equal.
 
Methods can be called like any other function, using parentheses after the
method.

.. code-block:: mini

   var L := []
   :put(L, 1, 2, 3)
   print('L = {L}\n')

.. code-block:: console

   L = 1 2 3

For convenience (i.e. similarity to other OOP languages), method calls can also
be written with the first argument before the method. Thus the code above is
equivalent to the following:

.. code-block:: mini

   var L := []
   L:put(1, 2, 3)
   print('L = {L}\n')

Finally, methods with symbol characters only can be invoked using infix
notation. The following are equivalent:

.. code-block:: mini

   ::+(A, B)
   A + B
   
   ::+(A, ::*(B, C))
   A + (B * C)

.. warning::

   *Minilang* allows any combination of symbol characters (listed above) to be
   used as an infix operator. This means there is no operator precedence in 
   *Minilang*. Hence, the parentheses in the last example are required; the 
   expression :mini:`A + B * C` will be evaluated as :mini:`(A + B) * C`.

