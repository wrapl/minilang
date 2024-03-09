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

      (1 + 2i) ^ (2 + 3i)
      :> -0.0151326724227227 - 0.179867483913335i


:mini:`meth (Z: complex):abs: real`
   Returns the absolute value (magnitude) of :mini:`Z`.


:mini:`meth (Z: complex):arg: real`
   Returns the complex argument of :mini:`Z`.


:mini:`meth (Z: complex):conj: real`
   Returns the complex conjugate of :mini:`Z`.


:mini:`meth math::acos(Arg₁: complex): complex`
   Returns :mini:`acos(Arg₁)`.

   .. code-block:: mini

      math::acos(1.2345 + 6.789i)
      :> 1.39274491905556 - 2.62959948793467i
      math::acos(-1.2345 + 6.789i)
      :> 1.74884773453423 - 2.62959948793467i


:mini:`meth math::acosh(Arg₁: complex): complex`
   Returns :mini:`acosh(Arg₁)`.

   .. code-block:: mini

      math::acosh(1.2345 + 6.789i)
      :> 2.62959948793467 + 1.39274491905556i
      math::acosh(-1.2345 + 6.789i)
      :> 2.62959948793467 + 1.74884773453423i


:mini:`meth math::asin(Arg₁: complex): complex`
   Returns :mini:`asin(Arg₁)`.

   .. code-block:: mini

      math::asin(1.2345 + 6.789i)
      :> 0.178051407739337 + 2.62959948793467i
      math::asin(-1.2345 + 6.789i)
      :> -0.178051407739337 + 2.62959948793467i


:mini:`meth math::asinh(Arg₁: complex): complex`
   Returns :mini:`asinh(Arg₁)`.

   .. code-block:: mini

      math::asinh(1.2345 + 6.789i)
      :> 2.61977023992049 + 1.38904733381322i
      math::asinh(-1.2345 + 6.789i)
      :> -2.61977023992049 + 1.38904733381322i


:mini:`meth math::atan(Arg₁: complex): complex`
   Returns :mini:`atan(Arg₁)`.

   .. code-block:: mini

      math::atan(1.2345 + 6.789i)
      :> 1.54433788133329 + 0.143460974564643i
      math::atan(-1.2345 + 6.789i)
      :> -1.54433788133329 + 0.143460974564643i


:mini:`meth math::atanh(Arg₁: complex): complex`
   Returns :mini:`atanh(Arg₁)`.

   .. code-block:: mini

      math::atanh(1.2345 + 6.789i)
      :> 0.0254155192875644 + 1.42907622916881i
      math::atanh(-1.2345 + 6.789i)
      :> -0.0254155192875644 + 1.42907622916881i


:mini:`meth math::cos(Arg₁: complex): complex`
   Returns :mini:`cos(Arg₁)`.

   .. code-block:: mini

      math::cos(1.2345 + 6.789i)
      :> 146.521288000429 - 419.139907811584i
      math::cos(-1.2345 + 6.789i)
      :> 146.521288000429 + 419.139907811584i


:mini:`meth math::cosh(Arg₁: complex): complex`
   Returns :mini:`cosh(Arg₁)`.

   .. code-block:: mini

      math::cosh(1.2345 + 6.789i)
      :> 1.63043250480246 + 0.762072763912413i
      math::cosh(-1.2345 + 6.789i)
      :> 1.63043250480246 - 0.762072763912413i


:mini:`meth math::exp(Arg₁: complex): complex`
   Returns :mini:`exp(Arg₁)`.

   .. code-block:: mini

      math::exp(1.2345 + 6.789i)
      :> 3.00632132754822 + 1.66513134304082i
      math::exp(-1.2345 + 6.789i)
      :> 0.254543682056692 + 0.140985815215994i


:mini:`meth math::log(Arg₁: complex): complex`
   Returns :mini:`log(Arg₁)`.

   .. code-block:: mini

      math::log(1.2345 + 6.789i)
      :> 1.93156878648542 + 1.39092338385419i
      math::log(-1.2345 + 6.789i)
      :> 1.93156878648542 + 1.75066926973561i


