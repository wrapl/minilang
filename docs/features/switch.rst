Switch Expressions
==================

The :mini:`switch`-expression in *Minilang* provides an alternative to an :mini:`if`-expression with multiple :mini:`elseif` branches when a single value is tested for the first match. For example:

.. code-block:: mini

   fun test(X) do
      if X = 1 then
         "one"
      elseif 2 <= X <= 3 then
         "few"
      elseif 4 <= X <= 7 then
         "several"
      else
         "many"
      end
   end
   
   fun test2(X) do
      switch X: integer
      case 1 do
         "one"
      case 2, 3 do
         "few"
      case 4 .. 7 do
         "several"
      else
         "many"
      end
   end

There are a few differences, a :mini:`switch`-expression needs a *switch provider*, in this case :mini:`integer`. The provider of a switch determines how *Minilang* treats the switch values to find a matching branch. A :mini:`switch`-expression also allows multiple values per-branch, and overall is faster than the corresponding :mini:`if`-expression. Note that the values in each branch are evaluated at compile time so can't refer to local variables.

Given a :mini:`switch`-expression with provider :mini:`P`, the compiler uses the method :mini:`compiler::switch(P, Cases...)` to implement the :mini:`switch`-expression. Here :mini:`Cases` is a list of lists, one list per :mini:`case` containing the case values. The implementation of :mini:`compiler::switch` for :mini:`P` must return another function, which takes a value and return an integer index (starting at :mini:`0`) corresponding to the chosen branch.

This design allows support for new kinds of :mini:`switch`-expressions to be added later, either to the language or by specific applications. Currently the following switch providers are available:

:mini:`type`
   Case values must be types. A case value matches if the switch value is of the same type or a sub-type. 
   
:mini:`integer`, :mini:`real`
   Case values must be numbers or numeric ranges. A case value matches if the switch value is equal to it (for numbers) or contained in it (for ranges).

:mini:`string`
   Case values must be strings or regular expressions. A case value matches if the switch value is equal to it (for strings) or matches it (for regular expressions).

Any :mini:`enum` type
   Case values must be values of the enum type or strings corresponding to values of the enum. A case value matches if the switch value is equal to it.
    
Any :mini:`flags` type
   Flag values must be values of the flags type, strings corresponding to values of the enum, or tuples of the previous. A case value matches if the switch value contains *at least* the same flags, it may have extra flags.

Any function
   Any function can be used as a switch provider as long as it accepts a list of lists and returns another function as described above.
