.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

math
====

.. rst-class:: mini-api

:mini:`meth (X: complex) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      (1 + 2i) ^ 2 :> -3 + 4i


:mini:`meth (X: complex) ^ (Y: number): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      (1 + 2i) ^ (2 + 3i) :> -0.0151327 - 0.179867i


:mini:`meth (Z: complex):abs: real`
   Returns the absolute value (magnitude) of :mini:`Z`.


:mini:`meth (Z: complex):arg: real`
   Returns the complex argument of :mini:`Z`.


:mini:`meth (Z: complex):conj: real`
   Returns the complex conjugate of :mini:`Z`.


:mini:`meth math::acos(Arg₁: complex): complex`
   Returns :mini:`acos(Arg₁)`.

   .. code-block:: mini

      math::acos(1.2345 + 6.789i) :> 1.39274 - 2.6296i
      math::acos(-1.2345 + 6.789i) :> 1.74885 - 2.6296i


:mini:`meth math::acosh(Arg₁: complex): complex`
   Returns :mini:`acosh(Arg₁)`.

   .. code-block:: mini

      math::acosh(1.2345 + 6.789i) :> 2.6296 + 1.39274i
      math::acosh(-1.2345 + 6.789i) :> 2.6296 + 1.74885i


:mini:`meth math::asin(Arg₁: complex): complex`
   Returns :mini:`asin(Arg₁)`.

   .. code-block:: mini

      math::asin(1.2345 + 6.789i) :> 0.178051 + 2.6296i
      math::asin(-1.2345 + 6.789i) :> -0.178051 + 2.6296i


:mini:`meth math::asinh(Arg₁: complex): complex`
   Returns :mini:`asinh(Arg₁)`.

   .. code-block:: mini

      math::asinh(1.2345 + 6.789i) :> 2.61977 + 1.38905i
      math::asinh(-1.2345 + 6.789i) :> -2.61977 + 1.38905i


:mini:`meth math::atan(Arg₁: complex): complex`
   Returns :mini:`atan(Arg₁)`.

   .. code-block:: mini

      math::atan(1.2345 + 6.789i) :> 1.54434 + 0.143461i
      math::atan(-1.2345 + 6.789i) :> -1.54434 + 0.143461i


:mini:`meth math::atanh(Arg₁: complex): complex`
   Returns :mini:`atanh(Arg₁)`.

   .. code-block:: mini

      math::atanh(1.2345 + 6.789i) :> 0.0254155 + 1.42908i
      math::atanh(-1.2345 + 6.789i) :> -0.0254155 + 1.42908i


:mini:`meth math::cos(Arg₁: complex): complex`
   Returns :mini:`cos(Arg₁)`.

   .. code-block:: mini

      math::cos(1.2345 + 6.789i) :> 146.521 - 419.14i
      math::cos(-1.2345 + 6.789i) :> 146.521 + 419.14i


:mini:`meth math::cosh(Arg₁: complex): complex`
   Returns :mini:`cosh(Arg₁)`.

   .. code-block:: mini

      math::cosh(1.2345 + 6.789i) :> 1.63043 + 0.762073i
      math::cosh(-1.2345 + 6.789i) :> 1.63043 - 0.762073i


:mini:`meth math::exp(Arg₁: complex): complex`
   Returns :mini:`exp(Arg₁)`.

   .. code-block:: mini

      math::exp(1.2345 + 6.789i) :> 3.00632 + 1.66513i
      math::exp(-1.2345 + 6.789i) :> 0.254544 + 0.140986i


:mini:`meth math::log(Arg₁: complex): complex`
   Returns :mini:`log(Arg₁)`.

   .. code-block:: mini

      math::log(1.2345 + 6.789i) :> 1.93157 + 1.39092i
      math::log(-1.2345 + 6.789i) :> 1.93157 + 1.75067i


