iterator
========

.. include:: <isonum.txt>

**type** :mini:`chainedstate`
   *Defined at line 48 in src/ml_iterfns.c*

**type** :mini:`chainedfunction`
   :Parents: :mini:`function`

   *Defined at line 50 in src/ml_iterfns.c*

**method** :mini:`iteratable Iteratable >> function Function` |rarr| :mini:`chainedfunction`
   *Defined at line 119 in src/ml_iterfns.c*

**method** :mini:`chainedfunction ChainedFunction >> function Function` |rarr| :mini:`chainedfunction`
   *Defined at line 130 in src/ml_iterfns.c*

**function** :mini:`first(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the first value produced by :mini:`Iteratable`.

   *Defined at line 156 in src/ml_iterfns.c*

**function** :mini:`first2(iteratable Iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the first key and value produced by :mini:`Iteratable`.

   *Defined at line 189 in src/ml_iterfns.c*

**function** :mini:`last(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the last value produced by :mini:`Iteratable`.

   *Defined at line 219 in src/ml_iterfns.c*

**function** :mini:`last2(iteratable Iteratable)` |rarr| :mini:`tuple(any, any)` or :mini:`nil`
   Returns the last key and value produced by :mini:`Iteratable`.

   *Defined at line 265 in src/ml_iterfns.c*

**function** :mini:`count(iteratable Iteratable)` |rarr| :mini:`integer`
   Returns the count of the values produced by :mini:`Iteratable`.

   *Defined at line 387 in src/ml_iterfns.c*

**function** :mini:`fold(function Function, iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`function(function( ... function(V₁, V₂), V₃) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   *Defined at line 447 in src/ml_iterfns.c*

**function** :mini:`min(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.

   *Defined at line 463 in src/ml_iterfns.c*

**function** :mini:`max(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`>`) produced by :mini:`Iteratable`.

   *Defined at line 477 in src/ml_iterfns.c*

**function** :mini:`sum(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.

   *Defined at line 491 in src/ml_iterfns.c*

**function** :mini:`prod(iteratable Iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.

   *Defined at line 505 in src/ml_iterfns.c*

**function** :mini:`fold2(function Arg₁, iteratable Arg₂)`
   *Defined at line 587 in src/ml_iterfns.c*

**function** :mini:`min2(iteratable Arg₁)`
   *Defined at line 599 in src/ml_iterfns.c*

**function** :mini:`max2(iteratable Arg₁)`
   *Defined at line 610 in src/ml_iterfns.c*

**type** :mini:`folded`
   :Parents: :mini:`iteratable`

   *Defined at line 626 in src/ml_iterfns.c*

**type** :mini:`foldedstate`
   *Defined at line 628 in src/ml_iterfns.c*

**method** :mini:`iteratable Arg₁ // function Arg₂`
   *Defined at line 696 in src/ml_iterfns.c*

**type** :mini:`limited`
   :Parents: :mini:`iteratable`

   *Defined at line 710 in src/ml_iterfns.c*

**type** :mini:`limitedstate`
   *Defined at line 718 in src/ml_iterfns.c*

**method** :mini:`iteratable Arg₁:limit(integer Arg₂)`
   *Defined at line 760 in src/ml_iterfns.c*

**type** :mini:`skipped`
   :Parents: :mini:`iteratable`

   *Defined at line 774 in src/ml_iterfns.c*

**method** :mini:`iteratable Arg₁:skip(integer Arg₂)`
   *Defined at line 805 in src/ml_iterfns.c*

**type** :mini:`tasks`
   :Parents: :mini:`function`

   *Defined at line 829 in src/ml_iterfns.c*

**function** :mini:`tasks()`
   *Defined at line 838 in src/ml_iterfns.c*

**method** :mini:`tasks Arg₁:add(any Arg₂)`
   *Defined at line 849 in src/ml_iterfns.c*

**method** :mini:`tasks Arg₁:wait`
   *Defined at line 860 in src/ml_iterfns.c*

**function** :mini:`parallel(iteratable Iteratable, integer Max?, integer Min?, function Function)` |rarr| :mini:`nil` or :mini:`error`
   Iterates through :mini:`Iteratable` and calls :mini:`Function(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.

   The call to :mini:`parallel` returns when all calls to :mini:`Function` return, or an error occurs.

   If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Function` will run at a time by pausing iteration through :mini:`Iteratable`.

   If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Function` drops to :mini:`Min`.

   *Defined at line 926 in src/ml_iterfns.c*

**type** :mini:`unique`
   :Parents: :mini:`iteratable`

   *Defined at line 979 in src/ml_iterfns.c*

**type** :mini:`uniquestate`
   *Defined at line 989 in src/ml_iterfns.c*

**function** :mini:`unique(any Arg₁)`
   *Defined at line 1036 in src/ml_iterfns.c*

**type** :mini:`grouped`
   :Parents: :mini:`iteratable`

   *Defined at line 1051 in src/ml_iterfns.c*

**type** :mini:`groupedstate`
   *Defined at line 1061 in src/ml_iterfns.c*

**function** :mini:`group(any Arg₁)`
   *Defined at line 1122 in src/ml_iterfns.c*

**type** :mini:`repeated`
   :Parents: :mini:`iteratable`

   *Defined at line 1138 in src/ml_iterfns.c*

**type** :mini:`repeatedstate`
   *Defined at line 1146 in src/ml_iterfns.c*

**function** :mini:`repeat(any Arg₁)`
   *Defined at line 1183 in src/ml_iterfns.c*

**type** :mini:`sequenced`
   :Parents: :mini:`iteratable`

   *Defined at line 1197 in src/ml_iterfns.c*

**type** :mini:`sequencedstate`
   *Defined at line 1204 in src/ml_iterfns.c*

**method** :mini:`iteratable Arg₁ || iteratable Arg₂`
   *Defined at line 1240 in src/ml_iterfns.c*

**method** :mini:`|| iteratable Arg₁`
   *Defined at line 1248 in src/ml_iterfns.c*

**type** :mini:`swapped`
   :Parents: :mini:`iteratable`

   *Defined at line 1261 in src/ml_iterfns.c*

**type** :mini:`swappedstate`
   *Defined at line 1268 in src/ml_iterfns.c*

**method** :mini:`iteratable Arg₁:swap`
   *Defined at line 1298 in src/ml_iterfns.c*

**type** :mini:`iteratable`
   The base type for any iteratable value.

   *Defined at line 52 in src/ml_types.c*

