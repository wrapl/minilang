function
========

.. include:: <isonum.txt>

:mini:`function`
   The base type of all functions.

   All functions are considered iteratable, they can return an iterator when called.

   :Parents: :mini:`iteratable`

   *Defined at line 56 in src/ml_types.c*

:mini:`meth !(Function: function, List: list)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.

   *Defined at line 367 in src/ml_types.c*

:mini:`meth !(Function: function, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 380 in src/ml_types.c*

:mini:`meth !(Function: function, List: list, Map: map)` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 407 in src/ml_types.c*

:mini:`partialfunction`
   :Parents: :mini:`function`

   *Defined at line 517 in src/ml_types.c*

:mini:`meth !!(Function: function, List: list)` |rarr| :mini:`partialfunction`
   *Defined at line 536 in src/ml_types.c*

:mini:`meth $(Arg₁: function, Arg₂: any)`
   *Defined at line 551 in src/ml_types.c*

:mini:`meth $(Arg₁: partialfunction, Arg₂: any)`
   *Defined at line 561 in src/ml_types.c*

