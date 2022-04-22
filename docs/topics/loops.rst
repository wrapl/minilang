Loops
=====

*Minilang* has several looping constructs:

Loop expressions
----------------

The simplest looping expression in *Minilang* is the :mini:`loop`-expression which repeatedly evaluates a block of code until the loop is exited using an :mini:`exit`-expression somewhere in the body of the loop.

.. parser-rule-diagram:: 'loop' block 'end'

.. parser-rule-diagram:: 'exit' expression?

The loop returns the value of the expression after :mini:`exit`, or :mini:`nil` if the expression in omitted.

A :mini:`next`-expression causes the loop to move onto its next iteration immediately.

.. parser-rule-diagram:: 'next'

Normally, :mini:`exit`-expressions are used within conditional expressions such as :mini:`if`-expressions or with :mini:`and` or :mini:`or`. For convenience, *Minilang* provides :mini:`while`-expressions and :mini:`until`-expressions.

.. parser-rule-diagram:: 'while' expression ( ',' expression )?

If the first expression after :mini:`while` evaluates to :mini:`nil` then the loop exits with the value of the second expression, or :mini:`nil` is the second expression is omitted. Otherwise, the :mini:`while`-expression evaluates to the result of the first expression.

.. code-block:: mini

   while X, Y
   :> behaves like
   if let Temp := X then Temp else exit Y end

   while X
   :> behaves like
   if let Temp := X then Temp else exit Temp :<nil>: end

.. parser-rule-diagram:: 'until' expression ( ',' expression )?

If the first expression after :mini:`until` evaluates to anything other than :mini:`nil` then the loop exits with the value of the second expression, or the value of the first expression if the second expression is omitted. Otherwise, the :mini:`until`-expression evaluates to :mini:`nil`.

.. code-block:: mini

   until X, Y
   :> behaves like
   if let Temp := X then exit Y else nil end

   until X
   :> behaves like
   if let Temp := X then exit Temp else nil end


Exiting nested loops
....................

The optional expression passed to :mini:`exit` is evaluated in the context of the surrounding loop. For example, to exit 2 loops at once, :mini:`exit exit Value` can be used. Likewise, to skip to the start of the surrounding loop, :mini:`exit next` can be used.

.. code-block:: mini

   let (I, J, K) := for I in 1 .. 10 do
      for J in 1 .. 10 do
         let K := math::sqrt((I * I) + (J * J))
         if K in integer then
            exit exit (I, J, K)
         end
      end
   end

   print('I = {I}, J = {J}, K = {K}\n')

.. code-block:: console

   I = 3, J = 4, K = 5

For expressions
---------------

.. parser-rule-diagram:: 'for' ( identifier ',' )? ( identifier | '(' identifier ( ',' identifier )* ')' )  'in' expression 'do' block ( 'else' block )? 'end'

A :mini:`for`-expressions loops over a sequence, binding the generated keys and values to local variables and evaluating a block of code for each iteration. The loop stops when the sequence is exhausted, or if a :mini:`exit` expression is used to exit the loop.

If a :mini:`for`-expression exhausts its sequence without exiting, the :mini:`else`-block will be evaluated and the returned as the value of the :mini:`for`-expression. If the :mini:`else`-block is omitted (usually the case), the value of the :mini:`for`-expression will be :mini:`nil`.

A :mini:`next`-expression can also be used in a :mini:`for`-expression which will advance to the next iteration of the sequence.

.. code-block:: mini

   for I in 1 .. 10 do
      I = 3 and next
      print('I = {I}\n')
      I = 8 and exit "Done!"
   end

.. code-block:: console

   I = 1
   I = 2
   I = 4
   I = 5
   I = 6
   I = 7
   I = 8
   Done!

Each expressions
----------------

.. parser-rule-diagram:: 'each' expression

An :mini:`each`-expression simply loops over a sequence.

.. code-block:: mini

   each X
   :> behaves like
   for Temp in X do end

