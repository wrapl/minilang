.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

array
=====

.. rst-class:: mini-api

.. _type-array:

:mini:`type array < array::const, buffer`
   Base type for multidimensional arrays.


.. _fun-array:

:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.
   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`meth (Array: array):update(Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


.. _type-array-any:

:mini:`type array::any < array::const::any, array`
   An array of any values.
   
   :mini:`(A: array::any) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::any) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-any:

:mini:`fun array::any(Sizes: list[integer]): array::any`
    Returns a new array of any values with the specified dimensions.


.. _type-array-complex:

:mini:`type array::complex < array::const::complex, array`
   Base type for arrays of complex numbers.


:mini:`meth (Arg₁: array::complex) ^ (Arg₂: complex)`
   *TBD*


.. _type-array-complex32:

:mini:`type array::complex32 < array::const::complex32, array::complex`
   An array of complex32 values.
   
   :mini:`(A: array::complex32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::complex32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-complex32:

:mini:`fun array::complex32(Sizes: list[integer]): array::complex32`
    Returns a new array of complex32 values with the specified dimensions.


.. _type-array-complex64:

:mini:`type array::complex64 < array::const::complex64, array::complex`
   An array of complex64 values.
   
   :mini:`(A: array::complex64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::complex64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-complex64:

:mini:`fun array::complex64(Sizes: list[integer]): array::complex64`
    Returns a new array of complex64 values with the specified dimensions.


.. _type-array-const:

:mini:`type array::const < address, sequence`
   *TBD*


.. _fun-array-hcat:

:mini:`fun array::hcat(Array₁: array::const, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the last dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::hcat(A, B) :> <<1 2 3 7 8 9> <4 5 6 10 11 12>>


.. _fun-array-vcat:

:mini:`fun array::vcat(Array₁: array::const, ...): array`
   Returns a new array with the values of :mini:`Array₁,  ...,  Arrayₙ` concatenated along the first dimension.

   .. code-block:: mini

      let A := $[[1, 2, 3], [4, 5, 6]] :> <<1 2 3> <4 5 6>>
      let B := $[[7, 8, 9], [10, 11, 12]]
      :> <<7 8 9> <10 11 12>>
      array::vcat(A, B) :> <<1 2 3> <4 5 6> <7 8 9> <10 11 12>>


:mini:`meth (A: array::const) != (B: array::const): array`
   Returns :mini:`A != B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A != B :> <<1 1 1> <0 1 1>>
      A != C :> <<0 1 1> <1 0 1>>


:mini:`meth (A: array::const) != (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array::const) != (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array::const) != (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ != B then 1 else 0 end`.


:mini:`meth (A: array::const) * (B: array::const): array`
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


:mini:`meth (A: array::const) * (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * (1 + 1i) :> <<1 + 1i 2 + 2i> <3 + 3i 4 + 4i>>


:mini:`meth (A: array::const) * (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * 2 :> <<2 4> <6 8>>


:mini:`meth (A: array::const) * (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ * B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A * 2.5 :> <<2.5 5> <7.5 10>>


:mini:`meth (A: array::const) ** (B: array::const): array`
   Returns an array with :mini:`Aᵢ * Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A ** B
      :> <<<7 2> <4 11>> <<56 16> <32 88>> <<21 6> <12 33>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array::const) + (B: array::const): array`
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


:mini:`meth (A: array::const) + (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + (1 + 1i) :> <<2 + 1i 3 + 1i> <4 + 1i 5 + 1i>>


:mini:`meth (A: array::const) + (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + 2 :> <<3 4> <5 6>>


:mini:`meth (A: array::const) + (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ + B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A + 2.5 :> <<3.5 4.5> <5.5 6.5>>


:mini:`meth (A: array::const) ++ (B: array::const): array`
   Returns an array with :mini:`Aᵢ + Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A ++ B
      :> <<<8 3> <5 12>> <<15 10> <12 19>> <<10 5> <7 14>>>
      C:shape :> [3, 2, 2]


:mini:`meth -(Array: array::const): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (A: array::const) - (B: array::const): array`
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


:mini:`meth (A: array::const) - (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - (1 + 1i) :> <<0 - 1i 1 - 1i> <2 - 1i 3 - 1i>>


:mini:`meth (A: array::const) - (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - 2 :> <<-1 0> <1 2>>


:mini:`meth (A: array::const) - (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ - B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A - 2.5 :> <<-1.5 -0.5> <0.5 1.5>>


:mini:`meth (A: array::const) -- (B: array::const): array`
   Returns an array with :mini:`Aᵢ - Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A -- B
      :> <<<-6 -1> <-3 -10>> <<1 6> <4 -3>> <<-4 1> <-1 -8>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array::const) . (B: array::const): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match,  skipping any dimensions of size :mini:`1`.


:mini:`meth (A: array::const) / (B: array::const): array`
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


:mini:`meth (A: array::const) / (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / (1 + 1i) :> <<0.5 - 0.5i 1 - 1i> <1.5 - 1.5i 2 - 2i>>


:mini:`meth (A: array::const) / (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / 2 :> <<0.5 1> <1.5 2>>


:mini:`meth (A: array::const) / (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ / B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A / 2.5 :> <<0.4 0.8> <1.2 1.6>>


:mini:`meth (A: array::const) // (B: array::const): array`
   Returns an array with :mini:`Aᵢ / Bⱼ` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
   

   .. code-block:: mini

      let A := array([1, 8, 3]) :> <1 8 3>
      let B := array([[7, 2], [4, 11]]) :> <<7 2> <4 11>>
      A:shape :> [3]
      B:shape :> [2, 2]
      let C := A // B
      :> <<<0.142857 0.5> <0.25 0.0909091>> <<1.14286 4> <2 0.727273>> <<0.428571 1.5> <0.75 0.272727>>>
      C:shape :> [3, 2, 2]


:mini:`meth (A: array::const) /\ (B: array::const): array`
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


:mini:`meth (A: array::const) /\ (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise and B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A /\ 2 :> <<0 2> <2 0>>


:mini:`meth (A: array::const) < (B: array::const): array`
   Returns :mini:`A < B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A < B :> <<1 0 1> <0 1 0>>
      A < C :> <<0 0 1> <0 0 0>>


:mini:`meth (A: array::const) < (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array::const) < (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array::const) < (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ < B then 1 else 0 end`.


:mini:`meth (A: array::const) <= (B: array::const): array`
   Returns :mini:`A <= B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A <= B :> <<1 0 1> <1 1 0>>
      A <= C :> <<1 0 1> <0 1 0>>


:mini:`meth (A: array::const) <= (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array::const) <= (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array::const) <= (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ <= B then 1 else 0 end`.


:mini:`meth (A: array::const) <> (B: array::const): integer`
   Compare the degrees,  dimensions and entries of  :mini:`A` and :mini:`B` and returns :mini:`-1`,  :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.


:mini:`meth (A: array::const) = (B: array::const): array`
   Returns :mini:`A = B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A = B :> <<0 0 0> <1 0 0>>
      A = C :> <<1 0 0> <0 1 0>>


:mini:`meth (A: array::const) = (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array::const) = (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array::const) = (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ = B then 1 else 0 end`.


:mini:`meth (A: array::const) > (B: array::const): array`
   Returns :mini:`A > B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A > B :> <<0 1 0> <0 0 1>>
      A > C :> <<0 1 0> <1 0 1>>


:mini:`meth (A: array::const) > (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array::const) > (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array::const) > (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ > B then 1 else 0 end`.


:mini:`meth (A: array::const) >< (B: array::const): array`
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


:mini:`meth (A: array::const) >< (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise xor B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A >< 2 :> <<3 0> <1 6>>


:mini:`meth (A: array::const) >= (B: array::const): array`
   Returns :mini:`A >= B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible,  i.e. either
   
   * :mini:`A:shape = B:shape` or
   * :mini:`B:shape` is a prefix of :mini:`A:shape`.
   
   When the shapes are not the same,  remaining dimensions are repeated (broadcast) to the required size.

   .. code-block:: mini

      let A := array([[1, 8, 3], [4, 5, 12]])
      :> <<1 8 3> <4 5 12>>
      let B := array([[7, 2, 9], [4, 11, 6]])
      :> <<7 2 9> <4 11 6>>
      let C := array([1, 5, 10]) :> <1 5 10>
      A >= B :> <<0 1 0> <1 0 1>>
      A >= C :> <<1 1 0> <1 1 1>>


:mini:`meth (A: array::const) >= (B: complex): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: array::const) >= (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (A: array::const) >= (B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if Aᵥ >= B then 1 else 0 end`.


:mini:`meth (Array: array::const)[Index₁: any, ...]: array`
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


:mini:`meth (Array: array::const)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.
   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present,  and :mini:`nil` otherwise.


:mini:`meth (A: array::const) \/ (B: array::const): array`
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


:mini:`meth (A: array::const) \/ (B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := Aᵥ bitwise or B`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A \/ 2 :> <<3 2> <3 6>>


:mini:`meth ^(Array: array::const): array`
   Returns the transpose of :mini:`Array`,  sharing the underlying data.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      ^A :> <<1 4> <2 5> <3 6>>


:mini:`meth (Array: array::const):copy: array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.


:mini:`meth (Array: array::const):copy(Function: function): array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth (Array: array::const):count: integer`
   Return the number of elements in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:count :> 6


:mini:`meth (Array: array::const):degree: integer`
   Return the degree of :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:degree :> 2


:mini:`meth (Array: array::const):expand(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth (Array: array::const):join(Start: integer, Count: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimensions at :mini:`Start .. (Start + Count)` with a single dimension with the same overall size.


:mini:`meth (A: array::const):max(B: array::const): array`
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


:mini:`meth (A: array::const):max(B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A max 2 :> <<2 2> <3 4>>


:mini:`meth (A: array::const):max(B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A max 2.5 :> <<2.5 2.5> <3 4>>


:mini:`meth (Array: array::const):maxidx: array`
   Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxidx :> <1 2 3>


:mini:`meth (Array: array::const):maxidx(Count: integer): array`
   Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxidx(1) :> <<<1> <3>> <<2> <1>>>
      A:maxidx(2) :> <<2 3> <2 1>>


:mini:`meth (Array: array::const):maxval: number`
   Returns the maximum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxval :> 20


:mini:`meth (Array: array::const):maxval(Count: integer): array`
   Returns a new array with the maximums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:maxval(1) :> <<19 20> <17 20>>
      A:maxval(2) :> <20 20>


:mini:`meth (A: array::const):min(B: array::const): array`
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


:mini:`meth (A: array::const):min(B: integer): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A min 2 :> <<1 2> <2 2>>


:mini:`meth (A: array::const):min(B: real): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(Aᵥ,  B)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      A min 2.5 :> <<1 2> <2.5 2.5>>


:mini:`meth (Array: array::const):minidx: array`
   Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minidx :> <1 2 1>


:mini:`meth (Array: array::const):minidx(Count: integer): array`
   Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minidx(1) :> <<<3> <1>> <<1> <2>>>
      A:minidx(2) :> <<2 1> <1 1>>


:mini:`meth (Array: array::const):minval: number`
   Returns the minimum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minval :> 4


:mini:`meth (Array: array::const):minval(Count: integer): array`
   Returns a new array with the minimums :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
      A:minval(1) :> <<12 4> <5 9>>
      A:minval(2) :> <4 5>


:mini:`meth (Array: array::const):permute(Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array`,  permuting the axes according to :mini:`Indices`.

   .. code-block:: mini

      let A := array([[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]])
      :> <<<1 2 3> <4 5 6>> <<7 8 9> <10 11 12>>>
      A:shape :> [2, 2, 3]
      let B := A:permute([2, 3, 1])
      :> <<<1 7> <2 8> <3 9>> <<4 10> <5 11> <6 12>>>
      B:shape :> [2, 3, 2]


:mini:`meth (Array: array::const):prod: number`
   Returns the product of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:prod :> 720


:mini:`meth (Array: array::const):prod(Count: integer): array`
   Returns a new array with the products of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:prod(1) :> <6 120>


:mini:`meth (Array: array::const):prods(Index: integer): array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array::const):reshape(Sizes: list): array`
   Returns a copy of :mini:`Array` with dimensions specified by :mini:`Sizes`.
   .. note::
   
      This method always makes a copy of the data so that changes to the returned array do not affect the original.


:mini:`meth (Array: array::const):shape: list`
   Return the shape of :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:shape :> [2, 3]


:mini:`meth (Array: array::const):size: integer`
   Return the size of :mini:`Array` in contiguous bytes,  or :mini:`nil` if :mini:`Array` is not contiguous.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:size :> 48
      let B := ^A :> <<1 4> <2 5> <3 6>>
      B:size :> nil


:mini:`meth (Array: array::const):split(Index: integer, Sizes: list): array`
   Returns an array sharing the underlying data with :mini:`Array` replacing the dimension at :mini:`Index` with new dimensions with sizes :mini:`Sizes`. The total count :mini:`Sizes₁ * Sizes₂ * ... * Sizesₙ` must equal the original size.


:mini:`meth (Array: array::const):strides: list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth (Array: array::const):sum: number`
   Returns the sum of the values in :mini:`Array`.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:sum :> 21


:mini:`meth (Array: array::const):sum(Index: integer): array`
   Returns a new array with the sums of :mini:`Array` in the last :mini:`Count` dimensions.

   .. code-block:: mini

      let A := array([[1, 2, 3], [4, 5, 6]])
      :> <<1 2 3> <4 5 6>>
      A:sum(1) :> <6 15>


:mini:`meth (Array: array::const):sums(Index: integer): array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth (Array: array::const):swap(Index₁: integer, Index₂: integer): array`
   Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index₁` and :mini:`Index₂` swapped.


:mini:`meth (Array: array::const):where: list`
   Returns a list of non-zero indices of :mini:`Array`.


:mini:`meth (Array: array::const):where(Function: function): list[tuple]`
   Returns list of indices :mini:`Array` where :mini:`Function(Arrayᵢ)` returns a non-nil value.


:mini:`meth ||(Array: array::const): number`
   Returns the norm of the values in :mini:`Array`.


:mini:`meth (Array: array::const) || (Arg₂: real): number`
   Returns the norm of the values in :mini:`Array`.


.. _type-array-const-any:

:mini:`type array::const::any < array::const`
   *TBD*


.. _type-array-const-complex:

:mini:`type array::const::complex < array::const`
   *TBD*


.. _type-array-const-complex32:

:mini:`type array::const::complex32 < array::const::complex`
   *TBD*


.. _type-array-const-complex64:

:mini:`type array::const::complex64 < array::const::complex`
   *TBD*


.. _type-array-const-float32:

:mini:`type array::const::float32 < array::const::real`
   *TBD*


.. _type-array-const-float64:

:mini:`type array::const::float64 < array::const::real`
   *TBD*


.. _type-array-const-int16:

:mini:`type array::const::int16 < array::const::integer`
   *TBD*


.. _type-array-const-int32:

:mini:`type array::const::int32 < array::const::integer`
   *TBD*


.. _type-array-const-int64:

:mini:`type array::const::int64 < array::const::integer`
   *TBD*


.. _type-array-const-int8:

:mini:`type array::const::int8 < array::const::integer`
   *TBD*


.. _type-array-const-integer:

:mini:`type array::const::integer < array::const::real`
   *TBD*


.. _type-array-const-real:

:mini:`type array::const::real < array::const::complex`
   *TBD*


.. _type-array-const-uint16:

:mini:`type array::const::uint16 < array::const::integer`
   *TBD*


.. _type-array-const-uint32:

:mini:`type array::const::uint32 < array::const::integer`
   *TBD*


.. _type-array-const-uint64:

:mini:`type array::const::uint64 < array::const::integer`
   *TBD*


.. _type-array-const-uint8:

:mini:`type array::const::uint8 < array::const::integer`
   *TBD*


.. _type-array-float32:

:mini:`type array::float32 < array::const::float32, array::real`
   An array of float32 values.
   
   :mini:`(A: array::float32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::float32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-float32:

:mini:`fun array::float32(Sizes: list[integer]): array::float32`
    Returns a new array of float32 values with the specified dimensions.


.. _type-array-float64:

:mini:`type array::float64 < array::const::float64, array::real`
   An array of float64 values.
   
   :mini:`(A: array::float64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::float64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-float64:

:mini:`fun array::float64(Sizes: list[integer]): array::float64`
    Returns a new array of float64 values with the specified dimensions.


.. _type-array-int16:

:mini:`type array::int16 < array::const::int16, array::integer`
   An array of int16 values.
   
   :mini:`(A: array::int16) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::int16) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-int16:

:mini:`fun array::int16(Sizes: list[integer]): array::int16`
    Returns a new array of int16 values with the specified dimensions.


.. _type-array-int32:

:mini:`type array::int32 < array::const::int32, array::integer`
   An array of int32 values.
   
   :mini:`(A: array::int32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::int32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-int32:

:mini:`fun array::int32(Sizes: list[integer]): array::int32`
    Returns a new array of int32 values with the specified dimensions.


.. _type-array-int64:

:mini:`type array::int64 < array::const::int64, array::integer`
   An array of int64 values.
   
   :mini:`(A: array::int64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::int64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-int64:

:mini:`fun array::int64(Sizes: list[integer]): array::int64`
    Returns a new array of int64 values with the specified dimensions.


.. _type-array-int8:

:mini:`type array::int8 < array::const::int8, array::integer`
   An array of int8 values.
   
   :mini:`(A: array::int8) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::int8) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-int8:

:mini:`fun array::int8(Sizes: list[integer]): array::int8`
    Returns a new array of int8 values with the specified dimensions.


.. _type-array-integer:

:mini:`type array::integer < array::const::integer, array::real`
   Base type for arrays of integers.


.. _type-array-real:

:mini:`type array::real < array::const::real, array::complex`
   Base type for arrays of real numbers.


:mini:`meth (Arg₁: array::real) ^ (Arg₂: real)`
   *TBD*


.. _type-array-uint16:

:mini:`type array::uint16 < array::const::uint16, array::integer`
   An array of uint16 values.
   
   :mini:`(A: array::uint16) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::uint16) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-uint16:

:mini:`fun array::uint16(Sizes: list[integer]): array::uint16`
    Returns a new array of uint16 values with the specified dimensions.


.. _type-array-uint32:

:mini:`type array::uint32 < array::const::uint32, array::integer`
   An array of uint32 values.
   
   :mini:`(A: array::uint32) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::uint32) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-uint32:

:mini:`fun array::uint32(Sizes: list[integer]): array::uint32`
    Returns a new array of uint32 values with the specified dimensions.


.. _type-array-uint64:

:mini:`type array::uint64 < array::const::uint64, array::integer`
   An array of uint64 values.
   
   :mini:`(A: array::uint64) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::uint64) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-uint64:

:mini:`fun array::uint64(Sizes: list[integer]): array::uint64`
    Returns a new array of uint64 values with the specified dimensions.


.. _type-array-uint8:

:mini:`type array::uint8 < array::const::uint8, array::integer`
   An array of uint8 values.
   
   :mini:`(A: array::uint8) := (B: number)`
      Sets the values in :mini:`A` to :mini:`B`.
   :mini:`(A: array::uint8) := (B: array | list)`
      Sets the values in :mini:`A` to those in :mini:`B`,  broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.


.. _fun-array-uint8:

:mini:`fun array::uint8(Sizes: list[integer]): array::uint8`
    Returns a new array of uint8 values with the specified dimensions.


:mini:`meth (A: complex) != (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) * (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) * A :> <<1 + 1i 2 + 2i> <3 + 3i 4 + 4i>>


:mini:`meth (A: complex) + (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) + A :> <<2 + 1i 3 + 1i> <4 + 1i 5 + 1i>>


:mini:`meth (A: complex) - (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) - A :> <<0 + 1i -1 + 1i> <-2 + 1i -3 + 1i>>


:mini:`meth (A: complex) / (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      (1 + 1i) / A
      :> <<1 + 1i 0.5 + 0.5i> <0.333333 + 0.333333i 0.25 + 0.25i>>


:mini:`meth (A: complex) < (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) <= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) = (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) > (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: complex) >= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (Arg₁: copy):const(Arg₂: array::const)`
   *TBD*


:mini:`meth (Arg₁: copy):copy(Arg₂: array::const)`
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


:mini:`meth (A: integer) != (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) * (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 * A :> <<2 4> <6 8>>


:mini:`meth (A: integer) + (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 + A :> <<3 4> <5 6>>


:mini:`meth (A: integer) - (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 - A :> <<1 0> <-1 -2>>


:mini:`meth (A: integer) / (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 / A :> <<2 1> <0.666667 0.5>>


:mini:`meth (A: integer) /\ (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise and Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 /\ A :> <<0 2> <2 0>>


:mini:`meth (A: integer) < (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) <= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) = (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) > (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) >< (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise xor Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 >< A :> <<3 0> <1 6>>


:mini:`meth (A: integer) >= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: integer) \/ (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A bitwise or Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 \/ A :> <<3 2> <3 6>>


:mini:`meth (A: integer):max(B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 max A :> <<2 2> <3 4>>


:mini:`meth (A: integer):min(B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2 min A :> <<1 2> <2 2>>


:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`,  transposed.


.. _type-matrix:

:mini:`type matrix < matrix::const, array`
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


.. _type-matrix-any:

:mini:`type matrix::any < matrix::const::any, matrix, array::any`
   A matrix of any values.


.. _type-matrix-complex:

:mini:`type matrix::complex < array::complex, matrix`
   Base type for matrices of complex numbers.


.. _type-matrix-complex32:

:mini:`type matrix::complex32 < matrix::const::complex32, matrix::complex, array::complex32`
   A matrix of complex32 values.


.. _type-matrix-complex64:

:mini:`type matrix::complex64 < matrix::const::complex64, matrix::complex, array::complex64`
   A matrix of complex64 values.


.. _type-matrix-const:

:mini:`type matrix::const < array::const`
   *TBD*


.. _type-matrix-const-any:

:mini:`type matrix::const::any < matrix::const, array::const::any`
   *TBD*


.. _type-matrix-const-complex:

:mini:`type matrix::const::complex < array::const::complex, matrix::const`
   *TBD*


.. _type-matrix-const-complex32:

:mini:`type matrix::const::complex32 < matrix::const::complex, array::const::complex32`
   *TBD*


.. _type-matrix-const-complex64:

:mini:`type matrix::const::complex64 < matrix::const::complex, array::const::complex64`
   *TBD*


.. _type-matrix-const-float32:

:mini:`type matrix::const::float32 < matrix::const::real, array::const::float32`
   *TBD*


.. _type-matrix-const-float64:

:mini:`type matrix::const::float64 < matrix::const::real, array::const::float64`
   *TBD*


.. _type-matrix-const-int16:

:mini:`type matrix::const::int16 < matrix::const::integer, array::const::int16`
   *TBD*


.. _type-matrix-const-int32:

:mini:`type matrix::const::int32 < matrix::const::integer, array::const::int32`
   *TBD*


.. _type-matrix-const-int64:

:mini:`type matrix::const::int64 < matrix::const::integer, array::const::int64`
   *TBD*


.. _type-matrix-const-int8:

:mini:`type matrix::const::int8 < matrix::const::integer, array::const::int8`
   *TBD*


.. _type-matrix-const-integer:

:mini:`type matrix::const::integer < matrix::const::real`
   *TBD*


.. _type-matrix-const-real:

:mini:`type matrix::const::real < array::const::real, matrix::const::complex`
   *TBD*


.. _type-matrix-const-uint16:

:mini:`type matrix::const::uint16 < matrix::const::integer, array::const::uint16`
   *TBD*


.. _type-matrix-const-uint32:

:mini:`type matrix::const::uint32 < matrix::const::integer, array::const::uint32`
   *TBD*


.. _type-matrix-const-uint64:

:mini:`type matrix::const::uint64 < matrix::const::integer, array::const::uint64`
   *TBD*


.. _type-matrix-const-uint8:

:mini:`type matrix::const::uint8 < matrix::const::integer, array::const::uint8`
   *TBD*


.. _type-matrix-float32:

:mini:`type matrix::float32 < matrix::const::float32, matrix::real, array::float32`
   A matrix of float32 values.


.. _type-matrix-float64:

:mini:`type matrix::float64 < matrix::const::float64, matrix::real, array::float64`
   A matrix of float64 values.


.. _type-matrix-int16:

:mini:`type matrix::int16 < matrix::const::int16, matrix::integer, array::int16`
   A matrix of int16 values.


.. _type-matrix-int32:

:mini:`type matrix::int32 < matrix::const::int32, matrix::integer, array::int32`
   A matrix of int32 values.


.. _type-matrix-int64:

:mini:`type matrix::int64 < matrix::const::int64, matrix::integer, array::int64`
   A matrix of int64 values.


.. _type-matrix-int8:

:mini:`type matrix::int8 < matrix::const::int8, matrix::integer, array::int8`
   A matrix of int8 values.


.. _type-matrix-integer:

:mini:`type matrix::integer < matrix::const::integer, matrix::real`
   Base type for matrices of integers.


.. _type-matrix-real:

:mini:`type matrix::real < matrix::const::real, array::real, matrix::complex`
   Base type for matrices of real numbers.


.. _type-matrix-uint16:

:mini:`type matrix::uint16 < matrix::const::uint16, matrix::integer, array::uint16`
   A matrix of uint16 values.


.. _type-matrix-uint32:

:mini:`type matrix::uint32 < matrix::const::uint32, matrix::integer, array::uint32`
   A matrix of uint32 values.


.. _type-matrix-uint64:

:mini:`type matrix::uint64 < matrix::const::uint64, matrix::integer, array::uint64`
   A matrix of uint64 values.


.. _type-matrix-uint8:

:mini:`type matrix::uint8 < matrix::const::uint8, matrix::integer, array::uint8`
   A matrix of uint8 values.


:mini:`meth (A: real) != (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A != Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) * (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A * Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 * A :> <<2.5 5> <7.5 10>>


:mini:`meth (A: real) + (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A + Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 + A :> <<3.5 4.5> <5.5 6.5>>


:mini:`meth (A: real) - (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A - Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 - A :> <<1.5 0.5> <-0.5 -1.5>>


:mini:`meth (A: real) / (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := A / Bᵥ`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 / A :> <<2.5 1.25> <0.833333 0.625>>


:mini:`meth (A: real) < (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A < Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) <= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A <= Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) = (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A = Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) > (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A > Bᵥ then 1 else 0 end`.


:mini:`meth (A: real) >= (B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := if A >= Bᵥ then 1 else 0 end`.


:mini:`meth (A: real):max(B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := max(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 max A :> <<2 2> <3 4>>


:mini:`meth (A: real):min(B: array::const): array`
   Returns an array :mini:`C` where each :mini:`Cᵥ := min(A,  Bᵥ)`.

   .. code-block:: mini

      let A := array([[1, 2], [3, 4]]) :> <<1 2> <3 4>>
      2.5 min A :> <<1 2> <2 2>>


.. _fun-array-new:

:mini:`fun array::new(Arg₁: type, Arg₂: list)`
   *TBD*


.. _fun-array-wrap:

:mini:`fun array::wrap(Type: type, Buffer: address, Sizes: list, Strides: list): array`
   Returns an array pointing to the contents of :mini:`Address` with the corresponding sizes and strides.

   .. code-block:: mini

      let B := buffer(16)
      :> <16:E00B48D5467F0000496E666F203D2049>
      array::wrap(array::uint16, B, [2, 2, 2], [8, 4, 2])
      :> <<<3040 54600> <32582 0>> <<28233 28518> <15648 18720>>>


.. _type-vector:

:mini:`type vector < vector::const, array`
   Arrays with exactly 1 dimension.


.. _type-vector-any:

:mini:`type vector::any < vector::const::any, vector, array::any`
   A vector of any values.


.. _type-vector-complex:

:mini:`type vector::complex < vector::const::complex, array::complex, vector`
   Base type for vectors of complex numbers.


.. _type-vector-complex32:

:mini:`type vector::complex32 < vector::const::complex32, vector::complex, array::complex32`
   A vector of complex32 values.


.. _type-vector-complex64:

:mini:`type vector::complex64 < vector::const::complex64, vector::complex, array::complex64`
   A vector of complex64 values.


.. _type-vector-const:

:mini:`type vector::const < array::const`
   *TBD*


.. _type-vector-const-any:

:mini:`type vector::const::any < vector::const, array::const::any`
   *TBD*


.. _type-vector-const-complex:

:mini:`type vector::const::complex < array::const::complex, vector::const`
   *TBD*


.. _type-vector-const-complex32:

:mini:`type vector::const::complex32 < vector::const::complex, array::const::complex32`
   *TBD*


.. _type-vector-const-complex64:

:mini:`type vector::const::complex64 < vector::const::complex, array::const::complex64`
   *TBD*


.. _type-vector-const-float32:

:mini:`type vector::const::float32 < vector::const::real, array::const::float32`
   *TBD*


.. _type-vector-const-float64:

:mini:`type vector::const::float64 < vector::const::real, array::const::float64`
   *TBD*


.. _type-vector-const-int16:

:mini:`type vector::const::int16 < vector::const::integer, array::const::int16`
   *TBD*


.. _type-vector-const-int32:

:mini:`type vector::const::int32 < vector::const::integer, array::const::int32`
   *TBD*


.. _type-vector-const-int64:

:mini:`type vector::const::int64 < vector::const::integer, array::const::int64`
   *TBD*


.. _type-vector-const-int8:

:mini:`type vector::const::int8 < vector::const::integer, array::const::int8`
   *TBD*


.. _type-vector-const-integer:

:mini:`type vector::const::integer < vector::const::real`
   *TBD*


.. _type-vector-const-real:

:mini:`type vector::const::real < array::const::real, vector::const::complex`
   *TBD*


.. _type-vector-const-uint16:

:mini:`type vector::const::uint16 < vector::const::integer, array::const::uint16`
   *TBD*


.. _type-vector-const-uint32:

:mini:`type vector::const::uint32 < vector::const::integer, array::const::uint32`
   *TBD*


.. _type-vector-const-uint64:

:mini:`type vector::const::uint64 < vector::const::integer, array::const::uint64`
   *TBD*


.. _type-vector-const-uint8:

:mini:`type vector::const::uint8 < vector::const::integer, array::const::uint8`
   *TBD*


.. _type-vector-float32:

:mini:`type vector::float32 < vector::const::float32, vector::real, array::float32`
   A vector of float32 values.


.. _type-vector-float64:

:mini:`type vector::float64 < vector::const::float64, vector::real, array::float64`
   A vector of float64 values.


.. _type-vector-int16:

:mini:`type vector::int16 < vector::const::int16, vector::integer, array::int16`
   A vector of int16 values.


.. _type-vector-int32:

:mini:`type vector::int32 < vector::const::int32, vector::integer, array::int32`
   A vector of int32 values.


.. _type-vector-int64:

:mini:`type vector::int64 < vector::const::int64, vector::integer, array::int64`
   A vector of int64 values.


.. _type-vector-int8:

:mini:`type vector::int8 < vector::const::int8, vector::integer, array::int8`
   A vector of int8 values.


.. _type-vector-integer:

:mini:`type vector::integer < vector::const::integer, vector::real`
   Base type for vectors of integers.


.. _type-vector-real:

:mini:`type vector::real < vector::const::real, array::real, vector`
   Base type for vectors of real numbers.


:mini:`meth (Vector: vector::real):softmax: vector`
   Returns :mini:`softmax(Vector)`.

   .. code-block:: mini

      let A := array([1, 4.2, 0.6, 1.23, 4.3, 1.2, 2.5])
      :> <1 4.2 0.6 1.23 4.3 1.2 2.5>
      let B := A:softmax
      :> <0.01659 0.406995 0.0111206 0.0208802 0.449799 0.0202631 0.0743513>


.. _type-vector-uint16:

:mini:`type vector::uint16 < vector::const::uint16, vector::integer, array::uint16`
   A vector of uint16 values.


.. _type-vector-uint32:

:mini:`type vector::uint32 < vector::const::uint32, vector::integer, array::uint32`
   A vector of uint32 values.


.. _type-vector-uint64:

:mini:`type vector::uint64 < vector::const::uint64, vector::integer, array::uint64`
   A vector of uint64 values.


.. _type-vector-uint8:

:mini:`type vector::uint8 < vector::const::uint8, vector::integer, array::uint8`
   A vector of uint8 values.


