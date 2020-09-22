number
======

.. include:: <isonum.txt>

:mini:`number`
   Base type for integers and reals.

   :Parents: :mini:`function`

   *Defined at line 983 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1001 in src/ml_types.c*

:mini:`int32`
   :Parents: :mini:`integer`

   *Defined at line 1004 in src/ml_types.c*

:mini:`int64`
   :Parents: :mini:`integer`

   *Defined at line 1014 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1061 in src/ml_types.c*

:mini:`meth integer(Arg₁: real)`
   *Defined at line 1102 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1107 in src/ml_types.c*

:mini:`double`
   :Parents: :mini:`real`

   *Defined at line 1114 in src/ml_types.c*

:mini:`meth integer(Real: real)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).

   *Defined at line 1145 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1157 in src/ml_types.c*

:mini:`meth real(Arg₁: int32)`
   *Defined at line 1197 in src/ml_types.c*

:mini:`meth real(Arg₁: int64)`
   *Defined at line 1202 in src/ml_types.c*

:mini:`meth real(Arg₁: integer)`
   *Defined at line 1213 in src/ml_types.c*

:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`

   *Defined at line 1279 in src/ml_types.c*

:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`

   *Defined at line 1287 in src/ml_types.c*

:mini:`meth ++(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`

   *Defined at line 1295 in src/ml_types.c*

:mini:`meth --(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`

   *Defined at line 1303 in src/ml_types.c*

:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.

   *Defined at line 1315 in src/ml_types.c*

:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.

   *Defined at line 1331 in src/ml_types.c*

:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.

   *Defined at line 1345 in src/ml_types.c*

:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.

   *Defined at line 1364 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1422 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1435 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1448 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1461 in src/ml_types.c*

:mini:`meth string(Arg₁: integer)`
   *Defined at line 1963 in src/ml_types.c*

:mini:`meth string(Arg₁: integer, Arg₂: integer)`
   *Defined at line 1970 in src/ml_types.c*

:mini:`meth string(Arg₁: real)`
   *Defined at line 1993 in src/ml_types.c*

:mini:`meth integer(Arg₁: string)`
   *Defined at line 2000 in src/ml_types.c*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *Defined at line 2012 in src/ml_types.c*

:mini:`meth real(Arg₁: string)`
   *Defined at line 2024 in src/ml_types.c*

