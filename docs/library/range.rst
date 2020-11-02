range
=====

.. include:: <isonum.txt>

:mini:`integeriter`
   *Defined at line 1651 in src/ml_types.c*

:mini:`integerrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1672 in src/ml_types.c*

:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).

   *Defined at line 1675 in src/ml_types.c*

:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.

   *Defined at line 1691 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.

   *Defined at line 1707 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1722 in src/ml_types.c*

:mini:`meth :in(X: real, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1734 in src/ml_types.c*

:mini:`realiter`
   *Defined at line 1767 in src/ml_types.c*

:mini:`realrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1790 in src/ml_types.c*

:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`
   *Defined at line 1793 in src/ml_types.c*

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1807 in src/ml_types.c*

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1821 in src/ml_types.c*

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`
   *Defined at line 1838 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: real)` |rarr| :mini:`realrange`
   *Defined at line 1853 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1870 in src/ml_types.c*

:mini:`meth :in(X: real, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1882 in src/ml_types.c*

