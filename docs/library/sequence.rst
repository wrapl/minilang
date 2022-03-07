.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

sequence
========

.. _fun-count:

:mini:`fun count(Sequence: any): integer`
   Returns the count of the values produced by :mini:`Sequence`. For some types of sequences (e.g. :mini:`list`,  :mini:`map`,  etc),  the count is simply retrieved. For all other types,  the sequence is iterated and the total number of values counted.

   .. code-block:: mini

      count([1, 2, 3, 4]) :> 4
      count(1 .. 10 ->? (2 | _)) :> 5


.. _fun-iter_key:

:mini:`fun iter_key(Value: any): any | nil`
   Used for iterating over a sequence.



.. _fun-iter_next:

:mini:`fun iter_next(Value: any): any | nil`
   Used for iterating over a sequence.



.. _fun-iter_value:

:mini:`fun iter_value(Value: any): any | nil`
   Used for iterating over a sequence.



.. _fun-iterate:

:mini:`fun iterate(Value: any): any | nil`
   Used for iterating over a sequence.



.. _fun-reduce:

:mini:`fun reduce(Initial?: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial,  V₁),  V₂) ...,  Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Sequence`.

   If :mini:`Initial` is omitted,  first value produced by :mini:`Sequence` is used.

   .. code-block:: mini

      reduce(1 .. 10, +) :> 55
      reduce([], 1 .. 10, :put) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


.. _fun-reduce2:

