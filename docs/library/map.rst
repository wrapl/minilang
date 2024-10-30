.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

map
===

.. rst-class:: mini-api

:mini:`meth (Key: any):in(Map: map): any | nil`
   Returns :mini:`Key` if it is in :mini:`Map`,  otherwise return :mini:`nil`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      "A" in M :> "A"
      "D" in M :> nil


:mini:`type map < sequence`
   A map of key-value pairs.
   Keys can be of any type supporting hashing and comparison.
   By default,  iterating over a map generates the key-value pairs in the order they were inserted,  however this ordering can be changed.


:mini:`meth map(Key₁ is Value₁, ...): map`
   Returns a new map with the specified keys and values.

   .. code-block:: mini

      map(A is 1, B is 2, C is 3)
      :> {"A" is 1, "B" is 2, "C" is 3}


:mini:`meth map(Sequence: sequence, ...): map`
   Returns a map of all the key and value pairs produced by :mini:`Sequence`.

   .. code-block:: mini

      map("cake") :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}


:mini:`meth map(): map`
   Returns a new map.

   .. code-block:: mini

      map() :> {}


:mini:`fun map::join(Map₁, : map, ..., Fn: function): map`
   Returns a new map containing the union of the keys of :mini:`Mapᵢ`,  and with values :mini:`Fn(V₁,  ...,  Vₙ)` where each :mini:`Vᵢ` comes from :mini:`Mapᵢ` (or :mini:`nil`).

   .. code-block:: mini

      let A := map(swap("apple"))
      :> {"a" is 1, "p" is 3, "l" is 4, "e" is 5}
      let B := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let C := map(swap("pear"))
      :> {"p" is 1, "e" is 2, "a" is 3, "r" is 4}
      map::join(A, B, C, tuple)
      :> {"a" is (1, 6, 3), "p" is (3, nil, 1), "l" is (4, nil, nil), "e" is (5, nil, 2), "b" is (nil, 1, nil), "n" is (nil, 5, nil), "r" is (nil, nil, 4)}


:mini:`fun map::join2(Map₁, : map, ..., Fn: function): map`
   Returns a new map containing the union of the keys of :mini:`Mapᵢ`,  and with values :mini:`Fn(K,  V₁,  ...,  Vₙ)` where each :mini:`Vᵢ` comes from :mini:`Mapᵢ` (or :mini:`nil`).

   .. code-block:: mini

      let A := map(swap("apple"))
      :> {"a" is 1, "p" is 3, "l" is 4, "e" is 5}
      let B := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let C := map(swap("pear"))
      :> {"p" is 1, "e" is 2, "a" is 3, "r" is 4}
      map::join2(A, B, C, tuple)
      :> {"a" is (a, 1, 6, 3), "p" is (p, 3, nil, 1), "l" is (l, 4, nil, nil), "e" is (e, 5, nil, 2), "b" is (b, nil, 1, nil), "n" is (n, nil, 5, nil), "r" is (r, nil, nil, 4)}


:mini:`meth (Map: map) :: (Key: string): map::node`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.

   .. code-block:: mini

      let M := copy({"A" is 1, "B" is 2, "C" is 3}, :const)
      M::A :> 1
      M::D :> nil


:mini:`meth (Map₁: map) * (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are also in :mini:`Map₂`. The values are chosen from :mini:`Map₂`.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A * B :> {"b" is 1, "a" is 4}


:mini:`meth (Map₁: map) + (Map₂: map): map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.
   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A + B
      :> {"b" is 1, "a" is 4, "n" is 5, "r" is 2, "e" is 3, "d" is 5}


