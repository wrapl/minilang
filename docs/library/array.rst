.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

array
=====

.. _fun-array:

:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.

   The shape and type of the array is determined from the elements in :mini:`List`.


.. _type-array:

:mini:`type array < address, sequence`
   Base type for multidimensional arrays.


.. _type-vector:

:mini:`type vector < array`
   Arrays with exactly 1 dimension.


.. _type-matrix:

:mini:`type matrix < array`
   Arrays with exactly 2 dimensions.


.. _type-array-complex:

:mini:`type array::complex < array`
   Base type for arrays of complex numbers.


.. _type-vector-complex:

:mini:`type vector::complex < array::complex, vector`
   Base type for vectors of complex numbers.


.. _type-matrix-complex:

:mini:`type matrix::complex < array::complex, matrix`
   Base type for matrices of complex numbers.


.. _type-array-real:

:mini:`type array::real < array::complex`
   Base type for arrays of real numbers.


.. _type-array-real:

:mini:`type array::real < array`
   Base type for arrays of real numbers.


.. _type-array-integer:

:mini:`type array::integer < array::real`
   Base type for arrays of integers.


.. _type-vector-real:

:mini:`type vector::real < array::real, vector`
   Base type for vectors of real numbers.


.. _type-vector-integer:

:mini:`type vector::integer < vector::real`
   Base type for vectors of integers.


.. _type-matrix-real:

:mini:`type matrix::real < array::real, matrix`
   Base type for matrices of real numbers.


.. _type-matrix-integer:

:mini:`type matrix::integer < matrix::real`
   Base type for matrices of integers.


:mini:`meth (Array: array):degree: integer`
   Return the degree of :mini:`Array`.


:mini:`meth (Array: array):shape: list`
   Return the shape of :mini:`Array`.


:mini:`meth (Array: array):count: integer`
   Return the number of elements in :mini:`Array`.


:mini:`meth ^(Array: array): array`
   Returns the transpose of :mini:`Array`,  sharing the underlying data.


:mini:`meth (Array: array):permute(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array`,  permuting the axes according to :mini:`Indices`.


:mini:`meth (Array: array):swap(Index₁: integer, Index₂: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index₁` and :mini:`Index₂` swapped.


