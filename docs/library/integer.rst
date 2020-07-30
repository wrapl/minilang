integer
=======

.. include:: <isonum.txt>

**method** :mini:`integer Arg₁ ^ integer Arg₂`

**method** :mini:`integer Arg₁ * valueparser Arg₂`

**method** :mini:`integer Arg₁ * stringparser Arg₂`

**type** :mini:`integer`
   :Parents: :mini:`number`


**type** :mini:`int32`
   :Parents: :mini:`integer`


**type** :mini:`int64`
   :Parents: :mini:`integer`


**type** :mini:`integer`
   :Parents: :mini:`number`


**method** :mini:`real(int32 Arg₁)`

**method** :mini:`real(int64 Arg₁)`

**method** :mini:`real(integer Arg₁)`

**method** :mini:`++ integer Int` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`


**method** :mini:`-- integer Int` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`


**method** :mini:`integer Int₁ / integer Int₂` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.


**method** :mini:`integer Int₁ % integer Int₂` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.


**method** :mini:`integer Int₁:div(integer Int₂)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.


**method** :mini:`integer Int₁:mod(integer Int₂)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.


**method** :mini:`integer Int₁ <> integer Int₂` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.


**method** :mini:`integer Int₁ <> real Real₂` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.


**method** :mini:`string(integer Arg₁)`

**method** :mini:`string(integer Arg₁, integer Arg₂)`

