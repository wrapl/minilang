.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

number
======

.. _type-complex:

:mini:`type complex < number`
   *TBD*


:mini:`meth complex(String: string): complex | error`
   Returns the complex number in :mini:`String` or an error if :mini:`String` does not contain a valid complex number.



:mini:`meth (A: complex) * (B: complex): real`
   complex :mini:`A * B`.



:mini:`meth (A: complex) * (B: double): complex`
   Returns :mini:`A * B`.



:mini:`meth (A: complex) * (B: integer): complex`
   Returns :mini:`A * B`.



:mini:`meth (A: complex) + (B: complex): real`
   complex :mini:`A + B`.



:mini:`meth (A: complex) + (B: double): complex`
   Returns :mini:`A + B`.



:mini:`meth (A: complex) + (B: integer): complex`
   Returns :mini:`A + B`.



:mini:`meth (A: complex) - (B: complex): real`
   complex :mini:`A - B`.



:mini:`meth (A: complex) - (B: double): complex`
   Returns :mini:`A - B`.



:mini:`meth (A: complex) - (B: integer): complex`
   Returns :mini:`A - B`.



:mini:`meth -(A: complex): complex`
   Returns :mini:`-A`.



:mini:`meth (A: complex) / (B: complex): real`
   complex :mini:`A / B`.



:mini:`meth (A: complex) / (B: integer): complex`
   Returns :mini:`A / B`.



:mini:`meth (A: complex) / (B: double): complex`
   Returns :mini:`A / B`.



:mini:`meth (Z: complex):i: real`
   Returns the imaginary component of :mini:`Z`.



:mini:`meth (Z: complex):r: real`
   Returns the real component of :mini:`Z`.



:mini:`meth real(Arg₁: complex)`
   *TBD*


:mini:`meth ~(A: complex): complex`
   Returns :mini:`~A`.



:mini:`meth (Buffer: string::buffer):append(Value: complex)`
   Appends :mini:`Value` to :mini:`Buffer`.



:mini:`meth (Buffer: string::buffer):append(Value: complex, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string for the real and imaginary components.



.. _type-double:

:mini:`type double < real`
   *TBD*


:mini:`meth (A: double) != (B: double): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) != (B: integer): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) * (B: integer): real`
   Returns :mini:`A * B`.



:mini:`meth (A: double) * (B: double): real`
   Returns :mini:`A * B`.



:mini:`meth (A: double) * (B: complex): complex`
   Returns :mini:`A * B`.



:mini:`meth (A: double) + (B: complex): complex`
   Returns :mini:`A + B`.



:mini:`meth (A: double) + (B: double): real`
   Returns :mini:`A + B`.



:mini:`meth (A: double) + (B: integer): real`
   Returns :mini:`A + B`.



:mini:`meth ++(Real: double): real`
   Returns :mini:`Real + 1`



:mini:`meth (A: double) - (B: double): real`
   Returns :mini:`A - B`.



:mini:`meth (A: double) - (B: integer): real`
   Returns :mini:`A - B`.



:mini:`meth (A: double) - (B: complex): complex`
   Returns :mini:`A - B`.



:mini:`meth -(A: double): real`
   Returns :mini:`-A`.



:mini:`meth --(Real: double): real`
   Returns :mini:`Real - 1`



:mini:`meth (A: double) / (B: double): real`
   Returns :mini:`A / B`.



:mini:`meth (A: double) / (B: integer): real`
   Returns :mini:`A / B`.



:mini:`meth (A: double) / (B: complex): complex`
   Returns :mini:`A / B`.



:mini:`meth (A: double) < (B: double): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) < (B: integer): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) <= (B: double): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) <= (B: integer): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.



:mini:`meth (Real₁: double) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Int₂`.



:mini:`meth (Real₁: double) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Real₂`.