:mini:`meth (Array: array):expand(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth (Array: array):split(Index: integer, Sizes: list): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimension at :mini:`Index` with new dimensions with sizes :mini:`Sizes`. The total count :mini:`Sizes₁ * Sizes₂ * ... * Sizesₙ` must equal the original size.


:mini:`meth (Array: array):join(Start: integer, Count: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimensions at :mini:`Start .. (Start + Count)` with a single dimension with the same overall size.


:mini:`meth (Array: array):strides: list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth (Array: array):size: integer`
   Return the size of :mini:`Array` in bytes.


:mini:`meth (Array: array)[Index₁: any, ...]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data,  indexed by :mini:`Indexᵢ`.

   Dimensions are copied to the output array,  applying the indices as follows:

   * If :mini:`Indexᵢ` is :mini:`nil` or :mini:`*` then the next dimension is copied unchanged.

   * If :mini:`Indexᵢ` is :mini:`..` then the remaining indices are applied to the last dimensions of :mini:`Array` and the dimensions in between are copied unchanged.

   * If :mini:`Indexᵢ` is an :mini:`integer` then the :mini:`Indexᵢ`-th value of the next dimension is selected and the dimension is dropped from the output.

   * If :mini:`Indexᵢ` is an :mini:`integer::range` then the corresponding slice of the next dimension is copied to the output.

   * If :mini:`Indexᵢ` is a :mini:`tuple[integer,  ...]` then the next dimensions are indexed by the corresponding integer in turn (i.e. :mini:`A[(I,  J,  K)]` gives the same result as :mini:`A[I,  J,  K]`).

   * If :mini:`Indexᵢ` is a :mini:`list[integer]` then the next dimension is copied as a sparse dimension with the respective entries.

   * If :mini:`Indexᵢ` is a :mini:`list[tuple[integer,  ...]]` then the appropriate dimensions are dropped and a single sparse dimension is added with the corresponding entries.

   * If :mini:`Indexᵢ` is an :mini:`array` with dimensions that matches the corresponding dimensions of :mini:`A` then a sparse dimension is added with entries corresponding to the non-zero values in :mini:`Indexᵢ` (i.e. :mini:`A[B]` is equivalent to :mini:`A[B:where]`).

   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.


:mini:`meth (Array: array)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present,  and :mini:`nil` otherwise.


:mini:`meth (A: array) <> (B: array): integer`
   Compare the degrees,  dimensions and entries of  :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.


.. _fun-array-int8:

:mini:`fun array::int8(Sizes: list[integer]): array::int8`
    Returns a new array of int8 values with the specified dimensions.


.. _type-array-int8:

:mini:`type array::int8 < array::integer`
   An array of int8 values.


.. _type-vector-int8:

:mini:`type vector::int8 < vector::integer, array::int8`
   A vector of int8 values.


.. _type-matrix-int8:

:mini:`type matrix::int8 < matrix::integer, array::int8`
   A matrix of int8 values.


.. _fun-array-uint8:

:mini:`fun array::uint8(Sizes: list[integer]): array::uint8`
    Returns a new array of uint8 values with the specified dimensions.


.. _type-array-uint8:

:mini:`type array::uint8 < array::integer`
   An array of uint8 values.


.. _type-vector-uint8:

:mini:`type vector::uint8 < vector::integer, array::uint8`
   A vector of uint8 values.


.. _type-matrix-uint8:

:mini:`type matrix::uint8 < matrix::integer, array::uint8`
   A matrix of uint8 values.


.. _fun-array-int16:

:mini:`fun array::int16(Sizes: list[integer]): array::int16`
    Returns a new array of int16 values with the specified dimensions.


.. _type-array-int16:

:mini:`type array::int16 < array::integer`
   An array of int16 values.


.. _type-vector-int16:

:mini:`type vector::int16 < vector::integer, array::int16`
   A vector of int16 values.


.. _type-matrix-int16:

:mini:`type matrix::int16 < matrix::integer, array::int16`
   A matrix of int16 values.


.. _fun-array-uint16:

:mini:`fun array::uint16(Sizes: list[integer]): array::uint16`
    Returns a new array of uint16 values with the specified dimensions.


.. _type-array-uint16:

:mini:`type array::uint16 < array::integer`
   An array of uint16 values.


.. _type-vector-uint16:

:mini:`type vector::uint16 < vector::integer, array::uint16`
   A vector of uint16 values.


.. _type-matrix-uint16:

:mini:`type matrix::uint16 < matrix::integer, array::uint16`
   A matrix of uint16 values.


.. _fun-array-int32:

:mini:`fun array::int32(Sizes: list[integer]): array::int32`
    Returns a new array of int32 values with the specified dimensions.


.. _type-array-int32:

:mini:`type array::int32 < array::integer`
   An array of int32 values.


.. _type-vector-int32:

:mini:`type vector::int32 < vector::integer, array::int32`
   A vector of int32 values.


.. _type-matrix-int32:

:mini:`type matrix::int32 < matrix::integer, array::int32`
   A matrix of int32 values.


.. _fun-array-uint32:

:mini:`fun array::uint32(Sizes: list[integer]): array::uint32`
    Returns a new array of uint32 values with the specified dimensions.


.. _type-array-uint32:

:mini:`type array::uint32 < array::integer`
   An array of uint32 values.


.. _type-vector-uint32:

:mini:`type vector::uint32 < vector::integer, array::uint32`
   A vector of uint32 values.


.. _type-matrix-uint32:

:mini:`type matrix::uint32 < matrix::integer, array::uint32`
   A matrix of uint32 values.


.. _fun-array-int64:

:mini:`fun array::int64(Sizes: list[integer]): array::int64`
    Returns a new array of int64 values with the specified dimensions.


.. _type-array-int64:

:mini:`type array::int64 < array::integer`
   An array of int64 values.


.. _type-vector-int64:

:mini:`type vector::int64 < vector::integer, array::int64`
   A vector of int64 values.


.. _type-matrix-int64:

:mini:`type matrix::int64 < matrix::integer, array::int64`
   A matrix of int64 values.


.. _fun-array-uint64:

:mini:`fun array::uint64(Sizes: list[integer]): array::uint64`
    Returns a new array of uint64 values with the specified dimensions.


.. _type-array-uint64:

:mini:`type array::uint64 < array::integer`
   An array of uint64 values.


.. _type-vector-uint64:

:mini:`type vector::uint64 < vector::integer, array::uint64`
   A vector of uint64 values.


.. _type-matrix-uint64:

:mini:`type matrix::uint64 < matrix::integer, array::uint64`
   A matrix of uint64 values.


.. _fun-array-float32:

:mini:`fun array::float32(Sizes: list[integer]): array::float32`
    Returns a new array of float32 values with the specified dimensions.


.. _type-array-float32:

:mini:`type array::float32 < array::real`
   An array of float32 values.


.. _type-vector-float32:

:mini:`type vector::float32 < vector::real, array::float32`
   A vector of float32 values.


.. _type-matrix-float32:

:mini:`type matrix::float32 < matrix::real, array::float32`
   A matrix of float32 values.


.. _fun-array-float64:

:mini:`fun array::float64(Sizes: list[integer]): array::float64`
    Returns a new array of float64 values with the specified dimensions.


.. _type-array-float64:

:mini:`type array::float64 < array::real`
   An array of float64 values.


.. _type-vector-float64:

:mini:`type vector::float64 < vector::real, array::float64`
   A vector of float64 values.


.. _type-matrix-float64:

:mini:`type matrix::float64 < matrix::real, array::float64`
   A matrix of float64 values.


.. _fun-array-complex32:

:mini:`fun array::complex32(Sizes: list[integer]): array::complex32`
    Returns a new array of complex32 values with the specified dimensions.


.. _type-array-complex32:

:mini:`type array::complex32 < array::complex`
   An array of complex32 values.


.. _type-vector-complex32:

:mini:`type vector::complex32 < vector::complex, array::complex32`
   A vector of complex32 values.


.. _type-matrix-complex32:

:mini:`type matrix::complex32 < matrix::complex, array::complex32`
   A matrix of complex32 values.


.. _fun-array-complex64:

:mini:`fun array::complex64(Sizes: list[integer]): array::complex64`
    Returns a new array of complex64 values with the specified dimensions.


.. _type-array-complex64:

:mini:`type array::complex64 < array::complex`
   An array of complex64 values.


.. _type-vector-complex64:

:mini:`type vector::complex64 < vector::complex, array::complex64`
   A vector of complex64 values.


.. _type-matrix-complex64:

:mini:`type matrix::complex64 < matrix::complex, array::complex64`
   A matrix of complex64 values.


.. _fun-array-any:

:mini:`fun array::any(Sizes: list[integer]): array::any`
    Returns a new array of any values with the specified dimensions.


.. _type-array-any:

:mini:`type array::any < array`
   An array of any values.


.. _type-vector-any:

:mini:`type vector::any < vector, array::any`
   A vector of any values.


.. _type-matrix-any:

:mini:`type matrix::any < matrix, array::any`
   A matrix of any values.


:mini:`meth (Array: array):reshape(Sizes: list): array`
   Returns a copy of :mini:`Array` with dimensions specified by :mini:`Sizes`.

   .. note::

      Currently this method always makes a copy of the data so that changes to the returned array do not affect the original.


:mini:`meth (Array: array):sums(Index: integer): array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array):prods(Index: integer): array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array):sum: number`
   Returns the sum of the values in :mini:`Array`.


:mini:`meth (Array: array):sum(Index: integer): array`
   Returns a new array with the sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array):prod: number`
   Returns the product of the values in :mini:`Array`.


:mini:`meth (Array: array):prod(Index: integer): array`
   Returns a new array with the products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth ||(Array: array): number`
   Returns the norm of the values in :mini:`Array`.