:mini:`fun reduce2(Initial: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial,  K₁,  V₁),  K₂,  V₂) ...,  Kₙ,  Vₙ)` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Sequence`.

   .. code-block:: mini

      reduce2([], "cake", fun(L, K, V) L:put((K, V))) :> [(1, c), (2, a), (3, k), (4, e)]


.. _fun-unique:

:mini:`fun unique(Sequence: any): sequence`
   Returns an sequence that returns the unique values produced by :mini:`Sequence`. Uniqueness is determined by using a :mini:`map`.

   .. code-block:: mini

      list(unique("banana")) :> ["b", "a", "n"]


:mini:`meth @(Value: any): sequence`
   Returns an infinite sequence that repeatedly produces :mini:`Value`. Should be used with :mini:`:limit` or paired with a finite sequence in :mini:`zip`,  :mini:`weave`,  etc.

   .. code-block:: mini

      list(@1 limit 10) :> [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]


:mini:`meth (Value: any) @ (Update: function): sequence`
   Returns an infinite sequence that repeatedly produces :mini:`Value`. Should be used with :mini:`:limit` or paired with a finite sequence in :mini:`zip`,  :mini:`weave`,  etc.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.

   .. code-block:: mini

      list(1 @ (_ + 1) limit 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]


.. _type-chained:

:mini:`type chained < function, sequence`
   A chained function or sequence,  consisting of a base function or sequence and any number of additional functions or filters.

   When used as a function or sequence,  the base is used to produce an initial result,  then the additional functions are applied in turn to the result.

   Filters do not affect the result but will shortcut a function call or skip an iteration if :mini:`nil` is returned. I.e. filters remove values from a sequence that fail a condition without affecting the values that pass.



.. _fun-chained:

:mini:`fun chained(Base: any, Fn₁, : function, ...): chained`
   Returns a new chained function or sequence with base :mini:`Base` and additional functions or filters :mini:`Fn₁,  ...,  Fnₙ`.

   .. code-block:: mini

      let F := chained(fun(X) X + 1, fun(X) X ^ 2)
      F(10) :> 121


:mini:`meth (Base: function) -> (Function: function): chained`
   Returns a chained function equivalent to :mini:`Function(Base(...))`.

   .. code-block:: mini

      let F := :upper -> (3 * _)
      F("hello") :> "HELLOHELLOHELLO"
      F("cake") :> "CAKECAKECAKE"


.. _type-sequence:

:mini:`type sequence`
   The base type for any sequence value.



.. _fun-all:

:mini:`fun all(Sequence: sequence): some | nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Sequence`. Otherwise returns :mini:`some`. If :mini:`Sequence` is empty,  then :mini:`some` is returned.

   .. code-block:: mini

      all([1, 2, 3, 4]) :> some
      all([1, 2, nil, 4]) :> nil
      all([]) :> some


.. _fun-batch:

:mini:`fun batch(Sequence: sequence, Size: integer, Shift?: integer, Function: function): sequence`
   Returns a new sequence that calls :mini:`Function` with each batch of :mini:`Size` values produced by :mini:`Sequence` and produces the results. If a :mini:`Shift` is provided then :mini:`Size - Shift` values of each batch come from the previous batch.

   .. code-block:: mini

      list(batch(1 .. 20, 4, tuple)) :> [(1, 2, 3, 4), (5, 6, 7, 8), (9, 10, 11, 12), (13, 14, 15, 16), (17, 18, 19, 20)]
      list(batch(1 .. 20, 4, 2, tuple)) :> [(1, 2, 3, 4), (3, 4, 5, 6), (5, 6, 7, 8), (7, 8, 9, 10), (9, 10, 11, 12), (11, 12, 13, 14), (13, 14, 15, 16), (15, 16, 17, 18), (17, 18, 19, 20)]


.. _fun-count2:

:mini:`fun count2(Sequence: sequence): map`
   Returns a map of the values produced by :mini:`Sequence` with associated counts.

   .. code-block:: mini

      count2("banana") :> {"b" is 1, "a" is 3, "n" is 2}


.. _fun-extremum:

:mini:`fun extremum(Sequence: sequence, Fn: function): tuple | nil`
   Returns a tuple with the key and value of the extremum value using :mini:`Fn(Value₁,  Value₂)` produced by :mini:`Sequence`. Returns :mini:`nil` if :mini:`Sequence` is empty.

   .. code-block:: mini

      extremum("cake", >) :> (2, a)
      extremum("cake", <) :> (3, k)


.. _fun-first:

:mini:`fun first(Sequence: sequence): any | nil`
   Returns the first value produced by :mini:`Sequence`.

   .. code-block:: mini

      first("cake") :> "c"
      first([]) :> nil


.. _fun-first2:

:mini:`fun first2(Sequence: sequence): tuple(any,  any) | nil`
   Returns the first key and value produced by :mini:`Sequence`.

   .. code-block:: mini

      first2("cake") :> (1, c)
      first2([]) :> nil


.. _fun-fold:

:mini:`fun fold(Sequence: sequence): sequence`
   Returns a new sequence that treats alternating values produced by :mini:`Sequence` as keys and values respectively.

   .. code-block:: mini

      map(fold(1 .. 10)) :> {1 is 2, 3 is 4, 5 is 6, 7 is 8, 9 is 10}


.. _fun-grid:

:mini:`fun grid(Sequence₁, : sequence, ..., Function: any): sequence`
   Returns a new sequence that produces :mini:`Function(V₁,  V₂,  ...,  Vₙ)` for all possible combinations of :mini:`V₁,  ...,  Vₙ`,  where :mini:`Vᵢ` are the values produced by :mini:`Sequenceᵢ`.

   .. code-block:: mini

      list(grid(1 .. 3, "cake", [true, false], tuple)) :> [(1, c, true), (1, c, false), (1, a, true), (1, a, false), (1, k, true), (1, k, false), (1, e, true), (1, e, false), (2, c, true), (2, c, false), (2, a, true), (2, a, false), (2, k, true), (2, k, false), (2, e, true), (2, e, false), (3, c, true), (3, c, false), (3, a, true), (3, a, false), (3, k, true), (3, k, false), (3, e, true), (3, e, false)]
      list(grid(1 .. 3, "cake", *)) :> ["c", "a", "k", "e", "cc", "aa", "kk", "ee", "ccc", "aaa", "kkk", "eee"]


.. _fun-key:

:mini:`fun key(Sequence: sequence)`
   Returns a new sequence which produces the keys of :mini:`Sequence`.

   .. code-block:: mini

      list(key({"A" is 1, "B" is 2, "C" is 3})) :> ["A", "B", "C"]


.. _fun-last:

:mini:`fun last(Sequence: sequence): any | nil`
   Returns the last value produced by :mini:`Sequence`.

   .. code-block:: mini

      last("cake") :> "e"
      last([]) :> nil


.. _fun-last2:

:mini:`fun last2(Sequence: sequence): tuple(any,  any) | nil`
   Returns the last key and value produced by :mini:`Sequence`.

   .. code-block:: mini

      last2("cake") :> (4, e)
      last2([]) :> nil


.. _fun-max:

:mini:`fun max(Sequence: sequence): any | nil`
   Returns the largest value (using :mini:`>`) produced by :mini:`Sequence`.

   .. code-block:: mini

      max([1, 5, 2, 10, 6]) :> 10


.. _fun-max2:

:mini:`fun max2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the largest value (using :mini:`>`) produced by :mini:`Sequence`.  Returns :mini:`nil` if :mini:`Sequence` is empty.

   .. code-block:: mini

      max2([1, 5, 2, 10, 6]) :> (4, 10)


.. _fun-min:

