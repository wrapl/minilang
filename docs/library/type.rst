.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

type
====

.. rst-class:: mini-api

:mini:`type decimal < real`
   *TBD*


:mini:`meth number(Arg₁: decimal)`
   *TBD*


:mini:`meth real(Arg₁: decimal)`
   *TBD*


:mini:`meth (Arg₁: decimal):scale`
   *TBD*


:mini:`meth (Arg₁: decimal):unscaled`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: decimal)`
   *TBD*


:mini:`meth decimal(Arg₁: integer)`
   *TBD*


:mini:`meth decimal(Arg₁: integer, Arg₂: integer)`
   *TBD*


:mini:`meth decimal(Arg₁: real)`
   *TBD*


:mini:`type type < function`
   Type of all types.
   Every type contains a set of named exports,  which allows them to be used as modules.


:mini:`fun type(Value: any): type`
   Returns the type of :mini:`Value`.


:mini:`meth (Type: type) :: (Name: string): any | error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.
   Returns an error if :mini:`Name` is not present.
   This allows types to behave as modules.


:mini:`meth (Type₁: type) * (Type₂: type): type`
   Returns the closest common parent type of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Type₁: type) < (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a strict parent of :mini:`Type₁`,  otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) <= (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a parent of :mini:`Type₁`,  otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) > (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a strict sub-type of :mini:`Type₁`,  otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) >= (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a sub-type of :mini:`Type₁`,  otherwise returns :mini:`nil`.


:mini:`meth ?(Type: type): type`
   Returns a union interface of :mini:`Type` and :mini:`type(nil)`.


:mini:`meth (Base: type)[Type₁, : type, ...]: type`
   Returns the generic type :mini:`Base[Type₁,  ...,  Typeₙ]`.


:mini:`meth (Type: type):constructor: function`
   Returns the constructor for :mini:`Type`.


:mini:`meth (Type: type):exports: map`
   Returns a map of all the exports from :mini:`Type`.


:mini:`meth (Type: type):name: string`
   Returns the name of :mini:`Type`.


:mini:`meth (Type: type):parents: list`
   Returns a list of the parent types of :mini:`Type`.


:mini:`meth (Type: type):rank: integer`
   Returns the rank of :mini:`Type`,  i.e. the depth of its inheritence tree.


:mini:`meth (Type₁: type) | (Type₂: type): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Type₁: type) | (Type₂: type::union): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Buffer: string::buffer):append(Value: type)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`meth ?(Type: type::union): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Type₁: type::union) | (Type₂: type): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Ref: weak::ref):get: any | nil`
   *TBD*


:mini:`type weakref`
   *TBD*


:mini:`fun weakref(Value: any): weakref`
   *TBD*


