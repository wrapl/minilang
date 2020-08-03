list
====

.. include:: <isonum.txt>

**method** :mini:`list(iteratable Iteratable)` |rarr| :mini:`list`
   Returns a list of all of the values produced by :mini:`Iteratable`.

   *Defined at line 309 in src/ml_iterfns.c*

**type** :mini:`list`
   A list of elements.

   :Parents: :mini:`function`

   *Defined at line 3008 in src/ml_types.c*

**type** :mini:`listnode`
   A node in a :mini:`list`.

   Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.

   Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.

   *Defined at line 3022 in src/ml_types.c*

**method** :mini:`list List:size` |rarr| :mini:`integer`
   Returns the length of :mini:`List`

   *Defined at line 3144 in src/ml_types.c*

**method** :mini:`list List:length` |rarr| :mini:`integer`
   Returns the length of :mini:`List`

   *Defined at line 3153 in src/ml_types.c*

**method** :mini:`list List:filter(function Filter)` |rarr| :mini:`list`
   Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.

   *Defined at line 3162 in src/ml_types.c*

**method** :mini:`list List[integer Index]` |rarr| :mini:`listnode` or :mini:`nil`
   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.

   *Defined at line 3223 in src/ml_types.c*

**type** :mini:`listslice`
   A slice of a list.

   *Defined at line 3270 in src/ml_types.c*

**method** :mini:`list List[integer From, integer To]` |rarr| :mini:`listslice`
   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).

   Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.

   *Defined at line 3277 in src/ml_types.c*

**method** :mini:`stringbuffer::append(stringbuffer Arg₁, list Arg₂)`
   *Defined at line 3312 in src/ml_types.c*

**method** :mini:`list List:push(any Values...)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.

   *Defined at line 3372 in src/ml_types.c*

**method** :mini:`list List:put(any Values...)` |rarr| :mini:`list`
   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.

   *Defined at line 3383 in src/ml_types.c*

**method** :mini:`list List:pop` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.

   *Defined at line 3394 in src/ml_types.c*

**method** :mini:`list List:pull` |rarr| :mini:`any` or :mini:`nil`
   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.

   *Defined at line 3402 in src/ml_types.c*

**method** :mini:`list List:copy` |rarr| :mini:`list`
   Returns a (shallow) copy of :mini:`List`.

   *Defined at line 3410 in src/ml_types.c*

**method** :mini:`list List₁ + list List₂` |rarr| :mini:`list`
   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`.

   *Defined at line 3420 in src/ml_types.c*

**method** :mini:`string(list List)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` surrounded by :mini:`[`, :mini:`]` and seperated by :mini:`,`.

   *Defined at line 3432 in src/ml_types.c*

**method** :mini:`string(list List, string Seperator)` |rarr| :mini:`string`
   Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.

   *Defined at line 3453 in src/ml_types.c*

