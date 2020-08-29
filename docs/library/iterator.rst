iterator
========

.. include:: <isonum.txt>

:mini:`filter`
   :Parents: :mini:`function`

   *Defined at line 21 in src/ml_iterfns.c*

:mini:`fun filter(Function: any)` |rarr| :mini:`filter`
   Returns a filter for use in chained functions and iterators.

   *Defined at line 26 in src/ml_iterfns.c*

:mini:`chainedstate`
   *Defined at line 96 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 98 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 182 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 193 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 204 in src/ml_iterfns.c*

:mini:`doublediterator`
   :Parents: :mini:`iteratable`

   *Defined at line 226 in src/ml_iterfns.c*

:mini:`doublediteratorstate`
   :Parents: :mini:`state`

   *Defined at line 236 in src/ml_iterfns.c*

:mini:`meth ->>(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 299 in src/ml_iterfns.c*

:mini:`fun all(Iteratable: iteratable)` |rarr| :mini:`some` or :mini:`nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Iterable`. Otherwise returns :mini:`some`.

   *Defined at line 332 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 353 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 386 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 416 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 462 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 573 in src/ml_iterfns.c*

:mini:`fun reduce(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.

   *Defined at line 631 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 661 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 675 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 689 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 703 in src/ml_iterfns.c*

:mini:`fun reduce2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 784 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 796 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 807 in src/ml_iterfns.c*

:mini:`stacked`
   :Parents: :mini:`iteratable`

   *Defined at line 823 in src/ml_iterfns.c*

:mini:`stackedstate`
   *Defined at line 825 in src/ml_iterfns.c*

:mini:`meth //(Iteratable: iteratable, Reducer: function)` |rarr| :mini:`stacked`
   *Defined at line 891 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 908 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 916 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 958 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 972 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 1003 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 1032 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 1047 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 1071 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1086 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1152 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1205 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1215 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1263 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1278 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1288 in src/ml_iterfns.c*

:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.

   *Defined at line 1349 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1373 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1381 in src/ml_iterfns.c*

:mini:`fun repeat(Value: any, ?Update: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   If :mini:`Update` is provided then :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.

   *Defined at line 1419 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1438 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1445 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1481 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1489 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1502 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1509 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1539 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 56 in src/ml_types.c*