:mini:`meth (Map₁: map) / (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are not in :mini:`Map₂`.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A / B :> {"n" is 5}


:mini:`meth (Map₁: map) /\\ (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are also in :mini:`Map₂`. The values are chosen from :mini:`Map₂`.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A /\ B :> {"b" is 1, "a" is 4}


:mini:`meth (Map₁: map) <=> (Map₂: map): map`
   Returns a tuple of :mini:`(Map₁ / Map₂,  Map₁ * Map₂,  Map₂ / Map₁)`.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A <=> B
      :> ({n is 5}, {b is 1, a is 6}, {r is 2, e is 3, d is 5})


:mini:`meth (Map₁: map) >< (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` and :mini:`Map₂` that are not in both.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A >< B :> {"n" is 5, "r" is 2, "e" is 3, "d" is 5}


:mini:`meth (Map: map)[Key: any]: any | nil`
   Returns the value corresponding to :mini:`Key` in :mini:`Map`,  or :mini:`nil` if :mini:`Key` is not in :mini:`Map`.

   .. code-block:: mini

      let M := copy({"A" is 1, "B" is 2, "C" is 3}, :const)
      M["A"] :> 1
      M["D"] :> nil


:mini:`meth (Map₁: map) \\/ (Map₂: map): map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.
   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.

   .. code-block:: mini

      let A := map(swap("banana"))
      :> {"b" is 1, "a" is 6, "n" is 5}
      let B := map(swap("bread"))
      :> {"b" is 1, "r" is 2, "e" is 3, "a" is 4, "d" is 5}
      A \/ B
      :> {"b" is 1, "a" is 4, "n" is 5, "r" is 2, "e" is 3, "d" is 5}


:mini:`meth (Map: map):count: integer`
   Returns the number of entries in :mini:`Map`.

   .. code-block:: mini

      {"A" is 1, "B" is 2, "C" is 3}:count :> 3


:mini:`meth (Map: map):first`
   Returns the first value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.


:mini:`meth (Map: map):first2`
   Returns the first key and value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.


:mini:`meth (Map: map):from(Key: any): sequence | nil`
   Returns the subset of :mini:`Map` after :mini:`Key` as a sequence.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3, "D" is 4, "E" is 5}
      map(M:from("C")) :> {"C" is 3, "D" is 4, "E" is 5}
      map(M:from("F")) :> {}


:mini:`meth (Map: map):last`
   Returns the last value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.


:mini:`meth (Map: map):last2`
   Returns the last key and value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.


:mini:`meth (Map: map):order: map::order`
   Returns the current ordering of :mini:`Map`.


:mini:`meth (Map: map):precount: integer`
   Returns the number of entries in :mini:`Map`.

   .. code-block:: mini

      {"A" is 1, "B" is 2, "C" is 3}:count :> 3


:mini:`meth (List: map):random: any`
   Returns a random (assignable) node from :mini:`Map`.

   .. code-block:: mini

      let M := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M:random :> "e"
      M:random :> "e"


:mini:`meth (Map: map):size: integer`
   Returns the number of entries in :mini:`Map`.

   .. code-block:: mini

      {"A" is 1, "B" is 2, "C" is 3}:size :> 3


:mini:`meth (Buffer: string::buffer):append(Map: map)`
   Appends a representation of :mini:`Map` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Map: map, Sep: string, Conn: string)`
   Appends the entries of :mini:`Map` to :mini:`Buffer` with :mini:`Conn` between keys and values and :mini:`Sep` between entries.


:mini:`type map::mutable < map`
   *TBD*


:mini:`meth (Map: map::mutable) :: (Key: string): map::node`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M::A :> 1
      M::D :> nil
      M::A := 10 :> 10
      M::D := 20 :> 20
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Map: map::mutable)[Key: any]: map::node`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M["A"] :> 1
      M["D"] :> nil
      M["A"] := 10 :> 10
      M["D"] := 20 :> 20
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Map: map::mutable)[Key: any, Fn: function]: map::node`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Fn(Key)` is called and the result inserted into :mini:`Map`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M["A", fun(Key) Key:code] :> 1
      M["D", fun(Key) Key:code] :> 68
      M :> {"A" is 1, "B" is 2, "C" is 3, "D" is 68}


:mini:`meth (Map: map::mutable):delete(Key: any): any | nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:delete("A") :> 1
      M:delete("D") :> nil
      M :> {"B" is 2, "C" is 3}


:mini:`meth (Map: map::mutable):empty: map`
   Deletes all keys and values from :mini:`Map` and returns it.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      :> {"A" is 1, "B" is 2, "C" is 3}
      M:empty :> {}


:mini:`meth (Arg₁: map::mutable):grow(Arg₂₁ is Value₁, ...)`
   *TBD*


:mini:`meth (Map: map::mutable):grow(Sequence: sequence, ...): map`
   Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Map` and returns :mini:`Map`.

   .. code-block:: mini

      map("cake"):grow("banana")
      :> {1 is "b", 2 is "a", 3 is "n", 4 is "a", 5 is "n", 6 is "a"}


:mini:`meth (Map: map::mutable):insert(Key: any, Value: any): any | nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
   Returns the previous value associated with :mini:`Key` if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:insert("A", 10) :> 1
      M:insert("D", 20) :> nil
      M :> {"A" is 10, "B" is 2, "C" is 3, "D" is 20}


