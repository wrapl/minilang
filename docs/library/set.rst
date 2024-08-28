.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

set
===

.. rst-class:: mini-api

:mini:`meth (Value: any):in(Set: set): any | nil`
   Returns :mini:`Key` if it is in :mini:`Map`,  otherwise return :mini:`nil`.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      "A" in S :> "A"
      "D" in S :> nil


:mini:`type set < sequence`
   A set of values.
   Values can be of any type supporting hashing and comparison.
   By default,  iterating over a set generates the values in the order they were inserted,  however this ordering can be changed.


:mini:`meth set(): set`
   Returns a new set.

   .. code-block:: mini

      set() :> {}


:mini:`meth set(Sequence: sequence, ...): set`
   Returns a set of all the values produced by :mini:`Sequence`.

   .. code-block:: mini

      set("cake") :> {c, a, k, e}


:mini:`meth (Set₁: set) * (Set₂: set): set`
   Returns a new set containing the values of :mini:`Set₁` which are also in :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A * B :> {b, a}


:mini:`meth (Set₁: set) + (Set₂: set): set`
   Returns a new set combining the values of :mini:`Set₁` and :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A + B :> {b, a, n, r, e, d}


:mini:`meth (Set₁: set) / (Set₂: set): set`
   Returns a new set containing the values of :mini:`Set₁` which are not in :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A / B :> {n}


:mini:`meth (Set₁: set) /\\ (Set₂: set): set`
   Returns a new set containing the values of :mini:`Set₁` which are also in :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A /\ B :> {b, a}


:mini:`meth (Set₁: set) < (Set₂: set): set`
   Returns a :mini:`Set₂` if :mini:`Set₁` is a strict subset of :mini:`Set₂`,  otherwise returns :mini:`nil`.

   .. code-block:: mini

      let A := set("bandana") :> {b, a, n, d}
      let B := set("ban") :> {b, a, n}
      let C := set("bread") :> {b, r, e, a, d}
      let D := set("bandana") :> {b, a, n, d}
      B < A :> {b, a, n, d}
      C < A :> nil
      D < A :> nil


:mini:`meth (Set₁: set) <= (Set₂: set): set`
   Returns a :mini:`Set₂` if :mini:`Set₁` is a subset of :mini:`Set₂`,  otherwise returns :mini:`nil`.

   .. code-block:: mini

      let A := set("bandana") :> {b, a, n, d}
      let B := set("ban") :> {b, a, n}
      let C := set("bread") :> {b, r, e, a, d}
      let D := set("bandana") :> {b, a, n, d}
      B <= A :> {b, a, n, d}
      C <= A :> nil
      D <= A :> {b, a, n, d}


:mini:`meth (Set₁: set) <=> (Set₂: set): set`
   Returns a tuple of :mini:`(Set₁ / Set₂,  Set₁ * Set₂,  Set₂ / Set₁)`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A <=> B :> ({n}, {b, a}, {r, e, d})


:mini:`meth (Set₁: set) > (Set₂: set): set`
   Returns a :mini:`Set₂` if :mini:`Set₁` is a strict superset of :mini:`Set₂`,  otherwise returns :mini:`nil`.

   .. code-block:: mini

      let A := set("bandana") :> {b, a, n, d}
      let B := set("ban") :> {b, a, n}
      let C := set("bread") :> {b, r, e, a, d}
      let D := set("bandana") :> {b, a, n, d}
      A > B :> {b, a, n}
      A > C :> nil
      A > D :> nil


:mini:`meth (Set₁: set) >< (Set₂: set): set`
   Returns a new set containing the values of :mini:`Set₁` and :mini:`Set₂` that are not in both.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A >< B :> {n, r, e, d}


:mini:`meth (Set₁: set) >= (Set₂: set): set`
   Returns a :mini:`Set₂` if :mini:`Set₁` is a superset of :mini:`Set₂`,  otherwise returns :mini:`nil`.

   .. code-block:: mini

      let A := set("bandana") :> {b, a, n, d}
      let B := set("ban") :> {b, a, n}
      let C := set("bread") :> {b, r, e, a, d}
      let D := set("bandana") :> {b, a, n, d}
      A >= B :> {b, a, n}
      A >= C :> nil
      A >= D :> {b, a, n, d}


