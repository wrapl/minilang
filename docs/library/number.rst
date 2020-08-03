number
======

.. include:: <isonum.txt>

:mini:`number`
   Base type for integers and reals.

   :Parents: :mini:`function`

   *Defined at line 921 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 939 in src/ml_types.c*

:mini:`int32`
   :Parents: :mini:`integer`

   *Defined at line 942 in src/ml_types.c*

:mini:`int64`
   :Parents: :mini:`integer`

   *Defined at line 952 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 999 in src/ml_types.c*

:mini:`meth integer(Arg₁: real)`
   *Defined at line 1040 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1045 in src/ml_types.c*

:mini:`double`
   :Parents: :mini:`real`

   *Defined at line 1052 in src/ml_types.c*

:mini:`meth integer(Real: real)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).

   *Defined at line 1083 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1095 in src/ml_types.c*

:mini:`meth real(Arg₁: int32)`
   *Defined at line 1135 in src/ml_types.c*

:mini:`meth real(Arg₁: int64)`
   *Defined at line 1140 in src/ml_types.c*

:mini:`meth real(Arg₁: integer)`
   *Defined at line 1151 in src/ml_types.c*

:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`

   *Defined at line 1217 in src/ml_types.c*

:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`

   *Defined at line 1225 in src/ml_types.c*

:mini:`meth ++(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`

   *Defined at line 1233 in src/ml_types.c*

:mini:`meth --(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`

   *Defined at line 1241 in src/ml_types.c*

:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.

   *Defined at line 1253 in src/ml_types.c*

:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.

   *Defined at line 1269 in src/ml_types.c*

:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.

   *Defined at line 1283 in src/ml_types.c*

:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.

   *Defined at line 1302 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1360 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1373 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1386 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1399 in src/ml_types.c*

:mini:`meth string(Arg₁: integer)`
   *Defined at line 1902 in src/ml_types.c*

:mini:`meth string(Arg₁: integer, Arg₂: integer)`
   *Defined at line 1909 in src/ml_types.c*

:mini:`meth string(Arg₁: real)`
   *Defined at line 1932 in src/ml_types.c*

:mini:`meth integer(Arg₁: string)`
   *Defined at line 1939 in src/ml_types.c*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *Defined at line 1944 in src/ml_types.c*

:mini:`meth real(Arg₁: string)`
   *Defined at line 1949 in src/ml_types.c*

