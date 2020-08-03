range
=====

.. include:: <isonum.txt>

:mini:`integeriter`
   *Defined at line 1437 in src/ml_types.c*

:mini:`integerrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1458 in src/ml_types.c*

:mini:`meth ..(Start: integer, Limit: integer)` |rarr| :mini:`integerrange`
   *Defined at line 1461 in src/ml_types.c*

:mini:`meth :by(Start: integer, Step: integer)` |rarr| :mini:`integerrange`
   *Defined at line 1476 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: integer)` |rarr| :mini:`integerrange`
   *Defined at line 1491 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1505 in src/ml_types.c*

:mini:`meth :in(X: real, Range: integerrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1517 in src/ml_types.c*

:mini:`realiter`
   *Defined at line 1550 in src/ml_types.c*

:mini:`realrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1573 in src/ml_types.c*

:mini:`meth ..(Start: number, Limit: number)` |rarr| :mini:`realrange`
   *Defined at line 1576 in src/ml_types.c*

:mini:`meth :by(Start: number, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1590 in src/ml_types.c*

:mini:`meth :by(Range: realrange, Step: number)` |rarr| :mini:`realrange`
   *Defined at line 1604 in src/ml_types.c*

:mini:`meth :in(Range: realrange, Count: integer)` |rarr| :mini:`realrange`
   *Defined at line 1621 in src/ml_types.c*

:mini:`meth :by(Range: integerrange, Step: real)` |rarr| :mini:`realrange`
   *Defined at line 1636 in src/ml_types.c*

:mini:`meth :in(X: integer, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1653 in src/ml_types.c*

:mini:`meth :in(X: real, Range: realrange)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1665 in src/ml_types.c*

