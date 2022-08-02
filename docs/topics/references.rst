References
==========

Like most interpreted languages, there are several ways to create mutable values in *Minilang*. These include variables, list and map elements, object fields, etc. Some interpreted languages treat each of these as different forms of assignment, so

* :mini:`Local := Value` is treated as :mini:`set_local(Local, Value)`,
* :mini:`List[Index] := Value` is treated as :mini:`set_index(List, Index, Value)`,
* :mini:`Object:field := Value` is treated as :mini:`set_field(Object, :field, Value)`,
* etc.

Instead *Minilang* expects operations such as element or field access to return assignable references. This means that

* :mini:`Local := Value` is treated as :mini:`assign(Local, Value)`,
* :mini:`List[Index] := Value` is treated as :mini:`assign(List[Index], Value)`,
* :mini:`Object:field := Value` is treated as :mini:`assign(Object:field, Value)`,
* etc.

Each type of reference has an internal method that defines assignment to the reference.

This approach has the advantage that any expression that returns a reference can be assigned to, including function calls, conditional or loops expressions, etc. The disadvantage (internally) is that when not used for assignment, the current value of a reference needs to be extracted as an extra step. This is handled automatically within *Minilang*.

Using the old value
-------------------

*Minilang* does not provide augmented assignments such as :mini:`X += Y`, etc. Instead the keyword :mini:`old` can be used in the right hand expression of an assignment to refer to the current value of the target reference. Note that even if the target of an assignment is a complex expression, it is only evaluated once.

.. code-block:: mini

   var X := 10
   print('X = {X}\n')
   X := old + 5
   print('X = {X}\n')

.. code-block:: console

   X = 10
   X = 15

:mini:`old` can be used multiple times, and in any position, allowing for more flexible updates to values.

.. code-block:: mini

   var L := [1, 2, 3]
   print('L = {L}\n')
   L[2] := old + (old * old)
   print('L = {L}\n')

.. code-block:: console

   L = [1, 2, 3]
   L = [1, 6, 3]

References in closures
----------------------

Passing arguments by reference
------------------------------

Creating references
-------------------
