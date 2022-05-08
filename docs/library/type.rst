.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

type
====

.. _value-MLNil:

:mini:`def MLNil: nil`
   *TBD*


.. _value-MLBlank:

:mini:`def MLBlank: blank`
   *TBD*


.. _type-any:

:mini:`type any`
   Base type for all values.


:mini:`meth (Value₁: any) != (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are not exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) != (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth #(Value: any): integer`
   Returns a hash for :mini:`Value` for use in lookup tables,  etc.


:mini:`meth (Arg₁: any) < (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) <= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Value₁: any) <> (Value₂: any): integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`,  :mini:`0` or :mini:`1`.
   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


:mini:`meth (Value₁: any) = (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) = (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) > (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) >= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Value: any):in(Type: type): Value | nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.


:mini:`meth (A: any):max(B: any): any`
   Returns :mini:`A` if :mini:`A > B` and :mini:`B` otherwise.


:mini:`meth (A: any):min(B: any): any`
   Returns :mini:`A` if :mini:`A < B` and :mini:`B` otherwise.


:mini:`meth (Buffer: string::buffer):append(Value: any)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: nil)`
   Appends :mini:`"nil"` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: some)`
   Appends :mini:`"some"` to :mini:`Buffer`.


.. _type-type:

:mini:`type type < function`
   Type of all types.
   Every type contains a set of named exports,  which allows them to be used as modules.


.. _fun-type:

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


:mini:`meth (Base: type)[Type₁, ..., Typeₙ: type]: type`
   Returns the generic type :mini:`Base[Type₁,  ...,  Typeₙ]`.


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


