.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

tuple
=====

.. _fun-tuple:

:mini:`fun tuple(Value₁: any, : ..., Valueₙ: any): tuple`
   Returns a tuple of values :mini:`Value₁,  ...,  Valueₙ`.



.. _type-tuple:

:mini:`type tuple < sequence`
   An immutable tuple of values.



:mini:`meth (Arg₁: tuple) != (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Tuple: tuple):size: integer`
   Returns the number of elements in :mini:`Tuple`.



:mini:`meth (Arg₁: tuple) < (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: tuple) <= (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Tuple₁: tuple) <> (Tuple₂: tuple): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Tuple₁` is less than,  equal to or greater than :mini:`Tuple₂` using lexicographical ordering.



:mini:`meth (Arg₁: tuple) = (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: tuple) > (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: tuple) >= (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Tuple: tuple)[Index: integer]: any | error`
   Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.

   Indexing starts at :mini:`1`. Negative indices count from the end,  with :mini:`-1` returning the last element.



:mini:`meth (Buffer: string::buffer):append(Value: tuple)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.



.. _type-tuple-iter:

:mini:`type tuple::iter`
   *TBD*


