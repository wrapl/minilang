Error Handling
==============

When an error occurs in *Minilang*, an error value is created and returned. If an error handler has been declared in the current block, execution jumps to the error handler, assigning the error value to the error variable. If no error handler declared, the error is propagated to the surrounding block, or returned from the current function if the outermost block is reached without any error handler.

.. parser-rule-diagram:: 'on' block

.. code-block:: mini
   :linenos:

   for I in -5 .. 5 do
      print('1 / {I} = {1 / I}\n')
   on Error do
      print('{Error:type}: {Error:message}\n')
   end

.. code-block:: console

   1 / -5 = -0.2
   1 / -4 = -0.25
   1 / -3 = -0.333333
   1 / -2 = -0.5
   1 / -1 = -1
   ValueError: Division by 0
   1 / 1 = 1
   1 / 2 = 0.5
   1 / 3 = 0.333333
   1 / 4 = 0.25
   1 / 5 = 0.2

.. note::

   Within an error handler, an error value is wrapped inside another value (with type :mini:`error`) in order to prevent it triggering another error each time it isused. The wrapped error value provides access to the error type, message and call history, and allows the original error to be raised again using :mini:`:raise`.

Generating Errors
-----------------

The function :mini:`error(Type, Message)` returns a new error, triggering the error handler in the calling function.

.. code-block:: mini
   :linenos:

   fun fact(N: integer) do
      N >= 0 or error("RangeError", "Factorial requires non-negative integer")
      var F := 1
      for I in 1 .. N do
         F := old * I
      end
      ret F
   end

   for N in [1, 5, -7, 0.4] do
      print('{N}! = {fact(N)}\n')
   on Error do
      print('{N}! caused {Error:type}: {Error:message}\n')
   end

.. code-block:: console

   1! = 1
   5! = 120
   -7! caused RangeError: Factorial requires non-negative integer
   0.4! caused TypeError: Expected integer not double for argument 1

The previous example also shows how the optional type checking can be enabled for function arguments.

Error Payloads
--------------

As well as an error message, an error value can also hold any other *Minilang* value. To construct such an error value, the function :mini:`raise(Type, Value)` can be used. The value held by an error can be retrieved using :mini:`:value`. For errors not created by :mini:`raise` (including all normal runtime errors), :mini:`Error:value` will return :mini:`nil`.
