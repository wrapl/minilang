function
========

.. include:: <isonum.txt>

:mini:`type function`
   The base type of all functions.


:mini:`meth !(Function: function, Tuple: tuple)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.


:mini:`meth !(Function: function, List: list)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.


:mini:`meth !(Function: function, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth !(Function: function, Tuple: tuple, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`meth !(Function: function, List: list, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


:mini:`type partialfunction < function, iteratable`

:mini:`meth :count(Arg₁: partialfunction)`

:mini:`meth :set(Arg₁: partialfunction)`

:mini:`meth !!(Function: function, List: list)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(List₁, List₂, ..., Args...)`.


:mini:`meth $(Function: function, Values...: any)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(Values..., Args...)`.


