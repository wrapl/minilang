.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

set
===

:mini:`meth (Key: any):in(Set: set): any | nil`
   Returns :mini:`Key` if it is in :mini:`Map`,  otherwise return :mini:`nil`.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      "A" in S :> "A"
      "D" in S :> nil


.. _type-set:

:mini:`type set < sequence`
   A set of values.
   Values can be of any type supporting hashing and comparison.
   By default,  iterating over a set generates the values in the order they were inserted,  however this ordering can be changed.


:mini:`meth set(Sequence: sequence, ...): set`
   Returns a set of all the key and value pairs produced by :mini:`Sequence`.

   .. code-block:: mini

      set("cake") :> {c, a, k, e}


:mini:`meth set(): set`
   Returns a new set.

   .. code-block:: mini

      set() :> {}


:mini:`meth (Set₁: set) * (Set₂: set): set`
   Returns a new set containing the entries of :mini:`Set₁` which are also in :mini:`Set₂`. The values are chosen from :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A * B :> {b, a}


:mini:`meth (Set₁: set) + (Set₂: set): set`
   Returns a new set combining the entries of :mini:`Set₁` and :mini:`Set₂`.
   If the same key is in both :mini:`Set₁` and :mini:`Set₂` then the corresponding value from :mini:`Set₂` is chosen.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A + B :> {b, a, n, r, e, d}


:mini:`meth (Set₁: set) / (Set₂: set): set`
   Returns a new set containing the entries of :mini:`Set₁` which are not in :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A / B :> {n}


:mini:`meth (Set₁: set) /\ (Set₂: set): set`
   Returns a new set containing the entries of :mini:`Set₁` which are also in :mini:`Set₂`. The values are chosen from :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A /\ B :> {b, a}


:mini:`meth (Set₁: set) >< (Set₂: set): set`
   Returns a new set containing the entries of :mini:`Set₁` and :mini:`Set₂` that are not in both.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A >< B :> {n, r, e, d}


:mini:`meth (Set: set)[Key: any]: some | nil`
   Returns the node corresponding to :mini:`Key` in :mini:`Set`. If :mini:`Key` is not in :mini:`Set` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Set` if assigned.

   .. code-block:: mini

      let M := set(["A", "B", "C"])
      M["A"] :> some
      M["D"] :> nil
      M :> {A, B, C}


:mini:`meth (Set₁: set) \/ (Set₂: set): set`
   Returns a new set combining the entries of :mini:`Set₁` and :mini:`Set₂`.
   If the same key is in both :mini:`Set₁` and :mini:`Set₂` then the corresponding value from :mini:`Set₂` is chosen.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A \/ B :> {b, a, n, r, e, d}


:mini:`meth (Set: set):count: integer`
   Returns the number of entries in :mini:`Set`.

   .. code-block:: mini

      set(["A", "B", "C"]):count :> 3


:mini:`meth (Set: set):delete(Key: any): some | nil`
   Removes :mini:`Key` from :mini:`Set` and returns the corresponding value if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := set(["A", "B", "C"])
      M:delete("A") :> some
      M:delete("D") :> nil
      M :> {B, C}


:mini:`meth (Set: set):empty: set`
   Deletes all keys and values from :mini:`Set` and returns it.

   .. code-block:: mini

      let M := set(["A", "B", "C"]) :> {A, B, C}
      M:empty :> {}


:mini:`meth (Set: set):from(Key: any): sequence | nil`
   Returns the subset of :mini:`Set` after :mini:`Key` as a sequence.

   .. code-block:: mini

      let M := set(["A", "B", "C", "D", "E"])
      set(M:from("C")) :> {C, D, E}
      set(M:from("F")) :> {}


:mini:`meth (Set: set):grow(Sequence: sequence, ...): set`
   Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Set` and returns :mini:`Set`.

   .. code-block:: mini

      set("cake"):grow("banana") :> {c, a, k, e, b, n}


