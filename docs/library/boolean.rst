boolean
=======

.. include:: <isonum.txt>

**type** :mini:`boolean`
   :Parents: :mini:`function`


**method** :mini:`- boolean Bool` |rarr| :mini:`boolean`
   Returns the logical inverse of :mini:`Bool`


**method** :mini:`boolean Bool₁ /\\ boolean Bool₂` |rarr| :mini:`boolean`
   Returns the logical and of :mini:`Bool₁` and :mini:`Bool₂`.


**method** :mini:`boolean Bool₁ \\/ boolean Bool₂` |rarr| :mini:`boolean`
   Returns the logical or of :mini:`Bool₁` and :mini:`Bool₂`.


**method** :mini:`boolean Bool₁ <> boolean Bool₂` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Bool₁` is less than, equal to or greater than :mini:`Bool₂` using lexicographical ordering.


**method** :mini:`boolean Bool₁ <op> boolean Bool₂` |rarr| :mini:`Bool₂` or :mini:`nil`
   :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`

   Returns :mini:`Bool₂` if :mini:`Bool₂ <op> Bool₁` is true, otherwise returns :mini:`nil`.

   :mini:`true` is considered greater than :mini:`false`.


**method** :mini:`string(boolean Arg₁)`