:mini:`meth (Set: set)[Value: any]: some | nil`
   Returns :mini:`Value` if it is in :mini:`Set`,  otherwise returns :mini:`nil`..

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S["A"] :> "A"
      S["D"] :> nil
      S :> {A, B, C}


:mini:`meth (Set₁: set) \\/ (Set₂: set): set`
   Returns a new set combining the values of :mini:`Set₁` and :mini:`Set₂`.

   .. code-block:: mini

      let A := set("banana") :> {b, a, n}
      let B := set("bread") :> {b, r, e, a, d}
      A \/ B :> {b, a, n, r, e, d}


:mini:`meth (Set: set):count: integer`
   Returns the number of values in :mini:`Set`.

   .. code-block:: mini

      set(["A", "B", "C"]):count :> 3


:mini:`meth (Set: set):first`
   Returns the first value in :mini:`Set` or :mini:`nil` if :mini:`Set` is empty.


:mini:`meth (Set: set):from(Value: any): sequence | nil`
   Returns the subset of :mini:`Set` after :mini:`Value` as a sequence.

   .. code-block:: mini

      let S := set(["A", "B", "C", "D", "E"])
      set(S:from("C")) :> {C, D, E}
      set(S:from("F")) :> {}


:mini:`meth (Set: set):last`
   Returns the last value in :mini:`Set` or :mini:`nil` if :mini:`Set` is empty.


:mini:`meth (Set: set):order: set::order`
   Returns the current ordering of :mini:`Set`.


:mini:`meth (Set: set):precount: integer`
   Returns the number of values in :mini:`Set`.

   .. code-block:: mini

      set(["A", "B", "C"]):count :> 3


:mini:`meth (List: set):random: any`
   Returns a random (assignable) node from :mini:`Set`.

   .. code-block:: mini

      let S := set("cake") :> {c, a, k, e}
      S:random :> "e"
      S:random :> "k"


:mini:`meth (Set: set):size: integer`
   Returns the number of values in :mini:`Set`.

   .. code-block:: mini

      set(["A", "B", "C"]):size :> 3


:mini:`meth (Buffer: string::buffer):append(Set: set)`
   Appends a representation of :mini:`Set` to :mini:`Buffer` of the form :mini:`"[" + repr(V₁) + ",  " + repr(V₂) + ",  " + ... + repr(Vₙ) + "]"`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append(set(1 .. 4))
      B:rest :> "{1, 2, 3, 4}"


:mini:`meth (Buffer: string::buffer):append(Set: set, Sep: string)`
   Appends a representation of :mini:`Set` to :mini:`Buffer` of the form :mini:`repr(V₁) + Sep + repr(V₂) + Sep + ... + repr(Vₙ)`,  where :mini:`repr(Vᵢ)` is a representation of the *i*-th element (using :mini:`:append`).

   .. code-block:: mini

      let B := string::buffer()
      B:append(set(1 .. 4), " - ")
      B:rest :> "1 - 2 - 3 - 4"


:mini:`type set::mutable < set`
   *TBD*


:mini:`meth (Set: set::mutable):delete(Value: any): some | nil`
   Removes :mini:`Value` from :mini:`Set` and returns it if found,  otherwise :mini:`nil`.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S:delete("A") :> some
      S:delete("D") :> nil
      S :> {B, C}


:mini:`meth (Set: set::mutable):empty: set`
   Deletes all values from :mini:`Set` and returns it.

   .. code-block:: mini

      let S := set(["A", "B", "C"]) :> {A, B, C}
      S:empty :> {}


:mini:`meth (Set: set::mutable):grow(Sequence: sequence, ...): set`
   Adds of all the values produced by :mini:`Sequence` to :mini:`Set` and returns :mini:`Set`.

   .. code-block:: mini

      set("cake"):grow("banana") :> {c, a, k, e, b, n}


:mini:`meth (Set: set::mutable):insert(Value: any): some | nil`
   Inserts :mini:`Value` into :mini:`Set`.
   Returns the previous value associated with :mini:`Key` if any,  otherwise :mini:`nil`.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S:insert("A") :> some
      S:insert("D") :> nil
      S :> {A, B, C, D}


