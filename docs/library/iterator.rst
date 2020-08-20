iterator
========

.. include:: <isonum.txt>

:mini:`chainedstate`
   *Defined at line 60 in src/ml_iterfns.c*

:mini:`chainedfunction`
   :Parents: :mini:`function`, :mini:`iteratable`

   *Defined at line 62 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 131 in src/ml_iterfns.c*

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 142 in src/ml_iterfns.c*

:mini:`meth ?>(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 153 in src/ml_iterfns.c*

:mini:`meth ?>(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 165 in src/ml_iterfns.c*

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 178 in src/ml_iterfns.c*

:mini:`meth ?>(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`
   *Defined at line 193 in src/ml_iterfns.c*

:mini:`doublediterator`
   :Parents: :mini:`iteratable`

   *Defined at line 216 in src/ml_iterfns.c*

:mini:`doublediteratorstate`
   :Parents: :mini:`state`

   *Defined at line 226 in src/ml_iterfns.c*

:mini:`meth ->>(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 288 in src/ml_iterfns.c*

:mini:`fun first(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 310 in src/ml_iterfns.c*

:mini:`fun first2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 343 in src/ml_iterfns.c*

:mini:`fun last(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 373 in src/ml_iterfns.c*

:mini:`fun last2(Iteratable: iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 419 in src/ml_iterfns.c*

:mini:`fun count(Iteratable: iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 542 in src/ml_iterfns.c*

:mini:`fun fold(?Initial: any, Iteratable: iteratable, Function: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(V₁, V₂), V₃) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   *Defined at line 602 in src/ml_iterfns.c*

:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 631 in src/ml_iterfns.c*

:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 645 in src/ml_iterfns.c*

:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 659 in src/ml_iterfns.c*

:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 673 in src/ml_iterfns.c*

:mini:`fun fold2(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 755 in src/ml_iterfns.c*

:mini:`fun min2(Arg₁: iteratable)`
   *Defined at line 767 in src/ml_iterfns.c*

:mini:`fun max2(Arg₁: iteratable)`
   *Defined at line 778 in src/ml_iterfns.c*

:mini:`folded`
   :Parents: :mini:`iteratable`

   *Defined at line 794 in src/ml_iterfns.c*

:mini:`foldedstate`
   *Defined at line 796 in src/ml_iterfns.c*

:mini:`meth //(Arg₁: iteratable, Arg₂: function)`
   *Defined at line 864 in src/ml_iterfns.c*

:mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 878 in src/ml_iterfns.c*

:mini:`limitedstate`
   *Defined at line 886 in src/ml_iterfns.c*

:mini:`meth :limit(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 928 in src/ml_iterfns.c*

:mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 942 in src/ml_iterfns.c*

:mini:`meth :skip(Arg₁: iteratable, Arg₂: integer)`
   *Defined at line 973 in src/ml_iterfns.c*

:mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 997 in src/ml_iterfns.c*

:mini:`fun tasks()`
   *Defined at line 1006 in src/ml_iterfns.c*

:mini:`meth :add(Arg₁: tasks, Arg₂: any)`
   *Defined at line 1017 in src/ml_iterfns.c*

:mini:`meth :wait(Arg₁: tasks)`
   *Defined at line 1028 in src/ml_iterfns.c*

:mini:`fun parallel(Iteratable: iteratable, Max: ?integer, Min: ?integer, Function: function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 1094 in src/ml_iterfns.c*

:mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 1147 in src/ml_iterfns.c*

:mini:`uniquestate`
   *Defined at line 1157 in src/ml_iterfns.c*

:mini:`fun unique(Arg₁: any)`
   *Defined at line 1204 in src/ml_iterfns.c*

:mini:`zipped`
   :Parents: :mini:`iteratable`

   *Defined at line 1219 in src/ml_iterfns.c*

:mini:`zippedstate`
   *Defined at line 1229 in src/ml_iterfns.c*

:mini:`fun zip(Arg₁: any)`
   *Defined at line 1290 in src/ml_iterfns.c*

:mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1307 in src/ml_iterfns.c*

:mini:`repeatedstate`
   *Defined at line 1315 in src/ml_iterfns.c*

:mini:`fun repeat(Arg₁: any)`
   *Defined at line 1353 in src/ml_iterfns.c*

:mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1367 in src/ml_iterfns.c*

:mini:`sequencedstate`
   *Defined at line 1374 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable, Arg₂: iteratable)`
   *Defined at line 1410 in src/ml_iterfns.c*

:mini:`meth ||(Arg₁: iteratable)`
   *Defined at line 1418 in src/ml_iterfns.c*

:mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1431 in src/ml_iterfns.c*

:mini:`swappedstate`
   *Defined at line 1438 in src/ml_iterfns.c*

:mini:`meth :swap(Arg₁: iteratable)`
   *Defined at line 1468 in src/ml_iterfns.c*

:mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 52 in src/ml_types.c*

