.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

list
====

.. rst-class:: mini-api

:mini:`type list < sequence`
   A list of elements.


:mini:`meth list(Sequence: sequence, ...): list`
   Returns a list of all of the values produced by :mini:`Sequence`.

   .. code-block:: mini

      list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


:mini:`meth list(): list`
   Returns an empty list.

   .. code-block:: mini

      list() :> []


:mini:`meth list(Tuple: tuple): list`
   Returns a list containing the values in :mini:`Tuple`.

   .. code-block:: mini

      list((1, 2, 3)) :> [1, 2, 3]


:mini:`meth (A: list) != (B: list): B | nil`
   Returns :mini:`B` if :mini:`A:size != B:size` or :mini:`Aᵢ != Bᵢ` for some :mini:`i`.

   .. code-block:: mini

      !=([1, 2, 3], [1, 2, 3]) :> nil
      !=([1, 2, 3], [1, 2]) :> [1, 2]
      !=([1, 2], [1, 2, 3]) :> [1, 2, 3]
      !=([1, 2, 3], [1, 2, 4]) :> [1, 2, 4]
      !=([1, 3, 2], [1, 2, 3]) :> [1, 2, 3]


:mini:`meth (List₁: list) + (List₂: list): list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.


:mini:`meth (A: list) < (B: list): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ < Bⱼ`.

   .. code-block:: mini

      <([1, 2, 3], [1, 2, 3]) :> nil
      <([1, 2, 3], [1, 2]) :> nil
      <([1, 2], [1, 2, 3]) :> [1, 2, 3]
      <([1, 2, 3], [1, 2, 4]) :> [1, 2, 4]
      <([1, 3, 2], [1, 2, 3]) :> nil


:mini:`meth (A: list) <= (B: list): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ <= Bⱼ`.

   .. code-block:: mini

      <=([1, 2, 3], [1, 2, 3]) :> [1, 2, 3]
      <=([1, 2, 3], [1, 2]) :> nil
      <=([1, 2], [1, 2, 3]) :> [1, 2, 3]
      <=([1, 2, 3], [1, 2, 4]) :> [1, 2, 4]
      <=([1, 3, 2], [1, 2, 3]) :> nil


:mini:`meth (A: list) = (B: list): B | nil`
   Returns :mini:`B` if :mini:`A:size = B:size` and :mini:`Aᵢ = Bᵢ` for each :mini:`i`.

   .. code-block:: mini

      =([1, 2, 3], [1, 2, 3]) :> [1, 2, 3]
      =([1, 2, 3], [1, 2]) :> nil
      =([1, 2], [1, 2, 3]) :> nil
      =([1, 2, 3], [1, 2, 4]) :> nil
      =([1, 3, 2], [1, 2, 3]) :> nil


:mini:`meth (A: list) > (B: list): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ > Bⱼ`.

   .. code-block:: mini

      >([1, 2, 3], [1, 2, 3]) :> nil
      >([1, 2, 3], [1, 2]) :> [1, 2]
      >([1, 2], [1, 2, 3]) :> nil
      >([1, 2, 3], [1, 2, 4]) :> nil
      >([1, 3, 2], [1, 2, 3]) :> [1, 2, 3]


:mini:`meth (A: list) >= (B: list): B | nil`
   Returns :mini:`B` if :mini:`Aᵢ = Bᵢ` for each :mini:`i = 1 .. j-1` and :mini:`Aⱼ >= Bⱼ`.

   .. code-block:: mini

      >=([1, 2, 3], [1, 2, 3]) :> [1, 2, 3]
      >=([1, 2, 3], [1, 2]) :> [1, 2]
      >=([1, 2], [1, 2, 3]) :> nil
      >=([1, 2, 3], [1, 2, 4]) :> nil
      >=([1, 3, 2], [1, 2, 3]) :> [1, 2, 3]


:mini:`meth (List: list)[Index: integer]: list::node | nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the interval of :mini:`List`.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.

   .. code-block:: mini

      let L := ["a", "b", "c", "d", "e", "f"]
      L[3] :> "c"
      L[-2] :> "e"
      L[8] :> nil


:mini:`meth (List: list)[Indices: list]: list`
   Returns a list containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.


:mini:`meth (List: list)[Indices: vector]: list`
   Returns a list containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.


:mini:`meth (List: list):bfind(Value: any): tuple[integer, integer]`
   Expects :mini:`List` is be already sorted according to :mini:`<>`. Returns :mini:`(I,  J)` where :mini:`List[I] = Value <= List[J]`.
   Note :mini:`I` can be :mini:`nil` and :mini:`J` can be :mini:`List:length + 1`.

   .. code-block:: mini

      let L := list("cake"):sort :> ["a", "c", "e", "k"]
      L:bfind("a") :> (1, 1)
      L:bfind("b") :> (nil, 2)
      L:bfind("c") :> (2, 2)
      L:bfind("z") :> (nil, 5)


