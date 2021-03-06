tuple
=====

.. include:: <isonum.txt>

:mini:`fun tuple(Value₁: any, : ..., Valueₙ: any)` |rarr| :mini:`tuple`
   Returns a tuple of values :mini:`Value₁, ..., Valueₙ`.


:mini:`type tuple`
   An immutable tuple of values.


:mini:`meth :size(Tuple: tuple)` |rarr| :mini:`integer`
   Returns the number of elements in :mini:`Tuple`.


:mini:`meth (Tuple: tuple)[Index: integer]` |rarr| :mini:`any` or :mini:`error`
   Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.

   Indexing starts at :mini:`1`. Negative indices count from the end, with :mini:`-1` returning the last element.


:mini:`meth string(Tuple: tuple)` |rarr| :mini:`string`
   Returns a string representation of :mini:`Tuple`.


:mini:`meth <>(Tuple₁: tuple, Tuple₂: tuple)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Tuple₁` is less than, equal to or greater than :mini:`Tuple₂` using lexicographical ordering.


:mini:`meth <op>(Tuple₁: tuple, Tuple₂: tuple)` |rarr| :mini:`Tuple₂` or :mini:`nil`
   :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`

   Returns :mini:`Tuple₂` if :mini:`Tuple₂ <op> Tuple₁` is true, otherwise returns :mini:`nil`.