:mini:`meth (Set: set::mutable):missing(Value: any): some | nil`
   If :mini:`Value` is present in :mini:`Set` then returns :mini:`nil`. Otherwise inserts :mini:`Value` into :mini:`Set` and returns :mini:`some`.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S:missing("A") :> nil
      S:missing("D") :> some
      S :> {A, B, C, D}


:mini:`meth (Set: set::mutable):order(Order: set::order): set`
   Sets the ordering


:mini:`meth (Set: set::mutable):pop: any | nil`
   Deletes the first value from :mini:`Set` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Set` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let S1 := set("cake") :> {c, a, k, e}
      S1:pop :> "c"
      S1 :> {a, k, e}
      
      :> LRU order
      let S2 := set("cake"):order(set::order::LRU)
      :> {c, a, k, e}
      S2["a"]; S2["e"]; S2["c"]; S2["k"]
      S2:pop :> "a"
      S2 :> {e, c, k}
      
      :> MRU order
      let S3 := set("cake"):order(set::order::MRU)
      :> {c, a, k, e}
      S3["a"]; S3["e"]; S3["c"]; S3["k"]
      S3:pop :> "k"
      S3 :> {c, e, a}


:mini:`meth (Set: set::mutable):pull: any | nil`
   Deletes the last value from :mini:`Set` according to its iteration order. Returns the deleted value,  or :mini:`nil` if :mini:`Set` is empty.

   .. code-block:: mini

      :> Insertion order (default)
      let S1 := set("cake") :> {c, a, k, e}
      S1:pull :> "e"
      S1 :> {c, a, k}
      
      :> LRU order
      let S2 := set("cake"):order(set::order::LRU)
      :> {c, a, k, e}
      S2["a"]; S2["e"]; S2["c"]; S2["k"]
      S2:pull :> "k"
      S2 :> {a, e, c}
      
      :> MRU order
      let S3 := set("cake"):order(set::order::MRU)
      :> {c, a, k, e}
      S3["a"]; S3["e"]; S3["c"]; S3["k"]
      S3:pull :> "a"
      S3 :> {k, c, e}


:mini:`meth (Set: set::mutable):push(Value: any, ...): set`
   Inserts each :mini:`Value` into :mini:`Set` at the start.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S:push("A") :> {A, B, C}
      S:push("D") :> {D, A, B, C}
      S:push("E", "B") :> {B, E, D, A, C}
      S :> {B, E, D, A, C}


:mini:`meth (Set: set::mutable):put(Value: any, ...): set`
   Inserts each :mini:`Value` into :mini:`Set` at the end.

   .. code-block:: mini

      let S := set(["A", "B", "C"])
      S:put("A") :> {B, C, A}
      S:put("D") :> {B, C, A, D}
      S:put("E", "B") :> {C, A, D, E, B}
      S :> {C, A, D, E, B}


:mini:`meth (Set: set::mutable):reverse: set`
   Reverses the iteration order of :mini:`Set` in-place and returns it.

   .. code-block:: mini

      let S := set("cake") :> {c, a, k, e}
      S:reverse :> {e, k, a, c}


:mini:`meth (Set: set::mutable):sort: Set`
   Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Valueᵢ < Valueⱼ` and returns :mini:`Set`.

   .. code-block:: mini

      let S := set("cake") :> {c, a, k, e}
      S:sort :> {a, c, e, k}


:mini:`meth (Set: set::mutable):sort(Cmp: function): Set`
   Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Cmp(Valueᵢ,  Valueⱼ)` and returns :mini:`Set`

   .. code-block:: mini

      let S := set("cake") :> {c, a, k, e}
      S:sort(>) :> {k, e, c, a}


:mini:`type set::order < enum`
   * :mini:`::Insert` - default ordering; inserted values are put at end, no reordering on access.
   * :mini:`::LRU` - inserted values are kept in ascending order, no reordering on access.
   * :mini:`::MRU` - inserted values are kept in descending order, no reordering on access.
   * :mini:`::Ascending` - inserted values are put at start, accessed values are moved to start.
   * :mini:`::Descending` - inserted values are put at end, accessed values are moved to end.


:mini:`meth (Visitor: visitor):const(Set: set): set`
   Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Visitor`.


:mini:`meth (Visitor: visitor):copy(Set: set): set`
   Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Copy`.


:mini:`meth (Visitor: visitor):visit(Set: set): set`
   Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Copy`.


