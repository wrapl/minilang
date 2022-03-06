.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

array
=====

.. _type-array:

:mini:`type array < address, sequence`
   Base type for multidimensional arrays.



:mini:`meth (A: array) != (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.



:mini:`meth (A: array) != (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.



:mini:`meth (A: array) != (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.



:mini:`meth (A: array) * (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.



:mini:`meth (A: array) * (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.



:mini:`meth (A: array) * (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.



:mini:`meth (A: array) + (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.



:mini:`meth (A: array) + (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.



:mini:`meth (A: array) + (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.



:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.



:mini:`meth (A: array) - (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.



:mini:`meth (A: array) - (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.



:mini:`meth (A: array) - (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.



:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.



:mini:`meth (A: array) / (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.



:mini:`meth (A: array) / (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.



:mini:`meth (A: array) / (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.



:mini:`meth (Array: array):copy: array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.



:mini:`meth (Array: array):copy(Function: function): array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.



:mini:`meth (Array: array):count: integer`
   Return the number of elements in :mini:`Array`.



:mini:`meth (Array: array):degree: integer`
   Return the degree of :mini:`Array`.



:mini:`meth (Array: array):expand(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.



:mini:`meth (Array: array):join(Start: integer, Count: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimensions at :mini:`Start .. (Start + Count)` with a single dimension with the same overall size.



:mini:`meth (Array: array):permute(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array`,  permuting the axes according to :mini:`Indices`.



:mini:`meth (Array: array):prod: number`
   Returns the product of the values in :mini:`Array`.



:mini:`meth (Array: array):prod(Index: integer): array`
   Returns a new array with the products of :mini:`Array` in the :mini:`Index`-th dimension.



:mini:`meth (Array: array):prods(Index: integer): array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.



:mini:`meth (Array: array):reshape(Sizes: list): array`
   Returns a copy of :mini:`Array` with dimensions specified by :mini:`Sizes`.

   .. note::

      Currently this method always makes a copy of the data so that changes to the returned array do not affect the original.



:mini:`meth (Array: array):shape: list`
   Return the shape of :mini:`Array`.



:mini:`meth (Array: array):size: integer`
   Return the size of :mini:`Array` in bytes.



:mini:`meth (Array: array):split(Index: integer, Sizes: list): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimension at :mini:`Index` with new dimensions with sizes :mini:`Sizes`. The total count :mini:`Sizes₁ * Sizes₂ * ... * Sizesₙ` must equal the original size.



:mini:`meth (Array: array):strides: list`
   Return the strides of :mini:`Array` in bytes.



:mini:`meth (Array: array):sum(Index: integer): array`
   Returns a new array with the sums of :mini:`Array` in the :mini:`Index`-th dimension.



:mini:`meth (Array: array):sum: number`
   Returns the sum of the values in :mini:`Array`.



:mini:`meth (Array: array):sums(Index: integer): array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.



:mini:`meth (Array: array):swap(Index₁: integer, Index₂: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index₁` and :mini:`Index₂` swapped.



:mini:`meth (Array: array):update(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.



:mini:`meth (Array: array):where(Function: function): list[tuple]`
   Returns list of indices :mini:`Array` where :mini:`Function(Arrayᵢ)` returns a non-nil value.



:mini:`meth (Array: array):where: list`
   Returns a list of non-zero indices of :mini:`Array`.



:mini:`meth (A: array) < (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.



:mini:`meth (A: array) < (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.



:mini:`meth (A: array) < (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.



:mini:`meth (A: array) <= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.



:mini:`meth (A: array) <= (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.



:mini:`meth (A: array) <= (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.



:mini:`meth (A: array) <> (B: array): integer`
   Compare the degrees,  dimensions and entries of  :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.



:mini:`meth (A: array) = (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.



:mini:`meth (A: array) = (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.



:mini:`meth (A: array) = (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.



:mini:`meth (A: array) > (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.



:mini:`meth (A: array) > (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.



:mini:`meth (A: array) > (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.



:mini:`meth (A: array) >= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.



:mini:`meth (A: array) >= (B: real): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.



:mini:`meth (A: array) >= (B: complex): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.



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



:mini:`meth ^(Array: array): array`
   Returns the transpose of :mini:`Array`,  sharing the underlying data.



:mini:`meth ||(Array: array): number`
   Returns the norm of the values in :mini:`Array`.



:mini:`meth (Array: array) || (Arg₂: real): number`
   Returns the norm of the values in :mini:`Array`.



.. _type-array-complex:

:mini:`type array::complex < array`
   Base type for arrays of complex numbers.



:mini:`meth (Arg₁: array::complex) ^ (Arg₂: complex)`
   *TBD*


.. _type-array-integer:

:mini:`type array::integer < array::real`
   Base type for arrays of integers.



.. _type-array-real:

:mini:`type array::real < array`
   Base type for arrays of real numbers.



.. _type-array-real:

:mini:`type array::real < array::complex`
   Base type for arrays of real numbers.



:mini:`meth (Arg₁: array::real) ^ (Arg₂: real)`
   *TBD*


:mini:`meth (A: complex) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.



:mini:`meth (A: complex) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.



:mini:`meth (A: complex) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.



:mini:`meth (A: complex) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.



:mini:`meth (A: complex) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.



:mini:`meth (A: complex) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.



:mini:`meth (A: complex) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.



:mini:`meth (A: complex) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.



:mini:`meth (A: complex) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.



:mini:`meth (A: complex) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.



:mini:`meth (A: integer) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.



:mini:`meth (A: integer) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.



:mini:`meth (A: integer) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.



:mini:`meth (A: integer) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.



:mini:`meth (A: integer) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.



:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.



:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`,  transposed.



.. _type-matrix:

:mini:`type matrix < array`
   Arrays with exactly 2 dimensions.



:mini:`meth (A: matrix):det: any`
   Returns the determinant of :mini:`A`.



:mini:`meth (A: matrix):tr: any`
   Returns the trace of :mini:`A`.



:mini:`meth (T: matrix) @ (X: vector): vector`
   Returns :mini:`X` transformed by :mini:`T`. :mini:`T` must be a :mini:`N` |times| :mini:`N` matrix and :mini:`X` a vector of size :mini:`N - 1`.



:mini:`meth (A: matrix) \ (B: vector): vector`
   Returns the solution :mini:`X` of :mini:`A . X = B`.



:mini:`meth \(A: matrix): matrix`
   Returns the inverse of :mini:`A`.



.. _type-matrix-complex:

:mini:`type matrix::complex < array::complex, matrix`
   Base type for matrices of complex numbers.



.. _type-matrix-integer:

:mini:`type matrix::integer < matrix::real`
   Base type for matrices of integers.



.. _type-matrix-real:

:mini:`type matrix::real < array::real, matrix`
   Base type for matrices of real numbers.



:mini:`meth (A: real) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.



:mini:`meth (A: real) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.



:mini:`meth (A: real) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.



:mini:`meth (A: real) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.



:mini:`meth (A: real) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.



:mini:`meth (A: real) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.



:mini:`meth (A: real) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.



:mini:`meth (A: real) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.



:mini:`meth (A: real) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.



:mini:`meth (A: real) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.



.. _type-vector:

:mini:`type vector < array`
   Arrays with exactly 1 dimension.



.. _type-vector-complex:

:mini:`type vector::complex < array::complex, vector`
   Base type for vectors of complex numbers.



.. _type-vector-integer:

:mini:`type vector::integer < vector::real`
   Base type for vectors of integers.



.. _type-vector-real:

:mini:`type vector::real < array::real, vector`
   Base type for vectors of real numbers.



