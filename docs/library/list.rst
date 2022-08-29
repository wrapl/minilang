.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

list
====

.. rst-class:: mini-api

:mini:`meth (Copy: copy):const(List: list): list::const`
   Returns a new constant list containing copies of the elements of :mini:`List` created using :mini:`Copy`.


:mini:`meth (Copy: copy):copy(List: list::const): list`
   Returns a new list containing copies of the elements of :mini:`List` created using :mini:`Copy`.


.. _type-list:

:mini:`type list < list::const`
   A list of elements.


:mini:`meth list(): list`
   Returns an empty list.

   .. code-block:: mini

      list() :> []


:mini:`meth list(Tuple: tuple): list`
   Returns a list containing the values in :mini:`Tuple`.

   .. code-block:: mini

      list((1, 2, 3)) :> [1, 2, 3]


:mini:`meth list(Sequence: sequence, ...): list`
   Returns a list of all of the values produced by :mini:`Sequence`.

   .. code-block:: mini

      list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


:mini:`meth (List: list)[Range: integer::range]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`Range:start` and ending at :mini:`Range:limit`,  both inclusive.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.


:mini:`meth (List: list)[From: integer, To: integer]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.


:mini:`meth (List: list):cycle: list`
   Permutes :mini:`List` in place with no sub-cycles.


:mini:`meth (List: list):delete(Index: integer): any | nil`
   Removes and returns the :mini:`Index`-th value from :mini:`List`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:delete(2) :> "a"
      L:delete(-1) :> "e"
      L :> ["c", "k"]


:mini:`meth (List: list):empty: list`
   Removes all elements from :mini:`List` and returns it.


:mini:`meth (List: list):filter(Filter: function): list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.

   .. code-block:: mini

      let L := [1, 2, 3, 4, 5, 6]
      L:filter(2 | _) :> [1, 3, 5]
      L :> [2, 4, 6]


:mini:`meth (List: list):grow(Sequence: sequence, ...): list`
   Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.

   .. code-block:: mini

      let L := [1, 2, 3]
      L:grow(4 .. 6) :> [1, 2, 3, 4, 5, 6]


:mini:`meth (List: list):permutations: sequence`
   Returns a sequence of all permutations of :mini:`List`,  performed in-place.


:mini:`meth (List: list):permute: list`
   .. deprecated:: 2.7.0
   
      Use :mini:`List:shuffle` instead.
   
   Permutes :mini:`List` in place.


:mini:`meth (List: list):pop: any | nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth (List: list):pop(Fn: function): any | nil`
   Removes and returns the first value where :mini:`Fn(Value)` is not :mini:`nil`.

   .. code-block:: mini

      let L := list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      L:pop(3 | _) :> 3
      L :> [1, 2, 4, 5, 6, 7, 8, 9, 10]


:mini:`meth (List: list):pull: any | nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth (List: list):pull(Fn: function): any | nil`
   Removes and returns the last value where :mini:`Fn(Value)` is not :mini:`nil`.

   .. code-block:: mini

      let L := list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      L:pull(3 | _) :> 9
      L :> [1, 2, 3, 4, 5, 6, 7, 8, 10]


:mini:`meth (List: list):push(Values: any, ...): list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.


:mini:`meth (List: list):put(Values: any, ...): list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.


:mini:`meth (List: list):reverse: list`
   Reverses :mini:`List` in-place and returns it.


:mini:`meth (List: list):shuffle: list`
   Shuffles :mini:`List` in place.


:mini:`meth (List: list):sort: List`
   Sorts :mini:`List` in-place using :mini:`<` and returns it.


:mini:`meth (List: list):sort(Compare: function): List`
   Sorts :mini:`List` in-place using :mini:`Compare` and returns it.


:mini:`meth (List: list):splice: list | nil`
   Removes all elements from :mini:`List`. Returns the removed elements as a new list.


:mini:`meth (List: list):splice(Index: integer, Count: integer): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.


:mini:`meth (List: list):splice(Index: integer, Count: integer, Source: list): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`,  then inserts the elements from :mini:`Source`,  leaving :mini:`Source` empty. Returns the removed elements as a new list.


:mini:`meth (List: list):splice(Index: integer, Source: list): nil`
   Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`,  leaving :mini:`Source` empty.


.. _type-list-const:

:mini:`type list::const < sequence`
   *TBD*


:mini:`meth (List₁: list::const) + (List₂: list::const): list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.


:mini:`meth (List: list::const)[Index: integer]: list::node | nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.
   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.

   .. code-block:: mini

      let L := ["a", "b", "c", "d", "e", "f"]
      L[3] :> "c"
      L[-2] :> "e"
      L[8] :> nil


:mini:`meth (List: list::const)[Indices: list]: list`
   Returns a list containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.


:mini:`meth (List: list::const):count: integer`
   Returns the length of :mini:`List`

   .. code-block:: mini

      [1, 2, 3]:count :> 3


:mini:`meth (List: list::const):find(Value: any): integer | nil`
   Returns the first position where :mini:`List[Position] = Value`.


:mini:`meth (List: list::const):length: integer`
   Returns the length of :mini:`List`

   .. code-block:: mini

      [1, 2, 3]:length :> 3


:mini:`meth (List: list::const):random: any`
   Returns a random (assignable) node from :mini:`List`.

   .. code-block:: mini

      let L := list("cake") :> ["c", "a", "k", "e"]
      L:random :> "k"
      L:random :> "c"


:mini:`meth (Buffer: string::buffer):append(List: list::const)`
   Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`"[" + repr(V₁) + ",  " + repr(V₂) + ",  " + ... + repr(Vₙ) + "]"`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append([1, 2, 3, 4])
      B:rest :> "[1, 2, 3, 4]"


:mini:`meth (Buffer: string::buffer):append(List: list::const, Sep: string)`
   Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`repr(V₁) + Sep + repr(V₂) + Sep + ... + repr(Vₙ)`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append([1, 2, 3, 4], " - ")
      B:rest :> "1 - 2 - 3 - 4"


.. _type-list-node:

:mini:`type list::node < list::node::const`
   A node in a :mini:`list`.
   Dereferencing a :mini:`list::node` returns the corresponding value from the :mini:`list`.
   Assigning to a :mini:`list::node` updates the corresponding value in the :mini:`list`.


.. _type-list-node-const:

:mini:`type list::node::const`
   A node in a :mini:`list`.
   Dereferencing a :mini:`list::node::const` returns the corresponding value from the :mini:`list`.


.. _type-list-slice:

:mini:`type list::slice`
   A slice of a list.


