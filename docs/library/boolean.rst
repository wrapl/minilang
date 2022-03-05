.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

boolean
=======

.. _type-boolean:

:mini:`type boolean`
   A boolean value (either :mini:`true` or :mini:`false`).



:mini:`meth boolean(String: string): boolean | error`
   Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).

   Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).

   Otherwise returns an error.



:mini:`meth (Arg₁: boolean) != (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.



:mini:`meth -(Bool: boolean): boolean`
   Returns the logical inverse of :mini:`Bool`



:mini:`meth (Bool₁: boolean) /\ (Bool₂: boolean, ...): boolean`
   Returns the logical and of :mini:`Bool₁` and :mini:`Bool₂`.



:mini:`meth (Arg₁: boolean) < (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: boolean) <= (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Bool₁: boolean) <> (Bool₂: boolean): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Bool₁` is less than,  equal to or greater than :mini:`Bool₂`. :mini:`true` is considered greater than :mini:`false`.



:mini:`meth (Arg₁: boolean) = (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: boolean) > (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: boolean) >= (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Bool₁: boolean) \/ (Bool₂: boolean, ...): boolean`
   Returns the logical or of :mini:`Bool₁` and :mini:`Bool₂`.



:mini:`meth (Buffer: string::buffer):append(Value: boolean)`
   Appends :mini`"true"` or :mini:`"false`" to :mini:`Buffer`.



