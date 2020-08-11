iterator
========

.. include:: <isonum.txt>

:mini:`chainedstate`
   *Defined at line 60 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`

   *Defined at line 62 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 131 in src/ml_iterfns.c*

:mini:`meth ?>(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 142 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 154 in src/ml_iterfns.c*

:mini:`meth ?>(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 169 in src/ml_iterfns.c*

:mini:`double`
   :Parents: :mini:`iteratable`

   *Defined at line 192 in src/ml_iterfns.c*

:mini:`doublestate`
   :Parents: :mini:`state`

   *Defined at line 202 in src/ml_iterfns.c*

:mini:`meth ->>(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 264 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 286 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 319 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 349 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 395 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 517 in src/ml_iterfns.c*

:mini:`fun fold(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(V₁, V₂), V₃) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   *Defined at line 577 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 606 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 620 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 634 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 648 in src/ml_iterfns.c*

:mini:`fun fold2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 730 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 742 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 753 in src/ml_iterfns.c*

:mini:`folded`
   :Parents: :mini:`iteratable`

   *Defined at line 769 in src/ml_iterfns.c*

:mini:`foldedstate`
   *Defined at line 771 in src/ml_iterfns.c*

:mini:`meth //(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 839 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 853 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 861 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 903 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 917 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 948 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 972 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 981 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 992 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1003 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1069 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1122 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1132 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1179 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1194 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1204 in src/ml_iterfns.c*

:mini:`fun zip(Arg₁: any)`
   *Defined at line 1265 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1282 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1290 in src/ml_iterfns.c*

:mini:`fun repeat(Arg₁: any)`
   *Defined at line 1328 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1342 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1349 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1385 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1393 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1406 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1413 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1443 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 52 in src/ml_types.c*

