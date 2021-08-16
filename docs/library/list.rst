list
====

.. include:: <isonum.txt>

:mini:`type list < iteratable`
   A list of elements.


:mini:`type listnode`
   A node in a :mini:`list`.

   Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.

   Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.


:mini:`meth list()`

:mini:`meth list(Arg₁: tuple)`

:mini:`meth list(Iteratable: iteratable)` |rarr| :mini:`list`
   Returns a list of all of the values produced by :mini:`Iteratable`.


:mini:`meth :count(List: list)` |rarr| :mini:`integer`
   Returns the length of :mini:`List`


:mini:`meth :length(List: list)` |rarr| :mini:`integer`
   Returns the length of :mini:`List`


:mini:`meth :filter(List: list, Filter: function)` |rarr| :mini:`list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.


:mini:`meth (List: list)[Index: integer]` |rarr| :mini:`listnode` or :mini:`nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.


:mini:`type listslice`
   A slice of a list.


:mini:`meth (List: list)[From: integer, To: integer]` |rarr| :mini:`listslice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: list)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: list, Arg₃: string)`

:mini:`meth :push(List: list, Values...: any)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.


:mini:`meth :put(List: list, Values...: any)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.


:mini:`meth :pop(List: list)` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth :pull(List: list)` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.


:mini:`meth :copy(List: list)` |rarr| :mini:`list`
   Returns a (shallow) copy of :mini:`List`.


:mini:`meth +(List₁: list, List₂: list)` |rarr| :mini:`list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.


:mini:`meth :splice(List: list, Index: integer, Count: integer)` |rarr| :mini:`list` or :mini:`nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.


:mini:`meth :splice(List: list, Index: integer, Count: integer, Source: list)` |rarr| :mini:`list` or :mini:`nil`
   Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`, then inserts the elements from :mini:`Source`, leaving :mini:`Source` empty. Returns the removed elements as a new list.


:mini:`meth :splice(List: list, Index: integer, Source: list)` |rarr| :mini:`nil`
   Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`, leaving :mini:`Source` empty.


:mini:`meth string(List: list)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` surrounded by :mini:`"["`, :mini:`"]"` and seperated by :mini:`", "`.


:mini:`meth string(List: list, Seperator: string)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.


:mini:`meth :reverse(Arg₁: list)`

:mini:`meth :sort(List: list)` |rarr| :mini:`List`

:mini:`meth :sort(List: list, Compare: function)` |rarr| :mini:`List`

