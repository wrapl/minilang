array
=====

:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.

   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`type array < address, sequence`
   Base type for multidimensional arrays.


:mini:`type vector < array`
   *TBD*

:mini:`type matrix < array`
   *TBD*

:mini:`type array::real < array`
   *TBD*

:mini:`type array::integer < array::real`
   *TBD*

:mini:`type vector::real < array::real, vector`
   *TBD*

:mini:`type vector::integer < vector::real`
   *TBD*

:mini:`type matrix::real < array::real, matrix`
   *TBD*

:mini:`type matrix::integer < matrix::real`
   *TBD*

:mini:`type array::complex < array`
   *TBD*

:mini:`type vector::complex < array::complex, vector`
   *TBD*

:mini:`type matrix::complex < array::complex, matrix`
   *TBD*

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


:mini:`meth (Arg₁: array):swap(Arg₂: integer, Arg₃: integer)`
   *TBD*

:mini:`meth (Array: array):expand(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth (Arg₁: array):split(Arg₂: integer, Arg₃: list)`
   *TBD*

:mini:`meth (Arg₁: array):join(Arg₂: integer, Arg₃: integer)`
   *TBD*

:mini:`meth (Array: array):strides: list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth (Array: array):size: integer`
   Return the size of :mini:`Array` in bytes.


:mini:`meth (Array: array)[Indices...: any]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by the corresponding :mini:`Indexᵢ`.

   * If :mini:`Indexᵢ` is :mini:`nil` then the :mini:`i`-th dimension is copied unchanged.

   * If :mini:`Indexᵢ` is an integer then the :mini:`Indexᵢ`-th value is selected and the :mini:`i`-th dimension is dropped from the result.

   * If :mini:`Indexᵢ` is a list of integers then the :mini:`i`-th dimension is copied as a sparse dimension with the respective entries.

   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.


:mini:`meth (Array: array)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present,  and :mini:`nil` otherwise.


:mini:`meth (Arg₁: array)[Arg₂: tuple]`
   *TBD*

:mini:`type array::int8 < array::integer`
   An array of int8 values.


:mini:`type vector::int8 < vector::integer, array::int8`
   *TBD*

:mini:`type matrix::int8 < matrix::integer, array::int8`
   *TBD*

:mini:`type array::uint8 < array::integer`
   An array of uint8 values.


:mini:`type vector::uint8 < vector::integer, array::uint8`
   *TBD*

:mini:`type matrix::uint8 < matrix::integer, array::uint8`
   *TBD*

:mini:`type array::int16 < array::integer`
   An array of int16 values.


:mini:`type vector::int16 < vector::integer, array::int16`
   *TBD*

:mini:`type matrix::int16 < matrix::integer, array::int16`
   *TBD*

:mini:`type array::uint16 < array::integer`
   An array of uint16 values.


:mini:`type vector::uint16 < vector::integer, array::uint16`
   *TBD*

:mini:`type matrix::uint16 < matrix::integer, array::uint16`
   *TBD*

:mini:`type array::int32 < array::integer`
   An array of int32 values.


:mini:`type vector::int32 < vector::integer, array::int32`
   *TBD*

:mini:`type matrix::int32 < matrix::integer, array::int32`
   *TBD*

:mini:`type array::uint32 < array::integer`
   An array of uint32 values.


:mini:`type vector::uint32 < vector::integer, array::uint32`
   *TBD*

:mini:`type matrix::uint32 < matrix::integer, array::uint32`
   *TBD*

:mini:`type array::int64 < array::integer`
   An array of int64 values.


:mini:`type vector::int64 < vector::integer, array::int64`
   *TBD*

:mini:`type matrix::int64 < matrix::integer, array::int64`
   *TBD*

:mini:`type array::uint64 < array::integer`
   An array of uint64 values.


:mini:`type vector::uint64 < vector::integer, array::uint64`
   *TBD*

:mini:`type matrix::uint64 < matrix::integer, array::uint64`
   *TBD*

:mini:`type array::float32 < array::real`
   An array of float32 values.


:mini:`type vector::float32 < vector::real, array::float32`
   *TBD*

:mini:`type matrix::float32 < matrix::real, array::float32`
   *TBD*

:mini:`type array::float64 < array::real`
   An array of float64 values.


:mini:`type vector::float64 < vector::real, array::float64`
   *TBD*

:mini:`type matrix::float64 < matrix::real, array::float64`
   *TBD*

:mini:`type array::complex32 < array::complex`
   An array of complex32 values.


:mini:`type vector::complex32 < vector::complex, array::complex32`
   *TBD*

:mini:`type matrix::complex32 < matrix::complex, array::complex32`
   *TBD*

:mini:`type array::complex64 < array::complex`
   An array of complex64 values.


:mini:`type vector::complex64 < vector::complex, array::complex64`
   *TBD*

:mini:`type matrix::complex64 < matrix::complex, array::complex64`
   *TBD*

:mini:`type array::any < array`
   An array of any values.


:mini:`type vector::any < vector, array::any`
   *TBD*

:mini:`type matrix::any < matrix, array::any`
   *TBD*

:mini:`meth (Arg₁: array):reshape(Arg₂: list)`
   *TBD*

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


:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (A: array) + (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.


:mini:`meth (A: integer) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.


:mini:`meth (A: array) + (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ + B`.


:mini:`meth (A: double) + (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A + Bᵥ`.


:mini:`meth (A: array) * (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.


:mini:`meth (A: integer) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.


:mini:`meth (A: array) * (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ * B`.


:mini:`meth (A: double) * (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A * Bᵥ`.


:mini:`meth (A: array) - (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.


:mini:`meth (A: integer) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.


:mini:`meth (A: array) - (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ - B`.


:mini:`meth (A: double) - (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A - Bᵥ`.


:mini:`meth (A: array) / (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.


:mini:`meth (A: integer) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.


:mini:`meth (A: array) / (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := Aᵥ / B`.


:mini:`meth (A: double) / (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := A / Bᵥ`.


:mini:`meth (A: array) = (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: integer) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) = (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: double) = (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) != (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: integer) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) != (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: double) != (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) < (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: integer) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) < (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: double) < (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) > (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: integer) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) > (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: double) > (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) <= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: integer) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) <= (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: double) <= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) >= (B: integer): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: integer) >= (B: array): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: array) >= (B: double): array`
   Returns an array :mini:`C` where :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: double) >= (B: array): array`
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


:mini:`meth (Array: array):where(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth (Array: array):where: list`
   Returns a list of non-zero indices of :mini:`Array`.


:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.


