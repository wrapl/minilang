.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

list
====

.. _type-list:

:mini:`type list < sequence`
   A list of elements.



:mini:`meth list(Tuple: tuple): list`
   Returns a list containing the values in :mini:`Tuple`.



:mini:`meth list(Sequence: sequence, ...): list`
   Returns a list of all of the values produced by :mini:`Sequence`.



:mini:`meth list(): list`
   Returns an empty list.



:mini:`meth (List₁: list) + (List₂: list): list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.



:mini:`meth (List: list):copy: list`
   Returns a (shallow) copy of :mini:`List`.



:mini:`meth (List: list):count: integer`
   Returns the length of :mini:`List`



:mini:`meth (List: list):filter(Filter: function): list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.



:mini:`meth (List: list):find(Value: any): integer | nil`
   Returns the first position where :mini:`List[Position] = Value`.



:mini:`meth (List: list):grow(Sequence: sequence, ...): list`
   Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.



:mini:`meth (List: list):length: integer`
   Returns the length of :mini:`List`



:mini:`meth (List: list):pop: any | nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.



:mini:`meth (List: list):pull: any | nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.



:mini:`meth (List: list):push(Values...: any): list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.



:mini:`meth (List: list):put(Values...: any): list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.



:mini:`meth (List: list):reverse: list`
   Reverses :mini:`List` in-place and returns it.



:mini:`meth (List: list):sort(Compare: function): List`
   Sorts :mini:`List` in-place using :mini:`Compare` and returns it.



:mini:`meth (List: list):sort: List`
   Sorts :mini:`List` in-place using :mini:`<` and returns it.



:mini:`meth (List: list):splice(Index: integer, Source: list): nil`
   Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`,  leaving :mini:`Source` empty.



:mini:`meth (List: list):splice: list | nil`
   Removes all elements from :mini:`List`. Returns the removed elements as a new list.



:mini:`meth (List: list):splice(Index: integer, Count: integer): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.



:mini:`meth (List: list):splice(Index: integer, Count: integer, Source: list): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`,  then inserts the elements from :mini:`Source`,  leaving :mini:`Source` empty. Returns the removed elements as a new list.



:mini:`meth (List: list)[Range: integer::range]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`Range:start` and ending at :mini:`Range:limit`,  both inclusive.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.



:mini:`meth (List: list)[Indices: list]: list`
   Returns a list containing the :mini:`List[Indices[1]]`,  :mini:`List[Indices[2]]`,  etc.



:mini:`meth (List: list)[From: integer, To: integer]: list::slice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.



:mini:`meth (List: list)[Index: integer]: listnode | nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list,  with :mini:`-1` returning the last node.



:mini:`meth (Arg₁: string::buffer):append(Arg₂: list, Arg₃: string)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: list)`
   *TBD*


.. _type-list-node:

:mini:`type list::node`
   A node in a :mini:`list`.

   Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.

   Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.



.. _type-list-slice:

:mini:`type list::slice`
   A slice of a list.



