iterator
========

.. include:: <isonum.txt>

:mini:`filter`
   :Parents: :mini:`function`

   *Defined at line 21 in src/ml_iterfns.c*

:mini:`fun filter(?Function: any)` |rarr| :mini:`filter`
   Returns a filter for use in chained functions and iterators.

   *Defined at line 28 in src/ml_iterfns.c*

:mini:`chainedstate`
   *Defined at line 97 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 99 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 180 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 191 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: function, Function: chainedfunction)` |rarr| :mini:`chainedfunction`
   *Defined at line 202 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 216 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction₁: chainedfunction, ChainedFunction₂: chainedfunction)` |rarr| :mini:`chainedfunction`
   *Defined at line 230 in src/ml_iterfns.c*

:mini:`doublediterator`
   :Parents: :mini:`iteratable`

   *Defined at line 254 in src/ml_iterfns.c*

:mini:`doublediteratorstate`
   :Parents: :mini:`state`

   *Defined at line 264 in src/ml_iterfns.c*

:mini:`meth ^(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 327 in src/ml_iterfns.c*

:mini:`fun all(Iteratable: iteratable)` |rarr| :mini:`some` or :mini:`nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Iterable`. Otherwise returns :mini:`some`.

   *Defined at line 360 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 381 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 414 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 444 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 490 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 601 in src/ml_iterfns.c*

:mini:`fun count2(Iteratable: iteratable)` |rarr| :mini:`map`
   Returns a map of the values produced by :mini:`Iteratable` with associated counts.

   *Defined at line 640 in src/ml_iterfns.c*

:mini:`fun reduce(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.

   *Defined at line 698 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 728 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 742 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 756 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 770 in src/ml_iterfns.c*

:mini:`meth *(Arg₁: string, Arg₂: iteratable)`
   *Defined at line 816 in src/ml_iterfns.c*

:mini:`fun reduce2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 894 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 906 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 917 in src/ml_iterfns.c*

:mini:`stacked`
   :Parents: :mini:`iteratable`

   *Defined at line 933 in src/ml_iterfns.c*

:mini:`stackedstate`
   *Defined at line 935 in src/ml_iterfns.c*

:mini:`meth //(Iteratable: iteratable, Reducer: function)` |rarr| :mini:`stacked`
   *Defined at line 1001 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 1018 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 1026 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 1068 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1082 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 1113 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 1142 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 1157 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 1181 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1196 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1262 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1315 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1325 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1373 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1388 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1398 in src/ml_iterfns.c*

:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.

   *Defined at line 1459 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1483 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1491 in src/ml_iterfns.c*

:mini:`meth @(Value: any)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   *Defined at line 1529 in src/ml_iterfns.c*

:mini:`meth @(Value: any, Update: function, Arg₃: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.

   *Defined at line 1540 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1559 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1566 in src/ml_iterfns.c*

:mini:`meth >>(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1602 in src/ml_iterfns.c*

:mini:`meth >>(Arg₁: iteratable)`
   *Defined at line 1610 in src/ml_iterfns.c*

:mini:`weaved`
   :Parents: :mini:`iteratable`

   *Defined at line 1624 in src/ml_iterfns.c*

:mini:`weavedstate`
   *Defined at line 1632 in src/ml_iterfns.c*

:mini:`fun weave(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable)` |rarr| :mini:`iteratable`
   Returns a new iteratable that produces interleaved values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.

   *Defined at line 1671 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1692 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1699 in src/ml_iterfns.c*

:mini:`fun swap(Iteratable: iteratable)`
   Returns a new iteratable which swaps the keys and values produced by :mini:`Iteratable`.

   *Defined at line 1729 in src/ml_iterfns.c*

:mini:`key`
   :Parents: :mini:`iteratable`

   *Defined at line 1746 in src/ml_iterfns.c*

:mini:`keystate`
   *Defined at line 1754 in src/ml_iterfns.c*

:mini:`fun swap(Iteratable: iteratable)`
   Returns a new iteratable which produces the keys of :mini:`Iteratable`.

   *Defined at line 1785 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 50 in src/ml_types.c*