:mini:`meth math::log10(Arg₁: complex): complex`
   Returns :mini:`log10(Arg₁)`.

   .. code-block:: mini

      math::log10(1.2345 + 6.789i)
      :> 0.838869665387177 + 0.604070350358072i
      math::log10(-1.2345 + 6.789i)
      :> 0.838869665387177 + 0.76030600348377i


:mini:`meth math::sin(Arg₁: complex): complex`
   Returns :mini:`sin(Arg₁)`.

   .. code-block:: mini

      math::sin(1.2345 + 6.789i)
      :> 419.14097082583 + 146.520916397013i
      math::sin(-1.2345 + 6.789i)
      :> -419.14097082583 + 146.520916397013i


:mini:`meth math::sinh(Arg₁: complex): complex`
   Returns :mini:`sinh(Arg₁)`.

   .. code-block:: mini

      math::sinh(1.2345 + 6.789i)
      :> 1.37588882274576 + 0.903058579128407i
      math::sinh(-1.2345 + 6.789i)
      :> -1.37588882274576 + 0.903058579128407i


:mini:`meth math::sqrt(Arg₁: complex): complex`
   Returns :mini:`sqrt(Arg₁)`.

   .. code-block:: mini

      math::sqrt(1.2345 + 6.789i)
      :> 2.01678294499314 + 1.68312609367665i
      math::sqrt(-1.2345 + 6.789i)
      :> 1.68312609367665 + 2.01678294499314i


:mini:`meth math::square(C: complex): complex`
   Returns :mini:`C * C`

   .. code-block:: mini

      math::square(1 + 2i) :> -3 + 4i


:mini:`meth math::tan(Arg₁: complex): complex`
   Returns :mini:`tan(Arg₁)`.

   .. code-block:: mini

      math::tan(1.2345 + 6.789i)
      :> 1.58008203338542e-06 + 1.0000019838211i
      math::tan(-1.2345 + 6.789i)
      :> -1.58008203338542e-06 + 1.0000019838211i


:mini:`meth math::tanh(Arg₁: complex): complex`
   Returns :mini:`tanh(Arg₁)`.

   .. code-block:: mini

      math::tanh(1.2345 + 6.789i)
      :> 0.905042091321087 + 0.130855248843389i
      math::tanh(-1.2345 + 6.789i)
      :> -0.905042091321087 + 0.130855248843389i


:mini:`fun random::seed(Arg₁: integer)`
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


:mini:`def math::p: real`
   Pi.


:mini:`def math::: real`
   Euler's constant.


:mini:`meth (X: number) ^ (Y: complex): number`
   Returns :mini:`X` raised to the power of :mini:`Y`.

   .. code-block:: mini

      2.3 ^ (1 + 2i) :> -0.218221674358723 + 2.28962427066977i


:mini:`type random < (MLFunction`
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

      let R := 2.3 ^ 1.5 :> 3.48812270426371
      type(R) :> <<double>>
      let C := -2.3 ^ 1.5
      :> -6.40757745721465e-16 - 3.48812270426371i
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

      math::acosh(1.2345) :> 0.672113366870564
      math::acosh(-1.2345) :> -nan


:mini:`meth math::asin(Arg₁: real): real`
   Returns :mini:`asin(Arg₁)`.

   .. code-block:: mini

      math::asin(1.2345) :> nan
      math::asin(-1.2345) :> nan


:mini:`meth math::asinh(Arg₁: real): real`
   Returns :mini:`asinh(Arg₁)`.

   .. code-block:: mini

      math::asinh(1.2345) :> 1.03787350829816
      math::asinh(-1.2345) :> -1.03787350829816


:mini:`meth math::atan(Arg₁: real): real`
   Returns :mini:`atan(Arg₁)`.

   .. code-block:: mini

      math::atan(1.2345) :> 0.88996059643618
      math::atan(-1.2345) :> -0.88996059643618


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

      math::cbrt(1.2345) :> 1.07274631515821
      math::cbrt(-1.2345) :> -1.07274631515821


:mini:`meth math::ceil(Arg₁: real): real`
   Returns :mini:`ceil(Arg₁)`.

   .. code-block:: mini

      math::ceil(1.2345) :> 2
      math::ceil(-1.2345) :> -1


