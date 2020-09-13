iterator
========

.. include:: <isonum.txt>

:mini:`filter`
   :Parents: :mini:`function`

   *Defined at line 21 in src/ml_iterfns.c*

:mini:`fun filter(?Function: any)` |rarr| :mini:`filter`
   Returns a filter for use in chained functions and iterators.

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

:mini:`meth >>(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 327 in src/ml_iterfns.c*

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

:mini:`fun reduce2(Arg₁: iteratable, Arg₂: function)`
:mini:`fun min2(Arg₁: iteratable)`
:mini:`fun max2(Arg₁: iteratable)`
:mini:`stacked`
   :Parents: :mini:`iteratable`

   *Defined at line 851 in src/ml_iterfns.c*

:mini:`stackedstate`
   *Defined at line 853 in src/ml_iterfns.c*

:mini:`meth //(Iteratable: iteratable, Reducer: function)` |rarr| :mini:`stacked`
   *Defined at line 919 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 936 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 944 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 986 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1000 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 1031 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 1060 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 1075 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 1099 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1114 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1233 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1243 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1306 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1316 in src/ml_iterfns.c*

:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1401 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1409 in src/ml_iterfns.c*

:mini:`fun repeat(Value: any, ?Update: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   If :mini:`Update` is provided then :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1466 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1473 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1509 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1517 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1530 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1537 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1567 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 56 in src/ml_types.c*

