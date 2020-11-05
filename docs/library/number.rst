number
======

.. include:: <isonum.txt>

:mini:`number`
   Base type for integers and reals.

   :Parents: :mini:`function`

   *Defined at line 1161 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1179 in src/ml_types.c*

:mini:`int32`
   :Parents: :mini:`integer`

   *Defined at line 1182 in src/ml_types.c*

:mini:`int64`
   :Parents: :mini:`integer`

   *Defined at line 1192 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1239 in src/ml_types.c*

:mini:`meth integer(Arg₁: real)`
   *Defined at line 1280 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1285 in src/ml_types.c*

:mini:`double`
   :Parents: :mini:`real`

   *Defined at line 1292 in src/ml_types.c*

:mini:`meth integer(Real: real)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).

   *Defined at line 1323 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1335 in src/ml_types.c*

:mini:`meth real(Arg₁: int32)`
   *Defined at line 1375 in src/ml_types.c*

:mini:`meth real(Arg₁: int64)`
   *Defined at line 1380 in src/ml_types.c*

:mini:`meth real(Arg₁: integer)`
   *Defined at line 1391 in src/ml_types.c*

:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`

   *Defined at line 1457 in src/ml_types.c*

:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`

   *Defined at line 1465 in src/ml_types.c*

:mini:`meth ++(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`

   *Defined at line 1473 in src/ml_types.c*

:mini:`meth --(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`

   *Defined at line 1481 in src/ml_types.c*

:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.

   *Defined at line 1493 in src/ml_types.c*

:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.

   *Defined at line 1509 in src/ml_types.c*

:mini:`meth |(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂`. if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.

   *Defined at line 1523 in src/ml_types.c*

:mini:`meth !|(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int₂`. if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.

   *Defined at line 1534 in src/ml_types.c*

:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.

   *Defined at line 1545 in src/ml_types.c*

:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.

   *Defined at line 1564 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1622 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1635 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1648 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1661 in src/ml_types.c*

:mini:`meth string(Arg₁: integer)`
   *Defined at line 2166 in src/ml_types.c*

:mini:`meth string(Arg₁: integer, Arg₂: integer)`
   *Defined at line 2173 in src/ml_types.c*

:mini:`meth string(Arg₁: real)`
   *Defined at line 2196 in src/ml_types.c*

:mini:`meth integer(Arg₁: string)`
   *Defined at line 2203 in src/ml_types.c*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *Defined at line 2215 in src/ml_types.c*

:mini:`meth real(Arg₁: string)`
   *Defined at line 2227 in src/ml_types.c*

