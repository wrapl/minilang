type
====

.. include:: <isonum.txt>

**type** :mini:`type`
   Type of all types.

   Every type contains a set of named exports, which allows them to be used as modules.

   The export :mini:`"of"` should be a convertor / constructor. E.g. :mini:`string::of(X)` returns :mini:`X` converted to a string.


**method** :mini:`type Type:rank` |rarr| :mini:`integer`
   Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.


**method** :mini:`string(type Type)` |rarr| :mini:`string`
   Returns a string representing :mini:`Type`.


**method** :mini:`type Type :: string Name` |rarr| :mini:`any` or :mini:`error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.

   Returns an error if :mini:`Name` is not present.

   This allows types to behave as modules.


**function** :mini:`type(any Value)` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.


