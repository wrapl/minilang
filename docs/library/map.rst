map
===

.. include:: <isonum.txt>

**type** :mini:`map`
   A map of key-value pairs.

   Keys can be of any type supporting hashing and comparison.

   Insert order is preserved.

   :Parents: :mini:`function`


**type** :mini:`mapnode`
   A node in a :mini:`map`.

   Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.

   Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.


**method** :mini:`map Map:size` |rarr| :mini:`integer`
   Returns the number of entries in :mini:`Map`.


**method** :mini:`map Map[any Key]` |rarr| :mini:`mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then the reference withh return :mini:`nil` when dereferenced and will insert :mini:`Key` into :mini:`Map` when assigned.


**method** :mini:`map Map[any Key, function Default]` |rarr| :mini:`mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.


**method** :mini:`map Map :: string Key` |rarr| :mini:`mapnode`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.


**method** :mini:`map Map:insert(any Key, any Value)` |rarr| :mini:`any` or :mini:`nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.


**method** :mini:`map Map:delete(any Key)` |rarr| :mini:`any` or :mini:`nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.


**method** :mini:`map Map₁ + map Map₂` |rarr| :mini:`map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.

   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.


**method** :mini:`string(map Map)` |rarr| :mini:`string`
   Returns a string containing the entries of :mini:`Map` surrounded by :mini:`{`, :mini:`}` with :mini:`is` between keys and values and :mini:`,` between entries.


**method** :mini:`string(map Map, string Seperator, string Connector)` |rarr| :mini:`string`
   Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.


