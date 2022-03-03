.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

function
========

.. _type-function:

:mini:`type function`
   The base type of all functions.


:mini:`meth (Function: function) ! (Tuple: tuple): any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.


:mini:`meth (Function: function) ! (List: list): any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.


:mini:`meth (Function: function) ! (Map: map): any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth (Function: function) ! (Tuple: tuple, Map: map): any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth (Function: function) ! (List: list, Map: map): any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


.. _type-function-partial:

:mini:`type function::partial < function, sequence`
   *TBD*

:mini:`meth (Arg₁: function::partial):arity`
   *TBD*

:mini:`meth (Arg₁: function::partial):set`
   *TBD*

:mini:`meth (Function: function) !! (List: list): partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(List₁,  List₂,  ...,  Args...)`.


:mini:`meth (Function: function) $ (Values...: any): partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(Values...,  Args...)`.


.. _type-function-argless:

:mini:`type function::argless < function, sequence`
   *TBD*

:mini:`meth /(Function: function): arglessfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function()`.


