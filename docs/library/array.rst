array
=====

.. include:: <isonum.txt>

:mini:`array`
   Base type for multidimensional arrays.

   :Parents: :mini:`buffer`, :mini:`iteratable`


:mini:`constructor array(List: list)` |rarr| :mini:`array`
   Returns a new array containing the values in :mini:`List`.

   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`array::int8`
   Arrays of signed 8 bit integers.

   :Parents: :mini:`array`


:mini:`array::uint8`
   Arrays of unsigned 8 bit integers.

   :Parents: :mini:`array`


:mini:`array::int16`
   Arrays of signed 16 bit integers.

   :Parents: :mini:`array`


:mini:`array::uint16`
   Arrays of unsigned 16 bit integers.

   :Parents: :mini:`array`


:mini:`array::int32`
   Arrays of signed 32 bit integers.

   :Parents: :mini:`array`


:mini:`array::uint32`
   Arrays of unsigned 32 bit integers.

   :Parents: :mini:`array`


:mini:`array::int64`
   Arrays of signed 64 bit integers.

   :Parents: :mini:`array`


:mini:`array::uint64`
   Arrays of unsigned 64 bit integers.

   :Parents: :mini:`array`


:mini:`array::float32`
   Arrays of 32 bit reals.

   :Parents: :mini:`array`


:mini:`array::float64`
   Arrays of 64 bit reals.

   :Parents: :mini:`array`


:mini:`array::any`
   Arrays of any *Minilang* values.

   :Parents: :mini:`array`


:mini:`meth :degree(Array: array)` |rarr| :mini:`integer`
   Return the degree of :mini:`Array`.


:mini:`meth :shape(Array: array)` |rarr| :mini:`list`
   Return the shape of :mini:`Array`.


:mini:`meth :count(Array: array)` |rarr| :mini:`integer`
   Return the number of elements in :mini:`Array`.


:mini:`meth :transpose(Array: array)` |rarr| :mini:`array`
   Returns the transpose of :mini:`Array`, sharing the underlying data.


:mini:`meth :permute(Array: array, Indices: list)` |rarr| :mini:`array`
   Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.


:mini:`meth :expand(Array: array, Indices: list)` |rarr| :mini:`array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth :strides(Array: array)` |rarr| :mini:`list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth :size(Array: array)` |rarr| :mini:`integer`
   Return the size of :mini:`Array` in bytes.


:mini:`meth [](Array: array, Indices...: any)` |rarr| :mini:`array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by the corresponding :mini:`Indexᵢ`.

   * If :mini:`Indexᵢ` is :mini:`nil` then the :mini:`i`-th dimension is copied unchanged.

   * If :mini:`Indexᵢ` is an integer then the :mini:`Indexᵢ`-th value is selected and the :mini:`i`-th dimension is dropped from the result.

   * If :mini:`Indexᵢ` is a list of integers then the :mini:`i`-th dimension is copied as a sparse dimension with the respective entries.

   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.


:mini:`meth [](Array: array, Indices: map)` |rarr| :mini:`array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present, and :mini:`nil` otherwise.


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


:mini:`meth :copy(Array: array, Function: function)` |rarr| :mini:`array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth :update(Array: array, Function: function)` |rarr| :mini:`array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth .(A: array, B: array)` |rarr| :mini:`array`
   Returns the inner product of :mini:`A` and :mini:`B`.


