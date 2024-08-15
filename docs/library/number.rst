.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

number
======

.. rst-class:: mini-api

:mini:`fun integer::random(Min?: number, Max?: number): integer`
   Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max` :math:`\leq 2^{32} - 1`).
   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :math:`2^{32} - 1`.


:mini:`fun real::random(Min?: number, Max?: number): real`
   Returns a random real between :mini:`Min` and :mini:`Max`.
   If omitted,  :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`1`.


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


:mini:`meth -(A: complex): complex`
   Returns :mini:`-A`.


:mini:`meth (A: complex) - (B: complex): real`
   complex :mini:`A - B`.


:mini:`meth (A: complex) - (B: double): complex`
   Returns :mini:`A - B`.


:mini:`meth (A: complex) - (B: integer): complex`
   Returns :mini:`A - B`.


:mini:`meth (A: complex) / (B: complex): real`
   complex :mini:`A / B`.


:mini:`meth (A: complex) / (B: double): complex`
   Returns :mini:`A / B`.


:mini:`meth (A: complex) / (B: integer): complex`
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


:mini:`type double < real`
   *TBD*


:mini:`meth (A: double) != (B: double): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) != (B: integer): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) * (B: complex): complex`
   Returns :mini:`A * B`.


:mini:`meth (A: double) * (B: double): real`
   Returns :mini:`A * B`.


:mini:`meth (A: double) * (B: integer): real`
   Returns :mini:`A * B`.


:mini:`meth (A: double) + (B: complex): complex`
   Returns :mini:`A + B`.


:mini:`meth (A: double) + (B: double): real`
   Returns :mini:`A + B`.


:mini:`meth (A: double) + (B: integer): real`
   Returns :mini:`A + B`.


:mini:`meth ++(Real: double): real`
   Returns :mini:`Real + 1`


:mini:`meth -(A: double): real`
   Returns :mini:`-A`.


:mini:`meth (A: double) - (B: complex): complex`
   Returns :mini:`A - B`.


:mini:`meth (A: double) - (B: double): real`
   Returns :mini:`A - B`.


:mini:`meth (A: double) - (B: integer): real`
   Returns :mini:`A - B`.


:mini:`meth --(Real: double): real`
   Returns :mini:`Real - 1`


:mini:`meth (A: double) / (B: complex): complex`
   Returns :mini:`A / B`.


:mini:`meth (A: double) / (B: double): real`
   Returns :mini:`A / B`.


:mini:`meth (A: double) / (B: integer): real`
   Returns :mini:`A / B`.


:mini:`meth (A: double) < (B: double): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) < (B: integer): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) <= (B: double): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) <= (B: integer): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.


:mini:`meth (Real₁: double) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Real₁: double) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (A: double) = (B: double): real`
   Returns :mini:`B` if :mini:`A = B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) = (B: integer): real`
   Returns :mini:`B` if :mini:`A = B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) > (B: double): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) > (B: integer): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) >= (B: double): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double) >= (B: integer): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: double):max(B: double): real`
   Returns :mini:`max(A,  B)`.


:mini:`meth (A: double):max(B: integer): real`
   Returns :mini:`max(A,  B)`.


:mini:`meth (A: double):min(B: double): real`
   Returns :mini:`min(A,  B)`.


:mini:`meth (A: double):min(B: integer): real`
   Returns :mini:`min(A,  B)`.


:mini:`meth (Buffer: string::buffer):append(Value: double)`
   Appends :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Value: double, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.


:mini:`type integer < real, function`
   A 64-bit signed integer value.
   
   :mini:`fun (I: integer)(Arg₁,  ...,  Argₙ): any | nil`
      Returns the :mini:`I`-th argument or :mini:`nil` if there is no :mini:`I`-th argument. Negative values of :mini:`I` are counted from the last argument.
      In particular,  :mini:`0(...)` always returns :mini:`nil` and :mini:`1` behaves as the identity function.

   .. code-block:: mini

      2("a", "b", "c") :> "b"
      -1("a", "b", "c") :> "c"
      4("a", "b", "c") :> nil
      0("a", "b", "c") :> nil


:mini:`meth integer(Real: double): integer`
   Converts :mini:`Real` to an integer (using default rounding).


