.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

array
=====

.. rst-class:: mini-api

:mini:`meth (A: any) !== (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: any) * (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let B := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 * B :> <<2 4> <6 8>>


:mini:`meth (A: any) + (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let B := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 + B :> <<3 4> <5 6>>


:mini:`meth (A: any) - (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let B := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 - B :> <<1 0> <-1 -2>>


:mini:`meth (A: any) / (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let B := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 / B :> <<2 1> <0.666667 0.5>>


:mini:`meth (A: any) < (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: any) <= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: any) == (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: any) > (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: any) >= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`type array < address, sequence`
   Base type for multidimensional arrays.


:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.
   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`fun array::hcat(Array₁: array, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the last dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::hcat(A, B) :> <<1 2 3 7 8 9> <4 5 6 10 11 12>>


:mini:`fun array::vcat(Array₁: array, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the first dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::vcat(A, B) :> <<1 2 3> <4 5 6> <7 8 9> <10 11 12>>


:mini:`meth (A: array) != (B: array): integer`
   Compare the degrees,  dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`nil` if they match and :mini:`B` otherwise.


:mini:`meth (A: array) !== (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array) !== (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array) !== (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array) !== (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array) * (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * 2 :> <<2 4> <6 8>>


:mini:`meth (A: array) * (B: array): array`
   Returns :mini:`A * B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A * B :> <<7 16 27> <40 55 72>>
      B * A :> <<7 16 27> <40 55 72>>
      A * C :> <<5 20 45> <20 50 90>>
      C * A :> <<5 20 45> <20 50 90>>
      B * C :> <<35 80 135> <50 110 180>>
      C * B :> <<35 80 135> <50 110 180>>


:mini:`meth (A: array) ** (B: array): array`
   Returns an array with :mini:`Aᵢ * Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A ** B
      :> <<<7 2> <4 11>> <<56 16> <32 88>> <<21 6> <12 33>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array) + (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + 2 :> <<3 4> <5 6>>


:mini:`meth (A: array) + (B: array): array`
   Returns :mini:`A + B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A + B :> <<8 10 12> <14 16 18>>
      B + A :> <<8 10 12> <14 16 18>>
      A + C :> <<6 12 18> <9 15 21>>
      C + A :> <<6 12 18> <9 15 21>>
      B + C :> <<12 18 24> <15 21 27>>
      C + B :> <<12 18 24> <15 21 27>>


:mini:`meth (A: array) ++ (B: array): array`
   Returns an array with :mini:`Aᵢ + Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A ++ B
      :> <<<8 3> <5 12>> <<15 10> <12 19>> <<10 5> <7 14>>>
      C:shape :> [3, 2, 2]


:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (A: array) - (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - 2 :> <<-1 0> <1 2>>


:mini:`meth (A: array) - (B: array): array`
   Returns :mini:`A - B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A - B :> <<-6 -6 -6> <-6 -6 -6>>
      B - A :> <<6 6 6> <6 6 6>>
      A - C :> <<-4 -8 -12> <-1 -5 -9>>
      C - A :> <<4 8 12> <1 5 9>>
      B - C :> <<2 -2 -6> <5 1 -3>>
      C - B :> <<-2 2 6> <-5 -1 3>>


:mini:`meth (A: array) -- (B: array): array`
   Returns an array with :mini:`Aᵢ - Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A -- B
      :> <<<-6 -1> <-3 -10>> <<1 6> <4 -3>> <<-4 1> <-1 -8>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.


:mini:`meth (A: array) / (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / 2 :> <<0.5 1> <1.5 2>>


:mini:`meth (A: array) / (B: array): array`
   Returns :mini:`A / B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A / B :> <<0.142857 0.25 0.333333> <0.4 0.454545 0.5>>
      B / A :> <<7 4 3> <2.5 2.2 2>>
      A / C :> <<0.2 0.2 0.2> <0.8 0.5 0.4>>
      C / A :> <<5 5 5> <1.25 2 2.5>>
      B / C :> <<1.4 0.8 0.6> <2 1.1 0.8>>
      C / B :> <<0.714286 1.25 1.66667> <0.5 0.909091 1.25>>


:mini:`meth (A: array) // (B: array): array`
   Returns an array with :mini:`Aᵢ / Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A // B
      :> <<<0.142857 0.5> <0.25 0.0909091>> <<1.14286 4> <2 0.727273>> <<0.428571 1.5> <0.75 0.272727>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array) /\\ (B: array): array`
   Returns :mini:`A /\ B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A /\ B :> <<1 0 1> <0 1 4>>
      B /\ A :> <<1 0 1> <0 1 4>>
      A /\ C :> <<1 2 3> <4 0 6>>
      C /\ A :> <<1 2 3> <4 0 6>>
      B /\ C :> <<5 8 9> <0 10 12>>
      C /\ B :> <<5 8 9> <0 10 12>>


:mini:`meth (A: array) /\\ (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise and B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A /\ 2 :> <<0 2> <2 0>>


:mini:`meth (A: array) < (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array) < (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array) < (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array) < (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array) <= (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array) <= (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array) <= (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array) <= (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array) <> (B: array): integer`
   Compare the degrees,  dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.


:mini:`meth (A: array) = (B: array): integer`
   Compare the degrees,  dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`B` if they match and :mini:`nil` otherwise.


:mini:`meth (A: array) == (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array) == (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array) == (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array) == (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array) > (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array) > (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array) > (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array) > (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array) >< (B: array): array`
   Returns :mini:`A >< B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A >< B :> <<7 10 11> <14 15 14>>
      B >< A :> <<7 10 11> <14 15 14>>
      A >< C :> <<5 10 15> <5 15 15>>
      C >< A :> <<5 10 15> <5 15 15>>
      B >< C :> <<7 10 15> <15 11 15>>
      C >< B :> <<7 10 15> <15 11 15>>


:mini:`meth (A: array) >< (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise xor B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A >< 2 :> <<3 0> <1 6>>


:mini:`meth (A: array) >= (B: any): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: array) >= (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: array) >= (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: array) >= (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (Array: array)[Index₁: any, ...]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data,  indexed by :mini:`Indexᵢ`.
   Dimensions are copied to the output array,  applying the indices as follows:
   
   * If :mini:`Indexᵢ` is :mini:`nil` or :mini:`*` then the next dimension is copied unchanged.
   
   * If :mini:`Indexᵢ` is :mini:`..` then the remaining indices are applied to the last dimensions of :mini:`Array` and the dimensions in between are copied unchanged.
   
   * If :mini:`Indexᵢ` is an :mini:`integer` then the :mini:`Indexᵢ`-th value of the next dimension is selected and the dimension is dropped from the output.
   
   * If :mini:`Indexᵢ` is an :mini:`integer::interval` then the corresponding slice of the next dimension is copied to the output.
   
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
      type(B) :> <<array::mutable::int8>>
      A[B] :> <19 16 12 20 17 20 20>
      let C := A:maxidx(2) :> <<2 3> <2 1>>
      type(C) :> <<matrix::mutable::int32>>
      A[C] :> <20 20>


:mini:`meth (Array: array)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.
   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present,  and :mini:`nil` otherwise.


:mini:`meth (A: array) \\/ (B: array): array`
   Returns :mini:`A \/ B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A \/ B :> <<7 10 11> <14 15 14>>
      B \/ A :> <<7 10 11> <14 15 14>>
      A \/ C :> <<5 10 15> <5 15 15>>
      C \/ A :> <<5 10 15> <5 15 15>>
      B \/ C :> <<7 10 15> <15 11 15>>
      C \/ B :> <<7 10 15> <15 11 15>>


:mini:`meth (A: array) \\/ (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise or B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A \/ 2 :> <<3 2> <3 6>>


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


:mini:`meth (A: array):max(B: array): array`
   Returns :mini:`A max B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A max B :> <<7 8 9> <10 11 12>>
      B max A :> <<7 8 9> <10 11 12>>
      A max C :> <<5 10 15> <5 10 15>>
      C max A :> <<5 10 15> <5 10 15>>
      B max C :> <<7 10 15> <10 11 15>>
      C max B :> <<7 10 15> <10 11 15>>


:mini:`meth (A: array):max(B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A max 2 :> <<2 2> <3 4>>


:mini:`meth (A: array):max(B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A max 2.5 :> <<2.5 2.5> <3 4>>


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


:mini:`meth (A: array):min(B: array): array`
   Returns :mini:`A min B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`A:shape` is a prefix of :mini:`B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      let B := array([[7, 8, 9], [10, 11, 12]])
      :> <<7 8 9> <10 11 12>>
      let C := array([5, 10, 15]) :> <5 10 15>
      A min B :> <<1 2 3> <4 5 6>>
      B min A :> <<1 2 3> <4 5 6>>
      A min C :> <<1 2 3> <4 5 6>>
      C min A :> <<1 2 3> <4 5 6>>
      B min C :> <<5 8 9> <5 10 12>>
      C min B :> <<5 8 9> <5 10 12>>


:mini:`meth (A: array):min(B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A min 2 :> <<1 2> <2 2>>


:mini:`meth (A: array):min(B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A min 2.5 :> <<1 2> <2.5 2.5>>


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
      A:minidx(2) :> <<2 1> <1 1229342293>>


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


:mini:`meth (Array: array):permute(Indices: integer, ...): array`
   Returns an array sharing the underlying data with :mini:`Array`,  permuting the axes according to :mini:`Indices`.

   .. code-block:: mini

      let A := array([[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]])
      :> <<<1 2 3> <4 5 6>> <<7 8 9> <10 11 12>>>
      A:shape :> [2, 2, 3]
      let B := A:permute(2, 3, 1)
      :> <<<1 7> <2 8> <3 9>> <<4 10> <5 11> <6 12>>>
      B:shape :> [2, 3, 2]


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


:mini:`meth (Array: array):reverse(Index: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimension :mini:`Index` reversed.


:mini:`meth (Array: array):shape: list`
   Return the shape of :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:shape :> [2, 3]


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


:mini:`meth (Array: array):swap: array`
   Returns the transpose of :mini:`Array`,  sharing the underlying data.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:swap :> <<1 4> <2 5> <3 6>>


:mini:`meth (Array: array):swap(Index₁: integer, Index₂: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index₁` and :mini:`Index₂` swapped.


:mini:`meth (Array: array):where: list`
   Returns a list of non-zero indices of :mini:`Array`.


:mini:`meth (Array: array):where(Function: function): list[tuple]`
   Returns list of indices :mini:`Array` where :mini:`Function(Arrayᵢ)` returns a non-nil value.


:mini:`meth ||(Array: array): number`
   Returns the norm of the values in :mini:`Array`.


:mini:`meth (Array: array) || (Arg₂: real): number`
   Returns the norm of the values in :mini:`Array`.


:mini:`type array::any < array`
   *TBD*


:mini:`fun array::any(Sizes: list[integer]): array::any`
    Returns a new array of any values with the specified dimensions.


:mini:`type array::complex < array`
   *TBD*


:mini:`meth (A: array::complex) * (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * (1 + 1i) :> <<1 + 1i 2 + 2i> <3 + 3i 4 + 4i>>


:mini:`meth (A: array::complex) + (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + (1 + 1i) :> <<2 + 1i 3 + 1i> <4 + 1i 5 + 1i>>


:mini:`meth (A: array::complex) - (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - (1 + 1i) :> <<0 - 1i 1 - 1i> <2 - 1i 3 - 1i>>


:mini:`meth (A: array::complex) / (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / (1 + 1i) :> <<0.5 - 0.5i 1 - 1i> <1.5 - 1.5i 2 - 2i>>


:mini:`meth (Arg₁: array::complex) ^ (Arg₂: complex)`
   *TBD*


:mini:`type array::complex32 < array::complex`
   *TBD*


:mini:`fun array::complex32(Sizes: list[integer]): array::complex32`
    Returns a new array of complex32 values with the specified dimensions.


:mini:`type array::complex64 < array::complex`
   *TBD*


:mini:`fun array::complex64(Sizes: list[integer]): array::complex64`
    Returns a new array of complex64 values with the specified dimensions.


:mini:`type array::float32 < array::real`
   *TBD*


:mini:`fun array::float32(Sizes: list[integer]): array::float32`
    Returns a new array of float32 values with the specified dimensions.


:mini:`type array::float64 < array::real`
   *TBD*


:mini:`fun array::float64(Sizes: list[integer]): array::float64`
    Returns a new array of float64 values with the specified dimensions.


:mini:`type array::int16 < array::integer`
   *TBD*


:mini:`fun array::int16(Sizes: list[integer]): array::int16`
    Returns a new array of int16 values with the specified dimensions.


:mini:`type array::int32 < array::integer`
   *TBD*


:mini:`fun array::int32(Sizes: list[integer]): array::int32`
    Returns a new array of int32 values with the specified dimensions.


:mini:`type array::int64 < array::integer`
   *TBD*


:mini:`fun array::int64(Sizes: list[integer]): array::int64`
    Returns a new array of int64 values with the specified dimensions.


:mini:`type array::int8 < array::integer`
   *TBD*


:mini:`fun array::int8(Sizes: list[integer]): array::int8`
    Returns a new array of int8 values with the specified dimensions.


:mini:`type array::integer < array::real`
   *TBD*


:mini:`meth (A: array::integer) * (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * 2 :> <<2 4> <6 8>>


:mini:`meth (A: array::integer) + (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + 2 :> <<3 4> <5 6>>


:mini:`meth (A: array::integer) - (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - 2 :> <<-1 0> <1 2>>


:mini:`meth (A: array::integer) / (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / 2 :> <<0.5 1> <1.5 2>>


:mini:`type array::iterator`
   *TBD*


:mini:`type array::mutable < array, buffer`
   *TBD*


:mini:`meth (Array: array::mutable):update(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`type array::mutable::any < array::any, array::mutable`
   An array of any values.
   
   :mini:`(A: array::mutable::any) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::any) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::complex < array::complex, array::mutable`
   Base type for arrays of complex numbers.


:mini:`type array::mutable::complex32 < array::complex32, array::mutable::complex`
   An array of complex32 values.
   
   :mini:`(A: array::mutable::complex32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::complex32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::complex64 < array::complex64, array::mutable::complex`
   An array of complex64 values.
   
   :mini:`(A: array::mutable::complex64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::complex64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::float32 < array::float32, array::mutable::real`
   An array of float32 values.
   
   :mini:`(A: array::mutable::float32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::float32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::float64 < array::float64, array::mutable::real`
   An array of float64 values.
   
   :mini:`(A: array::mutable::float64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::float64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::int16 < array::int16, array::mutable::integer`
   An array of int16 values.
   
   :mini:`(A: array::mutable::int16) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::int16) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::int32 < array::int32, array::mutable::integer`
   An array of int32 values.
   
   :mini:`(A: array::mutable::int32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::int32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::int64 < array::int64, array::mutable::integer`
   An array of int64 values.
   
   :mini:`(A: array::mutable::int64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::int64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::int8 < array::int8, array::mutable::integer`
   An array of int8 values.
   
   :mini:`(A: array::mutable::int8) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::int8) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::integer < array::integer, array::mutable::real`
   Base type for arrays of integers.


:mini:`type array::mutable::iterator < array::iterator`
   *TBD*


:mini:`type array::mutable::real < array::real, array::mutable::complex`
   Base type for arrays of real numbers.


:mini:`type array::mutable::uint16 < array::uint16, array::mutable::integer`
   An array of uint16 values.
   
   :mini:`(A: array::mutable::uint16) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::uint16) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::uint32 < array::uint32, array::mutable::integer`
   An array of uint32 values.
   
   :mini:`(A: array::mutable::uint32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::uint32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::uint64 < array::uint64, array::mutable::integer`
   An array of uint64 values.
   
   :mini:`(A: array::mutable::uint64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::uint64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::mutable::uint8 < array::uint8, array::mutable::integer`
   An array of uint8 values.
   
   :mini:`(A: array::mutable::uint8) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::mutable::uint8) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


:mini:`type array::real < array::complex`
   *TBD*


:mini:`meth (A: array::real) * (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * 2.5 :> <<2.5 5> <7.5 10>>


:mini:`meth (A: array::real) + (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + 2.5 :> <<3.5 4.5> <5.5 6.5>>


:mini:`meth (A: array::real) - (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - 2.5 :> <<-1.5 -0.5> <0.5 1.5>>


:mini:`meth (A: array::real) / (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / 2.5 :> <<0.4 0.8> <1.2 1.6>>


:mini:`meth (Arg₁: array::real) ^ (Arg₂: real)`
   *TBD*


:mini:`type array::uint16 < array::integer`
   *TBD*


:mini:`fun array::uint16(Sizes: list[integer]): array::uint16`
    Returns a new array of uint16 values with the specified dimensions.


:mini:`type array::uint32 < array::integer`
   *TBD*


:mini:`fun array::uint32(Sizes: list[integer]): array::uint32`
    Returns a new array of uint32 values with the specified dimensions.


:mini:`type array::uint64 < array::integer`
   *TBD*


:mini:`fun array::uint64(Sizes: list[integer]): array::uint64`
    Returns a new array of uint64 values with the specified dimensions.


:mini:`type array::uint8 < array::integer`
   *TBD*


:mini:`fun array::uint8(Sizes: list[integer]): array::uint8`
    Returns a new array of uint8 values with the specified dimensions.


:mini:`meth (A: complex) !== (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) * (B: array::complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) * A :> <<1 + 1i 2 + 2i> <3 + 3i 4 + 4i>>


:mini:`meth (A: complex) + (B: array::complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) + A :> <<2 + 1i 3 + 1i> <4 + 1i 5 + 1i>>


:mini:`meth (A: complex) - (B: array::complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) - A :> <<0 + 1i -1 + 1i> <-2 + 1i -3 + 1i>>


:mini:`meth (A: complex) / (B: array::complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) / A
      :> <<1 + 1i 0.5 + 0.5i> <0.333333 + 0.333333i 0.25 + 0.25i>>


:mini:`meth (A: complex) < (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) <= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) == (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) > (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) >= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`fun array::cat(Index: integer, Array₁: any, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the :mini:`Index`-th dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::cat(1, A, B)
      :> <<1 2 3> <4 5 6> <7 8 9> <10 11 12>>
      array::cat(2, A, B) :> <<1 2 3 7 8 9> <4 5 6 10 11 12>>


:mini:`meth (A: integer) !== (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) * (B: array::integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 * A :> <<2 4> <6 8>>


:mini:`meth (A: integer) + (B: array::integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 + A :> <<3 4> <5 6>>


:mini:`meth (A: integer) - (B: array::integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 - A :> <<1 0> <-1 -2>>


:mini:`meth (A: integer) / (B: array::integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 / A :> <<2 1> <0.666667 0.5>>


:mini:`meth (A: integer) /\\ (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise and Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 /\ A :> <<0 2> <2 0>>


:mini:`meth (A: integer) < (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) <= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) == (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) > (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) >< (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise xor Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 >< A :> <<3 0> <1 6>>


:mini:`meth (A: integer) >= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) \\/ (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise or Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 \/ A :> <<3 2> <3 6>>


:mini:`meth (A: integer):max(B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 max A :> <<2 2> <3 4>>


:mini:`meth (A: integer):min(B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 min A :> <<1 2> <2 2>>


:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth (List: list)[Indices: vector]: list`
   Returns a list containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.


:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`,  transposed.


:mini:`type matrix < array`
   Arrays with exactly 2 dimensions.


:mini:`meth (T: matrix) @ (X: vector): vector`
   Returns :mini:`X` transformed by :mini:`T`. :mini:`T` must be a :mini:`N` |times| :mini:`N` matrix and :mini:`X` a vector of size :mini:`N - 1`.


:mini:`meth \\(A: matrix): matrix`
   Returns the inverse of :mini:`A`.


:mini:`meth (A: matrix) \\ (B: vector): vector`
   Returns the solution :mini:`X` of :mini:`A . X = B`.


:mini:`meth (A: matrix):det: any`
   Returns the determinant of :mini:`A`.


:mini:`meth (A: matrix):tr: any`
   Returns the trace of :mini:`A`.


:mini:`type matrix::any < matrix, array::any`
   *TBD*


:mini:`type matrix::complex < array::complex, matrix`
   *TBD*


:mini:`type matrix::complex32 < matrix::complex, array::complex32`
   *TBD*


:mini:`type matrix::complex64 < matrix::complex, array::complex64`
   *TBD*


:mini:`type matrix::float32 < matrix::real, array::float32`
   *TBD*


:mini:`type matrix::float64 < matrix::real, array::float64`
   *TBD*


:mini:`type matrix::int16 < matrix::integer, array::int16`
   *TBD*


:mini:`type matrix::int32 < matrix::integer, array::int32`
   *TBD*


:mini:`type matrix::int64 < matrix::integer, array::int64`
   *TBD*


:mini:`type matrix::int8 < matrix::integer, array::int8`
   *TBD*


:mini:`type matrix::integer < matrix::real`
   *TBD*


:mini:`type matrix::mutable < matrix, array::mutable`
   *TBD*


:mini:`type matrix::mutable::any < matrix::any, matrix::mutable, array::mutable::any`
   A matrix of any values.


:mini:`type matrix::mutable::complex < array::mutable::complex, matrix::mutable`
   Base type for matrices of complex numbers.


:mini:`type matrix::mutable::complex32 < matrix::complex32, matrix::mutable::complex, array::mutable::complex32`
   A matrix of complex32 values.


:mini:`type matrix::mutable::complex64 < matrix::complex64, matrix::mutable::complex, array::mutable::complex64`
   A matrix of complex64 values.


:mini:`type matrix::mutable::float32 < matrix::float32, matrix::mutable::real, array::mutable::float32`
   A matrix of float32 values.


:mini:`type matrix::mutable::float64 < matrix::float64, matrix::mutable::real, array::mutable::float64`
   A matrix of float64 values.


:mini:`type matrix::mutable::int16 < matrix::int16, matrix::mutable::integer, array::mutable::int16`
   A matrix of int16 values.


:mini:`type matrix::mutable::int32 < matrix::int32, matrix::mutable::integer, array::mutable::int32`
   A matrix of int32 values.


:mini:`type matrix::mutable::int64 < matrix::int64, matrix::mutable::integer, array::mutable::int64`
   A matrix of int64 values.


:mini:`type matrix::mutable::int8 < matrix::int8, matrix::mutable::integer, array::mutable::int8`
   A matrix of int8 values.


:mini:`type matrix::mutable::integer < matrix::integer, matrix::mutable::real`
   Base type for matrices of integers.


:mini:`type matrix::mutable::real < matrix::real, array::mutable::real, matrix::mutable::complex`
   Base type for matrices of real numbers.


:mini:`type matrix::mutable::uint16 < matrix::uint16, matrix::mutable::integer, array::mutable::uint16`
   A matrix of uint16 values.


:mini:`type matrix::mutable::uint32 < matrix::uint32, matrix::mutable::integer, array::mutable::uint32`
   A matrix of uint32 values.


:mini:`type matrix::mutable::uint64 < matrix::uint64, matrix::mutable::integer, array::mutable::uint64`
   A matrix of uint64 values.


:mini:`type matrix::mutable::uint8 < matrix::uint8, matrix::mutable::integer, array::mutable::uint8`
   A matrix of uint8 values.


:mini:`type matrix::real < array::real, matrix::complex`
   *TBD*


:mini:`type matrix::uint16 < matrix::integer, array::uint16`
   *TBD*


:mini:`type matrix::uint32 < matrix::integer, array::uint32`
   *TBD*


:mini:`type matrix::uint64 < matrix::integer, array::uint64`
   *TBD*


:mini:`type matrix::uint8 < matrix::integer, array::uint8`
   *TBD*


:mini:`type permutation < vector::uint32`
   A permutation of numbers :mini:`1 .. N` (each number occurs exactly once).


:mini:`meth (A: real) !== (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) * (B: array::real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 * A :> <<2.5 5> <7.5 10>>


:mini:`meth (A: real) + (B: array::real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 + A :> <<3.5 4.5> <5.5 6.5>>


:mini:`meth (A: real) - (B: array::real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 - A :> <<1.5 0.5> <-0.5 -1.5>>


:mini:`meth (A: real) / (B: array::real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 / A :> <<2.5 1.25> <0.833333 0.625>>


:mini:`meth (A: real) < (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) <= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) == (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) > (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) >= (B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: real):max(B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 max A :> <<2 2> <3 4>>


:mini:`meth (A: real):min(B: array): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 min A :> <<1 2> <2 2>>


:mini:`type ref::any`
   *TBD*


:mini:`type ref::complex32`
   *TBD*


:mini:`type ref::complex64`
   *TBD*


:mini:`type ref::float32`
   *TBD*


:mini:`type ref::float64`
   *TBD*


:mini:`type ref::int16`
   *TBD*


:mini:`type ref::int32`
   *TBD*


:mini:`type ref::int64`
   *TBD*


:mini:`type ref::int8`
   *TBD*


:mini:`type ref::uint16`
   *TBD*


:mini:`type ref::uint32`
   *TBD*


:mini:`type ref::uint64`
   *TBD*


:mini:`type ref::uint8`
   *TBD*


:mini:`meth $(Slice: slice): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(Slice: slice): array`
   Returns an array with the contents of :mini:`Slice`,  transposed.


:mini:`fun array::new(Arg₁: type, Arg₂: list)`
   *TBD*


:mini:`fun array::wrap(Type: type, Buffer: address, Sizes: list, Strides: list): array`
   Returns an array pointing to the contents of :mini:`Address` with the corresponding sizes and strides.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:00000000000000000000000000000000>
      array::wrap(array::uint16, B, [2, 2, 2], [8, 4, 2])
      :> <<<0 0> <0 0>> <<0 0> <0 0>>>


:mini:`type vector < array`
   Arrays with exactly 1 dimension.


:mini:`type vector::any < vector, array::any`
   *TBD*


:mini:`type vector::complex < array::complex, vector`
   *TBD*


:mini:`type vector::complex32 < vector::complex, array::complex32`
   *TBD*


:mini:`type vector::complex64 < vector::complex, array::complex64`
   *TBD*


:mini:`type vector::float32 < vector::real, array::float32`
   *TBD*


:mini:`type vector::float64 < vector::real, array::float64`
   *TBD*


:mini:`type vector::int16 < vector::integer, array::int16`
   *TBD*


:mini:`type vector::int32 < vector::integer, array::int32`
   *TBD*


:mini:`type vector::int64 < vector::integer, array::int64`
   *TBD*


:mini:`type vector::int8 < vector::integer, array::int8`
   *TBD*


:mini:`type vector::integer < vector::real`
   *TBD*


:mini:`type vector::mutable < vector, array::mutable`
   *TBD*


:mini:`type vector::mutable::any < vector::any, vector::mutable, array::mutable::any`
   A vector of any values.


:mini:`type vector::mutable::complex < vector::complex, array::mutable::complex, vector::mutable`
   Base type for vectors of complex numbers.


:mini:`type vector::mutable::complex32 < vector::complex32, vector::mutable::complex, array::mutable::complex32`
   A vector of complex32 values.


:mini:`type vector::mutable::complex64 < vector::complex64, vector::mutable::complex, array::mutable::complex64`
   A vector of complex64 values.


:mini:`type vector::mutable::float32 < vector::float32, vector::mutable::real, array::mutable::float32`
   A vector of float32 values.


:mini:`type vector::mutable::float64 < vector::float64, vector::mutable::real, array::mutable::float64`
   A vector of float64 values.


:mini:`type vector::mutable::int16 < vector::int16, vector::mutable::integer, array::mutable::int16`
   A vector of int16 values.


:mini:`type vector::mutable::int32 < vector::int32, vector::mutable::integer, array::mutable::int32`
   A vector of int32 values.


:mini:`type vector::mutable::int64 < vector::int64, vector::mutable::integer, array::mutable::int64`
   A vector of int64 values.


:mini:`type vector::mutable::int8 < vector::int8, vector::mutable::integer, array::mutable::int8`
   A vector of int8 values.


:mini:`type vector::mutable::integer < vector::integer, vector::mutable::real`
   Base type for vectors of integers.


:mini:`type vector::mutable::real < vector::real, array::mutable::real, vector::mutable::complex`
   Base type for vectors of real numbers.


:mini:`type vector::mutable::uint16 < vector::uint16, vector::mutable::integer, array::mutable::uint16`
   A vector of uint16 values.


:mini:`type vector::mutable::uint32 < vector::uint32, vector::mutable::integer, array::mutable::uint32`
   A vector of uint32 values.


:mini:`type vector::mutable::uint64 < vector::uint64, vector::mutable::integer, array::mutable::uint64`
   A vector of uint64 values.


:mini:`type vector::mutable::uint8 < vector::uint8, vector::mutable::integer, array::mutable::uint8`
   A vector of uint8 values.


:mini:`type vector::real < array::real, vector::complex`
   *TBD*


:mini:`meth (Vector: vector::real):softmax: vector`
   Returns :mini:`softmax(Vector)`.

   .. code-block:: mini

      let A := array([1, 4.2, 0.6, 1.23, 4.3, 1.2, 2.5])
      :> <1 4.2 0.6 1.23 4.3 1.2 2.5>
      let B := A:softmax
      :> <0.01659 0.406995 0.0111206 0.0208802 0.449799 0.0202631 0.0743513>


:mini:`type vector::uint16 < vector::integer, array::uint16`
   *TBD*


:mini:`type vector::uint32 < vector::integer, array::uint32`
   *TBD*


:mini:`type vector::uint64 < vector::integer, array::uint64`
   *TBD*


:mini:`type vector::uint8 < vector::integer, array::uint8`
   *TBD*


:mini:`meth (Arg₁: visitor):const(Arg₂: array)`
   *TBD*


:mini:`meth (Arg₁: visitor):copy(Arg₂: array)`
   *TBD*


