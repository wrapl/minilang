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
   Returns a new map with the specified keys and values.

   .. code-block:: mini

      map(A is 1, B is 2, C is 3) :> {"A" is 1, "B" is 2, "C" is 3}


:mini:`meth map(Sequence: sequence, ...): map`
   Returns a map of all the key and value pairs produced by :mini:`Sequence`.

   .. code-block:: mini

      map("cake") :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}


:mini:`meth map(): map`
   Returns a new map.

   .. code-block:: mini

      map() :> {}


:mini:`meth (Map₁: map) * (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are also in :mini:`Map₂`. The values are chosen from :mini:`Map₂`.

   .. code-block:: mini

      let A := map(swap("banana")) :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread")) :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A * B :> {"b" is 1, "a" is 4}


:mini:`meth (Map₁: map) + (Map₂: map): map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.
   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.

   .. code-block:: mini

      let A := map(swap("banana")) :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread")) :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A + B :> {"b" is 1, "a" is 4, "n" is 5, "r" is 2, "e" is 3, "d" is 5}


:mini:`meth (Map₁: map) / (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are not in :mini:`Map₂`.

   .. code-block:: mini

      let A := map(swap("banana")) :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread")) :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A / B :> {"n" is 5}


:mini:`meth (Map: map) :: (Key: string): mapnode`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M::A :> 1
      M::D :> nil
      M::A := 10 :> 10
      M::D := 20 :> 20
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Map: map):count: integer`
   Returns the number of entries in :mini:`Map`.

   .. code-block:: mini

      {"A" is 1, "B" is 2, "C" is 3}:count :> 3


:mini:`meth (Map: map):delete(Key: any): any | nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:delete("A") :> 1
      M:delete("D") :> nil
      M :> {"B" is 2, "C" is 3}


:mini:`meth (Map: map):empty: map`
   Deletes all keys and values from :mini:`Map` and returns it.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3} :> {"A" is 1, "B" is 2, "C" is 3}
      M:empty :> {}


:mini:`meth (Map: map):grow(Sequence: sequence, ...): map`
   Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Map` and returns :mini:`Map`.

   .. code-block:: mini

      map( :> error("ParseError", "Expected <expression> not <end of input>")


:mini:`meth (Map: map):insert(Key: any, Value: any): any | nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
   Returns the previous value associated with :mini:`Key` if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:insert("A", 10) :> 1
      M:insert("D", 20) :> nil
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Arg₁: map):lru`
   *TBD*


:mini:`meth (Arg₁: map):lru(Arg₂: boolean)`
   *TBD*


:mini:`meth (Map: map):missing(Key: any): some | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`some` and returns :mini:`some`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:missing("A") :> nil
      M:missing("D") :> some
      M :> {"A" is 1, "B" is 2, "C" is 3, "D"}


:mini:`meth (Map: map):missing(Key: any, Fn: function): any | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Fn(Key)` and returns :mini:`some`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:missing("A", fun(Key) Key:code) :> nil
      M:missing("D", fun(Key) Key:code) :> some
      M :> {"A" is 1, "B" is 2, "C" is 3, "D" is 68}


:mini:`meth (Arg₁: map):pop`
   *TBD*


:mini:`meth (Arg₁: map):pop2`
   *TBD*


:mini:`meth (Arg₁: map):pull`
   *TBD*


:mini:`meth (Arg₁: map):pull2`
   *TBD*


:mini:`meth (Map: map):size: integer`
   Returns the number of entries in :mini:`Map`.

   .. code-block:: mini

      {"A" is 1, "B" is 2, "C" is 3}:size :> 3


:mini:`meth (Map: map):sort: Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Keyᵢ < Keyⱼ` and returns :mini:`Map`.

   .. code-block:: mini

      let M := map(swap("cake")) :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort :> {"a" is 2, "c" is 1, "e" is 4, "k" is 3}


:mini:`meth (Map: map):sort(Cmp: function): Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Keyᵢ,  Keyⱼ)` and returns :mini:`Map`

   .. code-block:: mini

      let M := map(swap("cake")) :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort(>) :> {"k" is 3, "e" is 4, "c" is 1, "a" is 2}


:mini:`meth (Map: map):sort2(Cmp: function): Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Keyᵢ,  Keyⱼ,  Valueᵢ,  Valueⱼ)` and returns :mini:`Map`

   .. code-block:: mini

      let M := map(swap("cake")) :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort(fun(K1, K2, V1, V2) V1 < V2) :> {"e" is 4, "k" is 3, "a" is 2, "c" is 1}


:mini:`meth (Map: map)[Key: any]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M["A"] :> 1
      M["D"] :> nil
      M["A"] := 10 :> 10
      M["D"] := 20 :> 20
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Map: map)[Key: any, Fn: function]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Fn(Key)` is called and the result inserted into :mini:`Map`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M["A", fun(Key) Key:code] :> 1
      M["D", fun(Key) Key:code] :> 68
      M :> {"A" is 1, "B" is 2, "C" is 3, "D" is 68}


:mini:`meth (Arg₁: string::buffer):append(Arg₂: map)`
   *TBD*


:mini:`meth (Map: string::buffer):append(Sep: map, Conn: string, Arg₄: string): string`
   Returns a string containing the entries of :mini:`Map` with :mini:`Conn` between keys and values and :mini:`Sep` between entries.


.. _type-map-node:

:mini:`type map::node`
   A node in a :mini:`map`.
   Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.
   Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.


