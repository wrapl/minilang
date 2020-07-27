list
====

.. include:: <isonum.txt>

.. topic:: **type** :mini:`list`

   A list of elements. 

   :Parents: :mini:`function`

.. topic:: **type** :mini:`listnode`

   A node in a :mini:`list`. Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`. Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`. 

   :Parents: 

.. topic:: **method** :mini:`list List:size` |rarr| :mini:`integer`

   Returns the length of :mini:`List` 

   :list List: 

.. topic:: **method** :mini:`list List:length` |rarr| :mini:`integer`

   Returns the length of :mini:`List` 

   :list List: 

.. topic:: **method** :mini:`list List:drop(function Filter)` |rarr| :mini:`list`

   Removes values :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list. 

   :list List: 
   :function Filter: 

.. topic:: **method** :mini:`list List[integer Index]` |rarr| :mini:`listnode` or :mini:`nil`

   Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`. Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node. 

   :list List: 
   :integer Index: 

.. topic:: **type** :mini:`listslice`

   A slice of a list. 

   :Parents: 

.. topic:: **method** :mini:`list List[integer From, integer To]` |rarr| :mini:`listslice`

   Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive). Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node. 

   :list List: 
   :integer From: 
   :integer To: 

.. topic:: **method** :mini:`list List:push(any Values...)` |rarr| :mini:`list`

   Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`. 

   :list List: 
   :any Values...: 

.. topic:: **method** :mini:`list List:put(any Values...)` |rarr| :mini:`list`

   Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`. 

   :list List: 
   :any Values...: 

.. topic:: **method** :mini:`list List:pop` |rarr| :mini:`any` or :mini:`nil`

   Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty. 

   :list List: 

.. topic:: **method** :mini:`list List:pull` |rarr| :mini:`any` or :mini:`nil`

   Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty. 

   :list List: 

.. topic:: **method** :mini:`list List:copy` |rarr| :mini:`list`

   Returns a (shallow) copy of :mini:`List`. 

   :list List: 

.. topic:: **method** :mini:`list List₁ + list List₂` |rarr| :mini:`list`

   Returns a new list with the elements of :mini:`List₁` followed by the elements of :mini:`List₂`. 

   :list List₁: 
   :list List₂: 

.. topic:: **method** :mini:`string(list List)` |rarr| :mini:`string`

   Returns a string containing the elements of :mini:`List` surrounded by :mini:`[`, :mini:`]` and seperated by :mini:`,`. 

   :list List: 

.. topic:: **method** :mini:`string(list List, string Seperator)` |rarr| :mini:`string`

   Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`. 

   :list List: 
   :string Seperator: 