:mini:`meth (Array: array) || (Arg₂: real): number`
   Returns the norm of the values in :mini:`Array`.


:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (A: array) + (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.


:mini:`meth (A: integer) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.


:mini:`meth (A: array) + (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.


:mini:`meth (A: real) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.


:mini:`meth (A: array) + (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.


:mini:`meth (A: complex) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.


:mini:`meth (A: array) * (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.


:mini:`meth (A: integer) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.


:mini:`meth (A: array) * (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.


:mini:`meth (A: real) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.


:mini:`meth (A: array) * (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.


:mini:`meth (A: complex) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.


:mini:`meth (A: array) - (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.


:mini:`meth (A: integer) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.


:mini:`meth (A: array) - (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.


:mini:`meth (A: real) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.


:mini:`meth (A: array) - (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.


:mini:`meth (A: complex) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.


:mini:`meth (A: array) / (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.


:mini:`meth (A: integer) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.


:mini:`meth (A: array) / (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.


:mini:`meth (A: real) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.


:mini:`meth (A: array) / (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.


:mini:`meth (A: complex) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.


:mini:`meth (Arg₁: array::complex) ^ (Arg₂: complex)`
   *TBD*

:mini:`meth (Arg₁: array::real) ^ (Arg₂: real)`
   *TBD*

:mini:`meth (A: array) = (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: integer) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) = (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: real) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) = (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: complex) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) != (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: integer) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) != (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: real) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) != (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: complex) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) < (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: integer) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) < (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: real) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) < (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: complex) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) > (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: integer) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) > (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: real) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) > (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: complex) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) <= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: integer) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) <= (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: real) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) <= (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: complex) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) >= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: integer) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) >= (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: real) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) >= (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: complex) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (Array: array):copy: array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.


:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`,  transposed.


:mini:`meth (Array: array):copy(Function: function): array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth (Array: array):update(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth (Array: array):where(Function: function): list[tuple]`
   Returns list of indices :mini:`Array` where :mini:`Function(Arrayᵢ)` returns a non-nil value.


:mini:`meth (Array: array):where: list`
   Returns a list of non-zero indices of :mini:`Array`.


:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.


:mini:`meth (T: matrix) @ (X: vector): vector`
   Returns :mini:`X` transformed by :mini:`T`. :mini:`T` must be a :mini:`N` |times| :mini:`N` matrix and :mini:`X` a vector of size :mini:`N - 1`.


:mini:`meth \(A: matrix): matrix`
   Returns the inverse of :mini:`A`.


:mini:`meth (A: matrix) \ (B: vector): vector`
   Returns the solution :mini:`X` of :mini:`A . X = B`.


:mini:`meth (A: matrix):det: any`
   Returns the determinant of :mini:`A`.


:mini:`meth (A: matrix):tr: any`
   Returns the trace of :mini:`A`.


