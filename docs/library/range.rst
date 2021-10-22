range
=====

:mini:`type integer::iter`
   *TBD*

:mini:`type integer::range < sequence`
   *TBD*

:mini:`meth (Start: integer) .. (Limit: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth (Start: integer) .. (Limit: integer, Step: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth (Start: integer):by(Step: integer): integer::range`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth (Range: integer::range):by(Step: integer): integer::range`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth (X: integer::range):count: integer`
   *TBD*

:mini:`meth (X: integer):in(Range: integer::range): X | nil`
   *TBD*

:mini:`meth (X: double):in(Range: integer::range): X | nil`
   *TBD*

:mini:`type real::iter`
   *TBD*

:mini:`type real::range < sequence`
   *TBD*

:mini:`meth (Start: number) .. (Limit: number): real::range`
   *TBD*

:mini:`meth (Start: number) .. (Limit: number, Arg₃: number): real::range`
   *TBD*

:mini:`meth (Start: number):by(Step: number): real::range`
   *TBD*

:mini:`meth (Range: real::range):by(Step: number): real::range`
   *TBD*

:mini:`meth (Range: integer::range):in(Count: integer): real::range`
   *TBD*

:mini:`meth (Range: real::range):in(Count: integer): real::range`
   *TBD*

:mini:`meth (Range: integer::range):by(Step: double): real::range`
   *TBD*

:mini:`meth (Range: integer::range):bin(Value: integer): integer | nil`
   *TBD*

:mini:`meth (Range: integer::range):bin(Value: double): integer | nil`
   *TBD*

:mini:`meth (Range: real::range):bin(Value: integer): integer | nil`
   *TBD*

:mini:`meth (Range: real::range):bin(Value: double): integer | nil`
   *TBD*

:mini:`meth (X: real::range):count: integer`
   *TBD*

:mini:`meth (X: integer):in(Range: real::range): X | nil`
   *TBD*

:mini:`meth (X: double):in(Range: real::range): X | nil`
   *TBD*