:mini:`meth integer(String: string): integer | error`
   Returns the base :mini:`10` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.

   .. code-block:: mini

      integer("123") :> 123
      integer("ABC")
      :> error("ValueError", "Error parsing integer")


:mini:`meth integer(String: string, Base: integer): integer | error`
   Returns the base :mini:`Base` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.


:mini:`fun integer::random_cycle(Max: integer): list`
   Returns a random cyclic permutation (no sub-cycles) of :mini:`1,  ...,  Max`.


:mini:`fun integer::random_permutation(Max: integer): list`
   Returns a random permutation of :mini:`1,  ...,  Max`.


:mini:`meth (A: integer) != (B: double): real`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) != (B: integer): integer`
   Returns :mini:`B` if :mini:`A != B`,  otherwise returns :mini:`nil`.


:mini:`meth (Int₁: integer) !| (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth (Int₁: integer) % (Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.
   Note: the result is calculated by rounding towards 0. In particular,  if :mini:`Int₁` is negative,  the result will be negative.
   For a nonnegative remainder,  use :mini:`Int₁ mod Int₂`.


:mini:`meth (A: integer) * (B: complex): complex`
   Returns :mini:`A * B`.


:mini:`meth (A: integer) * (B: double): real`
   Returns :mini:`A * B`.


:mini:`meth (A: integer) * (B: integer): integer`
   Returns :mini:`A * B`.


:mini:`meth (A: integer) + (B: complex): complex`
   Returns :mini:`A + B`.


:mini:`meth (A: integer) + (B: double): real`
   Returns :mini:`A + B`.


:mini:`meth (A: integer) + (B: integer): integer`
   Returns :mini:`A + B`.


:mini:`meth ++(Int: integer): integer`
   Returns :mini:`Int + 1`


:mini:`meth -(A: integer): integer`
   Returns :mini:`-A`.


:mini:`meth (A: integer) - (B: complex): complex`
   Returns :mini:`A - B`.


:mini:`meth (A: integer) - (B: double): real`
   Returns :mini:`A - B`.


:mini:`meth (A: integer) - (B: integer): integer`
   Returns :mini:`A - B`.


:mini:`meth --(Int: integer): integer`
   Returns :mini:`Int - 1`


:mini:`meth (A: integer) / (B: complex): complex`
   Returns :mini:`A / B`.


:mini:`meth (A: integer) / (B: double): real`
   Returns :mini:`A / B`.


:mini:`meth (Int₁: integer) / (Int₂: integer): integer | real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact,  otherwise as a real.

   .. code-block:: mini

      let N := 10 / 2 :> 5
      type(N) :> <<int32>>
      let R := 10 / 3 :> 3.33333333333333
      type(R) :> <<double>>


:mini:`meth (A: integer) /\\ (B: integer): integer`
   Returns the bitwise and of :mini:`A` and :mini:`B`.


:mini:`meth (A: integer) < (B: double): real`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) < (B: integer): integer`
   Returns :mini:`B` if :mini:`A < B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) << (B: integer): integer`
   Returns :mini:`A << B`.


:mini:`meth (A: integer) <= (B: double): real`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) <= (B: integer): integer`
   Returns :mini:`B` if :mini:`A <= B`,  otherwise returns :mini:`nil`.


:mini:`meth (Int₁: integer) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Int₁: integer) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (A: integer) = (B: double): real`
   Returns :mini:`B` if :mini:`A = B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) = (B: integer): integer`
   Returns :mini:`B` if :mini:`A = B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) > (B: double): real`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) > (B: integer): integer`
   Returns :mini:`B` if :mini:`A > B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) >< (B: integer): integer`
   Returns the bitwise xor of :mini:`A` and :mini:`B`.


