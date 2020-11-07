range
=====

.. include:: <isonum.txt>

:mini:`integeriter`
   *Defined at line 1700 in src/ml_types.c*

:mini:`integerrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1721 in src/ml_types.c*

:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).

   *Defined at line 1724 in src/ml_types.c*

:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.

   *Defined at line 1740 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.

   *Defined at line 1756 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1771 in src/ml_types.c*

:mini:`meth :in(X: real, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1783 in src/ml_types.c*

:mini:`realiter`
   *Defined at line 1816 in src/ml_types.c*

:mini:`realrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1839 in src/ml_types.c*

:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`
   *Defined at line 1842 in src/ml_types.c*

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1856 in src/ml_types.c*

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1870 in src/ml_types.c*

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`
   *Defined at line 1887 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: real)` |rarr| :mini:`realrange`
   *Defined at line 1902 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1919 in src/ml_types.c*

:mini:`meth :in(X: real, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1931 in src/ml_types.c*

