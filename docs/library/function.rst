function
========

.. include:: <isonum.txt>

**type** :mini:`function`
   The base type of all functions.

   All functions are considered iteratable, they can return an iterator when called.

   :Parents: :mini:`iteratable`

   *Defined at line 56 in src/ml_types.c*

**method** :mini:`function Function ! list List` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.

   *Defined at line 367 in src/ml_types.c*

**method** :mini:`function Function ! map Map` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 380 in src/ml_types.c*

**method** :mini:`function Function ! list List, map Map` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.

   *Defined at line 407 in src/ml_types.c*

**type** :mini:`partialfunction`
   :Parents: :mini:`function`

   *Defined at line 517 in src/ml_types.c*

**method** :mini:`function Function !! list List` |rarr| :mini:`partialfunction`
   *Defined at line 536 in src/ml_types.c*

**method** :mini:`function Arg₁ $ any Arg₂`
   *Defined at line 551 in src/ml_types.c*

**method** :mini:`partialfunction Arg₁ $ any Arg₂`
   *Defined at line 561 in src/ml_types.c*

