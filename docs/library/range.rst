range
=====

.. include:: <isonum.txt>

:mini:`integeriter`

:mini:`integerrange`
   :Parents: :mini:`iteratable`


:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).


:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.


:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.


:mini:`meth :count(X: integerrange)` |rarr| :mini:`integer`

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`meth :in(X: real, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`realiter`

:mini:`realrange`
   :Parents: :mini:`iteratable`


:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`

:mini:`meth :by(Range: integerrange, Step: real)` |rarr| :mini:`realrange`

:mini:`meth :count(X: realrange)` |rarr| :mini:`integer`

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`

:mini:`meth :in(X: real, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`

