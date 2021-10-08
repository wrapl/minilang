list
====

:mini:`type list < sequence`
   A list of elements.


:mini:`type list::node`
   A node in a :mini:`list`.

   Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.

   Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.


:mini:`meth list(): list`
   Returns an empty list.


:mini:`meth list(Tuple: tuple): list`
   Returns a list containing the values in :mini:`Tuple`.


:mini:`meth list(Sequence: sequence): list`
   Returns a list of all of the values produced by :mini:`Sequence`.


:mini:`meth :grow(List: list, Sequence: sequence): list`
   Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.


:mini:`meth :count(List: list): integer`
   Returns the length of :mini:`List`


:mini:`meth :length(List: list): integer`
   Returns the length of :mini:`List`


:mini:`meth :filter(List: list, Filter: function): list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.


:mini:`meth (List: list)[Index: integer]: listnode | nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.


:mini:`type list::slice`
   A slice of a list.


:mini:`meth (List: list)[From: integer, To: integer]: listslice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.


:mini:`meth :append(Arg₁: string::buffer, Arg₂: list)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: list, Arg₃: string)`
   *TBD*

:mini:`meth :push(List: list, Values...: any, ...): list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.


:mini:`meth :put(List: list, Values...: any, ...): list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.


:mini:`meth :pop(List: list): any | nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth :pull(List: list): any | nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth :copy(List: list): list`
   Returns a (shallow) copy of :mini:`List`.


:mini:`meth (List₁: list) + (List₂: list): list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.


:mini:`meth :splice(List: list, Index: integer, Count: integer): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.


:mini:`meth :splice(List: list, Index: integer, Count: integer, Source: list): list | nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`, then inserts the elements from :mini:`Source`, leaving :mini:`Source` empty. Returns the removed elements as a new list.


:mini:`meth :splice(List: list, Index: integer, Source: list): nil`
   Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`, leaving :mini:`Source` empty.


:mini:`meth string(List: list): string`
   Returns a string containing the elements of :mini:`List` surrounded by :mini:`"["`, :mini:`"]"` and seperated by :mini:`", "`.


:mini:`meth string(List: list, Seperator: string): string`
   Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.


:mini:`meth :reverse(List: list): list`
   Reverses :mini:`List` in-place and returns it.


:mini:`meth :sort(List: list): List`
   Sorts :mini:`List` in-place using :mini:`<` and returns it.


:mini:`meth :sort(List: list, Compare: function): List`
   Sorts :mini:`List` in-place using :mini:`Compare` and returns it.


