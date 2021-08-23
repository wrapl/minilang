tuple
=====

:mini:`fun tuple(Value₁: any, : ..., Valueₙ: any): tuple`
   Returns a tuple of values :mini:`Value₁, ..., Valueₙ`.


:mini:`type tuple`
   An immutable tuple of values.


:mini:`meth :size(Tuple: tuple): integer`
   Returns the number of elements in :mini:`Tuple`.


:mini:`meth (Tuple: tuple)[Index: integer]: any | error`
   Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.

   Indexing starts at :mini:`1`. Negative indices count from the end, with :mini:`-1` returning the last element.


:mini:`meth string(Tuple: tuple): string`
   Returns a string representation of :mini:`Tuple`.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: tuple)`
   *TBD*

:mini:`meth (Tuple₁: tuple) <> (Tuple₂: tuple): integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Tuple₁` is less than, equal to or greater than :mini:`Tuple₂` using lexicographical ordering.


:mini:`meth (Arg₁: tuple) = (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: tuple) != (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: tuple) < (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: tuple) <= (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: tuple) > (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: tuple) >= (Arg₂: tuple): tuple | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


