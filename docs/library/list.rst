list
====

.. include:: <isonum.txt>

:mini:`meth list(Iteratable: iteratable)` |rarr| :mini:`list`
   Returns a list of all of the values produced by :mini:`Iteratable`.

   *Defined at line 463 in src/ml_iterfns.c*

:mini:`list`
   A list of elements.

   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 3083 in src/ml_types.c*

:mini:`listnode`
   A node in a :mini:`list`.

   Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.

   Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.

   *Defined at line 3097 in src/ml_types.c*

:mini:`meth :size(List: list)` |rarr| :mini:`integer`
   Returns the length of :mini:`List`

   *Defined at line 3230 in src/ml_types.c*

:mini:`meth :length(List: list)` |rarr| :mini:`integer`
   Returns the length of :mini:`List`

   *Defined at line 3239 in src/ml_types.c*

:mini:`meth :filter(List: list, Filter: function)` |rarr| :mini:`list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.

   *Defined at line 3248 in src/ml_types.c*

:mini:`meth [](List: list, Index: integer)` |rarr| :mini:`listnode` or :mini:`nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.

   *Defined at line 3299 in src/ml_types.c*

:mini:`listslice`
   A slice of a list.

   *Defined at line 3346 in src/ml_types.c*

:mini:`meth [](List: list, From: integer, To: integer)` |rarr| :mini:`listslice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.

   *Defined at line 3353 in src/ml_types.c*

:mini:`meth stringbuffer::append(Arg₁: stringbuffer, Arg₂: list)`
   *Defined at line 3388 in src/ml_types.c*

:mini:`meth :push(List: list, Values...: any)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.

   *Defined at line 3448 in src/ml_types.c*

:mini:`meth :put(List: list, Values...: any)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.

   *Defined at line 3459 in src/ml_types.c*

:mini:`meth :pop(List: list)` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.

   *Defined at line 3470 in src/ml_types.c*

:mini:`meth :pull(List: list)` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.

   *Defined at line 3478 in src/ml_types.c*

:mini:`meth :copy(List: list)` |rarr| :mini:`list`
   Returns a (shallow) copy of :mini:`List`.

   *Defined at line 3486 in src/ml_types.c*

:mini:`meth +(List₁: list, List₂: list)` |rarr| :mini:`list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.

   *Defined at line 3496 in src/ml_types.c*

:mini:`meth string(List: list)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` surrounded by :mini:`[`, :mini:`]` and seperated by :mini:`,`.

   *Defined at line 3508 in src/ml_types.c*

:mini:`meth string(List: list, Seperator: string)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.

   *Defined at line 3529 in src/ml_types.c*

:mini:`meth :sort(List: list)` |rarr| :mini:`List`
   *Defined at line 3609 in src/ml_types.c*

:mini:`meth :sort(List: list, Compare: function)` |rarr| :mini:`List`
   *Defined at line 3616 in src/ml_types.c*

