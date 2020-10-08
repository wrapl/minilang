map
===

.. include:: <isonum.txt>

:mini:`meth map(Iteratable: iteratable)` |rarr| :mini:`map`
   Returns a map of all the key and value pairs produced by :mini:`Iteratable`.

   *Defined at line 575 in src/ml_iterfns.c*

:mini:`map`
   A map of key-value pairs.

   Keys can be of any type supporting hashing and comparison.

   Insert order is preserved.

   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 3850 in src/ml_types.c*

:mini:`mapnode`
   A node in a :mini:`map`.

   Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.

   Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.

   *Defined at line 3866 in src/ml_types.c*

:mini:`meth :size(Map: map)` |rarr| :mini:`integer`
   Returns the number of entries in :mini:`Map`.

   *Defined at line 4073 in src/ml_types.c*

:mini:`meth [](Map: map, Key: any)` |rarr| :mini:`mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then the reference withh return :mini:`nil` when dereferenced and will insert :mini:`Key` into :mini:`Map` when assigned.

   *Defined at line 4138 in src/ml_types.c*

:mini:`meth [](Map: map, Key: any, Default: function)` |rarr| :mini:`mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.

   *Defined at line 4169 in src/ml_types.c*

:mini:`meth ::(Map: map, Key: string)` |rarr| :mini:`mapnode`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.

   *Defined at line 4194 in src/ml_types.c*

:mini:`meth :insert(Map: map, Key: any, Value: any)` |rarr| :mini:`any` or :mini:`nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.

   *Defined at line 4210 in src/ml_types.c*

:mini:`meth :delete(Map: map, Key: any)` |rarr| :mini:`any` or :mini:`nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.

   *Defined at line 4224 in src/ml_types.c*

:mini:`meth :missing(Map: map, Key: any)` |rarr| :mini:`any` or :mini:`nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.

   *Defined at line 4242 in src/ml_types.c*

:mini:`meth stringbuffer::append(Arg₁: stringbuffer, Arg₂: map)`
   *Defined at line 4278 in src/ml_types.c*

:mini:`meth +(Map₁: map, Map₂: map)` |rarr| :mini:`map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.

   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.

   *Defined at line 4356 in src/ml_types.c*

:mini:`meth string(Map: map)` |rarr| :mini:`string`
   Returns a string containing the entries of :mini:`Map` surrounded by :mini:`{`, :mini:`}` with :mini:`is` between keys and values and :mini:`,` between entries.

   *Defined at line 4390 in src/ml_types.c*

:mini:`meth string(Map: map, Seperator: string, Connector: string)` |rarr| :mini:`string`
   Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.

   *Defined at line 4409 in src/ml_types.c*

