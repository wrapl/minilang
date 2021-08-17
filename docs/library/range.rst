range
=====

:mini:`type integeriter`
   *TBD*

:mini:`type integerrange < sequence`
   *TBD*

:mini:`meth ..(Start: integer, Limit: integer): integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth :by(Start: integer, Step: integer): integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth :by(Range: integerrange, Step: integer): integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth :count(X: integerrange): integer`
   *TBD*

:mini:`meth :in(X: integer, Range: integerrange): X | nil`
   *TBD*

:mini:`meth :in(X: double, Range: integerrange): X | nil`
   *TBD*

:mini:`type realiter`
   *TBD*

:mini:`type realrange < sequence`
   *TBD*

:mini:`meth ..(Start: number, Limit: number): realrange`
   *TBD*

:mini:`meth :by(Start: number, Step: number): realrange`
   *TBD*

:mini:`meth :by(Range: realrange, Step: number): realrange`
   *TBD*

:mini:`meth :in(Range: integerrange, Count: integer): realrange`
   *TBD*

:mini:`meth :in(Range: realrange, Count: integer): realrange`
   *TBD*

:mini:`meth :by(Range: integerrange, Step: double): realrange`
   *TBD*

:mini:`meth :bin(Range: integerrange, Value: integer): integer | nil`
   *TBD*

:mini:`meth :bin(Range: integerrange, Value: double): integer | nil`
   *TBD*

:mini:`meth :bin(Range: realrange, Value: integer): integer | nil`
   *TBD*

:mini:`meth :bin(Range: realrange, Value: double): integer | nil`
   *TBD*

:mini:`meth :count(X: realrange): integer`
   *TBD*

:mini:`meth :in(X: integer, Range: realrange): X | nil`
   *TBD*

:mini:`meth :in(X: double, Range: realrange): X | nil`
   *TBD*

