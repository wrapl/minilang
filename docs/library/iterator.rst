iterator
========

.. include:: <isonum.txt>

:mini:`chainedstate`
   *Defined at line 60 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 62 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 132 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 143 in src/ml_iterfns.c*

:mini:`meth >-(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 154 in src/ml_iterfns.c*

:mini:`meth >-(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 166 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 179 in src/ml_iterfns.c*

:mini:`meth >-(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 194 in src/ml_iterfns.c*

:mini:`doublediterator`
   :Parents: :mini:`iteratable`

   *Defined at line 217 in src/ml_iterfns.c*

:mini:`doublediteratorstate`
   :Parents: :mini:`state`

   *Defined at line 227 in src/ml_iterfns.c*

:mini:`meth ->>(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 290 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 312 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 346 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 377 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 424 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 547 in src/ml_iterfns.c*

:mini:`fun reduce(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.

   *Defined at line 607 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 637 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 651 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 665 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 679 in src/ml_iterfns.c*

:mini:`fun reduce2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 762 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 774 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 785 in src/ml_iterfns.c*

:mini:`stacked`
   :Parents: :mini:`iteratable`

   *Defined at line 801 in src/ml_iterfns.c*

:mini:`stackedstate`
   *Defined at line 803 in src/ml_iterfns.c*

:mini:`meth //(Iteratable: iteratable, Reducer: function)` |rarr| :mini:`stacked`
   *Defined at line 872 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 889 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 897 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 939 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 953 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 984 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 1013 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 1028 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 1052 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1067 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1133 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1186 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1196 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1244 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1259 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1269 in src/ml_iterfns.c*

:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.

   *Defined at line 1331 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1355 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1363 in src/ml_iterfns.c*

:mini:`fun repeat(Value: any, ?Update: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   If :mini:`Update` is provided then :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.

   *Defined at line 1401 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1420 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1427 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1463 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1471 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1484 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1491 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1521 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 56 in src/ml_types.c*

