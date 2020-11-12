array
=====

.. include:: <isonum.txt>

:mini:`array`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 7 in src/ml_array.c*

:mini:`meth :shape(Array: array)` |rarr| :mini:`list`
   *Defined at line 268 in src/ml_array.c*

:mini:`meth :strides(Array: array)` |rarr| :mini:`list`
   *Defined at line 279 in src/ml_array.c*

:mini:`meth :size(Array: array)` |rarr| :mini:`integer`
   *Defined at line 290 in src/ml_array.c*

:mini:`meth :count(Array: array)` |rarr| :mini:`integer`
   *Defined at line 300 in src/ml_array.c*

:mini:`meth :degree(Array: array)` |rarr| :mini:`integer`
   *Defined at line 309 in src/ml_array.c*

:mini:`meth :transpose(Array: array)` |rarr| :mini:`array`
   *Defined at line 316 in src/ml_array.c*

:mini:`meth :permute(Array: array, Indices: list)` |rarr| :mini:`array`
   *Defined at line 329 in src/ml_array.c*

:mini:`meth :expand(Array: array, Indices: list)` |rarr| :mini:`array`
   *Defined at line 354 in src/ml_array.c*

:mini:`meth [](Array: array, Indices...: any)` |rarr| :mini:`array`
   *Defined at line 484 in src/ml_array.c*

:mini:`meth [](Array: array, Indices: map)` |rarr| :mini:`array`
   *Defined at line 492 in src/ml_array.c*

:mini:`arrayiterator`
   *Defined at line 564 in src/ml_array.c*

:mini:`meth :sums(Arg₁: array, Arg₂: integer)`
   *Defined at line 1419 in src/ml_array.c*

:mini:`meth :prods(Arg₁: array, Arg₂: integer)`
   *Defined at line 1456 in src/ml_array.c*

:mini:`meth :sum(Arg₁: array)`
   *Defined at line 1493 in src/ml_array.c*

:mini:`meth :sum(Arg₁: array, Arg₂: integer)`
   *Defined at line 1521 in src/ml_array.c*

:mini:`meth :prod(Arg₁: array)`
   *Defined at line 1583 in src/ml_array.c*

:mini:`meth :prod(Arg₁: array, Arg₂: integer)`
   *Defined at line 1611 in src/ml_array.c*

:mini:`meth -(Arg₁: array)`
   *Defined at line 1673 in src/ml_array.c*

:mini:`meth :copy(Array: array)` |rarr| :mini:`array`
   *Defined at line 2089 in src/ml_array.c*

:mini:`meth :copy(Array: array, Function: function)` |rarr| :mini:`array`
   *Defined at line 2144 in src/ml_array.c*

:mini:`meth :update(Array: array, Function: function)` |rarr| :mini:`array`
   *Defined at line 2272 in src/ml_array.c*

:mini:`meth .(A: array, B: array)` |rarr| :mini:`array`
   Returns the inner product of :mini:`A` and :mini:`B`.

   *Defined at line 2501 in src/ml_array.c*