:mini:`meth (Map: map::mutable):missing(Key: any): some | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`some` and returns :mini:`some`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:missing("A") :> nil
      M:missing("D") :> some
      M :> {"A" is 1, "B" is 2, "C" is 3, "D"}


:mini:`meth (Map: map::mutable):missing(Key: any, Fn: function): any | nil`
   If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Fn(Key)` and returns :mini:`some`.

   .. code-block:: mini

      let M := {"A" is 1, "B" is 2, "C" is 3}
      M:missing("A", fun(Key) Key:code) :> nil
      M:missing("D", fun(Key) Key:code) :> some
      M :> {"A" is 1, "B" is 2, "C" is 3, "D" is 68}


:mini:`meth (Map: map::mutable):order(Order: map::order): map`
   Sets the ordering


:mini:`meth (Map: map::mutable):pop: any | nil`
   Deletes the first key-value pair from :mini:`Map` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Map` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M1:pop :> "c"
      M1 :> {2 is "a", 3 is "k", 4 is "e"}
      
      :> LRU order
      let M2 := map("cake"):order(map::order::LRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pop :> "a"
      M2 :> {4 is "e", 1 is "c", 3 is "k"}
      
      :> MRU order
      let M3 := map("cake"):order(map::order::MRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pop :> "k"
      M3 :> {1 is "c", 4 is "e", 2 is "a"}


:mini:`meth (Map: map::mutable):pop2: tuple[any, any] | nil`
   Deletes the first key-value pair from :mini:`Map` according to its iteration order. Returns the deleted key-value pair,  or :mini:`nil` if :mini:`Map` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M1:pop2 :> (1, c)
      M1 :> {2 is "a", 3 is "k", 4 is "e"}
      
      :> LRU order
      let M2 := map("cake"):order(map::order::LRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pop2 :> (2, a)
      M2 :> {4 is "e", 1 is "c", 3 is "k"}
      
      :> MRU order
      let M3 := map("cake"):order(map::order::MRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pop2 :> (3, k)
      M3 :> {1 is "c", 4 is "e", 2 is "a"}


:mini:`meth (Map: map::mutable):pull: any | nil`
   Deletes the last key-value pair from :mini:`Map` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Map` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M1:pull :> "e"
      M1 :> {1 is "c", 2 is "a", 3 is "k"}
      
      :> LRU order
      let M2 := map("cake"):order(map::order::LRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pull :> "k"
      M2 :> {2 is "a", 4 is "e", 1 is "c"}
      
      :> MRU order
      let M3 := map("cake"):order(map::order::MRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pull :> "a"
      M3 :> {3 is "k", 1 is "c", 4 is "e"}


:mini:`meth (Map: map::mutable):pull2: tuple[any, any] | nil`
   Deletes the last key-value pair from :mini:`Map` according to its iteration order. Returns the deleted key-value pair,  or :mini:`nil` if :mini:`Map` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M1:pull2 :> (4, e)
      M1 :> {1 is "c", 2 is "a", 3 is "k"}
      
      :> LRU order
      let M2 := map("cake"):order(map::order::LRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pull2 :> (3, k)
      M2 :> {2 is "a", 4 is "e", 1 is "c"}
      
      :> MRU order
      let M3 := map("cake"):order(map::order::MRU)
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pull2 :> (2, a)
      M3 :> {3 is "k", 1 is "c", 4 is "e"}


:mini:`meth (Map: map::mutable):push(Key: any, Value: any): map`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   .. code-block:: mini

      let M := {"B" is 2, "C" is 3, "A" is 1}:order(map::order::Descending)
      M:push("A", 10) :> {"A" is 10, "B" is 2, "C" is 3}
      M:push("D", 20)
      :> {"D" is 20, "A" is 10, "B" is 2, "C" is 3}
      M :> {"D" is 20, "A" is 10, "B" is 2, "C" is 3}


:mini:`meth (Map: map::mutable):put(Key: any, Value: any): map`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   .. code-block:: mini

      let M := {"B" is 2, "C" is 3, "A" is 1}:order(map::order::Descending)
      M:put("A", 10) :> {"B" is 2, "C" is 3, "A" is 10}
      M:put("D", 20)
      :> {"B" is 2, "C" is 3, "A" is 10, "D" is 20}
      M :> {"B" is 2, "C" is 3, "A" is 10, "D" is 20}


:mini:`meth (Map: map::mutable):reverse: map`
   Reverses the iteration order of :mini:`Map` in-place and returns it.

   .. code-block:: mini

      let M := map("cake")
      :> {1 is "c", 2 is "a", 3 is "k", 4 is "e"}
      M:reverse :> {4 is "e", 3 is "k", 2 is "a", 1 is "c"}


:mini:`meth (Map: map::mutable):sort: Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Keyᵢ < Keyⱼ` and returns :mini:`Map`.

   .. code-block:: mini

      let M := map(swap("cake"))
      :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort :> {"a" is 2, "c" is 1, "e" is 4, "k" is 3}


:mini:`meth (Map: map::mutable):sort(Cmp: function): Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Keyᵢ,  Keyⱼ)` and returns :mini:`Map`

   .. code-block:: mini

      let M := map(swap("cake"))
      :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort(>) :> {"k" is 3, "e" is 4, "c" is 1, "a" is 2}


:mini:`meth (Map: map::mutable):sort2(Cmp: function): Map`
   Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Keyᵢ,  Keyⱼ,  Valueᵢ,  Valueⱼ)` and returns :mini:`Map`

   .. code-block:: mini

      let M := map(swap("cake"))
      :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      M:sort(fun(K1, K2, V1, V2) V1 < V2)
      :> {"e" is 4, "k" is 3, "a" is 2, "c" is 1}


:mini:`meth (Map: map::mutable):take(Source: map::mutable): map`
   Inserts the key-value pairs from :mini:`Source` into :mini:`Map`,  leaving :mini:`Source` empty.

   .. code-block:: mini

      let A := map(swap("cat"))
      :> {"c" is 1, "a" is 2, "t" is 3}
      let B := map(swap("cake"))
      :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}
      A:take(B)
      :> {"c" is 1, "a" is 2, "t" is 3, "k" is 3, "e" is 4}
      A :> {"c" is 1, "a" is 2, "t" is 3, "k" is 3, "e" is 4}
      B :> {}


:mini:`type map::node`
   A node in a :mini:`map`.
   Dereferencing a :mini:`map::node::const` returns the corresponding value from the :mini:`map`.


:mini:`type map::node::mutable < map::node`
   A node in a :mini:`map`.
   Dereferencing a :mini:`map::node` returns the corresponding value from the :mini:`map`.
   Assigning to a :mini:`map::node` updates the corresponding value in the :mini:`map`.


:mini:`type map::node::mutable`
   A node in a :mini:`map`.
   Dereferencing a :mini:`map::node` returns the corresponding value from the :mini:`map`.
   Assigning to a :mini:`map::node` updates the corresponding value in the :mini:`map`.


:mini:`type map::order < enum`
   * :mini:`::Insert` - default ordering; inserted pairs are put at end, no reordering on access.
   * :mini:`::LRU` - inserted pairs are kept in ascending key order, no reordering on access.
   * :mini:`::MRU` - inserted pairs are kept in descending key order, no reordering on access.
   * :mini:`::Ascending` - inserted pairs are put at start, accessed pairs are moved to start.
   * :mini:`::Descending` - inserted pairs are put at end, accessed pairs are moved to end.


:mini:`fun map::reduce(Sequence: sequence, Reduce: function)`
   Creates a new map,  :mini:`Map`,  then applies :mini:`Map[Key] := Reduce(old,  Value)` for each :mini:`Key`,  :mini:`Value` pair generated by :mini:`Sequence`,  finally returning :mini:`Map`.

   .. code-block:: mini

      map::reduce(swap("banana"); L := [], I) L:put(I)
      :> {"b" is [1], "a" is [2, 4, 6], "n" is [3, 5]}


:mini:`meth (Copy: visitor):const(Map: map): map::const`
   Returns a new constant map containing copies of the keys and values of :mini:`Map` created using :mini:`Copy`.


:mini:`meth (Copy: visitor):copy(Map: map): map`
   Returns a new map contains copies of the keys and values of :mini:`Map` created using :mini:`Copy`.


:mini:`meth (Copy: visitor):visit(Map: map): map`
   Returns a new map contains copies of the keys and values of :mini:`Map` created using :mini:`Copy`.


