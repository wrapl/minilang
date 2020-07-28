tuple
=====

.. include:: <isonum.txt>

**type** :mini:`tuple`
   An immutable tuple of values.


**function** :mini:`tuple(any Value₁, ... , any Valueₙ)` |rarr| :mini:`tuple`
   Returns a tuple of values :mini:`Value₁, ..., Valueₙ`.


**method** :mini:`tuple Tuple:size` |rarr| :mini:`integer`
   Returns the number of elements in :mini:`Tuple`.


**method** :mini:`tuple Tuple[integer Index]` |rarr| :mini:`any` or :mini:`error`
   Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.

   Indexing starts at :mini:`1`. Negative indices count from the end, with :mini:`-1` returning the last element.


**method** :mini:`string(tuple Tuple)` |rarr| :mini:`string`
   Returns a string representation of :mini:`Tuple`.


**method** :mini:`tuple Tuple₁ <> tuple Tuple₂` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Tuple₁` is less than, equal to or greater than :mini:`Tuple₂` using lexicographical ordering.


**method** :mini:`tuple Tuple₁ <op> tuple Tuple₂` |rarr| :mini:`Tuple₂` or :mini:`nil`
   :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`

   Returns :mini:`Tuple₂` if :mini:`Tuple₂ <op> Tuple₁` is true, otherwise returns :mini:`nil`.


