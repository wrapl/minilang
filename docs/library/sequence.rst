.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

sequence
========

.. _fun-all:

:mini:`fun all(Sequence: sequence): some | nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Sequence`. Otherwise returns :mini:`some`. If :mini:`Sequence` is empty,  then :mini:`some` is returned.



:mini:`meth @(Value: any): sequence`
   Returns an sequence that repeatedly produces :mini:`Value`.



:mini:`meth (Value: any) @ (Update: function): sequence`
   Returns an sequence that repeatedly produces :mini:`Value`.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.



.. _fun-batch:

:mini:`fun batch(Sequence: sequence, Size: integer, Shift?: integer, Function: function): sequence`
   Returns a new sequence that calls :mini:`Function` with each batch of :mini:`Size` values produced by :mini:`Sequence` and produces the results.



.. _fun-buffered:

:mini:`fun buffered(Size: integer, Sequence: any): sequence`
   Returns an sequence that buffers the keys and values from :mini:`Sequence` in advance,  buffering at most :mini:`Size` pairs.



.. _type-chained:

:mini:`type chained < function, sequence`
   A chained function or sequence,  consisting of a base function or sequence and any number of additional functions or filters.

   When used as a function or sequence,  the base is used to produce an initial result,  then the additional functions are applied in turn to the result.

   Filters do not affect the result but will shortcut a function call or skip an iteration if :mini:`nil` is returned. I.e. filters remove values from a sequence that fail a condition without affecting the values that pass.



.. _fun-chained:

:mini:`fun chained(Base: any, Fn₁, : function, ...): chained`
   Returns a new chained function or sequence with base :mini:`Base` and additional functions or filters :mini:`Fn₁,  ...,  Fnₙ`.



.. _fun-count:

:mini:`fun count(Sequence: any): integer`
   Returns the count of the values produced by :mini:`Sequence`.



.. _fun-count2:

:mini:`fun count2(Sequence: sequence): map`
   Returns a map of the values produced by :mini:`Sequence` with associated counts.



.. _fun-extremum:

:mini:`fun extremum(Sequence: sequence, Fn: function): tuple | nil`
   *TBD*


.. _type-filter:

:mini:`type filter < function`
   A function marked as a filter when used in a chained function or sequence.



.. _fun-filter:

:mini:`fun filter(Function?: any): filter`
   Returns a filter for use in chained functions and sequences.



.. _fun-first:

:mini:`fun first(Sequence: sequence): any | nil`
   Returns the first value produced by :mini:`Sequence`.



.. _fun-first2:

:mini:`fun first2(Sequence: sequence): tuple(any,  any) | nil`
   Returns the first key and value produced by :mini:`Sequence`.



.. _fun-fold:

:mini:`fun fold(Sequence: sequence): sequence`
   Returns a new sequence that treats alternating values produced by :mini:`Sequence` as keys and values respectively.



:mini:`meth (Base: function) -> (Function: function): sequence`
   Returns a chained function equivalent to :mini:`Function(Base(...))`.



.. _fun-grid:

:mini:`fun grid(Sequence₁, : sequence, ..., Function: any): sequence`
   Returns a new sequence that produces :mini:`Function(V₁,  V₂,  ...,  Vₙ)` for all possible combinations of :mini:`V₁,  ...,  Vₙ`,  where :mini:`Vᵢ` are the values produced by :mini:`Sequenceᵢ`.



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



.. _fun-key:

:mini:`fun key(Sequence: sequence)`
   Returns a new sequence which produces the keys of :mini:`Sequence`.



.. _fun-last:

:mini:`fun last(Sequence: sequence): any | nil`
   Returns the last value produced by :mini:`Sequence`.



.. _fun-last2:

:mini:`fun last2(Sequence: sequence): tuple(any,  any) | nil`
   Returns the last key and value produced by :mini:`Sequence`.



.. _fun-max:

:mini:`fun max(Sequence: sequence): any | nil`
   Returns the largest value (using :mini:`>`) produced by :mini:`Sequence`.



.. _fun-max2:

:mini:`fun max2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the largest value (using :mini:`>`) produced by :mini:`Sequence`.



.. _fun-min:

:mini:`fun min(Sequence: sequence): any | nil`
   Returns the smallest value (using :mini:`<`) produced by :mini:`Sequence`.



.. _fun-min2:

:mini:`fun min2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the smallest value (using :mini:`<`) produced by :mini:`Sequence`.



.. _fun-pair:

:mini:`fun pair(Sequence₁: sequence, Sequence₂: sequence): sequence`
   Returns a new sequence that produces the values from :mini:`Sequence₁` as keys and the values from :mini:`Sequence₂` as values.



.. _fun-prod:

:mini:`fun prod(Sequence: sequence): any | nil`
   Returns the product of the values (using :mini:`*`) produced by :mini:`Sequence`.



.. _fun-reduce:

:mini:`fun reduce(Initial?: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial,  V₁),  V₂) ...,  Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Sequence`.

   If :mini:`Initial` is omitted,  first value produced by :mini:`Sequence` is used.



