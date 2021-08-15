Language
========

*Minilang* has a simple syntax, similar to *Pascal* and *Oberon-2*. Keywords are in lower case, statements are delimited by semicolons ``;``, these can be and are usually omitted at the end of a line.

As *Minilang* is designed to be embedded into larger applications, there is no implicit requirement to store *Minilang* code in source files with any specific extension (or indeed to store *Minilang* in files at all). However, :file:`.mini` is used for the sample and test scripts and there are highlighters created for *GtkSourceview* and *Visual Studio Code* that recognize this extension. 

The following is an example of *Minilang* code showing an
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

Comments
--------

Line comments start with ``:>`` and run until the end of the line.

Block comments start with ``:<`` and end with ``>:`` and can be nested.

.. code-block:: mini

   :> This is a line comment.
   
   :<
      This is a block comment
      spanning multiple lines
      that contains another :< comment >:
   >:

Identifiers and Keywords
------------------------

Identifiers start with a letter or underscore and can contain letters, digits and underscores. *Minilang* is case sensitive.

.. code-block:: mini

   A
   factorial
   _Var
   X1
   X2

The following identifiers are reserved as keywords.

.. code-block:: mini

   if then elseif else end loop while until exit next
   for each to in is when fun ret susp meth with do on
   nil and or not old def let var _


Whitespace and Line Breaks
--------------------------

*Minilang* code consists of declarations (variables and functions) and expressions to evaluate. The bodies of complex expressions such as :mini:`if`, :mini:`for`, etc, can contain multiple declarations and expressions, in any order. Both semicolons ``;`` and line breaks can be used to separate declarations and expressions, however if a line break occurs where a token is required then it will be ignored. Other whitespace (spaces and tabs) have no significance other than to separate tokens or within string literals.

For example the following are equivalent as the semicolons are replaced by line breaks:

.. code-block:: mini

   do print("Hello "); print("world"); end
   
   do
      print("Hello ")
      print("world")
   end

The following are also equivalent as the line break occurs after an infix operator where at least one more token is required to complete the expression:

.. code-block:: mini

   let X := "Hello " + "world"
   
   let X := "Hello " +
      "world"

However the following code is not equivalent to the code above as the line break occurs before the infix operator and hence no token is required to complete the expression: 

.. code-block:: mini

   let X := "Hello "
      + "world"
      
Instead the above code is equivalent to following where semicolons have been added to show the separate declaration and expression (with a prefix operation):

.. code-block:: mini

   let X := "Hello ";
   
   + "world";


Blocks
------

A block in is a group of expressions and declarations. A block returns the result of the last expression in the block. Every block creates a new identifier scope; identifiers declared in a block are not visible outside that block (although they are visible within nested blocks). Some constructs such as the bodies of :mini:`if`-expressions, :mini:`for`-expressions, etc, are always blocks. A :mini:`do`-expression wraps a block into a single expression.

When any code is loaded in *Minilang*, it is implicitly treated as a block.

.. code-block:: mini

   var X := do
      let Y := 7
      print("Y = ", Y, "\n")
      Y - 5
   end

   if X = 2 then
      let Y := 10
      let Z := 11
      print("X = ", X, "\n")
      print("Y = ", Y, "\n")
      print("Z = ", Z, "\n")
   end

.. code-block:: console

   Y = 7
   X = 2
   Y = 10
   Z = 11

The code above has three blocks;

#. the body of :mini:`do`-expression,
#. the :mini:`then`-clause of the :mini:`if`-expression,
#. the top-level block containing the entire code.

The identifier :mini:`X` is declared in the top-level block and so is visible  throughout the code. The identifier :mini:`Y` is declared twice in two separate blocks, each block sees its local definition. Finally, the identifier :mini:`Z` is only declared in the :mini:`then`-block and is only visible there.


Declarations
------------

All identifiers in *Minilang* (other than those provided by the compiler / embedding) must be explicitly declared. Declarations are only visible within their containing block and can be referenced before their actual declaration. This allows (among other things) mutually recursive functions. 

