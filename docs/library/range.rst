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


:mini:`meth :by(Start: integer, Step: integer): integer::range`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth :by(Range: integer::range, Step: integer): integer::range`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth :count(X: integer::range): integer`
   *TBD*

:mini:`meth :in(X: integer, Range: integer::range): X | nil`
   *TBD*

:mini:`meth :in(X: double, Range: integer::range): X | nil`
   *TBD*

:mini:`type real::iter`
   *TBD*

:mini:`type real::range < sequence`
   *TBD*

:mini:`meth (Start: number) .. (Limit: number): real::range`
   *TBD*

:mini:`meth (Start: number) .. (Limit: number, Arg₃: number): real::range`
   *TBD*

:mini:`meth :by(Start: number, Step: number): real::range`
   *TBD*

:mini:`meth :by(Range: real::range, Step: number): real::range`
   *TBD*

:mini:`meth :in(Range: integer::range, Count: integer): real::range`
   *TBD*

:mini:`meth :in(Range: real::range, Count: integer): real::range`
   *TBD*

:mini:`meth :by(Range: integer::range, Step: double): real::range`
   *TBD*

:mini:`meth :bin(Range: integer::range, Value: integer): integer | nil`
   *TBD*

:mini:`meth :bin(Range: integer::range, Value: double): integer | nil`
   *TBD*

:mini:`meth :bin(Range: real::range, Value: integer): integer | nil`
   *TBD*

:mini:`meth :bin(Range: real::range, Value: double): integer | nil`
   *TBD*

:mini:`meth :count(X: real::range): integer`
   *TBD*

:mini:`meth :in(X: integer, Range: real::range): X | nil`
   *TBD*

:mini:`meth :in(X: double, Range: real::range): X | nil`
   *TBD*

