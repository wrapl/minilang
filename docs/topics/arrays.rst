Arrays
======

*Minilang* can optionally be built with support for multidimensional arrays. There are several different types of array, each capable of storing different types of values:

* :mini:`array::uint`\ ``N``, N = 8, 16, 32 or 64. Arrays of ``N`` bit unsigned integers.
* :mini:`array::int`\ ``N``, N = 8, 16, 32 or 64. Arrays of ``N`` bit signed integers.
* :mini:`array::float`\ ``N``, N = 32 or 64. Arrays of ``N`` bit floating point real numbers.
* :mini:`array::complex`\ ``N``, N = 32 or 64. Arrays of ``2 * N`` bit floating point complex numbers. Only available if *Minilang* was built with support for complex numbers.
* :mini:`array::any`. Arrays of any *Minilang* value. Not all operations are available for these arrays.

Each array type (except :mini:`array::any`) also has other parent types denoting their properties:

* Integer array types are subtypes of :mini:`array::integer`.
* Real array types (including integer array types) are subtypes of :mini:`array::real`.
* Complex array types (including real and integer array types) are subtypes of :mini:`array::complex`.

If an array has exactly 1 or 2 dimensions, it will have type :mini:`vector::`\ ``type`` or :mini:`matrix::`\ ``type`` respectively. Each vector and matrix type is also a subtype of one or more of :mini:`vector::integer`, :mini:`vector::real`, ..., :mini:`matrix::integer`, ... etc.

.. graphviz::

   digraph hierarchy {
      size="180,120";
      rankdir="LR";
      fontsize="40pt"
      concentrate=true;
      overlap=false;
      packMode="node";
      outputorder="edgesfirst";
      node [shape=box,fontsize=24];
      "array":e -> "vector":w;
      "array":e -> "matrix":w;
      "array":e -> "array::complex":w;
      "array::complex":e -> "vector::complex":w;
      "vector":e -> "vector::complex":w;
      "array::complex":e -> "matrix::complex":w;
      "matrix":e -> "matrix::complex":w;
      "array::complex":e -> "array::real":w;
      "array":e -> "array::real":w;
      "array::real":e -> "array::integer":w;
      "array::real":e -> "vector::real":w;
      "vector":e -> "vector::real":w;
      "vector::real":e -> "vector::integer":w;
      "array::real":e -> "matrix::real":w;
      "matrix":e -> "matrix::real":w;
      "matrix::real":e -> "matrix::integer":w;
      "array::integer":e -> "array::uint8":w;
      "vector::integer":e -> "vector::uint8":w;
      "array::uint8":e -> "vector::uint8":w;
      "matrix::integer":e -> "matrix::uint8":w;
      "array::uint8":e -> "matrix::uint8":w;
      "array::integer":e -> "array::int8":w;
      "vector::integer":e -> "vector::int8":w;
      "array::int8":e -> "vector::int8":w;
      "matrix::integer":e -> "matrix::int8":w;
      "array::int8":e -> "matrix::int8":w;
      "array::integer":e -> "array::uint16":w;
      "vector::integer":e -> "vector::uint16":w;
      "array::uint16":e -> "vector::uint16":w;
      "matrix::integer":e -> "matrix::uint16":w;
      "array::uint16":e -> "matrix::uint16":w;
      "array::integer":e -> "array::int16":w;
      "vector::integer":e -> "vector::int16":w;
      "array::int16":e -> "vector::int16":w;
      "matrix::integer":e -> "matrix::int16":w;
      "array::int16":e -> "matrix::int16":w;
      "array::integer":e -> "array::uint32":w;
      "vector::integer":e -> "vector::uint32":w;
      "array::uint32":e -> "vector::uint32":w;
      "matrix::integer":e -> "matrix::uint32":w;
      "array::uint32":e -> "matrix::uint32":w;
      "array::integer":e -> "array::int32":w;
      "vector::integer":e -> "vector::int32":w;
      "array::int32":e -> "vector::int32":w;
      "matrix::integer":e -> "matrix::int32":w;
      "array::int32":e -> "matrix::int32":w;
      "array::integer":e -> "array::uint64":w;
      "vector::integer":e -> "vector::uint64":w;
      "array::uint64":e -> "vector::uint64":w;
      "matrix::integer":e -> "matrix::uint64":w;
      "array::uint64":e -> "matrix::uint64":w;
      "array::integer":e -> "array::int64":w;
      "vector::integer":e -> "vector::int64":w;
      "array::int64":e -> "vector::int64":w;
      "matrix::integer":e -> "matrix::int64":w;
      "array::int64":e -> "matrix::int64":w;
      "array::real":e -> "array::float32":w;
      "vector::real":e -> "vector::float32":w;
      "array::float32":e -> "vector::float32":w;
      "matrix::real":e -> "matrix::float32":w;
      "array::float32":e -> "matrix::float32":w;
      "array::real":e -> "array::float64":w;
      "vector::real":e -> "vector::float64":w;
      "array::float64":e -> "vector::float64":w;
      "matrix::real":e -> "matrix::float64":w;
      "array::float64":e -> "matrix::float64":w;
      "array::complex":e -> "array::complex32":w;
      "vector::complex":e -> "vector::complex32":w;
      "array::complex32":e -> "vector::complex32":w;
      "matrix::complex":e -> "matrix::complex32":w;
      "array::complex32":e -> "matrix::complex32":w;
      "array::complex":e -> "array::complex64":w;
      "vector::complex":e -> "vector::complex64":w;
      "array::complex64":e -> "vector::complex64":w;
      "matrix::complex":e -> "matrix::complex64":w;
      "array::complex64":e -> "matrix::complex64":w;
      "array":e -> "array::any":w;
      "vector":e -> "vector::any":w;
      "array::any":e -> "vector::any":w;
      "matrix":e -> "matrix::any":w;
      "array::any":e -> "matrix::any":w;
   }

