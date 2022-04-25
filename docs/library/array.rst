.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

array
=====

.. _fun-mlcborreadcomplex32:

:mini:`fun mlcborreadcomplex32(Arg₁: any)`
   *TBD*


.. _fun-mlcborreadcomplex64:

:mini:`fun mlcborreadcomplex64(Arg₁: any)`
   *TBD*


.. _type-array:

:mini:`type array < buffer, sequence`
   Base type for multidimensional arrays.


.. _fun-array:

:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.
   The shape and type of the array is determined from the elements in :mini:`List`.


.. _fun-array-hcat:

:mini:`fun array::hcat(Array₁: array, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the last dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::hcat(A, B) :> <<1 2 3 7 8 9> <4 5 6 10 11 12>>


.. _fun-array-vcat:

:mini:`fun array::vcat(Array₁: array, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the first dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::vcat(A, B) :> <<1 2 3> <4 5 6> <7 8 9> <10 11 12>>


:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.


:mini:`meth (A: array) <> (B: array): integer`
   Compare the degrees,  dimensions and entries of  :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.


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
   
   * If :mini:`Indexᵢ` is an :mini:`array::int8` with dimensions matching the corresponding dimensions of :mini:`A` then a sparse dimension is added with entries corresponding to the non-zero values in :mini:`Indexᵢ` (i.e. :mini:`A[B]` is equivalent to :mini:`A[B:where]`).
   * If :mini:`Indexᵢ` is an :mini:`array::int32` with all but last dimensions matching the corresponding dimensions of :mini:`A` then a sparse dimension is added with entries corresponding indices in the last dimension of :mini:`Indexᵢ`.
   
   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A[1] :> <<19 16 12> <4 7 20>>
      A[1, 2] :> <4 7 20>
      A[1, 2, 3] :> 20
      A[nil, 2] :> <<4 7 20> <20 9 20>>
      A[.., 3] :> <<12 20> <8 20>>
      A[.., 1 .. 2] :> <<<19 16> <4 7>> <<5 17> <20 9>>>
      A[(1, 2, 3)] :> 20
      A[[(1, 2, 3), (2, 1, 1)]] :> <20 5>
      let B := A > 10 :> <<<1 1 1> <0 0 1>> <<0 1 0> <1 0 1>>>
      type(B) :> <<array::int8>>
      A[B] :> <19 16 12 20 17 20 20>
      let C := A:maxidx(2) :> <<2 3> <2 1>>
      type(C) :> <<matrix::int32>>
      A[C] :> <20 20>


:mini:`meth (Array: array)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.
   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present,  and :mini:`nil` otherwise.


:mini:`meth ^(Array: array): array`
   Returns the transpose of :mini:`Array`,  sharing the underlying data.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      ^A :> <<1 4> <2 5> <3 6>>


:mini:`meth (Array: array):copy: array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.


:mini:`meth (Array: array):copy(Function: function): array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth (Array: array):count: integer`
   Return the number of elements in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:count :> 6


:mini:`meth (Array: array):degree: integer`
   Return the degree of :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:degree :> 2


:mini:`meth (Array: array):expand(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth (Array: array):join(Start: integer, Count: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimensions at :mini:`Start .. (Start + Count)` with a single dimension with the same overall size.


:mini:`meth (Array: array):maxidx: array`
   Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxidx :> <1 2 3>


:mini:`meth (Array: array):maxidx(Count: integer): array`
   Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxidx(1) :> <<<1> <3>> <<2> <1>>>
      A:maxidx(2) :> <<2 3> <2 1>>


:mini:`meth (Array: array):maxval: number`
   Returns the maximum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxval :> 20


:mini:`meth (Array: array):maxval(Count: integer): array`
   Returns a new array with the maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxval(1) :> <<19 20> <17 20>>
      A:maxval(2) :> <20 20>


:mini:`meth (Array: array):minidx: array`
   Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minidx :> <1 2 1>


:mini:`meth (Array: array):minidx(Count: integer): array`
   Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minidx(1) :> <<<3> <1>> <<1> <2>>>
      A:minidx(2) :> <<2 1> <1 1>>


:mini:`meth (Array: array):minval: number`
   Returns the minimum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minval :> 4


:mini:`meth (Array: array):minval(Count: integer): array`
   Returns a new array with the minimums :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minval(1) :> <<12 4> <5 9>>
      A:minval(2) :> <4 5>


:mini:`meth (Array: array):permute(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array`,  permuting the axes according to :mini:`Indices`.

   .. code-block:: mini

      let A := array([[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]])
      :> <<<1 2 3> <4 5 6>> <<7 8 9> <10 11 12>>>
      A:shape :> [2, 2, 3]
      let B := A:permute([2, 3, 1])
      :> <<<1 7> <2 8> <3 9>> <<4 10> <5 11> <6 12>>>
      B:shape :> [2, 3, 2]


:mini:`meth (Array: array):prod: number`
   Returns the product of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:prod :> 720


:mini:`meth (Array: array):prod(Count: integer): array`
   Returns a new array with the products of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:prod(1) :> <6 120>


:mini:`meth (Array: array):prods(Index: integer): array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array):reshape(Sizes: list): array`
   Returns a copy of :mini:`Array` with dimensions specified by :mini:`Sizes`.
   .. note::
   
      This method always makes a copy of the data so that changes to the returned array do not affect the original.


:mini:`meth (Array: array):shape: list`
   Return the shape of :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:shape :> [2, 3]


:mini:`meth (Array: array):size: integer`
   Return the size of :mini:`Array` in contiguous bytes,  or :mini:`nil` if :mini:`Array` is not contiguous.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:size :> 48
      let B := ^A :> <<1 4> <2 5> <3 6>>
      B:size :> nil


:mini:`meth (Array: array):split(Index: integer, Sizes: list): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimension at :mini:`Index` with new dimensions with sizes :mini:`Sizes`. The total count :mini:`Sizes₁ * Sizes₂ * ... * Sizesₙ` must equal the original size.


:mini:`meth (Array: array):strides: list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth (Array: array):sum: number`
   Returns the sum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:sum :> 21


:mini:`meth (Array: array):sum(Index: integer): array`
   Returns a new array with the sums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:sum(1) :> <6 15>


:mini:`meth (Array: array):sums(Index: integer): array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array):swap(Index₁: integer, Index₂: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index₁` and :mini:`Index₂` swapped.


:mini:`meth (Array: array):update(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth (Array: array):where: list`
   Returns a list of non-zero indices of :mini:`Array`.


:mini:`meth (Array: array):where(Function: function): list[tuple]`
   Returns list of indices :mini:`Array` where :mini:`Function(Arrayᵢ)` returns a non-nil value.


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

:mini:`type array::real < array::complex`
   Base type for arrays of real numbers.


.. _type-array-real:

:mini:`type array::real < array`
   Base type for arrays of real numbers.


:mini:`meth (Arg₁: array::real) ^ (Arg₂: real)`
   *TBD*


.. _fun-array-cat:

:mini:`fun array::cat(Index: integer, Array₁: any, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the :mini:`Index`-th dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::cat(1, A, B)
      :> <<1 2 3> <4 5 6> <7 8 9> <10 11 12>>
      array::cat(2, A, B) :> <<1 2 3 7 8 9> <4 5 6 10 11 12>>


:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`,  transposed.


.. _type-matrix:

:mini:`type matrix < array`
   Arrays with exactly 2 dimensions.


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


.. _type-matrix-complex:

:mini:`type matrix::complex < array::complex, matrix`
   Base type for matrices of complex numbers.


.. _type-matrix-integer:

:mini:`type matrix::integer < matrix::real`
   Base type for matrices of integers.


.. _type-matrix-real:

:mini:`type matrix::real < array::real, matrix`
   Base type for matrices of real numbers.


.. _fun-array-new:

:mini:`fun array::new(Arg₁: type, Arg₂: list)`
   *TBD*


.. _fun-array-wrap:

:mini:`fun array::wrap(Type: type, Buffer: buffer, Sizes: list, Strides: list): array`
   Returns an array pointing to the contents of :mini:`Address` with the corresponding sizes and strides.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:C0EB2EF83F7F00000000000000000000>
      array::wrap(array::uint16, B, [2, 2, 2], [8, 4, 2])
      :> <<<60352 63534> <32575 0>> <<0 0> <0 0>>>


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


:mini:`meth (Vector: vector::real):softmax: vector`
   Returns :mini:`softmax(Vector)`.

   .. code-block:: mini

      let A := array([1, 4.2, 0.6, 1.23, 4.3, 1.2, 2.5])
      :> <1 4.2 0.6 1.23 4.3 1.2 2.5>
      let B := A:softmax
      :> <0.01659 0.406995 0.0111206 0.0208802 0.449799 0.0202631 0.0743513>