:mini:`meth (Set: set):insert(Key: any, Value: any): some | nil`
   Inserts :mini:`Key` into :mini:`Set` with corresponding value :mini:`Value`.
   Returns the previous value associated with :mini:`Key` if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let M := set(["A", "B", "C"])
      M:insert("A") :> some
      M:insert("D") :> nil
      M :> {A, B, C, D}


:mini:`meth (Set: set):missing(Key: any): some | nil`
   If :mini:`Key` is present in :mini:`Set` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Set` with value :mini:`some` and returns :mini:`some`.

   .. code-block:: mini

      let M := set(["A", "B", "C"])
      M:missing("A") :> nil
      M:missing("D") :> some
      M :> {A, B, C, D}


:mini:`meth (Set: set):order: integer`
   Returns the current ordering of :mini:`Set`.
   
   * :mini:`0` |harr| default ordering; inserted pairs are put at end,  no reordering on access.
   * :mini:`1` |harr| MRU ordering; inserted pairs are put at start,  accessed pairs are moved to start.
   * :mini:`-1` |harr| LRU ordering; inserted pairs are put at end,  accessed paires are moved to end.


:mini:`meth (Set: set):order(Order: integer): set`
   Sets the ordering


:mini:`meth (Set: set):pop: any | nil`
   Deletes the first value from :mini:`Set` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Set` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := set("cake") :> {c, a, k, e}
      M1:pop :> "c"
      M1 :> {a, k, e}
      
      :> LRU order
      let M2 := set("cake"):order(-1) :> {c, a, k, e}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pop :> "c"
      M2 :> {a, k, e}
      
      :> MRU order
      let M3 := set("cake"):order(1) :> {c, a, k, e}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pop :> "c"
      M3 :> {a, k, e}


:mini:`meth (Set: set):pull: any | nil`
   Deletes the last value from :mini:`Set` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Set` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let M1 := set("cake") :> {c, a, k, e}
      M1:pull :> "e"
      M1 :> {c, a, k}
      
      :> LRU order
      let M2 := set("cake"):order(-1) :> {c, a, k, e}
      M2[2]; M2[4]; M2[1]; M2[3]
      M2:pull :> "e"
      M2 :> {c, a, k}
      
      :> MRU order
      let M3 := set("cake"):order(1) :> {c, a, k, e}
      M3[2]; M3[4]; M3[1]; M3[3]
      M3:pull :> "e"
      M3 :> {c, a, k}


:mini:`meth (List: set):random: any`
   Returns a random (assignable) node from :mini:`Set`.

   .. code-block:: mini

      let M := set("cake") :> {c, a, k, e}
      M:random :> "a"
      M:random :> "a"


:mini:`meth (Set: set):reverse: set`
   Reverses the iteration order of :mini:`Set` in-place and returns it.

   .. code-block:: mini

      let M := set("cake") :> {c, a, k, e}
      M:reverse :> {e, k, a, c}


:mini:`meth (Set: set):size: integer`
   Returns the number of entries in :mini:`Set`.

   .. code-block:: mini

      set(["A", "B", "C"]):size :> 3


:mini:`meth (Set: set):sort: Set`
   Sorts the entries (changes the iteration order) of :mini:`Set` using :mini:`Keyᵢ < Keyⱼ` and returns :mini:`Set`.

   .. code-block:: mini

      let M := set("cake") :> {c, a, k, e}
      M:sort :> {a, c, e, k}


:mini:`meth (Set: set):sort(Cmp: function): Set`
   Sorts the entries (changes the iteration order) of :mini:`Set` using :mini:`Cmp(Keyᵢ,  Keyⱼ)` and returns :mini:`Set`

   .. code-block:: mini

      let M := set("cake") :> {c, a, k, e}
      M:sort(>) :> {k, e, c, a}


:mini:`meth (Buffer: string::buffer):append(Set: set)`
   Appends a representation of :mini:`Set` to :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):append(Set: set, Sep: string)`
   Appends the entries of :mini:`Set` to :mini:`Buffer` with :mini:`Conn` between keys and values and :mini:`Sep` between entries.


.. _type-set-node:

:mini:`type set::node`
   A node in a :mini:`set`.


