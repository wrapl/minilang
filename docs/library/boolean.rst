.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

boolean
=======

.. _fun-boolean-random:

:mini:`fun boolean::random(P?: number): boolean`
   Returns a random boolean that has probability :mini:`P` of being :mini:`true`. If omitted,  :mini:`P` defaults to :mini:`0.5`.


.. _type-boolean:

:mini:`type boolean`
   A boolean value (either :mini:`true` or :mini:`false`).


:mini:`meth boolean(String: string): boolean | error`
   Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).
   Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).
   Otherwise returns an error.


:mini:`meth (Arg₁: boolean) != (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true != true :> nil
      true != false :> false
      false != true :> true
      false != false :> nil


:mini:`meth -(Bool: boolean): boolean`
   Returns the logical inverse of :mini:`Bool`


:mini:`meth (Bool₁: boolean) /\ (Bool₂: boolean, ...): boolean`
   Returns the logical and of :mini:`Bool₁` and :mini:`Bool₂`.

   .. code-block:: mini

      true /\ true :> true
      true /\ false :> false
      false /\ true :> false
      false /\ false :> false


:mini:`meth (Arg₁: boolean) < (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true < true :> nil
      true < false :> nil
      false < true :> true
      false < false :> nil


:mini:`meth (Arg₁: boolean) <= (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true <= true :> true
      true <= false :> nil
      false <= true :> true
      false <= false :> false


:mini:`meth (Bool₁: boolean) <> (Bool₂: boolean): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Bool₁` is less than,  equal to or greater than :mini:`Bool₂`. :mini:`true` is considered greater than :mini:`false`.


:mini:`meth (Arg₁: boolean) = (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true = true :> true
      true = false :> nil
      false = true :> nil
      false = false :> false


:mini:`meth (Arg₁: boolean) > (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true > true :> nil
      true > false :> false
      false > true :> nil
      false > false :> nil


:mini:`meth (Bool₁: boolean) >< (Bool₂: boolean): boolean`
   Returns the logical xor of :mini:`Bool₁` and :mini:`Bool₂`.

   .. code-block:: mini

      true >< true :> false
      true >< false :> true
      false >< true :> true
      false >< false :> false


:mini:`meth (Arg₁: boolean) >= (Arg₂: boolean): boolean | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      true >= true :> true
      true >= false :> false
      false >= true :> nil
      false >= false :> false


:mini:`meth (Bool₁: boolean) \/ (Bool₂: boolean, ...): boolean`
   Returns the logical or of :mini:`Bool₁` and :mini:`Bool₂`.

   .. code-block:: mini

      true \/ true :> true
      true \/ false :> true
      false \/ true :> true
      false \/ false :> false


:mini:`meth (Buffer: string::buffer):append(Value: boolean)`
   Appends :mini:`"true"` or :mini:`"false"` to :mini:`Buffer`.