:mini:`meth (List: list):bfind(Value: any, Compare: function): tuple[integer, integer]`
   Expects :mini:`List` is be already sorted according to :mini:`Compare` (which should behave like :mini:`<>`). Returns :mini:`(I,  J)` where :mini:`List[I] = Value <= List[J]`.
   Note :mini:`I` can be :mini:`nil` and :mini:`J` can be :mini:`List:length + 1`.

   .. code-block:: mini

      let L := list("cake"):sort :> ["a", "c", "e", "k"]
      L:bfind("a", <>) :> (1, 1)
      L:bfind("b", <>) :> (nil, 2)
      L:bfind("c", <>) :> (2, 2)
      L:bfind("z", <>) :> (nil, 5)


:mini:`meth (List: list):count: integer`
   Returns the length of :mini:`List`

   .. code-block:: mini

      [1, 2, 3]:count :> 3


:mini:`meth (List: list):find(Value: any): integer | nil`
   Returns the first position where :mini:`List[Position] = Value`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:find("a") :> 2
      L:find("b") :> nil


:mini:`meth (List: list):find(Value: any, Compare: function): integer | nil`
   Returns the first position where :mini:`Compare(Value,  List[Position])` returns a non-nil value.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:find("b", <) :> 1
      L:find("b", >) :> 2


:mini:`meth (List: list):first`
   Returns the first value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.


:mini:`meth (List: list):first2`
   Returns the first index and value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.


:mini:`meth (List: list):last`
   Returns the last value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.


:mini:`meth (List: list):last2`
   Returns the last index and value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.


:mini:`meth (List: list):length: integer`
   Returns the length of :mini:`List`

   .. code-block:: mini

      [1, 2, 3]:length :> 3


:mini:`meth (List: list):precount: integer`
   Returns the length of :mini:`List`

   .. code-block:: mini

      [1, 2, 3]:precount :> 3


:mini:`meth (List: list):random: any`
   Returns a random (assignable) node from :mini:`List`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:random :> "a"
      L:random :> "a"


:mini:`meth (Buffer: string::buffer):append(List: list)`
   Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`"[" + repr(V₁) + ",  " + repr(V₂) + ",  " + ... + repr(Vₙ) + "]"`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append([1, 2, 3, 4])
      B:rest :> "[1, 2, 3, 4]"


:mini:`meth (Buffer: string::buffer):append(List: list, Sep: string)`
   Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`repr(V₁) + Sep + repr(V₂) + Sep + ... + repr(Vₙ)`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append([1, 2, 3, 4], " - ")
      B:rest :> "1 - 2 - 3 - 4"


:mini:`type list::mutable < list`
   *TBD*


:mini:`meth (List: list::mutable)[Interval: integer::interval]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`,  both inclusive.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.


:mini:`meth (List: list::mutable)[Interval: integer::range]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`,  both inclusive.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.


:mini:`meth (List: list::mutable)[From: integer, To: integer]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.


:mini:`meth (List: list::mutable):cycle: list`
   Permutes :mini:`List` in place with no sub-cycles.


:mini:`meth (List: list::mutable):delete(Index: integer): any | nil`
   Removes and returns the :mini:`Index`-th value from :mini:`List`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:delete(2) :> "a"
      L:delete(-1) :> "e"
      L :> ["c", "k"]


:mini:`meth (List: list::mutable):empty: list`
   Removes all elements from :mini:`List` and returns it.


:mini:`meth (List: list::mutable):filter(Filter: function): list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.

   .. code-block:: mini

      let L := [1, 2, 3, 4, 5, 6]
      L:filter(2 | _) :> [1, 3, 5]
      L :> [2, 4, 6]


:mini:`meth (List: list::mutable):grow(Sequence: sequence, ...): list`
   Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.

   .. code-block:: mini

      let L := [1, 2, 3]
      L:grow(4 .. 6) :> [1, 2, 3, 4, 5, 6]


:mini:`meth (List: list::mutable):insert(Index: integer, Value: any): list`
   Inserts :mini:`Value` in the :mini:`Index`-th position in :mini:`List`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:insert(2, "b") :> ["c", "b", "a", "k", "e"]
      L:insert(-2, "f") :> ["c", "b", "a", "f", "k", "e"]
      L :> ["c", "b", "a", "f", "k", "e"]


:mini:`meth (List: list::mutable):order: permutation`
   Returns the ordering of the elements of :mini:`List` as a permutation,  index of first element,  index of second element,  ...,  index of last element,  when compared by :mini:`<=`.

   .. code-block:: mini

      let L := ["D", "B", "A", "C"] :> ["D", "B", "A", "C"]
      L:order :> <3 2 4 1>