:mini:`meth math::log10(Arg₁: complex): complex`
   Returns :mini:`log10(Arg₁)`.

   .. code-block:: mini

      math::log10(1.2345 + 6.789i) :> 0.83887 + 0.60407i
      math::log10(-1.2345 + 6.789i) :> 0.83887 + 0.760306i


:mini:`meth math::sin(Arg₁: complex): complex`
   Returns :mini:`sin(Arg₁)`.

   .. code-block:: mini

      math::sin(1.2345 + 6.789i) :> 419.141 + 146.521i
      math::sin(-1.2345 + 6.789i) :> -419.141 + 146.521i


:mini:`meth math::sinh(Arg₁: complex): complex`
   Returns :mini:`sinh(Arg₁)`.

   .. code-block:: mini

      math::sinh(1.2345 + 6.789i) :> 1.37589 + 0.903059i
      math::sinh(-1.2345 + 6.789i) :> -1.37589 + 0.903059i


:mini:`meth math::sqrt(Arg₁: complex): complex`
   Returns :mini:`sqrt(Arg₁)`.

   .. code-block:: mini

      math::sqrt(1.2345 + 6.789i) :> 2.01678 + 1.68313i
      math::sqrt(-1.2345 + 6.789i) :> 1.68313 + 2.01678i


:mini:`meth math::square(C: complex): complex`
   Returns :mini:`C * C`

   .. code-block:: mini

      math::square(1 + 2i) :> -3 + 4i


:mini:`meth math::tan(Arg₁: complex): complex`
   Returns :mini:`tan(Arg₁)`.

   .. code-block:: mini

      math::tan(1.2345 + 6.789i) :> 1.58008e-06 + 1i
      math::tan(-1.2345 + 6.789i) :> -1.58008e-06 + 1i


:mini:`meth math::tanh(Arg₁: complex): complex`
   Returns :mini:`tanh(Arg₁)`.

   .. code-block:: mini

      math::tanh(1.2345 + 6.789i) :> 0.905042 + 0.130855i
      math::tanh(-1.2345 + 6.789i) :> -0.905042 + 0.130855i


.. _fun-mlrandomseed:

:mini:`fun mlrandomseed(Arg₁: integer)`
   *TBD*


:mini:`meth !(N: integer): integer`
   Returns the factorial of :mini:`N`.

   .. code-block:: mini

      !10 :> 3628800


:mini:`meth (N: integer) ! (R: integer): integer`
   Returns the number of ways of choosing :mini:`R` elements from :mini:`N`.


:mini:`meth (X: integer) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      let N := 2 ^ 2 :> 4
      type(N) :> <<int32>>
      let R := 2 ^ -1 :> 0.5
      type(R) :> <<double>>


:mini:`meth (N: integer):abs: integer`
   Returns the absolute value of :mini:`N`.


:mini:`meth (N: integer):floor: integer`
   Returns the floor of :mini:`N` (:mini:`= N` for an integer).


:mini:`meth (A: integer):gcd(B: integer): integer`
   Returns the greatest common divisor of :mini:`A` and :mini:`B`.


:mini:`meth math::sqrt(Arg₁: integer): integer | real`
   Returns the square root of :mini:`Arg₁`.


:mini:`meth math::square(N: integer): integer`
   Returns :mini:`N * N`

   .. code-block:: mini

      math::square(10) :> 100


.. _value-math-pi:

:mini:`def math::pi: real`
   Pi.


.. _value-math-e:

:mini:`def math::e: real`
   Euler's constant.


:mini:`meth (X: number) ^ (Y: complex): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      2.3 ^ (1 + 2i) :> -0.218222 + 2.28962i


.. _type-random:

:mini:`type random < function`
   *TBD*


:mini:`meth (X: real) % (Y: real): real`
   Returns the remainder of :mini:`X` on division by :mini:`Y`.


