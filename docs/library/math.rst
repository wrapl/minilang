math
====

.. include:: <isonum.txt>

:mini:`meth %(Arg₁: real, Arg₂: real)`

:mini:`meth ^(Arg₁: integer, Arg₂: integer)`

:mini:`meth ^(Arg₁: real, Arg₂: integer)`

:mini:`meth ^(Arg₁: real, Arg₂: real)`

:mini:`meth ^(Arg₁: complex, Arg₂: integer)`

:mini:`meth ^(Arg₁: complex, Arg₂: number)`

:mini:`meth ^(Arg₁: number, Arg₂: complex)`

:mini:`meth !(Arg₁: integer)`

:mini:`meth !(Arg₁: integer, Arg₂: integer)`

:mini:`meth atan(Arg₁: real, Arg₂: real)` |rarr| :mini:`number`

:mini:`meth ceil(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth fabs(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth floor(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth sqrt(Arg₁: integer)` |rarr| :mini:`number`

:mini:`meth erf(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth erfc(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth hypot(Arg₁: number, Arg₂: number)` |rarr| :mini:`number`

:mini:`meth lgamma(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth cbrt(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth expm1(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth log1p(Arg₁: number)` |rarr| :mini:`number`

:mini:`meth remainder(Arg₁: number, Arg₂: number)` |rarr| :mini:`number`

:mini:`meth round(Arg₁: number)` |rarr| :mini:`number`

:mini:`fun integer::random(Min?: number, Max?: number)` |rarr| :mini:`integer`
   Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max <= 2³² - 1`.

   If omitted, :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`2³² - 1`.


:mini:`fun integer::random_permutation(Max: integer)`

:mini:`fun integer::random_cycle(Max: integer)`

:mini:`fun real::random(Min?: number, Max?: number)` |rarr| :mini:`real`
   Returns a random real between :mini:`Min` and :mini:`Max`.

   If omitted, :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`1`.


