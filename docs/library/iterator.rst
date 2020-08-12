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

   *Defined at line 518 in src/ml_iterfns.c*

:mini:`fun fold(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(V₁, V₂), V₃) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   *Defined at line 578 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 607 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 621 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 635 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 649 in src/ml_iterfns.c*

:mini:`fun fold2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 731 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 743 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 754 in src/ml_iterfns.c*

:mini:`folded`
   :Parents: :mini:`iteratable`

   *Defined at line 770 in src/ml_iterfns.c*

:mini:`foldedstate`
   *Defined at line 772 in src/ml_iterfns.c*

:mini:`meth //(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 840 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 854 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 862 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 904 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 918 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 949 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 973 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 982 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 993 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1004 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1070 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1123 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1133 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1180 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1195 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1205 in src/ml_iterfns.c*

:mini:`fun zip(Arg₁: any)`
   *Defined at line 1266 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1283 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1291 in src/ml_iterfns.c*

:mini:`fun repeat(Arg₁: any)`
   *Defined at line 1329 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1343 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1350 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1386 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1394 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1407 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1414 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1444 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 52 in src/ml_types.c*

