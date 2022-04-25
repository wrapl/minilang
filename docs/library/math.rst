.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

math
====

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


:mini:`meth math::square(C: complex): complex`
   Returns :mini:`C * C`

   .. code-block:: mini

      math::square(1 + 2i) :> -3 + 4i


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


:mini:`meth math::atan(Arg₁: real, Arg₂: real): real`
   Returns :mini:`atan(Arg₂ / Arg₁)`.


:mini:`meth math::square(R: real): real`
   Returns :mini:`R * R`

   .. code-block:: mini

      math::square(1.234) :> 1.52276


