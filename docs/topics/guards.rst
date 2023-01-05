Guards
======

Simple Guards
-------------

:mini:`nil` is used throughout *Minilang* to denote the absence of any other value. This includes the initial values of variables or object fields before assignment, indexing a :mini:`map` or :mini:`list` with a missing key or index and the result of binary comparisons which are logically false.

.. code-block:: mini

   var X
   X :> nil
   let L := [1, 2, 3]
   L[4] :> nil
   let M := {"A" is 1, "B" is 2}
   M["C"] :> nil
   2 < 1 :> nil

As a result, it is often required to check the result of an expression for :mini:`nil` before using the result in a function call.

.. code-block:: mini

   let M := {"A" is [1, 2], "B" is [3, 4]}
   M["A"][1] :> 1
   M["C"][1] :> MethodError: no method found for [](nil, int32)

   if let T := M["A"] then T[1] end :> 1
   if let T := M["C"] then T[1] end :> nil

*Minilang* provides a shorthand for this type of check using *guarded* arguments. A simple guarded argument consists of an empty pair of braces :mini:`{}` following an expression in a function call.

.. parser-rule-diagram:: expression '{' '}'

If the expression in a simple guarded argument evaluates to :mini:`nil` then the **innermost** function call containing the guarded argument evaluates immediately to :mini:`nil` without invoking the called function.

.. code-block:: mini

   let M := {"A" is [1, 2], "B" is [3, 4]}
   M["A"]{}[1] :> 1
   M["C"]{}[1] :> nil

General Guards
--------------

Other than simply checking for :mini:`nil`, guarded arguments can also check their preceeding expression for arbitrarily complex conditions by including another expression inside the braces. Within the braces, the keyword :mini:`it` refers to the value of the preceeding expression.

.. parser-rule-diagram:: expression '{' expression '}'

.. code-block:: mini

   for I in 1 .. 10 do
      print("I = ", I{2 | it}, "\n")
   end

.. code-block:: console

   I = 2
   I = 4
   I = 6
   I = 8
   I = 10