There are 3 types of declaration:

#. :mini:`var Name` binds :mini:`Name` to a new variable with an initial value of :mini:`nil`. Variables can be reassigned using :mini:`Name :=  Expression`. A variable declaration can optionally include an initial expression to evaluate and assign to the variable :mini:`var Name := Expression`, this is equivalent to :mini:`var Name; Name := Expression`.

#. :mini:`let Name := Expression` binds :mini:`Name` to the result of evaluating :mini:`Expression`. :mini:`Name` cannot be reassigned later in the block, hence the intial expression is required.

#. :mini:`def Name := Expression` binds :mini:`Name` to the result of evaluating :mini:`Expression`. Unlike a :mini:`let`-declaration, :mini:`Expression` is evaluated once only when the code is first loaded. Consequently, :mini:`Expression` can only contain references to identifiers that are visible at load time (e.g. global identifiers or other :mini:`def`-declarations).
 
Declarations are visible in nested blocks (including nested functions), unless they are shadowed by another declaration.

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


Function Declarations
~~~~~~~~~~~~~~~~~~~~~

Functions are first class values in *Minilang*, they can be assigned to variables or used to initialize identifiers. For convenience, instead of writing :mini:`let Name := fun(Args...) Body`, we can write :mini:`fun Name(Args...) Body`. For example:

.. code-block:: mini

   fun fact(N) do
      if N < 2 then
         return 1
      else
         return N * fact(N - 1)
      end
   end
   
Note that this shorthand is only for :mini:`let`-declarations, if another type of declaration is required (:mini:`var` or :mini:`def`) then the full declaration must be written.

Compound Declarations
~~~~~~~~~~~~~~~~~~~~~

*Minilang* provides no language support for modules, classes and probably some other useful features. Instead, *Minilang* allows for these features to be implemented as functions provided by the runtime, with evaluation at load time to remove any additional overhead from function calls. *Minilang* provides some syntax sugar constructs to simplify writing these types of declaration. More details can be found in :doc:`features/modules`.

Imports, Classes, etc.
......................

A declaration of the form :mini:`Expression: Name(Args...)` is equivalent to :mini:`def Name := Expression(Args...)`. This type of declaration is useful for declaring imported modules, classes, etc. For example:

.. code-block:: mini

   import: utils("lib/utils.mini")
   
   class: point(:X, :Y)


Exports, etc.
.............

A declaration of the form :mini:`Expression: Declaration` is equivalent to :mini:`Declaration; Expression("Name", Name)` where *Name* is the identifier in the *Declaration*. Any type of declaration (:mini:`var`, :mini:`let`, :mini:`def`, :mini:`fun` or another compound declaration) is allowed. This form is useful for declaring exports. For example:

.. code-block:: mini

   export: fun add(X, Y) X + Y
   
   export: var Z

Compound declarations can be combined. For example, the following code shows how a module which exports a class.

.. code-block:: mini

   import: utils("lib/utils.mini")
   
   export: class: point(:X, :Y)
   
Destructuring Declarations
~~~~~~~~~~~~~~~~~~~~~~~~~~

Multiple identifiers can be declared and initialized with contents of a single aggregrate value (such as a tuple, list, map, module, etc). This avoids the need to declare a temporary identifier to hold the result. There are two forms of destructing declaration. Note that both forms can be used with :mini:`var`, :mini:`let` or :mini:`def`, for brevity only the :mini:`let` forms are shown below.

#. :mini:`let (Name₁, Name₂, ...) := Expression`. Effectively equivalent to the following:

   .. code-block:: mini
   
      let Temp := Expression
      let Name₁ := Temp[1]
      let Name₂ := Temp[2]
   
#. :mini:`let (Name₁, Name₂, ...) in Expression`. Effectively equivalent to the following:

   .. code-block:: mini
   
      let Temp := Expression
      let Name₁ := Temp["Name₁"]
      let Name₂ := Temp["Name₂"]

