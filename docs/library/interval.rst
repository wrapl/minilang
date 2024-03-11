.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

interval
========

.. rst-class:: mini-api

:mini:`meth (X: double):between(Interval: integer::interval): X | nil`
   *TBD*


:mini:`meth (X: double):between(Interval: real::interval): X | nil`
   *TBD*


:mini:`meth (Scale: integer) * (Interval: integer::interval): integer::interval`
   Returns a interval


:mini:`meth (Shift: integer) + (Interval: integer::interval): integer::interval`
   Returns a interval


:mini:`meth (Start: integer) .. (Limit: integer): integer::interval`
   Returns a interval from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth (Start: integer) .. (Limit: integer, Step: integer): integer::range`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive) with step :mini:`Step`.


:mini:`meth (Start: integer) ..< (Limit: integer): integer::interval`
   Returns a interval from :mini:`Start` to :mini:`Limit` (exclusive).


:mini:`meth (X: integer):between(Interval: integer::interval): X | nil`
   *TBD*


:mini:`meth (X: integer):between(Interval: real::interval): X | nil`
   *TBD*


:mini:`meth (Start: integer):by(Step: integer): integer::range`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth (Start: integer):up: integer::interval`
   Returns an unlimited interval from :mini:`Start`.


:mini:`meth (Start: integer):up(Count: integer): integer::interval`
   Returns a interval from :mini:`Start` to :mini:`Start + Count - 1` (inclusive).


:mini:`type integer::interval < sequence`
   *TBD*


:mini:`meth (A: integer::interval) != (B: integer::interval): integer::interval | nil`
   Returns a interval


:mini:`meth (Interval: integer::interval) * (Scale: integer): integer::interval`
   Returns a interval


:mini:`meth (Interval: integer::interval) + (Shift: integer): integer::interval`
   Returns a interval


:mini:`meth (Interval: integer::interval) - (Shift: integer): integer::interval`
   Returns a interval


:mini:`meth (A: integer::interval) = (B: integer::interval): integer::interval | nil`
   Returns a interval


:mini:`meth (Interval: integer::interval):by(Step: double): real::range`
   *TBD*


:mini:`meth (Interval: integer::interval):by(Step: integer): integer::range`
   Returns a range with the same limits as :mini:`Interval` but with step :mini:`Step`.


:mini:`meth (Interval: integer::interval):count: integer`
   Returns the number of values in :mini:`Interval`.


:mini:`meth (Interval: integer::interval):first: integer`
   Returns the start of :mini:`Interval`.


:mini:`meth (Interval: integer::interval):in(Count: integer): integer::range | real::range`
   *TBD*


:mini:`meth (Interval: integer::interval):last: integer`
   Returns the limit of :mini:`Interval`.


:mini:`meth (Interval: integer::interval):limit: integer`
   Returns the limit of :mini:`Interval`.


:mini:`meth (Interval: integer::interval):precount: integer`
   Returns the number of values in :mini:`Interval`.


:mini:`meth (Interval: integer::interval):random: integer`
   *TBD*


:mini:`meth (Interval: integer::interval):start: integer`
   Returns the start of :mini:`Interval`.


:mini:`meth (Buffer: string::buffer):append(Value: integer::interval)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`type integer::range < sequence`
   *TBD*


:mini:`meth (Interval: integer::range):bin(Value: double): integer | nil`
   *TBD*


:mini:`meth (Interval: integer::range):bin(Value: integer): integer | nil`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(Value: integer::range)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Start: number) .. (Limit: number): real::interval`
   *TBD*


:mini:`type real::interval < sequence`
   *TBD*


:mini:`meth (Interval: real::interval):by(Step: number): real::range`
   *TBD*


:mini:`meth (Interval: real::interval):count: integer`
   Returns the number of values in :mini:`Interval`.


:mini:`meth (Interval: real::interval):first: real`
   Returns the start of :mini:`Interval`.


:mini:`meth (Interval: real::interval):in(Count: integer): real::range`
   *TBD*


:mini:`meth (Interval: real::interval):last: real`
   Returns the limit of :mini:`Interval`.


:mini:`meth (Interval: real::interval):limit: real`
   Returns the limit of :mini:`Interval`.


:mini:`meth (Interval: real::interval):precount: integer`
   Returns the number of values in :mini:`Interval`.


:mini:`meth (Interval: real::interval):random: real`
   *TBD*


:mini:`meth (Interval: real::interval):start: real`
   Returns the start of :mini:`Interval`.


:mini:`meth (Buffer: string::buffer):append(Value: real::interval)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`type real::range < sequence`
   *TBD*


