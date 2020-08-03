type
====

.. include:: <isonum.txt>

**function** :mini:`type(any Value)` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.

   *Defined at line 61 in src/ml_types.c*

**type** :mini:`type`
   Type of all types.

   Every type contains a set of named exports, which allows them to be used as modules.

   :Parents: :mini:`function`

   *Defined at line 71 in src/ml_types.c*

**method** :mini:`type Type:rank` |rarr| :mini:`integer`
   Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.

   *Defined at line 79 in src/ml_types.c*

**method** :mini:`string(type Type)` |rarr| :mini:`string`
   Returns a string representing :mini:`Type`.

   *Defined at line 159 in src/ml_types.c*

**method** :mini:`type Type :: string Name` |rarr| :mini:`any` or :mini:`error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.

   Returns an error if :mini:`Name` is not present.

   This allows types to behave as modules.

   *Defined at line 184 in src/ml_types.c*

