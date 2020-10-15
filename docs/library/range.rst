range
=====

.. include:: <isonum.txt>

:mini:`integeriter`
   *Defined at line 1519 in src/ml_types.c*

:mini:`integerrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1540 in src/ml_types.c*

:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).

   *Defined at line 1543 in src/ml_types.c*

:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.

   *Defined at line 1559 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.

   *Defined at line 1575 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1590 in src/ml_types.c*

:mini:`meth :in(X: real, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1602 in src/ml_types.c*

:mini:`realiter`
   *Defined at line 1635 in src/ml_types.c*

:mini:`realrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1658 in src/ml_types.c*

:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`
   *Defined at line 1661 in src/ml_types.c*

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1675 in src/ml_types.c*

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1689 in src/ml_types.c*

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`
   *Defined at line 1706 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: real)` |rarr| :mini:`realrange`
   *Defined at line 1721 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1738 in src/ml_types.c*

:mini:`meth :in(X: real, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1750 in src/ml_types.c*

