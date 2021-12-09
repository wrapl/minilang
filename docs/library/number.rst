number
======

:mini:`type number`
   Base type for numbers.


:mini:`type complex < number`
   *TBD*

:mini:`meth complex(Arg₁: real)`
   *TBD*

:mini:`meth real(Arg₁: complex)`
   *TBD*

:mini:`meth (Z: complex):r: real`
   Returns the real component of :mini:`Z`.


:mini:`meth (Z: complex):i: real`
   Returns the imaginary component of :mini:`Z`.


:mini:`type real < complex`
   *TBD*

:mini:`type real < number`
   *TBD*

:mini:`type integer < real, function`
   *TBD*

:mini:`meth real(Arg₁: int32)`
   *TBD*

:mini:`meth real(Arg₁: int64)`
   *TBD*

:mini:`type integer < real, function`
   *TBD*

:mini:`meth real(Arg₁: integer)`
   *TBD*

:mini:`meth integer(Arg₁: double)`
   *TBD*

:mini:`meth double(Arg₁: int32)`
   *TBD*

:mini:`meth double(Arg₁: int64)`
   *TBD*

:mini:`meth integer(Real: double): integer`
   Converts :mini:`Real` to an integer (using default rounding).


:mini:`type double < real`
   *TBD*

:mini:`meth double(Arg₁: integer)`
   *TBD*

:mini:`meth -(Arg₁: integer)`
   *TBD*

:mini:`meth -(Arg₁: double)`
   *TBD*

:mini:`meth (Arg₁: integer) + (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) + (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) + (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) + (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) - (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) - (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) - (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) - (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) * (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) * (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) * (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) * (Arg₂: double)`
   *TBD*

:mini:`meth ~(Arg₁: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) /\ (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) \/ (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) >< (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) << (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) >> (Arg₂: integer)`
   *TBD*

:mini:`meth ++(Int: integer): integer`
   Returns :mini:`Int + 1`


:mini:`meth --(Int: integer): integer`
   Returns :mini:`Int - 1`


:mini:`meth ++(Real: double): real`
   Returns :mini:`Real + 1`


:mini:`meth --(Real: double): real`
   Returns :mini:`Real - 1`


:mini:`meth (Arg₁: double) / (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) / (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) / (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: complex) / (Arg₂: complex)`
   *TBD*

:mini:`meth (Arg₁: complex) / (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) / (Arg₂: complex)`
   *TBD*

:mini:`meth (Arg₁: complex) / (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) / (Arg₂: complex)`
   *TBD*

:mini:`meth ~(Arg₁: complex)`
   *TBD*

:mini:`meth (Int₁: integer) / (Int₂: integer): integer | real`
   Returns :mini:`Int₁ / Int₂` as an integer if the division is exact,  otherwise as a real.


:mini:`meth (Int₁: integer) % (Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding towards 0. In particular,  if :mini:`Int₁` is negative,  the result will be negative.

   For a nonnegative remainder,  use :mini:`Int₁ mod Int₂`.


:mini:`meth (Int₁: integer) | (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth (Int₁: integer) !| (Int₂: integer): integer`
   Returns :mini:`Int₂` if it is not divisible by :mini:`Int₁` and :mini:`nil` otherwise.


:mini:`meth (Int₁: integer):div(Int₂: integer): integer`
   Returns the quotient of :mini:`Int₁` divided by :mini:`Int₂`.

   The result is calculated by rounding down in all cases.


:mini:`meth (Int₁: integer):mod(Int₂: integer): integer`
   Returns the remainder of :mini:`Int₁` divided by :mini:`Int₂`.

   Note: the result is calculated by rounding down in all cases. In particular,  the result is always nonnegative.


:mini:`meth (Arg₁: integer) = (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) = (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) = (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) = (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) != (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) != (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) != (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) != (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) < (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) < (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) < (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) < (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) > (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) > (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) > (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) > (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) <= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) <= (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) <= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) <= (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: integer) >= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: double) >= (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) >= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) >= (Arg₂: double)`
   *TBD*

:mini:`meth (Int₁: integer) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (Real₁: double) <> (Int₂: integer): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Int₂`.


:mini:`meth (Int₁: integer) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Int₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Real₁: double) <> (Real₂: double): integer`
   Returns :mini:`-1`,  :mini:`0` or :mini:`1` depending on whether :mini:`Real₁` is less than,  equal to or greater than :mini:`Real₂`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: integer, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: complex)`
   *TBD*

:mini:`meth integer(Arg₁: string)`
   *TBD*

:mini:`meth integer(Arg₁: string, Arg₂: integer)`
   *TBD*

:mini:`meth double(Arg₁: string)`
   *TBD*

:mini:`meth real(Arg₁: string)`
   *TBD*

:mini:`meth complex(Arg₁: string)`
   *TBD*

:mini:`meth number(Arg₁: string)`
   *TBD*