Expressions
-----------

Other than declarations, everything else in *Minilang* is an expression
(something that can be evaluated).

Literals
~~~~~~~~

The simplest expressions are single values.

Nil
   :mini:`nil`.

Integers
   :mini:`1`, :mini:`-257`. Note that the leading ``-`` is parsed as part of a negative number, so that :mini:`2-1` (with no spaces) will be parsed as ``2 -1`` (and be invalid syntax) and not ``2 - 1``.

Reals
   :mini:`1.2`, :mini:`.13`, :mini:`-1.3e5`.

Strings
   :mini:`"Hello world!\n"`, :mini:`'X = {X}'`. Strings can be written using double quotes or single quotes. Strings written with single quotes can have embedded expressions (between ``{`` and ``}``) and may span multiple lines (the line breaks are embedded in the string).

Regular Expressions
   :mini:`r".*\.c"`, :mini:`ri".*\.c"`. *Minilang* uses `TRE <https://github.com/laurikari/tre/>`_ as its regular expression implementation, the precise syntax supported can be found here `<https://laurikari.net/tre/documentation/regex-syntax/>`_.

Lists
   :mini:`[1, 2, 3]`, :mini:`["a", 1.23, [nil]]`. The values in a list can be of any type including other lists and maps.

Maps
   :mini:`{"a" is 1, 10 is "string"}`. The keys of a map have to be immutable and comparable (e.g. numbers, strings, tuples, etc). The values can be of any type.

Tuples
   :mini:`(1, 2, 3)`, :mini:`("a", 1.23, [nil])`. Like lists, tuples can contain values of any type. Tuple differ from lists by being immutable; once constructed the elements of a tuple cannot be modified. This allows them to be used as keys in maps. They can also be used for destructing assignments, 

Methods
   :mini:`:length`, :mini:`:X`, :mini:`<>`, :mini:`+`, :mini:`:"[]"`. Methods consisting only of the characters ``!``, ``@``, ``#``, ``$``, ``%``, ``^``, ``&``, ``*``, ``-``, ``+``, ``=``, ``|``, ``\\``, ``~``, `````, ``/``, ``?``, ``<``, ``>`` or ``.`` can be written directly without surrounding ``:"`` and ``"``.

Functions
   :mini:`fun(A, B) A + B`. If the last argument to a function or method call is an anonymous function then the following shorthand can be used: :mini:`f(1, 2, fun(A, B) A + B)` can be written as :mini:`f(1, 2; A, B) A + B`. 

Conditional Expressions
~~~~~~~~~~~~~~~~~~~~~~~

The expression :mini:`A and B` returns :mini:`nil` if the value of :mini:`A` is :mini:`nil`, otherwise it returns the value of :mini:`B`.

The expression :mini:`A or B` returns :mini:`A` if the value of :mini:`A` is not :mini:`nil`, otherwise it returns the value of :mini:`B`.

.. note::

   Both :mini:`and`-expressions and :mini:`or`-expressions only evaluate their second expression if required.

The expression :mini:`not A` returns :mini:`nil` if the value of :mini:`A` is not :mini:`nil`, otherwise it returns :mini:`some` (a value whose only notable property is being different to :mini:`nil`).

If Expressions
~~~~~~~~~~~~~~

The :mini:`if`-expression, :mini:`if ... then ... else ... end` evaluates each condition until one has a value other than :mini:`nil` and returns the value of the selected branch. For example:

.. code-block:: mini

   var X := 1
   print(if X % 2 = 0 then "even" else "odd" end, "\n")

will print ``odd``.

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

The :mini:`else`-clause is optional, if omitted and every condition evaluates to :mini:`nil` then the :mini:`if`-expression returns :mini:`nil`.

Switch Expressions
~~~~~~~~~~~~~~~~~~

There are two types of :mini:`switch`-expression in *Minilang*. The basic :mini:`switch`-expression chooses a branch based on an integer value, with cases corresponding to ``0, 1, 2, ...``, etc. 
   