:mini:`fun min(Sequence: sequence): any | nil`
   Returns the smallest value (using :mini:`<`) produced by :mini:`Sequence`.

   .. code-block:: mini

      min([1, 5, 2, 10, 6]) :> 1


.. _fun-min2:

:mini:`fun min2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the smallest value (using :mini:`<`) produced by :mini:`Sequence`.  Returns :mini:`nil` if :mini:`Sequence` is empty.

   .. code-block:: mini

      min2([1, 5, 2, 10, 6]) :> (1, 1)


.. _fun-pair:

:mini:`fun pair(Sequence₁: sequence, Sequence₂: sequence): sequence`
   Returns a new sequence that produces the values from :mini:`Sequence₁` as keys and the values from :mini:`Sequence₂` as values.



.. _fun-prod:

:mini:`fun prod(Sequence: sequence): any | nil`
   Returns the product of the values (using :mini:`*`) produced by :mini:`Sequence`.

   .. code-block:: mini

      prod([1, 5, 2, 10, 6]) :> 600


.. _fun-sum:

:mini:`fun sum(Sequence: sequence): any | nil`
   Returns the sum of the values (using :mini:`+`) produced by :mini:`Sequence`.

   .. code-block:: mini

      sum([1, 5, 2, 10, 6]) :> 24


.. _fun-swap:

:mini:`fun swap(Sequence: sequence)`
   Returns a new sequence which swaps the keys and values produced by :mini:`Sequence`.

   .. code-block:: mini

      map(swap("cake")) :> {"c" is 1, "a" is 2, "k" is 3, "e" is 4}


.. _fun-unfold:

:mini:`fun unfold(Sequence: sequence): sequence`
   Returns a new sequence that treats produces alternatively the keys and values produced by :mini:`Sequence`.

   .. code-block:: mini

      list(unfold("cake")) :> [1, "c", 2, "a", 3, "k", 4, "e"]


.. _fun-unpack:

:mini:`fun unpack(Sequence: sequence): sequence`
   Returns a new sequence unpacks each value generated by :mini:`Sequence` as keys and values respectively.

   .. code-block:: mini

      let L := [("A", "a"), ("B", "b"), ("C", "c")]
      map(unpack(L)) :> {"A" is "a", "B" is "b", "C" is "c"}


.. _fun-weave:

:mini:`fun weave(Sequence₁, : sequence, ...): sequence`
   Returns a new sequence that produces interleaved values :mini:`Vᵢ` from each of :mini:`Sequenceᵢ`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.

   .. code-block:: mini

      list(weave(1 .. 3, "cake")) :> [1, "c", 2, "a", 3, "k"]


.. _fun-zip:

:mini:`fun zip(Sequence₁, : sequence, ..., Function: any): sequence`
   Returns a new sequence that produces :mini:`Function(V₁₁,  ...,  Vₙ₁),  Function(V₁₂,  ...,  Vₙ₂),  ...` where :mini:`Vᵢⱼ` is the :mini:`j`-th value produced by :mini:`Sequenceᵢ`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.

   .. code-block:: mini

      list(zip(1 .. 3, "cake", tuple)) :> [(1, c), (2, a), (3, k)]


:mini:`meth (Base: sequence) -> (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F(V₁)),  ...,  (Kₙ,  F(Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.



:mini:`meth (Base: sequence) ->! (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F ! V₁),  ...,  (Kₙ,  F ! Vₙ)` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.

   .. code-block:: mini

      map({"A" is [1, 2], "B" is [3, 4], "C" is [5, 6]} ->! +) :> {"A" is 3, "B" is 7, "C" is 11}


:mini:`meth (Base: sequence) ->!? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F ! Vⱼ` returns non-:mini:`nil`.

   .. code-block:: mini

      map({"A" is [1, 2], "B" is [3, 3], "C" is [5, 6]} ->!? !=) :> {"A" is [1, 2], "C" is [5, 6]}


:mini:`meth (Sequence: sequence) ->> (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.

   .. code-block:: mini

      list(1 .. 5 ->> (1 .. _)) :> [1, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1, 2, 3, 4, 5]


:mini:`meth (Base: sequence) ->? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F(Vⱼ)` returns non-:mini:`nil`.

   .. code-block:: mini

      list(1 .. 10 ->? (2 | _)) :> [2, 4, 6, 8, 10]


:mini:`meth (Sequence: sequence) // (Initial: any, Fn: function): sequence`
   Returns an sequence that produces :mini:`Initial`,  :mini:`Fn(Initial,  V₁)`,  :mini:`Fn(Fn(Initial,  V₁),  V₂)`,  ... .

   .. code-block:: mini

      list(1 .. 10 // (10, +)) :> [11, 13, 16, 20, 25, 31, 38, 46, 55, 65]


:mini:`meth (Sequence: sequence) // (Fn: function): sequence`
   Returns an sequence that produces :mini:`V₁`,  :mini:`Fn(V₁,  V₂)`,  :mini:`Fn(Fn(V₁,  V₂),  V₃)`,  ... .

   .. code-block:: mini

      list(1 .. 10 // +) :> [1, 3, 6, 10, 15, 21, 28, 36, 45, 55]


:mini:`meth (Sequence: sequence):join(Separator: string): string`
   Joins the elements of :mini:`Sequence` into a string using :mini:`Separator` between elements.

   .. code-block:: mini

      (1 .. 10):join :> "12345678910"


:mini:`meth (Sequence: sequence):join: string`
   Joins the elements of :mini:`Sequence` into a string.

   .. code-block:: mini

      1 .. 10 join "," :> "1,2,3,4,5,6,7,8,9,10"


:mini:`meth (Sequence: sequence):limit(Fn: function): sequence`
   Returns an sequence that stops when :mini:`Fn(Value)` is :mini:`nil`.

   .. code-block:: mini

      list("banana") :> ["b", "a", "n", "a", "n", "a"]
      list("banana" limit (_ != "n")) :> ["b", "a"]


:mini:`meth (Sequence: sequence):limit(Limit: integer): sequence`
   Returns an sequence that produces at most :mini:`Limit` values from :mini:`Sequence`.

   .. code-block:: mini

      list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      list(1 .. 10 limit 5) :> [1, 2, 3, 4, 5]


:mini:`meth (Sequence: sequence):skip(Skip: integer): sequence`
   Returns an sequence that skips the first :mini:`Skip` values from :mini:`Sequence` and then produces the rest.

   .. code-block:: mini

      list(1 .. 10) :> [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      list(1 .. 10 skip 5) :> [6, 7, 8, 9, 10]


:mini:`meth (Base: sequence) => (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F(K₁,  V₁)),  ...,  (Kₙ,  F(Kₙ,  Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.

   .. code-block:: mini

      map("cake" => *) :> {1 is "c", 2 is "aa", 3 is "kkk", 4 is "eeee"}


:mini:`meth (Base: sequence) => (F₁: function, F₂: function): sequence`
   Returns a chained sequence equivalent to :mini:`(F₁(K₁,  V₁),  F₂(K₁,  V₁)),  ...,  (F₁(Kₙ,  Vₙ),  F₂(Kₙ,  Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.

   .. code-block:: mini

      map("cake" => (tuple, *)) :> {(1, c) is "c", (2, a) is "aa", (3, k) is "kkk", (4, e) is "eeee"}


:mini:`meth (Sequence: sequence) =>> (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Key,  Value)` for each key and value generated by :mini:`Sequence`.

   .. code-block:: mini

      list("cake" =>> *) :> ["c", "a", "a", "k", "k", "k", "e", "e", "e", "e"]


:mini:`meth (Base: sequence) =>? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F(Kⱼ,  Vⱼ)` returns non-:mini:`nil`.

   .. code-block:: mini

      let M := map(1 .. 10 -> fun(X) X ^ 2 % 10) :> {1 is 1, 2 is 4, 3 is 9, 4 is 6, 5 is 5, 6 is 6, 7 is 9, 8 is 4, 9 is 1, 10 is 0}
      map(M =>? !=) :> {2 is 4, 3 is 9, 4 is 6, 7 is 9, 8 is 4, 9 is 1, 10 is 0}


:mini:`meth (Sequence₁: sequence) >> (Sequence₂: sequence): Sequence`
   Returns an sequence that produces the values from :mini:`Sequence₁` followed by those from :mini:`Sequence₂`.

   .. code-block:: mini

      list(1 .. 3 >> "cake") :> [1, 2, 3, "c", "a", "k", "e"]


:mini:`meth >>(Sequence: sequence): Sequence`
   Returns an sequence that repeatedly produces the values from :mini:`Sequence` (for use with :mini:`limit`).

   .. code-block:: mini

      list(>>(1 .. 3) limit 10) :> [1, 2, 3, 1, 2, 3, 1, 2, 3, 1]


:mini:`meth (Sequence: sequence) ^ (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.

   .. deprecated:: 2.5.0

      Use :mini:`->>` instead.

   .. code-block:: mini

      list(1 .. 5 ^ (1 .. _)) :> [1, 1, 2, 1, 2, 3, 1, 2, 3, 4, 1, 2, 3, 4, 5]


