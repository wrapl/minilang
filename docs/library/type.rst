.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

type
====

.. rst-class:: mini-api

.. _type-type:

:mini:`type type < function`
   Type of all types.
   Every type contains a set of named exports,  which allows them to be used as modules.


.. _value-MLBlank:

:mini:`def MLBlank: blank`
   *TBD*


.. _fun-type:

:mini:`fun type(Value: any): type`
   Returns the type of :mini:`Value`.


:mini:`meth (Type: type) :: (Name: string): any | error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.
   Returns an error if :mini:`Name` is not present.
   This allows types to behave as modules.


:mini:`meth ?(Type: type): type`
   Returns a union interface of :mini:`Type` and :mini:`type(nil)`.


:mini:`meth (Type: type):exports: map`
   Returns a map of all the exports from :mini:`Type`.


:mini:`meth (Type: type):parents: list`
   Returns a list of the parent types of :mini:`Type`.


:mini:`meth (Type: type):rank: integer`
   Returns the rank of :mini:`Type`,  i.e. the depth of its inheritence tree.


:mini:`meth (Type₁: type) | (Type₂: type): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Buffer: string::buffer):append(Value: type)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


