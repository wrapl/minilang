range
=====

.. include:: <isonum.txt>

**type** :mini:`integeriter`
   *Defined at line 1437 in src/ml_types.c*

**type** :mini:`integerrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1458 in src/ml_types.c*

**method** :mini:`integer Start .. integer Limit` |rarr| :mini:`integerrange`
   *Defined at line 1461 in src/ml_types.c*

**method** :mini:`integer Start:by(integer Step)` |rarr| :mini:`integerrange`
   *Defined at line 1476 in src/ml_types.c*

**method** :mini:`integerrange Range:by(integer Step)` |rarr| :mini:`integerrange`
   *Defined at line 1491 in src/ml_types.c*

**method** :mini:`integer X:in(integerrange Range)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1505 in src/ml_types.c*

**method** :mini:`real X:in(integerrange Range)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1517 in src/ml_types.c*

**type** :mini:`realiter`
   *Defined at line 1550 in src/ml_types.c*

**type** :mini:`realrange`
   :Parents: :mini:`iteratable`

   *Defined at line 1573 in src/ml_types.c*

**method** :mini:`number Start .. number Limit` |rarr| :mini:`realrange`
   *Defined at line 1576 in src/ml_types.c*

**method** :mini:`number Start:by(number Step)` |rarr| :mini:`realrange`
   *Defined at line 1590 in src/ml_types.c*

**method** :mini:`realrange Range:by(number Step)` |rarr| :mini:`realrange`
   *Defined at line 1604 in src/ml_types.c*

**method** :mini:`realrange Range:in(integer Count)` |rarr| :mini:`realrange`
   *Defined at line 1621 in src/ml_types.c*

**method** :mini:`integerrange Range:by(real Step)` |rarr| :mini:`realrange`
   *Defined at line 1636 in src/ml_types.c*

**method** :mini:`integer X:in(realrange Range)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1653 in src/ml_types.c*

**method** :mini:`real X:in(realrange Range)` |rarr| :mini:`X` or :mini:`nil`
   *Defined at line 1665 in src/ml_types.c*

