number
======

.. include:: <isonum.txt>

:mini:`number`
   Base type for integers and reals.

   :Parents: :mini:`function`

   *Defined at line 935 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 953 in src/ml_types.c*

:mini:`int32`
   :Parents: :mini:`integer`

   *Defined at line 956 in src/ml_types.c*

:mini:`int64`
   :Parents: :mini:`integer`

   *Defined at line 966 in src/ml_types.c*

:mini:`integer`
   :Parents: :mini:`number`

   *Defined at line 1013 in src/ml_types.c*

:mini:`meth integer(Arg₁: real)`
   *Defined at line 1054 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1059 in src/ml_types.c*

:mini:`double`
   :Parents: :mini:`real`

   *Defined at line 1066 in src/ml_types.c*

:mini:`meth integer(Real: real)` |rarr| :mini:`integer`
   Converts :mini:`Real` to an integer (using default rounding).

   *Defined at line 1097 in src/ml_types.c*

:mini:`real`
   :Parents: :mini:`number`

   *Defined at line 1109 in src/ml_types.c*

:mini:`meth real(Arg₁: int32)`
   *Defined at line 1149 in src/ml_types.c*

:mini:`meth real(Arg₁: int64)`
   *Defined at line 1154 in src/ml_types.c*

:mini:`meth real(Arg₁: integer)`
   *Defined at line 1165 in src/ml_types.c*

:mini:`meth ++(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int + 1`

   *Defined at line 1231 in src/ml_types.c*

:mini:`meth --(Int: integer)` |rarr| :mini:`integer`
   Returns :mini:`Int - 1`

   *Defined at line 1239 in src/ml_types.c*

:mini:`meth ++(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real + 1`

   *Defined at line 1247 in src/ml_types.c*

:mini:`meth --(Real: real)` |rarr| :mini:`real`
   Returns :mini:`Real - 1`

   *Defined at line 1255 in src/ml_types.c*

:mini:`meth /(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer` or :mini:`real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact, otherwise as a real.

   *Defined at line 1267 in src/ml_types.c*

:mini:`meth %(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int₁` is negative, the result will be negative.

   For a nonnegative remainder, use :mini:`Int₁ mod Int₂`.

   *Defined at line 1283 in src/ml_types.c*

:mini:`meth :div(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.

   *Defined at line 1297 in src/ml_types.c*

:mini:`meth :mod(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.

   *Defined at line 1316 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1374 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Int₂: integer)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Int₂`.

   *Defined at line 1387 in src/ml_types.c*

:mini:`meth <>(Int₁: integer, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1400 in src/ml_types.c*

:mini:`meth <>(Real₁: real, Real₂: real)` |rarr| :mini:`integer`
   Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than, equal to or greater than :mini:`Real₂`.

   *Defined at line 1413 in src/ml_types.c*

:mini:`meth string(Arg₁: integer)`
   *Defined at line 1915 in src/ml_types.c*

:mini:`meth string(Arg₁: integer, Arg₂: integer)`
   *Defined at line 1922 in src/ml_types.c*

:mini:`meth string(Arg₁: real)`
   *Defined at line 1945 in src/ml_types.c*

:mini:`meth integer(Arg₁: string)`
   *Defined at line 1952 in src/ml_types.c*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *Defined at line 1957 in src/ml_types.c*

:mini:`meth real(Arg₁: string)`
   *Defined at line 1962 in src/ml_types.c*