:mini:`meth (X: real) ^ (Y: integer): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      2.3 ^ 2 :> 5.29


:mini:`meth (X: real) ^ (Y: real): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      let R := 2.3 ^ 1.5 :> 3.48812
      type(R) :> <<double>>
      let C := -2.3 ^ 1.5 :> -6.40758e-16 - 3.48812i
      type(C) :> <<complex>>


:mini:`meth (R: real):arg: real`
   Returns the complex argument of :mini:`R` (:mini:`= 0` for a real number).


:mini:`meth (R: real):conj: real`
   Returns the complex conjugate of :mini:`R` (:mini:`= R` for a real number).


:mini:`meth math::abs(Arg₁: real): real`
   Returns :mini:`abs(Arg₁)`.

   .. code-block:: mini

      math::abs(1.2345) :> 1.2345
      math::abs(-1.2345) :> 1.2345


:mini:`meth math::acos(Arg₁: real): real`
   Returns :mini:`acos(Arg₁)`.

   .. code-block:: mini

      math::acos(1.2345) :> nan
      math::acos(-1.2345) :> nan


:mini:`meth math::acosh(Arg₁: real): real`
   Returns :mini:`acosh(Arg₁)`.

   .. code-block:: mini

      math::acosh(1.2345) :> 0.672113
      math::acosh(-1.2345) :> -nan


:mini:`meth math::asin(Arg₁: real): real`
   Returns :mini:`asin(Arg₁)`.

   .. code-block:: mini

      math::asin(1.2345) :> nan
      math::asin(-1.2345) :> nan


:mini:`meth math::asinh(Arg₁: real): real`
   Returns :mini:`asinh(Arg₁)`.

   .. code-block:: mini

      math::asinh(1.2345) :> 1.03787
      math::asinh(-1.2345) :> -1.03787


:mini:`meth math::atan(Arg₁: real): real`
   Returns :mini:`atan(Arg₁)`.

   .. code-block:: mini

      math::atan(1.2345) :> 0.889961
      math::atan(-1.2345) :> -0.889961


:mini:`meth math::atan(Arg₁: real, Arg₂: real): real`
   Returns :mini:`atan(Arg₂ / Arg₁)`.


:mini:`meth math::atanh(Arg₁: real): real`
   Returns :mini:`atanh(Arg₁)`.

   .. code-block:: mini

      math::atanh(1.2345) :> -nan
      math::atanh(-1.2345) :> -nan


:mini:`meth math::cbrt(Arg₁: real): real`
   Returns :mini:`cbrt(Arg₁)`.

   .. code-block:: mini

      math::cbrt(1.2345) :> 1.07275
      math::cbrt(-1.2345) :> -1.07275


:mini:`meth math::ceil(Arg₁: real): real`
   Returns :mini:`ceil(Arg₁)`.

   .. code-block:: mini

      math::ceil(1.2345) :> 2
      math::ceil(-1.2345) :> -1


:mini:`meth math::cos(Arg₁: real): real`
   Returns :mini:`cos(Arg₁)`.

   .. code-block:: mini

      math::cos(1.2345) :> 0.329993
      math::cos(-1.2345) :> 0.329993


:mini:`meth math::cosh(Arg₁: real): real`
   Returns :mini:`cosh(Arg₁)`.

   .. code-block:: mini

      math::cosh(1.2345) :> 1.86382
      math::cosh(-1.2345) :> 1.86382


:mini:`meth math::erf(Arg₁: real): real`
   Returns :mini:`erf(Arg₁)`.

   .. code-block:: mini

      math::erf(1.2345) :> 0.919162
      math::erf(-1.2345) :> -0.919162


:mini:`meth math::erfc(Arg₁: real): real`
   Returns :mini:`erfc(Arg₁)`.

   .. code-block:: mini

      math::erfc(1.2345) :> 0.0808376
      math::erfc(-1.2345) :> 1.91916


:mini:`meth math::exp(Arg₁: real): real`
   Returns :mini:`exp(Arg₁)`.

   .. code-block:: mini

      math::exp(1.2345) :> 3.43666
      math::exp(-1.2345) :> 0.29098