.. code-block:: mini
   
   switch Expression
   case
      Block
   case
      Block
   else
      Block
   end
   
When evaluated, :mini:`Expression` must evaluate to a non-negative integer, ``n``. The ``n + 1``-th :mini:`case` block is then evaluated if present, otherwise the :mini:`else` block option is evaluated. If no else block is present, then the value :mini:`nil` is used.

The general :mini:`switch`-expression selects a branch corresponding to a value based on a specific *switch provider*.

.. code-block:: mini

   switch Expression: Provider
   case Expressions do
      Block
   case Expressions do
      Block
   else
      Block
   end

*Minilang* includes switch providers for several basic types including numbers, strings and types. More details can be found in :doc:`/features/switch`.

Loop Expressions
~~~~~~~~~~~~~~~~

A :mini:`loop`-expression, :mini:`loop ... end` evaluates its code repeatedly until an :mini:`exit`-expression is evaluated: :mini:`exit Value` exits a loop and returns the given value as the value of the loop. The value can be omitted, in which case the loop evaluates to :mini:`nil`.

.. code-block:: mini

   var I := 1
   print('Found fizzbuzz at I = {loop
      if I % 3 = 0 and I % 5 = 0 then
         exit I
      end
      I := I + 1
   end}\n')


A :mini:`next`-expression jumps to the start of the next iteration of the loop.

If an expression is passed to :mini:`exit`, it is evaluated outside the loop. This allows control of nested loops, for example: :mini:`exit exit Value` or :mini:`exit next`.

A :mini:`while`-expression, :mini:`while Expression`, is equivalent to :mini:`if not Expression then exit end`. Similarly, an :mini:`until`-expression, :mini:`until Expression`, is equivalent to :mini:`if Expression then exit end`. An exit value can be specified using :mini:`while Expression, Value` or :mini:`until Expression, Value`.

For Expressions
~~~~~~~~~~~~~~~

The for expression, :mini:`for Value in Collection do ... end` is used to iterate through a collection of values.

.. code-block:: mini

   for X in [1, 2, 3, 4, 5] do
      print('X = {X}\n')
   end

If the collection has a key associated with each value, then a second variable can be added, :mini:`for Key, Value in Collection do ... end`. When iterating through a list, the index of each value is used as the key.

.. code-block:: mini

   for Key, Value in {"a" is 1, "b" is 2, "c" is 3} do
      print('{Key} -> {Value}\n')
   end

For loops can also use destructing assignments to simplify iterating over collections of tuples, lists, etc.

.. code-block:: mini

   for Key, (First, Second) in {"a" is (1, 10), "b" is (2, 20), "c" is (3, 30)} do
      print('{Key} -> {First}, {Second}\n')
   end

A for loop is also an expression (like most things in *Minilang*), and can return a value using :mini:`exit`, :mini:`while` or :mini:`until`. Unlike a basic loop expression, a for loop can also end when it runs out of values. In this case, the value of the for loop is :mini:`nil`. An optional :mini:`else` clause can be added to the for loop to give a different value in this case.

.. code-block:: mini

   var L := [1, 2, 3, 4, 5]
   
   print('Index of 3 is {for I, X in L do until X = 3, I end}\n')
   print('Index of 6 is {for I, X in L do until X = 6, I end}\n')
   print('Index of 6 is {for I, X in L do until X = 6, I else "not found" end}\n')

.. code-block:: console

   Index of 3 is 3
   Index of 6 is
   Index of 6 is not found
   
Iteratables
...........

For loops are not restricted to using lists and maps. Any value can be used in a for loop if it is iteratable, i.e. can generate a sequence of values (or key / value pairs for the two variable version).

In order to loop over a range of numbers, *Minilang* has a range type, created using the :mini:`..` operator.

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

The default step size is :mini:`1` but can be changed using the :mini:`:by` method.

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

Functions
~~~~~~~~~

.. _minilang/functions:

