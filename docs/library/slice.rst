.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

slice
=====

.. rst-class:: mini-api

:mini:`type slice < sequence`
   A slice of elements.


:mini:`meth slice(): slice`
   Returns an empty slice.

   .. code-block:: mini

      slice() :> []


:mini:`meth slice(Sequence: sequence, ...): slice`
   Returns a list of all of the values produced by :mini:`Sequence`.

   .. code-block:: mini

      slice(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


:mini:`meth slice(Tuple: tuple): slice`
   Returns a slice containing the values in :mini:`Tuple`.

   .. code-block:: mini

      slice((1, 2, 3)) :> [1, 2, 3]


:mini:`meth (Arg₁: slice) != (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) + (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) < (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) <= (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) = (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) > (Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: slice) >= (Arg₂: slice)`
   *TBD*


:mini:`meth (Slice: slice)[Index: integer]: slice::index | nil`
   Returns the :mini:`Index`-th node in :mini:`Slice` or :mini:`nil` if :mini:`Index` is outside the interval of :mini:`List`.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.

   .. code-block:: mini

      let L := slice(["a", "b", "c", "d", "e", "f"])
      L[3] :> "c"
      L[-2] :> "e"
      L[8] :> nil


:mini:`meth (Arg₁: slice):afind(Arg₂: any, Arg₃: function)`
   *TBD*


:mini:`meth (Arg₁: slice):afind(Arg₂: any, Arg₃: function, Arg₄: function)`
   *TBD*


:mini:`meth (Arg₁: slice):afinder`
   *TBD*


:mini:`meth (Arg₁: slice):bfind(Arg₂: any)`
   *TBD*


:mini:`meth (Arg₁: slice):bfind(Arg₂: any, Arg₃: function)`
   *TBD*


:mini:`meth (Slice: slice):capacity: integer`
   Returns the capacity of :mini:`Slice`

   .. code-block:: mini

      slice([1, 2, 3]):capacity :> 3


:mini:`meth (Slice: slice):count: integer`
   Returns the length of :mini:`Slice`

   .. code-block:: mini

      slice([1, 2, 3]):count :> 3


:mini:`meth (Arg₁: slice):find(Arg₂: any)`
   *TBD*


:mini:`meth (Arg₁: slice):find(Arg₂: any, Arg₃: function)`
   *TBD*


:mini:`meth (Slice: slice):first`
   Returns the first value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.


:mini:`meth (Slice: slice):first2`
   Returns the first index and value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.


:mini:`meth (Slice: slice):last`
   Returns the last value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.


:mini:`meth (Slice: slice):last2`
   Returns the last index and value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.


:mini:`meth (Slice: slice):length: integer`
   Returns the length of :mini:`Slice`

   .. code-block:: mini

      slice([1, 2, 3]):length :> 3


:mini:`meth (Slice: slice):offset: integer`
   Returns the offset of :mini:`Slice`

   .. code-block:: mini

      slice([1, 2, 3]):offset :> 0


:mini:`meth (Slice: slice):precount: integer`
   Returns the length of :mini:`Slice`

   .. code-block:: mini

      slice([1, 2, 3]):precount :> 3


:mini:`meth (Arg₁: slice):random`
   *TBD*


:mini:`meth (Arg₁: slice):sort`
   *TBD*


:mini:`meth (Arg₁: slice):sort(Arg₂: function)`
   *TBD*


:mini:`meth (Arg₁: slice):sort(Arg₂: method)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: slice, Arg₃: string)`
   *TBD*


:mini:`type slice::index`
   An assignable reference to an index of a slice.


:mini:`type slice::iter`
   *TBD*


:mini:`type slice::mutable < slice`
   *TBD*


:mini:`meth (Slice: slice::mutable)[Interval: integer::interval]: slice::slice`
   Returns a slice of :mini:`Slice` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`,  both inclusive.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the slice,  with :mini:`-1` returning the last node.


:mini:`meth (Slice: slice::mutable)[Interval: integer::range]: slice::slice`
   Returns a slice of :mini:`Slice` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`,  both inclusive.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the slice,  with :mini:`-1` returning the last node.


:mini:`meth (Slice: slice::mutable)[Indices: integer, Arg₃: integer]: slice`
   Returns a slice containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.


:mini:`meth (Arg₁: slice::mutable):cycle`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):delete(Arg₂: integer)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):empty`
   *TBD*


:mini:`meth (Slice: slice::mutable):filter(Filter: function): slice`
   Removes every :mini:`Value` from :mini:`Slice` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.

   .. code-block:: mini

      let L := slice([1, 2, 3, 4, 5, 6])
      L:filter(2 | _) :> [1, 3, 5]
      L :> [2, 4, 6]


:mini:`meth (Slice: slice::mutable):grow(Sequence: sequence, ...): slice`
   Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.

   .. code-block:: mini

      let L := slice([1, 2, 3])
      L:grow(4 .. 6) :> [1, 2, 3, 4, 5, 6]


:mini:`meth (Arg₁: slice::mutable):insert(Arg₂: integer, Arg₃: any)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):order`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):order(Arg₂: function)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):permutations`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):permute`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):pop`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):pop(Arg₂: function)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):pull`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):pull(Arg₂: function)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):push(...)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):put(...)`
   *TBD*


:mini:`meth (Slice: slice::mutable):remove(Filter: function): slice`
   Removes every :mini:`Value` from :mini:`Slice` for which :mini:`Function(Value)` returns non-:mini:`nil` and returns those values in a new list.

   .. code-block:: mini

      let L := slice([1, 2, 3, 4, 5, 6])
      L:remove(2 | _) :> [2, 4, 6]
      L :> [1, 3, 5]


:mini:`meth (Arg₁: slice::mutable):reverse`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):shuffle`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):splice`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):splice(Arg₂: integer, Arg₃: integer)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):splice(Arg₂: integer, Arg₃: integer, Arg₄: slice::mutable)`
   *TBD*


:mini:`meth (Arg₁: slice::mutable):splice(Arg₂: integer, Arg₃: slice::mutable)`
   *TBD*


:mini:`type slice::mutable::iter < slice::iter`
   *TBD*


:mini:`type slice::slice`
   A sub-slice.


:mini:`meth (Arg₁: visitor):const(Arg₂: slice::mutable)`
   *TBD*


:mini:`meth (Arg₁: visitor):copy(Arg₂: slice)`
   *TBD*


:mini:`meth (Arg₁: visitor):visit(Arg₂: slice)`
   *TBD*