:mini:`meth (List: list::mutable):order(Compare: function): permutation`
   Returns the ordering of the elements of :mini:`List` as a permutation,  index of first element,  index of second element,  ...,  index of last element,  when compared by :mini:`Compare`.

   .. code-block:: mini

      let L := ["D", "B", "A", "C"] :> ["D", "B", "A", "C"]
      L:order(>) :> <1 4 2 3>


:mini:`meth (List: list::mutable):permutations: sequence`
   Returns a sequence of all permutations of :mini:`List`,  performed in-place.


:mini:`meth (List: list::mutable):permute: list`
   .. deprecated:: 2.7.0
   
      Use :mini:`List:shuffle` instead.
   
   Permutes :mini:`List` in place.


:mini:`meth (Arg₁: list::mutable):permute(Arg₂: list)`
   *TBD*


:mini:`meth (Arg₁: list::mutable):permute(Arg₂: permutation)`
   *TBD*


:mini:`meth (List: list::mutable):pop: any | nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth (List: list::mutable):pop(Fn: function): any | nil`
   Removes and returns the first value where :mini:`Fn(Value)` is not :mini:`nil`.

   .. code-block:: mini

      let L := list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      L:pop(3 | _) :> 3
      L :> [1, 2, 4, 5, 6, 7, 8, 9, 10]


:mini:`meth (List: list::mutable):pull: any | nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth (List: list::mutable):pull(Fn: function): any | nil`
   Removes and returns the last value where :mini:`Fn(Value)` is not :mini:`nil`.

   .. code-block:: mini

      let L := list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      L:pull(3 | _) :> 9
      L :> [1, 2, 3, 4, 5, 6, 7, 8, 10]


:mini:`meth (List: list::mutable):push(Values: any, ...): list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.


:mini:`meth (List: list::mutable):put(Values: any, ...): list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.


:mini:`meth (List: list::mutable):remove(Filter: function): list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` doesn't return non-:mini:`nil` and returns those values in a new list.

   .. code-block:: mini

      let L := [1, 2, 3, 4, 5, 6]
      L:remove(2 | _) :> [2, 4, 6]
      L :> [1, 3, 5]


:mini:`meth (List: list::mutable):reverse: list`
   Reverses :mini:`List` in-place and returns it.


:mini:`meth (List: list::mutable):shuffle: list`
   Shuffles :mini:`List` in place.


:mini:`meth (List: list::mutable):sort: List`
   Sorts :mini:`List` in-place using :mini:`<=` and returns it.


:mini:`meth (List: list::mutable):sort(Compare: function): List`
   Sorts :mini:`List` in-place using :mini:`Compare` and returns it.


:mini:`meth (List: list::mutable):sort(Compare: method): List`
   Sorts :mini:`List` in-place using :mini:`Compare` and returns it.


:mini:`meth (List: list::mutable):splice: list | nil`
   Removes all elements from :mini:`List`. Returns the removed elements as a new list.


:mini:`meth (List: list::mutable):splice(Index: integer, Count: integer): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.


:mini:`meth (List: list::mutable):splice(Index: integer, Count: integer, Source: list::mutable): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`,  then inserts the elements from :mini:`Source`,  leaving :mini:`Source` empty. Returns the removed elements as a new list.


:mini:`meth (List: list::mutable):splice(Index: integer, Source: list::mutable): nil`
   Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`,  leaving :mini:`Source` empty.


:mini:`meth (List: list::mutable):take(Source: list::mutable): nil`
   Appends the elements from :mini:`Source` onto :mini:`List`,  leaving :mini:`Source` empty.


:mini:`type list::node`
   A node in a :mini:`list`.
   Dereferencing a :mini:`list::node::const` returns the corresponding value from the :mini:`list`.


:mini:`type list::node::mutable`
   A node in a :mini:`list`.
   Dereferencing a :mini:`list::node` returns the corresponding value from the :mini:`list`.
   Assigning to a :mini:`list::node` updates the corresponding value in the :mini:`list`.


:mini:`type list::node::mutable < list::node`
   A node in a :mini:`list`.
   Dereferencing a :mini:`list::node` returns the corresponding value from the :mini:`list`.
   Assigning to a :mini:`list::node` updates the corresponding value in the :mini:`list`.


:mini:`type list::slice`
   A sub-list.


:mini:`meth list(Arg₁: names)`
   *TBD*


:mini:`meth (Visitor: visitor):const(List: list::mutable): list::const`
   Returns a new constant list containing copies of the elements of :mini:`List` created using :mini:`Copy`.


:mini:`meth (Visitor: visitor):copy(List: list): list`
   Returns a new list containing copies of the elements of :mini:`List` created using :mini:`Copy`.


:mini:`meth (Visitor: visitor):visit(List: list): list`
   Returns a new list containing copies of the elements of :mini:`List` created using :mini:`Copy`.


