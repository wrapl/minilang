.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

map
===

.. _type-map:

:mini:`type map < sequence`
   A map of key-value pairs.

   Keys can be of any type supporting hashing and comparison.

   Insert order is preserved.



:mini:`meth map(Key₁ is Value₁, ...): map`
   *TBD*


:mini:`meth map(Sequence: sequence, ...): map`
   Returns a map of all the key and value pairs produced by :mini:`Sequence`.



:mini:`meth map(): map`
   *TBD*


:mini:`meth (Map₁: map) * (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are also in :mini:`Map₂`. The values are chosen from :mini:`Map₁`.



:mini:`meth (Map₁: map) + (Map₂: map): map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.

   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.



:mini:`meth (Map₁: map) / (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are not in :mini:`Map₂`.



:mini:`meth (Map: map) :: (Key: string): mapnode`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.



:mini:`meth (Map: map):count: integer`
   Returns the number of entries in :mini:`Map`.



:mini:`meth (Map: map):delete(Key: any): any | nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any,  otherwise :mini:`nil`.



:mini:`meth (Map: map):empty: map`
   Deletes all keys and values from :mini:`Map` and returns it.



:mini:`meth (Map: map):grow(Sequence: sequence, ...): map`
   Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Map` and returns :mini:`Map`.



:mini:`meth (Map: map):insert(Key: any, Value: any): any | nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any,  otherwise :mini:`nil`.



:mini:`meth (Map: map):missing(Key: any, Function: function): any | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Function()` and returns :mini:`some`.



:mini:`meth (Map: map):missing(Key: any): some | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`some` and returns :mini:`some`.



:mini:`meth (Map: map):size: integer`
   Returns the number of entries in :mini:`Map`.



:mini:`meth (Map: map):sort(Compare: function): Map`
   *TBD*


:mini:`meth (Map: map):sort: Map`
   *TBD*


:mini:`meth (Map: map):sort2(Compare: function): Map`
   *TBD*


:mini:`meth (Map: map)[Key: any]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.



:mini:`meth (Map: map)[Key: any, Default: function]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.



:mini:`meth (Arg₁: string::buffer):append(Arg₂: map)`
   *TBD*


:mini:`meth (Map: string::buffer):append(Seperator: map, Connector: string, Arg₄: string): string`
   Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.



.. _type-map-node:

:mini:`type map::node`
   A node in a :mini:`map`.

   Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.

   Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.



