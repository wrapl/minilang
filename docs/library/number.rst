number
======

.. include:: <isonum.txt>

:mini:`number`
   Base type for integers and reals.

   :Parents: :mini:`function`

   *Defined at line 1162 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1180 in src/ml_types.c*

:mini:`int32`
   :Parents: :mini:`integer`

   *Defined at line 1183 in src/ml_types.c*

:mini:`int64`
   :Parents: :mini:`integer`

   *Defined at line 1193 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1240 in src/ml_types.c*

:mini:`meth integer(Arg₁: real)`
   *Defined at line 1281 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1286 in src/ml_types.c*

:mini:`double`
   :Parents: :mini:`real`

   *Defined at line 1293 in src/ml_types.c*

:mini:`meth integer(Real: real)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).

   *Defined at line 1324 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1336 in src/ml_types.c*

:mini:`meth real(Arg₁: int32)`
   *Defined at line 1376 in src/ml_types.c*

:mini:`meth real(Arg₁: int64)`
   *Defined at line 1381 in src/ml_types.c*

:mini:`meth real(Arg₁: integer)`
   *Defined at line 1392 in src/ml_types.c*

:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`

   *Defined at line 1458 in src/ml_types.c*

:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`

   *Defined at line 1466 in src/ml_types.c*

:mini:`meth ++(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`

   *Defined at line 1474 in src/ml_types.c*

:mini:`meth --(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`

   *Defined at line 1482 in src/ml_types.c*

:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.

   *Defined at line 1494 in src/ml_types.c*

:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.

   *Defined at line 1510 in src/ml_types.c*

:mini:`meth |(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂`. if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.

   *Defined at line 1524 in src/ml_types.c*

:mini:`meth !|(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂`. if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.

   *Defined at line 1535 in src/ml_types.c*

:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.

   *Defined at line 1546 in src/ml_types.c*

:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.

   *Defined at line 1565 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1623 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1636 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1649 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1662 in src/ml_types.c*

:mini:`meth string(Arg₁: integer)`
   *Defined at line 2167 in src/ml_types.c*

:mini:`meth string(Arg₁: integer, Arg₂: integer)`
   *Defined at line 2174 in src/ml_types.c*

:mini:`meth string(Arg₁: real)`
   *Defined at line 2197 in src/ml_types.c*

:mini:`meth integer(Arg₁: string)`
   *Defined at line 2204 in src/ml_types.c*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *Defined at line 2216 in src/ml_types.c*

:mini:`meth real(Arg₁: string)`
   *Defined at line 2228 in src/ml_types.c*

