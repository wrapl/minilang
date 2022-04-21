Types
=====

Each value in *Minilang* has a type which can be determined at runtime. The types of values are used for deciding how  operations and methods behave on those values. The type of a value can be obtained using :mini:`type(Value)`. Types are usually displayed as :samp:`<<{NAME}>>`.

.. code-block:: mini

   print(type(1))
   print(type(1.0))
   print(type("Hello world"))
   print(type([]))

.. code-block:: console

   <<int32>>
   <<double>>
   <<string>>
   <<list>>

Types are values themselves, and can be stored in variables or passed to functions. The type of a type is :mini:`type`.

.. code-block:: mini

   print(type)
   print(type(string))
   print(type(type))

.. code-block:: console

   <<type>>
   <<type>>
   <<type>>

Types are Functions
-------------------

This reveals an important feature of *Minilang*; types can be used as functions. Calling a type as a function is the *Minilang* way of constructing values of a given type.

.. code-block:: mini

   let S := string(1234)
   print('{S} -> {type(S)}\n')

.. code-block:: console

   1234 -> <<string>>

Type Inheritance
----------------

Each type in *Minilang* can optionally have parent types. Values of a type inherit behaviour from its parent types, unless that behaviour is overridden in the child type. The set of all types in *Minilang* forms a tree under inheritance. A subset of that tree is shown in the :doc:`/library/hierarchy`.

The parents of a type can be obtained using :mini:`Type:parents`. The result is returned as a list in no particular order.

.. code-block:: mini

   print(type(1))
   print(type(1):parents)

.. code-block:: console

   <<int32>>
   [<<number>>, <<integer>>, <<real>>, <<function>>, <<complex>>]

Types have Exports
------------------

Types in *Minilang* can also have named exports, accessible as :mini:`Type::Name`. This is used to collect related values by type, and allows types to be used as modules (when *Minilang* is built with module support).

.. code-block:: mini

   print(real::random(10))
   print(real::Inf)

.. code-block:: console

   5.084
   inf

Generic Types
-------------

*Minilang* can be built with support for generic types. These are types that are parameterised by other types, along with rules for determining whether one generic type is a parent type of another.


