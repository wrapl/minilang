.. include:: <isonum.txt>

type
====

.. _type-any:

:mini:`type any`
   Base type for all values.


.. _fun-type:

:mini:`fun type(Value: any): type`
   Returns the type of :mini:`Value`.


.. _type-type:

:mini:`type type < function`
   Type of all types.

   Every type contains a set of named exports,  which allows them to be used as modules.


:mini:`meth (Type: type):rank: integer`
   Returns the rank of :mini:`Type`,  i.e. the depth of its inheritence tree.


:mini:`meth (Type: type):exports: map`
   Returns a map of all the exports from :mini:`Type`.


:mini:`meth (Type: type):parents: list`
   *TBD*

:mini:`meth (Type₁: type) | (Type₂: type): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth ?(Type: type): type`
   Returns a union interface of :mini:`Type` and :mini:`type(nil)`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: type)`
   *TBD*

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


:mini:`meth (Base: type)[Type₁, ..., Typeₙ: type]: type`
   Returns the generic type :mini:`Base[Type₁,  ...,  Typeₙ]`.


:mini:`meth (Value: any):in(Type: type): Value | nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any):trace`
   *TBD*

:mini:`meth (Value₁: any) <> (Value₂: any): integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`,  :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


:mini:`meth #(Value: any): integer`
   Returns a hash for :mini:`Value` for use in lookup tables,  etc.


:mini:`meth (Value₁: any) = (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Value₁: any) != (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are not exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) = (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) != (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) < (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) <= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) > (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) >= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: any)`
   *TBD*