:mini:`meth math::expm1(Arg₁: real): real`
   Returns :mini:`expm1(Arg₁)`.

   .. code-block:: mini

      math::expm1(1.2345) :> 2.43666
      math::expm1(-1.2345) :> -0.70902


:mini:`meth math::floor(Arg₁: real): real`
   Returns :mini:`floor(Arg₁)`.

   .. code-block:: mini

      math::floor(1.2345) :> 1
      math::floor(-1.2345) :> -2


:mini:`meth math::gamma(Arg₁: real): real`
   Returns :mini:`gamma(Arg₁)`.

   .. code-block:: mini

      math::gamma(1.2345) :> -0.0946016
      math::gamma(-1.2345) :> 1.42639


:mini:`meth math::hypot(Arg₁: real, Arg₂: real): real`
   Returns :mini:`hypot(Arg₁,  Arg₂)`.


:mini:`meth math::log(Arg₁: real): real`
   Returns :mini:`log(Arg₁)`.

   .. code-block:: mini

      math::log(1.2345) :> 0.210666
      math::log(-1.2345) :> 0.210666 + 3.14159i


:mini:`meth math::log10(Arg₁: real): real`
   Returns :mini:`log10(Arg₁)`.

   .. code-block:: mini

      math::log10(1.2345) :> 0.0914911
      math::log10(-1.2345) :> 0.0914911 + 1.36438i


:mini:`meth math::log1p(Arg₁: real): real`
   Returns :mini:`log1p(Arg₁)`.

   .. code-block:: mini

      math::log1p(1.2345) :> 0.804017
      math::log1p(-1.2345) :> -nan


:mini:`meth math::logit(Arg₁: real): real`
   Returns :mini:`logit(Arg₁)`.

   .. code-block:: mini

      math::logit(1.2345) :> -nan
      math::logit(-1.2345) :> -nan


:mini:`meth math::rem(Arg₁: real, Arg₂: real): real`
   Returns :mini:`rem(Arg₁,  Arg₂)`.


:mini:`meth math::round(Arg₁: real): real`
   Returns :mini:`round(Arg₁)`.

   .. code-block:: mini

      math::round(1.2345) :> 1
      math::round(-1.2345) :> -1


:mini:`meth math::round(Arg₁: real, Arg₂: real): real`
   Returns :mini:`round(Arg₁ * Arg₂) / Arg₂`.

   .. code-block:: mini

      math::round(1.2345, 100) :> 1.23
      math::round(-1.2345, 32) :> -1.25


:mini:`meth math::sin(Arg₁: real): real`
   Returns :mini:`sin(Arg₁)`.

   .. code-block:: mini

      math::sin(1.2345) :> 0.943983
      math::sin(-1.2345) :> -0.943983


:mini:`meth math::sinh(Arg₁: real): real`
   Returns :mini:`sinh(Arg₁)`.

   .. code-block:: mini

      math::sinh(1.2345) :> 1.57284
      math::sinh(-1.2345) :> -1.57284


:mini:`meth math::sqrt(Arg₁: real): real`
   Returns :mini:`sqrt(Arg₁)`.

   .. code-block:: mini

      math::sqrt(1.2345) :> 1.11108
      math::sqrt(-1.2345) :> 1.11108i


:mini:`meth math::square(R: real): real`
   Returns :mini:`R * R`

   .. code-block:: mini

      math::square(1.234) :> 1.52276


:mini:`meth math::tan(Arg₁: real): real`
   Returns :mini:`tan(Arg₁)`.

   .. code-block:: mini

      math::tan(1.2345) :> 2.86061
      math::tan(-1.2345) :> -2.86061


:mini:`meth math::tanh(Arg₁: real): real`
   Returns :mini:`tanh(Arg₁)`.

   .. code-block:: mini

      math::tanh(1.2345) :> 0.84388
      math::tanh(-1.2345) :> -0.84388