:mini:`meth (A: double) = (B: double): real`
   Returns :mini:`B` if :mini:`A == B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) = (B: integer): real`
   Returns :mini:`B` if :mini:`A == B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) > (B: double): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) > (B: integer): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) >= (B: integer): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: double) >= (B: double): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.



:mini:`meth (Buffer: string::buffer):append(Value: double, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.



:mini:`meth (Buffer: string::buffer):append(Value: double)`
   Appends :mini:`Value` to :mini:`Buffer`.



.. _type-integer:

:mini:`type integer < real, function`
   *TBD*


:mini:`meth integer(Real: double): integer`
   Converts :mini:`Real` to an integer (using default rounding).



:mini:`meth integer(String: string): integer | error`
   Returns the base :mini:`10` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.

   .. collapse:: Example

      .. code-block:: mini

         integer("123") :> 123
         integer("ABC") :> error("ValueError", "Error parsing integer")


:mini:`meth integer(String: string, Base: integer): integer | error`
   Returns the base :mini:`Base` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.



:mini:`meth (A: integer) != (B: integer): integer`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) != (B: double): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.



:mini:`meth (Int₁: integer) !| (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.



:mini:`meth (Int₁: integer) % (Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular,  if :mini:`Int₁` is negative,  the result will be negative.

   For a nonnegative remainder,  use :mini:`Int₁ mod Int₂`.



:mini:`meth (A: integer) * (B: integer): integer`
   Returns :mini:`A * B`.



:mini:`meth (A: integer) * (B: double): real`
   Returns :mini:`A * B`.



:mini:`meth (A: integer) * (B: complex): complex`
   Returns :mini:`A * B`.



:mini:`meth (A: integer) + (B: double): real`
   Returns :mini:`A + B`.



:mini:`meth (A: integer) + (B: complex): complex`
   Returns :mini:`A + B`.



:mini:`meth (A: integer) + (B: integer): integer`
   Returns :mini:`A + B`.



:mini:`meth ++(Int: integer): integer`
   Returns :mini:`Int + 1`



:mini:`meth (A: integer) - (B: integer): integer`
   Returns :mini:`A - B`.



:mini:`meth (A: integer) - (B: double): real`
   Returns :mini:`A - B`.



:mini:`meth (A: integer) - (B: complex): complex`
   Returns :mini:`A - B`.



:mini:`meth -(A: integer): integer`
   Returns :mini:`-A`.



:mini:`meth --(Int: integer): integer`
   Returns :mini:`Int - 1`



:mini:`meth (A: integer) / (B: double): real`
   Returns :mini:`A / B`.



:mini:`meth (A: integer) / (B: complex): complex`
   Returns :mini:`A / B`.



:mini:`meth (Int₁: integer) / (Int₂: integer): integer | real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact,  otherwise as a real.

   .. collapse:: Example

      .. code-block:: mini

         let N := 10 / 2 :> 5
         type(N) :> <<int32>>
         let R := 10 / 3 :> 3.33333
         type(R) :> <<double>>


:mini:`meth (A: integer) /\ (B: integer): integer`
   Returns :mini:`A & B`.



:mini:`meth (Int₁: integer):div(Int₂: integer): integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.



:mini:`meth (Int₁: integer):mod(Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular,  the result is always nonnegative.



:mini:`meth (A: integer) < (B: double): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) < (B: integer): integer`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) << (B: integer): integer`
   Returns :mini:`A << B`.



:mini:`meth (A: integer) <= (B: integer): integer`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) <= (B: double): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.



:mini:`meth (Int₁: integer) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Int₂`.



:mini:`meth (Int₁: integer) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Real₂`.



:mini:`meth (A: integer) = (B: integer): integer`
   Returns :mini:`B` if :mini:`A == B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) = (B: double): real`
   Returns :mini:`B` if :mini:`A == B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) > (B: integer): integer`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) > (B: double): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) >< (B: integer): integer`
   Returns :mini:`A ^ B`.



:mini:`meth (A: integer) >= (B: double): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) >= (B: integer): integer`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.



:mini:`meth (A: integer) >> (B: integer): integer`
   Returns :mini:`A >> B`.



:mini:`meth (A: integer) \/ (B: integer): integer`
   Returns :mini:`A | B`.



:mini:`meth real(Arg₁: integer)`
   *TBD*


:mini:`meth (Int₁: integer) | (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.



:mini:`meth ~(A: integer): integer`
   Returns :mini:`~A`.



:mini:`meth (Buffer: string::buffer):append(Value: integer, Base: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`Base`.



:mini:`meth (Buffer: string::buffer):append(Value: integer, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.



:mini:`meth (Buffer: string::buffer):append(Value: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`10`.



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


:mini:`meth number(String: string): integer | real | complex | error`
   Returns the number in :mini:`String` or an error if :mini:`String` does not contain a valid number.