Functions in *Minilang* are first class values. That means they can be passed to other functions and stored in variables, lists, maps, etc. Functions have access to variables in their surrounding scope when they were created.

   The general syntax of a function is :mini:`fun(Name₁, Name₂, ...) Expression`. Calling a function is achieved by the traditional syntax :mini:`Function(Expression, Expression, ...)`. 

.. code-block:: mini

   let add := fun(A, B) A + B
   let sub := fun(A, B) A - B
   
   print('add(2, 3) = {add(2, 3)}\n')
   
.. code-block:: console

   add(2, 3) = 5

Note that :mini:`Function` can be a variable containing a function, or any
expression which returns a function.

.. code-block:: mini

   var X := (if nil then add else sub end)(10, 3) :> 7
   
   let f := fun(A) fun(B) A + B
   
   var Y := f(2)(3) :> 5

As a shorthand, the code :mini:`var Name := fun(Name₁, Name₂, ...) Expression` can be written
as :mini:`fun Name(Name₁, Name₂, ...) Expression`. Internally, the two forms are identical.

.. code-block:: mini

   fun add(A, B) A + B

Although a function contains a single expression, this expression can be a block expression, :mini:`do ... end`. A block can contain any number of declarations and expressions, which are evaluated in sequence. The last value evaluated is returned as the value of the block. A return expression, :mini:`ret Expression`, returns the value of :mini:`Expression` from the enclosing function. If :mini:`Expression` is omitted, then :mini:`nil` is returned.  

.. code-block:: mini

   fun fact(N) do
      var F := 1
      for I in 1 .. N do
         F := F * I
      end
      ret F
   end 

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

Generators
..........

*Minilang* functions can be used as generators using suspend expressions, :mini:`susp Key, Value`. If :mini:`Key` is omitted, :mini:`nil` is used as the key. The function must return :mini:`nil` when it has no more values to produce.

.. code-block:: mini

   fun squares(N) do
      for I in 1 .. N do
         susp I, I * I
      end
      ret nil
   end
   
   for I, S in squares(10) do
      print('I = {I}, I² = {S}\n')
   end

.. code-block:: console

   I = 1, I² = 1
   I = 2, I² = 4
   I = 3, I² = 9
   I = 4, I² = 16
   I = 5, I² = 25
   I = 6, I² = 36
   I = 7, I² = 49
   I = 8, I² = 64
   I = 9, I² = 81
   I = 10, I² = 100

Types
~~~~~

Every value in *Minilang* has an associated type. The type of a value can be obtained by calling :mini:`type(Value)`.

.. code-block:: mini

   print(type(10), "\n")
   print(type("Hello"), "\n")
   print(type(integer), "\n")
   print(type(type), "\n")

.. code-block:: console

   <<integer>>
   <<string>>
   <<type>>
   <<type>>

Types are displayed as their names enclosed between `<<` and `>>`. Note that :mini:`type` is itself a type (whose type is itself, :mini:`type`). Most types can be called as functions which return instances of that type based on the arguments passed.

For example:

:mini:`boolean(X)`, :mini:`integer(X)`, :mini:`real(X)`, :mini:`number(X)`, :mini:`string(X)`, :mini:`regex(X)`
   Convert :mini:`X` to an integer, real, number (integer or real), string or regular expression respectively.

:mini:`list(X)`, :mini:`map(X)`
   These expect :mini:`X` to be iteratable and the values (and keys) produced by :mini:`X` into a list or map respectively.

:mini:`tuple(X₁, X₂, ...)`
   Constructs a new tuple with values :mini:`X₁, X₂, ...`.

:mini:`method(X)`, :mini:`method()`
   Returns the (unique) method with name :mini:`X`. If no name is passed then a completely new anonymous method is returned.

:mini:`type(X)`
   Returns the type of :mini:`X`. 

:mini:`stringbuffer()`
   Returns a new stringbuffer.

Generics
........

