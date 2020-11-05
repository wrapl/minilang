type
====

.. include:: <isonum.txt>

:mini:`fun type(Value: any)` |rarr| :mini:`type`
   Returns the type of :mini:`Value`.

   *Defined at line 63 in src/ml_types.c*

:mini:`type`
   Type of all types.

   Every type contains a set of named exports, which allows them to be used as modules.

   :Parents: :mini:`function`

   *Defined at line 86 in src/ml_types.c*

:mini:`meth :rank(Type: type)` |rarr| :mini:`integer`
   Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.

   *Defined at line 95 in src/ml_types.c*

:mini:`meth :parents(Type: type)` |rarr| :mini:`list`
   *Defined at line 104 in src/ml_types.c*

:mini:`meth string(Type: type)` |rarr| :mini:`string`
   Returns a string representing :mini:`Type`.

   *Defined at line 211 in src/ml_types.c*

:mini:`meth ::(Type: type, Name: string)` |rarr| :mini:`any` or :mini:`error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.

   Returns an error if :mini:`Name` is not present.

   This allows types to behave as modules.

   *Defined at line 236 in src/ml_types.c*

