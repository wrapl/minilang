.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

any
===

.. rst-class:: mini-api

:mini:`type any`
   Base type for all values.


:mini:`meth (Value₁: any) != (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are not exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) != (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ != Argᵢ` for i = 2,  ...,  n and :mini:`nil` otherwise.


:mini:`meth #(Value: any): integer`
   Returns a hash for :mini:`Value` for use in lookup tables,  etc.


:mini:`meth (Arg₁: any) < (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ < Arg₂ < ... < Argₙ` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) <= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ <= Arg₂ <= ... <= Argₙ` and :mini:`nil` otherwise.


:mini:`meth (Value₁: any) <> (Value₂: any): integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`,  :mini:`0` or :mini:`1`.
   This comparison is based on the types and internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


:mini:`meth (Value₁: any) = (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) = (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ = Arg₂ = ... = Argₙ` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) > (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ > Arg₂ > ... > Argₙ` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) >= (Arg₂: any, Arg₃: any, ...): any | nil`
   Returns :mini:`Argₙ` if :mini:`Arg₁ >= Arg₂ >= ... >= Argₙ` and :mini:`nil` otherwise.


:mini:`meth (Value: any):in(Type: type): Value | nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.


:mini:`meth (A: any):max(B: any): any`
   Returns :mini:`A` if :mini:`A > B` and :mini:`B` otherwise.


:mini:`meth (A: any):min(B: any): any`
   Returns :mini:`A` if :mini:`A < B` and :mini:`B` otherwise.


:mini:`meth (Buffer: string::buffer):append(Value: any)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


