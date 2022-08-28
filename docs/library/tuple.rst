.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

tuple
=====

.. rst-class:: mini-api

:mini:`meth (Copy: copy):const(Tuple: tuple): tuple`
   Returns a new constant tuple containing copies of the elements of :mini:`Tuple` created using :mini:`Copy`.


:mini:`meth (Copy: copy):copy(Tuple: tuple): tuple`
   Returns a new tuple containing copies of the elements of :mini:`Tuple` created using :mini:`Copy`.


.. _type-tuple:

:mini:`type tuple < function, sequence`
   An immutable tuple of values.
   
   :mini:`(Tuple: tuple)(Arg₁,  ...,  Argₙ)`
      Returns :mini:`(Tuple[1](Arg₁,  ...,  Argₙ),  ...,  Tuple[k](Arg₁,  ...,  Argₙ))`


.. _fun-tuple:

:mini:`fun tuple(Value₁: any, : ..., Valueₙ: any): tuple`
   Returns a tuple of values :mini:`Value₁,  ...,  Valueₙ`.


:mini:`meth (A: tuple) != (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`A:size != B:size` or :mini:`Aᵢ != Bᵢ` for some :mini:`i`.

   .. code-block:: mini

      !=((1, 2, 3), (1, 2, 3)) :> nil
      !=((1, 2, 3), (1, 2)) :> (1, 2)
      !=((1, 2), (1, 2, 3)) :> (1, 2, 3)
      !=((1, 2, 3), (1, 2, 4)) :> (1, 2, 4)
      !=((1, 3, 2), (1, 2, 3)) :> (1, 2, 3)


:mini:`meth (A: tuple) < (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ < Bⱼ`.

   .. code-block:: mini

      <((1, 2, 3), (1, 2, 3)) :> nil
      <((1, 2, 3), (1, 2)) :> nil
      <((1, 2), (1, 2, 3)) :> (1, 2, 3)
      <((1, 2, 3), (1, 2, 4)) :> (1, 2, 4)
      <((1, 3, 2), (1, 2, 3)) :> nil


:mini:`meth (A: tuple) <= (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ <= Bⱼ`.

   .. code-block:: mini

      <=((1, 2, 3), (1, 2, 3)) :> (1, 2, 3)
      <=((1, 2, 3), (1, 2)) :> nil
      <=((1, 2), (1, 2, 3)) :> (1, 2, 3)
      <=((1, 2, 3), (1, 2, 4)) :> (1, 2, 4)
      <=((1, 3, 2), (1, 2, 3)) :> nil


:mini:`meth (Tuple₁: tuple) <> (Tuple₂: tuple): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Tuple₁` is less than,  equal to or greater than :mini:`Tuple₂` using lexicographical ordering.


:mini:`meth (A: tuple) = (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`A:size = B:size` and :mini:`Aᵢ = Bᵢ` for each :mini:`i`.

   .. code-block:: mini

      =((1, 2, 3), (1, 2, 3)) :> (1, 2, 3)
      =((1, 2, 3), (1, 2)) :> nil
      =((1, 2), (1, 2, 3)) :> nil
      =((1, 2, 3), (1, 2, 4)) :> nil
      =((1, 3, 2), (1, 2, 3)) :> nil


:mini:`meth (A: tuple) > (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ > Bⱼ`.

   .. code-block:: mini

      >((1, 2, 3), (1, 2, 3)) :> nil
      >((1, 2, 3), (1, 2)) :> (1, 2)
      >((1, 2), (1, 2, 3)) :> nil
      >((1, 2, 3), (1, 2, 4)) :> nil
      >((1, 3, 2), (1, 2, 3)) :> (1, 2, 3)


:mini:`meth (A: tuple) >= (B: tuple): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ >= Bⱼ`.

   .. code-block:: mini

      >=((1, 2, 3), (1, 2, 3)) :> (1, 2, 3)
      >=((1, 2, 3), (1, 2)) :> (1, 2)
      >=((1, 2), (1, 2, 3)) :> nil
      >=((1, 2, 3), (1, 2, 4)) :> nil
      >=((1, 3, 2), (1, 2, 3)) :> (1, 2, 3)


:mini:`meth (Tuple: tuple)[Index: integer]: any | error`
   Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.
   Indexing starts at :mini:`1`. Negative indices count from the end,  with :mini:`-1` returning the last element.


:mini:`meth (Tuple: tuple):size: integer`
   Returns the number of elements in :mini:`Tuple`.


:mini:`meth (Buffer: string::buffer):append(Value: tuple)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


