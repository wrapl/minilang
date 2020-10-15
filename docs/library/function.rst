function
========

.. include:: <isonum.txt>

:mini:`function`
   The base type of all functions.

   *Defined at line 54 in src/ml_types.c*

:mini:`meth !(Function: function, Tuple: tuple)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.

   *Defined at line 381 in src/ml_types.c*

:mini:`meth !(Function: function, List: list)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.

   *Defined at line 392 in src/ml_types.c*

:mini:`meth !(Function: function, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 405 in src/ml_types.c*

:mini:`meth !(Function: function, Tuple: tuple, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 432 in src/ml_types.c*

:mini:`meth !(Function: function, List: list, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 464 in src/ml_types.c*

:mini:`partialfunction`
   :Parents: :mini:`function`

   *Defined at line 574 in src/ml_types.c*

:mini:`meth !!(Function: function, List: list)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(List..., Args...)`.

   *Defined at line 593 in src/ml_types.c*

:mini:`meth $(Function: function, Arg: any)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(Arg, Args...)`.

   *Defined at line 609 in src/ml_types.c*

