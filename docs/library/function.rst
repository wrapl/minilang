function
========

.. include:: <isonum.txt>

**type** :mini:`function`
   The base type of all functions.

   All functions are considered iteratable, they can return an iterator when called.

   :Parents: :mini:`iteratable`


**method** :mini:`function Function ! list List` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments.


**method** :mini:`function Function ! map Map` |rarr| :mini:`any`
   Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


**method** :mini:`function Function ! list List, map Map` |rarr| :mini:`any`
   Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.

   Returns an error if any of the keys in :mini:`Map` is not a string or method.


**type** :mini:`partialfunction`
   :Parents: :mini:`function`


**method** :mini:`function Function !! list List` |rarr| :mini:`partialfunction`

**method** :mini:`function Arg₁ $ any Arg₂`

**method** :mini:`partialfunction Arg₁ $ any Arg₂`