*Minilang* can optionally be built with support for generic types such as :mini:`list[integer]`, :mini:`map[string, tuple[string, number]]`, etc. More details can be found in :doc:`/features/generics`.

Classes
~~~~~~~

If selected when embedding *Minilang*, user defined types can be created using the :mini:`class` type.

:mini:`class(Arg₁, Arg₂, ...)`
   Creates a new class with additional properties based on the types of :mini:`Arg₁, Arg₂, ...`:

   :mini:`string`
      Sets the name of the class. If omitted, an anonymous name (of the form :mini:`"object:7f501a8c5f30"`) is used. The name is used for debugging and display purposes only.
   
   :mini:`class`
      Adds a parent class. Multiple parent classes are allowed.
   
   :mini:`method`
      Adds a field. Instances of this class will have space allocated for all fields, fields cannot be added or removed from instances later. Fields are accessed using the associated method.
      
   :mini:`Name is Value`
      Named arguments add shared values to the class. If :mini:`Class` is a class, then :mini:`Class::Name` will return the shared value called *Name*.

Certain shared values have special meaning. If :mini:`c` is a class, then:

* The name :mini:`c::new` is always set to a function equivalent to the following:
  
  .. code-block:: mini
  
   fun(Arg₁, Arg₂, ...) do
      let Instance := new instance of c
      c::init(Instance, Arg₁, Arg₂, ...)
      ret Instance
   end
  
This cannot be overridden, if *new* is passed as a named argument to :mini:`class()`, it is ignored.

* The value of :mini:`c::init` is used as the initializer and should be a callable value (function, method, etc). This value is called by :mini:`c::new` to initialize a new instance :mini:`c` with the given arguments.

  If *init* is not set, a default initializer is set which assigns positional arguments to the instance fields in order. Any named arguments are assigned to the corresponding field by name. 

* The value of :mini:`c::of` is used as the constructor and should be a callable value (function, method, etc). This value is called when the class is called as a function, i.e. :mini:`c(...)` is equivalent to :mini:`c::of(...)`. 

  If *of* is not set, a default constructor is set which simply calls :mini:`c::new`.

Methods
~~~~~~~

Methods are first class objects in *Minilang*. They can be created using a colon ``:`` followed by one or more alphanumeric characters, or any combination of characters surrounded by quotes.

Methods consisting of only the characters ``!``, ``@``, ``#``, ``$``, ``%``, ``^``, ``&``, ``*``, ``-``, ``+``, ``=``, ``|``, ``\``, ``~``, `````, ``/``, ``?``, ``<``, ``>`` or ``.`` can be written directly, without any leading ``:`` or quotes.

Methods behave as *atoms*, that is two methods with the same characters internally point to the same object, and are thus identically equal. 

.. code-block:: mini

   :put
   :write
   :"write" :> same as previous method
   :"do+struff"
   +
   <>

Methods provide type-dependant function calls. Each method is effectively a mapping from lists of types to functions. When called with arguments, a method looks through its entries for the best match based on the types of *all* of the arguments and calls the corresponding function.

.. code-block:: mini

   var L := []
   :put(L, 1, 2, 3)
   print('L = {L}\n')

.. code-block:: console

   L = 1 2 3

For convenience (i.e. similarity to other OOP languages), method calls can also
be written with their first argument before the method. Thus the code above is
equivalent to the following:

.. code-block:: mini

   var L := []
   L:put(1, 2, 3)
   print('L = {L}\n')

Methods with only symbol characters or that are valid identifiers can be invoked using infix notation. The following are equivalent:

.. code-block:: mini

   +(A, B)
   A + B
   
   +(A, *(B, C))
   A + (B * C)
   
   list(1 .. 10 limit 5)
   list(:limit(..(1, 10), 5))

.. warning::

   *Minilang* allows any combination of symbol characters (listed above) as well as any identifier to be used as an infix operator. As a result, there is no operator precedence in *Minilang*. Hence, the parentheses in the last example are required; the expression :mini:`A + B * C` will be evaluated as :mini:`(A + B) * C`.

