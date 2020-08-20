function
========

.. include:: <isonum.txt>

:mini:`function`
   The base type of all functions.

   *Defined at line 56 in src/ml_types.c*

:mini:`meth !(Function: function, List: list)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.

   *Defined at line 370 in src/ml_types.c*

:mini:`meth !(Function: function, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 383 in src/ml_types.c*

:mini:`meth !(Function: function, List: list, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 410 in src/ml_types.c*

:mini:`partialfunction`
   :Parents: :mini:`function`

   *Defined at line 520 in src/ml_types.c*

:mini:`meth !!(Function: function, List: list)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(List..., Args...)`.

   *Defined at line 539 in src/ml_types.c*

:mini:`meth $(Function: function, Arg: any)` |rarr| :mini:`partialfunction`
   Returns a function equivalent to :mini:`fun(Args...) Function(Arg, Args...)`.

   *Defined at line 555 in src/ml_types.c*

