iterator
========

.. include:: <isonum.txt>

:mini:`chainedstate`
   *Defined at line 48 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`

   *Defined at line 50 in src/ml_iterfns.c*

:mini:`meth >>(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 119 in src/ml_iterfns.c*

:mini:`meth >>(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 130 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 156 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 189 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 219 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 265 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 387 in src/ml_iterfns.c*

:mini:`fun fold(Function: function, Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(V₁, V₂), V₃) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   *Defined at line 447 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 463 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 477 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 491 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 505 in src/ml_iterfns.c*

:mini:`fun fold2(Arg₁: function, Arg₂: iteratable)`
   *Defined at line 587 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 599 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 610 in src/ml_iterfns.c*

:mini:`folded`
   :Parents: :mini:`iteratable`

   *Defined at line 626 in src/ml_iterfns.c*

:mini:`foldedstate`
   *Defined at line 628 in src/ml_iterfns.c*

:mini:`meth //(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 696 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 710 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 718 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 760 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 774 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 805 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 829 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 838 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 849 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 860 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 926 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 979 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 989 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1036 in src/ml_iterfns.c*

:mini:`grouped`
   :Parents: :mini:`iteratable`

   *Defined at line 1051 in src/ml_iterfns.c*

:mini:`groupedstate`
   *Defined at line 1061 in src/ml_iterfns.c*

:mini:`fun group(Arg₁: any)`
   *Defined at line 1122 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1138 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1146 in src/ml_iterfns.c*

:mini:`fun repeat(Arg₁: any)`
   *Defined at line 1183 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1197 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1204 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1240 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1248 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1261 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1268 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1298 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 52 in src/ml_types.c*

