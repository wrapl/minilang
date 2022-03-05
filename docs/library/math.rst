.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

math
====

:mini:`meth (Arg₁: complex):SquareMethod`
   *TBD*


:mini:`meth (Z: complex):abs: real`
   Returns the absolute value (magnitude) of :mini:`Z`.



:mini:`meth (Z: complex):arg: real`
   Returns the complex argument of :mini:`Z`.



:mini:`meth (Z: complex):conj: real`
   Returns the complex conjugate of :mini:`Z`.



:mini:`meth (X: complex) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth (X: complex) ^ (Y: number): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth (N: integer) ! (R: integer): integer`
   Returns the number of ways of choosing :mini:`R` elements from :mini:`N`.



:mini:`meth !(N: integer): integer`
   Returns the factorial of :mini:`N`.



:mini:`meth (Arg₁: integer):SquareMethod`
   *TBD*


:mini:`meth (N: integer):abs: integer`
   Returns the absolute value of :mini:`N`.



:mini:`meth (N: integer):floor: integer`
   Returns the floor of :mini:`N` (:mini:`= N` for an integer).



:mini:`meth (A: integer):gcd(B: integer): integer`
   Returns the greatest common divisor of :mini:`A` and :mini:`B`.



:mini:`meth (X: integer) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth math::sqrt(Arg₁: integer): integer | real`
   Returns the square root of :mini:`Arg₁`.



.. _fun-integer-random:

:mini:`fun integer::random(Min?: number, Max?: number): integer`
   Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max <= 2³² - 1`.

   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`2³² - 1`.



.. _fun-integer-random_cycle:

:mini:`fun integer::random_cycle(Max: integer): list`
   Returns a random cyclic permutation (no sub-cycles) of :mini:`1,  ...,  Max`.



.. _fun-integer-random_permutation:

:mini:`fun integer::random_permutation(Max: integer): list`
   Returns a random permutation of :mini:`1,  ...,  Max`.



.. _value-math-pi:

:mini:`def math::pi: real`
   Pi.



.. _value-math-e:

:mini:`def math::e: real`
   Euler's constant.



:mini:`meth (X: number) ^ (Y: complex): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth (X: real) % (Y: real): real`
   Returns the remainder of :mini:`X` on division by :mini:`Y`.



:mini:`meth (Arg₁: real):SquareMethod`
   *TBD*


:mini:`meth (R: real):arg: real`
   Returns the complex argument of :mini:`R` (:mini:`= 0` for a real number).



:mini:`meth (R: real):conj: real`
   Returns the complex conjugate of :mini:`R` (:mini:`= R` for a real number).



:mini:`meth (X: real) ^ (Y: real): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth (X: real) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.



:mini:`meth math::atan(Arg₁: real, Arg₂: real): real`
   Returns :mini:`atan(Arg₂ / Arg₁)`.



:mini:`meth math::cbrt(Arg₁: real): real`
   Returns :mini:`cbrt(Arg₁)`.



:mini:`meth math::ceil(Arg₁: real): real`
   Returns :mini:`ceil(Arg₁)`.



:mini:`meth math::erf(Arg₁: real): real`
   Returns :mini:`erf(Arg₁)`.



:mini:`meth math::erfc(Arg₁: real): real`
   Returns :mini:`erfc(Arg₁)`.



:mini:`meth math::expm1(Arg₁: real): real`
   Returns :mini:`expm1(Arg₁)`.



:mini:`meth math::fabs(Arg₁: real): real`
   Returns :mini:`fabs(Arg₁)`.



:mini:`meth math::floor(Arg₁: real): real`
   Returns :mini:`floor(Arg₁)`.



:mini:`meth math::hypot(Arg₁: real, Arg₂: real): real`
   Returns :mini:`hypot(Arg₁,  Arg₂)`.



:mini:`meth math::lgamma(Arg₁: real): real`
   Returns :mini:`lgamma(Arg₁)`.



:mini:`meth math::log(Arg₁: real): real`
   Returns :mini:`log(Arg₁)`.



:mini:`meth math::log10(Arg₁: real): real`
   Returns :mini:`log10(Arg₁)`.



:mini:`meth math::log1p(Arg₁: real): real`
   Returns :mini:`log1p(Arg₁)`.



:mini:`meth math::remainder(Arg₁: real, Arg₂: real): real`
   Returns :mini:`remainder(Arg₁,  Arg₂)`.



:mini:`meth math::round(Arg₁: real): real`
   Returns :mini:`round(Arg₁)`.



:mini:`meth math::sqrt(Arg₁: real): real`
   Returns :mini:`sqrt(Arg₁)`.



.. _fun-real-random:

:mini:`fun real::random(Min?: number, Max?: number): real`
   Returns a random real between :mini:`Min` and :mini:`Max`.

   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`1`.