:mini:`meth math::cos(Arg₁: real): real`
   Returns :mini:`cos(Arg₁)`.

   .. code-block:: mini

      math::cos(1.2345) :> 0.329993157678568
      math::cos(-1.2345) :> 0.329993157678568


:mini:`meth math::cosh(Arg₁: real): real`
   Returns :mini:`cosh(Arg₁)`.

   .. code-block:: mini

      math::cosh(1.2345) :> 1.86381998863995
      math::cosh(-1.2345) :> 1.86381998863995


:mini:`meth math::erf(Arg₁: real): real`
   Returns :mini:`erf(Arg₁)`.

   .. code-block:: mini

      math::erf(1.2345) :> 0.919162396413566
      math::erf(-1.2345) :> -0.919162396413566


:mini:`meth math::erfc(Arg₁: real): real`
   Returns :mini:`erfc(Arg₁)`.

   .. code-block:: mini

      math::erfc(1.2345) :> 0.0808376035864342
      math::erfc(-1.2345) :> 1.91916239641357


:mini:`meth math::exp(Arg₁: real): real`
   Returns :mini:`exp(Arg₁)`.

   .. code-block:: mini

      math::exp(1.2345) :> 3.43665976117046
      math::exp(-1.2345) :> 0.290980216109441


:mini:`meth math::expm1(Arg₁: real): real`
   Returns :mini:`expm1(Arg₁)`.

   .. code-block:: mini

      math::expm1(1.2345) :> 2.43665976117046
      math::expm1(-1.2345) :> -0.709019783890559


:mini:`meth math::floor(Arg₁: real): real`
   Returns :mini:`floor(Arg₁)`.

   .. code-block:: mini

      math::floor(1.2345) :> 1
      math::floor(-1.2345) :> -2


:mini:`meth math::gamma(Arg₁: real): real`
   Returns :mini:`gamma(Arg₁)`.

   .. code-block:: mini

      math::gamma(1.2345) :> -0.0946016466793967
      math::gamma(-1.2345) :> 1.42638586810001


:mini:`meth math::hypot(Arg₁: real, Arg₂: real): real`
   Returns :mini:`hypot(Arg₁,  Arg₂)`.


:mini:`meth math::log(Arg₁: real): real`
   Returns :mini:`log(Arg₁)`.

   .. code-block:: mini

      math::log(1.2345) :> 0.210666029803097
      math::log(-1.2345)
      :> 0.210666029803097 + 3.14159265358979i


:mini:`meth math::log10(Arg₁: real): real`
   Returns :mini:`log10(Arg₁)`.

   .. code-block:: mini

      math::log10(1.2345) :> 0.091491094267951
      math::log10(-1.2345)
      :> 0.091491094267951 + 1.36437635384184i


:mini:`meth math::log1p(Arg₁: real): real`
   Returns :mini:`log1p(Arg₁)`.

   .. code-block:: mini

      math::log1p(1.2345) :> 0.804017489391369
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

      math::sin(1.2345) :> 0.943983323944511
      math::sin(-1.2345) :> -0.943983323944511


:mini:`meth math::sinh(Arg₁: real): real`
   Returns :mini:`sinh(Arg₁)`.

   .. code-block:: mini

      math::sinh(1.2345) :> 1.57283977253051
      math::sinh(-1.2345) :> -1.57283977253051


:mini:`meth math::sqrt(Arg₁: real): real`
   Returns :mini:`sqrt(Arg₁)`.

   .. code-block:: mini

      math::sqrt(1.2345) :> 1.11108055513541
      math::sqrt(-1.2345) :> 1.11108055513541i


:mini:`meth math::square(R: real): real`
   Returns :mini:`R * R`

   .. code-block:: mini

      math::square(1.234) :> 1.522756


:mini:`meth math::tan(Arg₁: real): real`
   Returns :mini:`tan(Arg₁)`.

   .. code-block:: mini

      math::tan(1.2345) :> 2.86061483997194
      math::tan(-1.2345) :> -2.86061483997194


:mini:`meth math::tanh(Arg₁: real): real`
   Returns :mini:`tanh(Arg₁)`.

   .. code-block:: mini

      math::tanh(1.2345) :> 0.843879656896602
      math::tanh(-1.2345) :> -0.843879656896602