Constructing Arrays
-------------------

Arrays can be created by calling the corresponding type with a list of dimension sizes. All the entries of new array will be initialized to 0 and can be assigned later.

.. code-block:: mini
   :linenos:

   let A := array::int32([2, 2])
   print('A = {A}')
   A[1, 1] := 11
   A[1, 2] := 12
   A[2, 1] := 21
   A[2, 2] := 22
   print('A = {A}')

.. code-block:: console

   A = <<0 0> <0 0>>
   A = <<11 12> <21 22>>

Instead of assigning each value later, a function can be passed to the array constructor to populate the values. The function will be called with the indices of each entry and the returned value will be assigned to the entry in the new array.

.. code-block:: mini
   :linenos:

   let A := array::int32([2, 2], fun(I, J) I * 10 + J)
   :> or
   let A := array::int32([2, 2]; I, J) I * 10 + J

.. code-block:: console

   A = <<11 12> <21 22>>

Arrays can also be constructed directly from nested lists using :mini:`array`. In this case, the array type and shape is inferred from the list values and structure.

.. code-block:: mini
   :linenos:

   let A := array([[1, 2], [3, 4]])
   print('A: {type(A)}{A:shape}\n')
   let B := array([1.0, 2.0, 3.0])
   print('B: {type(B)}{B:shape}\n')
   let C := array(["A", "B", "C"])
   print('C: {type(C)}{C:shape}\n')

.. code-block:: console

   A: <<matrix::int64>>[2, 2]
   B: <<vector::float64>>[3]
   C: <<vector::any>>[3]

For convenience, the prefix operators :mini:`$` and :mini:`^` can be used with lists to construct arrays more easily. :mini:`$List` is equivalent to :mini:`array(List)`. :mini:`^List` adds an extra dimension of size 1 and then transposes the resulting array. It is mainly used for constructing column vectors without requiring each value being wrapped in its own list.

.. code-block:: mini
   :linenos:

   let A := $[[1, 2], [3, 4]]
   print('A: {type(A)}{A:shape}\n')
   let B := $[1.0, 2.0, 3.0]
   print('B: {type(B)}{B:shape}\n')
   let C := ^["A", "B", "C"]
   print('C: {type(C)}{C:shape}\n')

.. code-block:: console

   A: <<matrix::int64>>[2, 2]
   B: <<vector::float64>>[3]
   C: <<vector::any>>[3, 1]

Array Operations
----------------

The standard numeric infix operations (:mini:`+`, :mini:`-`, :mini:`*`, :mini:`/` and :mini:`^`) are all available for arrays and operate element-wise. When used with 2 arrays with different dimension counts, the smaller array is used repeatedly for each of the additional dimensions in the bigger array. This is often called *broadcasting* in other array libraries.