.. _fun-reduce2:

:mini:`fun reduce2(Initial: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial,  K₁,  V₁),  K₂,  V₂) ...,  Kₙ,  Vₙ)` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Sequence`.



.. _type-sequence:

:mini:`type sequence`
   The base type for any sequence value.



:mini:`meth (Base: sequence) !> (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F ! V₁),  ...,  (Kₙ,  F ! Vₙ)` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.



:mini:`meth (Base: sequence) !>? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F ! Vⱼ` returns non-:mini:`nil`.



:mini:`meth (Base: sequence) -> (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F(V₁)),  ...,  (Kₙ,  F(Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.



:mini:`meth (Sequence: sequence) ->> (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.



:mini:`meth (Base: sequence) ->? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F(Vⱼ)` returns non-:mini:`nil`.



:mini:`meth (Sequence: sequence) // (Fn: function): sequence`
   Returns an sequence that produces :mini:`V₁`,  :mini:`Fn(V₁,  V₂)`,  :mini:`Fn(Fn(V₁,  V₂),  V₃)`,  ... .



:mini:`meth (Sequence: sequence) // (Initial: any, Fn: function): sequence`
   Returns an sequence that produces :mini:`Initial`,  :mini:`Fn(Initial,  V₁)`,  :mini:`Fn(Fn(Initial,  V₁),  V₂)`,  ... .



:mini:`meth (Sequence: sequence):join(Separator: string): string`
   Joins the elements of :mini:`Sequence` into a string using :mini:`Separator` between elements.



:mini:`meth (Sequence: sequence):join: string`
   Joins the elements of :mini:`Sequence` into a string.



:mini:`meth (Sequence: sequence):limit(Fn: function): sequence`
   Returns an sequence that stops when :mini:`Fn(Value)` is non-:mini:`nil`.



:mini:`meth (Sequence: sequence):limit(Limit: integer): sequence`
   Returns an sequence that produces at most :mini:`Limit` values from :mini:`Sequence`.



:mini:`meth (Sequence: sequence):skip(Skip: integer): sequence`
   Returns an sequence that skips the first :mini:`Skip` values from :mini:`Sequence` and then produces the rest.



:mini:`meth (Base: sequence) => (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(K₁,  F(K₁,  V₁)),  ...,  (Kₙ,  F(Kₙ,  Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.



:mini:`meth (Base: sequence) => (F₁: function, F₂: function): sequence`
   Returns a chained sequence equivalent to :mini:`(F₁(K₁,  V₁),  F₂(K₁,  V₁)),  ...,  (F₁(Kₙ,  Vₙ),  F₂(Kₙ,  Vₙ))` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base`.



:mini:`meth (Sequence: sequence) =>> (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Key,  Value)` for each key and value generated by :mini:`Sequence`.



:mini:`meth (Base: sequence) =>? (F: function): sequence`
   Returns a chained sequence equivalent to :mini:`(Kⱼ,  Vⱼ),  ...` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Base` and :mini:`F(Kⱼ,  Vⱼ)` returns non-:mini:`nil`.



:mini:`meth >>(Sequence: sequence): Sequence`
   Returns an sequence that repeatedly produces the values from :mini:`Sequence` (for use with :mini:`limit`).



:mini:`meth (Sequence₁: sequence) >> (Sequence₂: sequence): Sequence`
   Returns an sequence that produces the values from :mini:`Sequence₁` followed by those from :mini:`Sequence₂`.



:mini:`meth (Sequence: sequence) ^ (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.

   .. deprecated:: 2.5.0

      Use :mini:`->>` instead.



.. _fun-sum:

:mini:`fun sum(Sequence: sequence): any | nil`
   Returns the sum of the values (using :mini:`+`) produced by :mini:`Sequence`.



.. _fun-swap:

:mini:`fun swap(Sequence: sequence)`
   Returns a new sequence which swaps the keys and values produced by :mini:`Sequence`.



.. _fun-unfold:

:mini:`fun unfold(Sequence: sequence): sequence`
   Returns a new sequence that treats produces alternatively the keys and values produced by :mini:`Sequence`.



.. _fun-unique:

:mini:`fun unique(Sequence: any): sequence`
   Returns an sequence that returns the unique values produced by :mini:`Sequence`. Uniqueness is determined by using a :mini:`map`.



.. _fun-weave:

:mini:`fun weave(Sequence₁, : sequence, ...): sequence`
   Returns a new sequence that produces interleaved values :mini:`Vᵢ` from each of :mini:`Sequenceᵢ`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.



.. _fun-zip:

:mini:`fun zip(Sequence₁, : sequence, ..., Function: any): sequence`
   Returns a new sequence that produces :mini:`Function(V₁₁,  ...,  Vₙ₁),  Function(V₁₂,  ...,  Vₙ₂),  ...` where :mini:`Vᵢⱼ` is the :mini:`j`-th value produced by :mini:`Sequenceᵢ`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.



