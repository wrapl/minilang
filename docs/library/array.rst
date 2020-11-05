array
=====

.. include:: <isonum.txt>

:mini:`array`
   :Parents: :mini:`buffer`

   *Defined at line 7 in src/ml_array.c*

:mini:`meth :shape(Array: array)` |rarr| :mini:`list`
   *Defined at line 268 in src/ml_array.c*

:mini:`meth :strides(Array: array)` |rarr| :mini:`list`
   *Defined at line 279 in src/ml_array.c*

:mini:`meth :size(Array: array)` |rarr| :mini:`integer`
   *Defined at line 290 in src/ml_array.c*

:mini:`meth :degree(Array: array)` |rarr| :mini:`integer`
   *Defined at line 300 in src/ml_array.c*

:mini:`meth :transpose(Array: array)` |rarr| :mini:`array`
   *Defined at line 307 in src/ml_array.c*

:mini:`meth :permute(Array: array, Indices: list)` |rarr| :mini:`array`
   *Defined at line 320 in src/ml_array.c*

:mini:`meth :expand(Array: array, Indices: list)` |rarr| :mini:`array`
   *Defined at line 345 in src/ml_array.c*

:mini:`meth [](Array: array, Indices...: any)` |rarr| :mini:`array`
   *Defined at line 475 in src/ml_array.c*

:mini:`meth [](Array: array, Indices: map)` |rarr| :mini:`array`
   *Defined at line 483 in src/ml_array.c*

:mini:`meth :sums(Arg₁: array, Arg₂: integer)`
   *Defined at line 1285 in src/ml_array.c*

:mini:`meth :prods(Arg₁: array, Arg₂: integer)`
   *Defined at line 1322 in src/ml_array.c*

:mini:`meth :sum(Arg₁: array)`
   *Defined at line 1359 in src/ml_array.c*

:mini:`meth :sum(Arg₁: array, Arg₂: integer)`
   *Defined at line 1387 in src/ml_array.c*

:mini:`meth :prod(Arg₁: array)`
   *Defined at line 1449 in src/ml_array.c*

:mini:`meth :prod(Arg₁: array, Arg₂: integer)`
   *Defined at line 1477 in src/ml_array.c*

:mini:`meth -(Arg₁: array)`
   *Defined at line 1539 in src/ml_array.c*

:mini:`meth :copy(Array: array)` |rarr| :mini:`array`
   *Defined at line 1955 in src/ml_array.c*

:mini:`meth :copy(Array: array, Function: function)` |rarr| :mini:`array`
   *Defined at line 2010 in src/ml_array.c*

:mini:`meth :update(Array: array, Function: function)` |rarr| :mini:`array`
   *Defined at line 2138 in src/ml_array.c*

:mini:`meth .(A: array, B: array)` |rarr| :mini:`array`
   Returns the inner product of :mini:`A` and :mini:`B`.

   *Defined at line 2367 in src/ml_array.c*

