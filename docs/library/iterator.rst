iterator
========

.. include:: <isonum.txt>

:mini:`filter`
   :Parents: :mini:`function`


:mini:`fun filter(?Function: any)` |rarr| :mini:`filter`
   Returns a filter for use in chained functions and iterators.


:mini:`chainedstate`

:mini:`chainedfunction`
   :Parents: :mini:`function`, :mini:`iteratable`


:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(Iteratable: function, Function: chainedfunction)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(ChainedFunction₁: chainedfunction, ChainedFunction₂: chainedfunction)` |rarr| :mini:`chainedfunction`

:mini:`doublediterator`
   :Parents: :mini:`iteratable`


:mini:`doublediteratorstate`
   :Parents: :mini:`state`


:mini:`meth ^(Arg₁: iteratable, Arg₂: function)`

:mini:`fun all(Iteratable: iteratable)` |rarr| :mini:`some` or :mini:`nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Iterable`. Otherwise returns :mini:`some`.


:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.


:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.


:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.


:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.


:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.


:mini:`fun count2(Iteratable: iteratable)` |rarr| :mini:`map`
   Returns a map of the values produced by :mini:`Iteratable` with associated counts.


:mini:`fun reduce(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.


:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.


:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.


:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.


:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.


:mini:`meth *(Arg₁: string, Arg₂: iteratable)`

:mini:`fun reduce2(Arg₁: iteratable, Arg₂: function)`

:mini:`fun min2(Arg₁: iteratable)`

:mini:`fun max2(Arg₁: iteratable)`

:mini:`stacked`
   :Parents: :mini:`iteratable`


:mini:`stackedstate`

:mini:`meth //(Iteratable: iteratable, Reducer: function)` |rarr| :mini:`stacked`

:mini:`limited`
   :Parents: :mini:`iteratable`


:mini:`limitedstate`

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`

:mini:`skipped`
   :Parents: :mini:`iteratable`


:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`

:mini:`tasks`
   :Parents: :mini:`function`


:mini:`fun tasks()`

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`

:mini:`meth :wait(Arg₁: tasks)`

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.


:mini:`unique`
   :Parents: :mini:`iteratable`


:mini:`uniquestate`

:mini:`fun unique(Arg₁: any)`

:mini:`zipped`
   :Parents: :mini:`iteratable`


:mini:`zippedstate`

:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.


:mini:`repeated`
   :Parents: :mini:`iteratable`


:mini:`repeatedstate`

:mini:`meth @(Value: any)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.


:mini:`meth @(Value: any, Update: function, Arg₃: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.


:mini:`sequenced`
   :Parents: :mini:`iteratable`


:mini:`sequencedstate`

:mini:`meth >>(Arg₁: iteratable, Arg₂: iteratable)`

:mini:`meth >>(Arg₁: iteratable)`

:mini:`weaved`
   :Parents: :mini:`iteratable`


:mini:`weavedstate`

:mini:`fun weave(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable)` |rarr| :mini:`iteratable`
   Returns a new iteratable that produces interleaved values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.


:mini:`swapped`
   :Parents: :mini:`iteratable`


:mini:`swappedstate`

:mini:`fun swap(Iteratable: iteratable)`
   Returns a new iteratable which swaps the keys and values produced by :mini:`Iteratable`.


:mini:`key`
   :Parents: :mini:`iteratable`


:mini:`keystate`

:mini:`fun swap(Iteratable: iteratable)`
   Returns a new iteratable which produces the keys of :mini:`Iteratable`.


:mini:`iteratable`
   The base type for any iteratable value.


