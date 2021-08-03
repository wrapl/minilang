number
======

.. include:: <isonum.txt>

:mini:`type number`
   Base type for numbers.


:mini:`type real < number`
   Base type for real numbers.


:mini:`type integer < real, function`

:mini:`meth real(Arg₁: int32)`

:mini:`meth real(Arg₁: int64)`

:mini:`type integer < real, function`

:mini:`meth real(Arg₁: integer)`

:mini:`meth integer(Arg₁: double)`

:mini:`meth double(Arg₁: int32)`

:mini:`meth double(Arg₁: int64)`

:mini:`meth integer(Real: double)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).


:mini:`type double < real`

:mini:`meth double(Arg₁: integer)`

:mini:`type complex < number`

:mini:`meth complex(Arg₁: real)`

:mini:`meth real(Arg₁: complex)`

:mini:`meth :r(Z: complex)` |rarr| :mini:`real`
   Returns the real component of :mini:`Z`.


:mini:`meth :i(Z: complex)` |rarr| :mini:`real`
   Returns the imaginary component of :mini:`Z`.


:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`


:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`


:mini:`meth ++(Real: double)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`


:mini:`meth --(Real: double)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`


:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.


:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.


:mini:`meth |(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂` if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth !|(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂` if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.


:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.


:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.


:mini:`meth <>(Real₁: double, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.


:mini:`meth <>(Int₁: integer, Real₂: double)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.


:mini:`meth <>(Real₁: double, Real₂: double)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.


:mini:`meth string(Arg₁: integer)`

:mini:`meth string(Arg₁: integer, Arg₂: integer)`

:mini:`meth string(Arg₁: double)`

:mini:`meth string(Arg₁: complex)`

:mini:`meth integer(Arg₁: string)`

:mini:`meth integer(Arg₁: string, Arg₂: integer)`

:mini:`meth double(Arg₁: string)`

:mini:`meth real(Arg₁: string)`

:mini:`meth complex(Arg₁: string)`

:mini:`meth number(Arg₁: string)`

