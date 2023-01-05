.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

range
=====

.. rst-class:: mini-api

:mini:`meth (X: double):in(Range: integer::range): X | nil`
   *TBD*


:mini:`meth (X: double):in(Range: real::range): X | nil`
   *TBD*


:mini:`meth (Scale: integer) * (Range: integer::range): integer::range`
   Returns a range


:mini:`meth (Shift: integer) + (Range: integer::range): integer::range`
   Returns a range


:mini:`meth (Shift: integer) - (Range: integer::range): integer::range`
   Returns a range


:mini:`meth (Start: integer) .. (Limit: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth (Start: integer) .. (Limit: integer, Step: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth (Start: integer) ..< (Limit: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (exclusive).


:mini:`meth (Start: integer):by(Step: integer): integer::range`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth (X: integer):in(Range: integer::range): X | nil`
   *TBD*


:mini:`meth (X: integer):in(Range: real::range): X | nil`
   *TBD*


:mini:`meth (Start: integer):up: integer::range`
   Returns an unlimited range from :mini:`Start`.


:mini:`meth (Start: integer):up(Count: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Start + Count - 1` (inclusive).


.. _type-integer-range:

:mini:`type integer::range < sequence`
   *TBD*


:mini:`meth (A: integer::range) != (B: integer::range): integer::range | nil`
   Returns a range


:mini:`meth (Range: integer::range) * (Scale: integer): integer::range`
   Returns a range


:mini:`meth (Range: integer::range) + (Shift: integer): integer::range`
   Returns a range


:mini:`meth (Range: integer::range) - (Shift: integer): integer::range`
   Returns a range


:mini:`meth (A: integer::range) = (B: integer::range): integer::range | nil`
   Returns a range


:mini:`meth (Range: integer::range):bin(Value: double): integer | nil`
   *TBD*


:mini:`meth (Range: integer::range):bin(Value: integer): integer | nil`
   *TBD*


:mini:`meth (Range: integer::range):by(Step: double): real::range`
   *TBD*


:mini:`meth (Range: integer::range):by(Step: integer): integer::range`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth (Range: integer::range):count: integer`
   Returns the number of values in :mini:`Range`.


:mini:`meth (Range: integer::range):first: integer`
   Returns the start of :mini:`Range`.


:mini:`meth (Range: integer::range):in(Count: integer): real::range`
   *TBD*


:mini:`meth (Range: integer::range):last: integer`
   Returns the limit of :mini:`Range`.


:mini:`meth (Range: integer::range):limit: integer`
   Returns the limit of :mini:`Range`.


:mini:`meth (Range: integer::range):random: integer`
   *TBD*


:mini:`meth (Range: integer::range):start: integer`
   Returns the start of :mini:`Range`.


:mini:`meth (Range: integer::range):step: integer`
   Returns the limit of :mini:`Range`.


:mini:`meth (Buffer: string::buffer):append(Value: integer::range)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Start: number) .. (Limit: number): real::range`
   *TBD*


:mini:`meth (Start: number) .. (Limit: number, Arg₃: number): real::range`
   *TBD*


:mini:`meth (Start: number):by(Step: number): real::range`
   *TBD*


.. _type-real-range:

:mini:`type real::range < sequence`
   *TBD*


:mini:`meth (Range: real::range):bin(Value: double): integer | nil`
   *TBD*


:mini:`meth (Range: real::range):bin(Value: integer): integer | nil`
   *TBD*


:mini:`meth (Range: real::range):by(Step: number): real::range`
   *TBD*


:mini:`meth (Range: real::range):count: integer`
   Returns the number of values in :mini:`Range`.


:mini:`meth (Range: real::range):first: real`
   Returns the start of :mini:`Range`.


:mini:`meth (Range: real::range):in(Count: integer): real::range`
   *TBD*


:mini:`meth (Range: real::range):last: real`
   Returns the limit of :mini:`Range`.


:mini:`meth (Range: real::range):limit: real`
   Returns the limit of :mini:`Range`.


:mini:`meth (Range: real::range):random: real`
   *TBD*


:mini:`meth (Range: real::range):start: real`
   Returns the start of :mini:`Range`.


:mini:`meth (Range: real::range):step: real`
   Returns the step of :mini:`Range`.


:mini:`meth (Buffer: string::buffer):append(Value: real::range)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


