type
====

.. include:: <isonum.txt>

:mini:`fun type(Value: any)` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.


:mini:`type`
   Type of all types.

   Every type contains a set of named exports, which allows them to be used as modules.

   :Parents: :mini:`function`


:mini:`meth :rank(Type: type)` |rarr| :mini:`integer`
   Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.


:mini:`meth :parents(Type: type)` |rarr| :mini:`list`

:mini:`meth string(Type: type)` |rarr| :mini:`string`
   Returns a string representing :mini:`Type`.


:mini:`meth ::(Type: type, Name: string)` |rarr| :mini:`any` or :mini:`error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.

   Returns an error if :mini:`Name` is not present.

   This allows types to behave as modules.