:mini:`meth (A: integer) >= (B: double): real`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) >= (B: integer): integer`
   Returns :mini:`B` if :mini:`A >= B`,  otherwise returns :mini:`nil`.


:mini:`meth (A: integer) >> (B: integer): integer`
   Returns :mini:`A >> B`.


:mini:`meth (A: integer) \\/ (B: integer): integer`
   Returns the bitwise or of :mini:`A` and :mini:`B`.


:mini:`meth (A: integer):bsf: integer`
   Returns the index of the least significant 1-bit of :mini:`A`,  or :mini:`0` if :mini:`A = 0`.

   .. code-block:: mini

      16:bsf :> 5
      10:bsf :> 2
      0:bsf :> 0


:mini:`meth (A: integer):bsr: integer`
   Returns the index of the most significant 1-bit of :mini:`A`,  or :mini:`0` if :mini:`A = 0`.

   .. code-block:: mini

      16:bsr :> 5
      10:bsr :> 4
      0:bsr :> 0


:mini:`meth (X: integer):dec: integer`
   Atomic equivalent to :mini:`X := old - 1`.


:mini:`meth (X: integer):dec(Y: integer): integer`
   Atomic equivalent to :mini:`X := old - Y`.


:mini:`meth (Int₁: integer):div(Int₂: integer): integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.
   The result is calculated by rounding down in all cases.


:mini:`meth (X: integer):inc: integer`
   Atomic equivalent to :mini:`X := old + 1`.


:mini:`meth (X: integer):inc(Y: integer): integer`
   Atomic equivalent to :mini:`X := old + Y`.


:mini:`meth (A: integer):max(B: double): real`
   Returns :mini:`max(A,  B)`.


:mini:`meth (A: integer):max(B: integer): integer`
   Returns :mini:`max(A,  B)`.


:mini:`meth (A: integer):min(B: double): real`
   Returns :mini:`min(A,  B)`.


:mini:`meth (A: integer):min(B: integer): integer`
   Returns :mini:`min(A,  B)`.


:mini:`meth (Int₁: integer):mod(Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.
   Note: the result is calculated by rounding down in all cases. In particular,  the result is always nonnegative.


:mini:`meth (A: integer):popcount: integer`
   Returns the number of bits set in :mini:`A`.


:mini:`meth real(Arg₁: integer)`
   *TBD*


:mini:`meth (Int₁: integer) | (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth ~(A: integer): integer`
   Returns :mini:`~A`.


:mini:`meth (Buffer: string::buffer):append(Value: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`10`.


:mini:`meth (Buffer: string::buffer):append(Value: integer, Base: integer)`
   Appends :mini:`Value` to :mini:`Buffer` in base :mini:`Base`.


:mini:`meth (Buffer: string::buffer):append(Value: integer, Format: string)`
   Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.


:mini:`type number`
   Base type for numbers.


:mini:`meth (Number: number):isfinite(Arg₂: double): number | nil`
   Returns :mini:`Number` if it is finite (neither |plusmn|\ |infin| nor ``NaN``),  otherwise returns :mini:`nil`.


:mini:`meth (Number: number):isnan(Arg₂: double): number | nil`
   Returns :mini:`Number` if it is ``NaN``,  otherwise returns :mini:`Number`.


:mini:`type real < complex`
   *TBD*


:mini:`def real::NaN: real`
   Not a number.


:mini:`def real::Inf: real`
   Positive infinity.


:mini:`meth real(String: string): real | error`
   Returns the real number in :mini:`String` or an error if :mini:`String` does not contain a valid real number.


:mini:`meth (Real₁: real) % (Real₂: real): integer`
   Returns the remainder of :mini:`Real₁` divided by :mini:`Real₂`.
   Note: the result is calculated by rounding towards 0. In particular,  if :mini:`Real₁` is negative,  the result will be negative.
   For a nonnegative remainder,  use :mini:`Real₁ mod Real₂`.


:mini:`meth complex(Arg₁: real)`
   *TBD*


:mini:`meth complex(Arg₁: real, Arg₂: real)`
   *TBD*


:mini:`meth (Real₁: real):div(Real₂: real): integer`
   Returns the quotient of :mini:`Real₁` divided by :mini:`Real₂`.
   The result is calculated by rounding down in all cases.


:mini:`meth (Int₁: real):mod(Int₂: real): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.
   Note: the result is calculated by rounding down in all cases. In particular,  the result is always nonnegative.


:mini:`meth number(String: string): integer | real | complex | error`
   Returns the number in :mini:`String` or an error if :mini:`String` does not contain a valid number.


