array
=====

.. include:: <isonum.txt>

:mini:`type array < buffer, iteratable`
   Base type for multidimensional arrays.


:mini:`constructor array(List: list)` |rarr| :mini:`array`
   Returns a new array containing the values in :mini:`List`.

   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`type array::int8 < array`
   Arrays of signed 8 bit integers.


:mini:`type array::uint8 < array`
   Arrays of unsigned 8 bit integers.


:mini:`type array::int16 < array`
   Arrays of signed 16 bit integers.


:mini:`type array::uint16 < array`
   Arrays of unsigned 16 bit integers.


:mini:`type array::int32 < array`
   Arrays of signed 32 bit integers.


:mini:`type array::uint32 < array`
   Arrays of unsigned 32 bit integers.


:mini:`type array::int64 < array`
   Arrays of signed 64 bit integers.


:mini:`type array::uint64 < array`
   Arrays of unsigned 64 bit integers.


:mini:`type array::float32 < array`
   Arrays of 32 bit reals.


:mini:`type array::float64 < array`
   Arrays of 64 bit reals.


:mini:`type array::any < array`
   Arrays of any *Minilang* values.


:mini:`meth :degree(Array: array)` |rarr| :mini:`integer`
   Return the degree of :mini:`Array`.


:mini:`meth :shape(Array: array)` |rarr| :mini:`list`
   Return the shape of :mini:`Array`.


:mini:`meth :count(Array: array)` |rarr| :mini:`integer`
   Return the number of elements in :mini:`Array`.


:mini:`meth ^(Array: array)` |rarr| :mini:`array`
   Returns the transpose of :mini:`Array`, sharing the underlying data.


:mini:`meth :permute(Array: array, Indices: list)` |rarr| :mini:`array`
   Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.


:mini:`meth :swap(Arg₁: array, Arg₂: integer, Arg₃: integer)`

:mini:`meth :expand(Array: array, Indices: list)` |rarr| :mini:`array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth :split(Arg₁: array, Arg₂: integer, Arg₃: list)`

:mini:`meth :join(Arg₁: array, Arg₂: integer, Arg₃: integer)`

:mini:`meth :strides(Array: array)` |rarr| :mini:`list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth :size(Array: array)` |rarr| :mini:`integer`
   Return the size of :mini:`Array` in bytes.


:mini:`meth (Array: array)[Indices...: any]` |rarr| :mini:`array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by the corresponding :mini:`Indexᵢ`.

   * If :mini:`Indexᵢ` is :mini:`nil` then the :mini:`i`-th dimension is copied unchanged.

   * If :mini:`Indexᵢ` is an integer then the :mini:`Indexᵢ`-th value is selected and the :mini:`i`-th dimension is dropped from the result.

   * If :mini:`Indexᵢ` is a list of integers then the :mini:`i`-th dimension is copied as a sparse dimension with the respective entries.

   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.


:mini:`meth (Array: array)[Indices: map]` |rarr| :mini:`array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present, and :mini:`nil` otherwise.


:mini:`meth :reshape(Arg₁: array, Arg₂: list)`

:mini:`meth :sums(Array: array, Index: integer)` |rarr| :mini:`array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :prods(Array: array, Index: integer)` |rarr| :mini:`array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :sum(Array: array)` |rarr| :mini:`integer` or :mini:`real`
   Returns the sum of the values in :mini:`Array`.


:mini:`meth :sum(Array: array, Index: integer)` |rarr| :mini:`array`
   Returns a new array with the sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :prod(Array: array)` |rarr| :mini:`integer` or :mini:`real`
   Returns the product of the values in :mini:`Array`.


:mini:`meth :prod(Array: array, Index: integer)` |rarr| :mini:`array`
   Returns a new array with the products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth -(Array: array)` |rarr| :mini:`array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth :copy(Array: array)` |rarr| :mini:`array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.


:mini:`meth $(List: list)` |rarr| :mini:`array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(List: list)` |rarr| :mini:`array`
   Returns an array with the contents of :mini:`List`, transposed.


:mini:`meth :copy(Array: array, Function: function)` |rarr| :mini:`array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth :update(Array: array, Function: function)` |rarr| :mini:`array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth .(A: array, B: array)` |rarr| :mini:`array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match, skipping any dimensions of size :mini:`1`.


