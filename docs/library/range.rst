range
=====

.. include:: <isonum.txt>

:mini:`type integeriter`

:mini:`type integerrange < sequence`

:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth :count(X: integerrange)` |rarr| :mini:`integer`

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`meth :in(X: double, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`type realiter`

:mini:`type realrange < sequence`

:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`

:mini:`meth :in(Range: integerrange, Count: integer)` |rarr| :mini:`realrange`

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`

:mini:`meth :by(Range: integerrange, Step: double)` |rarr| :mini:`realrange`

:mini:`meth :bin(Range: integerrange, Value: integer)` |rarr| :mini:`integer` or :mini:`nil`

:mini:`meth :bin(Range: integerrange, Value: double)` |rarr| :mini:`integer` or :mini:`nil`

:mini:`meth :bin(Range: realrange, Value: integer)` |rarr| :mini:`integer` or :mini:`nil`

:mini:`meth :bin(Range: realrange, Value: double)` |rarr| :mini:`integer` or :mini:`nil`

:mini:`meth :count(X: realrange)` |rarr| :mini:`integer`

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`meth :in(X: double, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`

