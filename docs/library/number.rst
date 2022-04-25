.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

number
======

.. _fun-integer-random:

:mini:`fun integer::random(Min?: number, Max?: number): integer`
   Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max <= 2³² - 1`.
   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`2³² - 1`.


.. _fun-real-random:

:mini:`fun real::random(Min?: number, Max?: number): real`
   Returns a random real between :mini:`Min` and :mini:`Max`.
   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`1`.


.. _type-complex:

:mini:`type complex < number`
   *TBD*


:mini:`meth complex(String: string): complex | error`
   Returns the complex number in :mini:`String` or an error if :mini:`String` does not contain a valid complex number.


:mini:`meth (Z: complex):i: real`
   Returns the imaginary component of :mini:`Z`.


:mini:`meth (Z: complex):r: real`
   Returns the real component of :mini:`Z`.


:mini:`meth real(Arg₁: complex)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(Value: complex)`
   Appends :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: complex, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string for the real and imaginary components.


.. _type-double:

:mini:`type double < real`
   *TBD*


:mini:`meth ++(Real: double): real`
   Returns :mini:`Real + 1`


:mini:`meth --(Real: double): real`
   Returns :mini:`Real - 1`


:mini:`meth (Real₁: double) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Real₁: double) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (Buffer: string::buffer):append(Value: double)`
   Appends :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: double, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.


.. _type-integer:

:mini:`type integer < real, function`
   *TBD*


:mini:`meth integer(String: string): integer | error`
   Returns the base :mini:`10` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.

   .. code-block:: mini

      integer("123") :> 123
      integer("ABC")
      :> error("ValueError", "Error parsing integer")


:mini:`meth integer(String: string, Base: integer): integer | error`
   Returns the base :mini:`Base` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.


:mini:`meth integer(Real: double): integer`
   Converts :mini:`Real` to an integer (using default rounding).


.. _fun-integer-random_cycle:

:mini:`fun integer::random_cycle(Max: integer): list`
   Returns a random cyclic permutation (no sub-cycles) of :mini:`1,  ...,  Max`.


.. _fun-integer-random_permutation:

:mini:`fun integer::random_permutation(Max: integer): list`
   Returns a random permutation of :mini:`1,  ...,  Max`.


:mini:`meth (Int₁: integer) !| (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth (Int₁: integer) % (Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.
   Note: the result is calculated by rounding towards 0. In particular,  if :mini:`Int₁` is negative,  the result will be negative.
   For a nonnegative remainder,  use :mini:`Int₁ mod Int₂`.


:mini:`meth ++(Int: integer): integer`
   Returns :mini:`Int + 1`


:mini:`meth --(Int: integer): integer`
   Returns :mini:`Int - 1`


:mini:`meth (Int₁: integer) / (Int₂: integer): integer | real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact,  otherwise as a real.

   .. code-block:: mini

      let N := 10 / 2 :> 5
      type(N) :> <<int32>>
      let R := 10 / 3 :> 3.33333
      type(R) :> <<double>>


:mini:`meth (A: integer) << (B: integer): integer`
   Returns :mini:`A << B`.


:mini:`meth (Int₁: integer) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Int₁: integer) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (A: integer) >> (B: integer): integer`
   Returns :mini:`A >> B`.


:mini:`meth (Int₁: integer):div(Int₂: integer): integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.
   The result is calculated by rounding down in all cases.


:mini:`meth (Int₁: integer):mod(Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.
   Note: the result is calculated by rounding down in all cases. In particular,  the result is always nonnegative.


:mini:`meth real(Arg₁: integer)`
   *TBD*


:mini:`meth (Int₁: integer) | (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth (Buffer: string::buffer):append(Value: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`10`.


:mini:`meth (Buffer: string::buffer):append(Value: integer, Base: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`Base`.


:mini:`meth (Buffer: string::buffer):append(Value: integer, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.


.. _type-number:

:mini:`type number`
   Base type for numbers.


:mini:`meth (Number: number):isfinite(Arg₂: double): number | nil`
   Returns :mini:`Number` if it is finite (neither |plusmn|\ |infin| nor ``NaN``),  otherwise returns :mini:`nil`.


:mini:`meth (Number: number):isnan(Arg₂: double): number | nil`
   Returns :mini:`Number` if it is ``NaN``,  otherwise returns :mini:`Number`.


.. _type-real:

:mini:`type real < complex`
   *TBD*


.. _value-real-NaN:

:mini:`def real::NaN: real`
   Not a number.


.. _value-real-Inf:

:mini:`def real::Inf: real`
   Positive infinity.


:mini:`meth real(String: string): real | error`
   Returns the real number in :mini:`String` or an error if :mini:`String` does not contain a valid real number.


:mini:`meth complex(Arg₁: real)`
   *TBD*


:mini:`meth complex(Arg₁: real, Arg₂: real)`
   *TBD*


:mini:`meth number(String: string): integer | real | complex | error`
   Returns the number in :mini:`String` or an error if :mini:`String` does not contain a valid number.


