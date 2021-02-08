iterator
========

.. include:: <isonum.txt>

:mini:`type filter < function`

:mini:`fun filter(Function?: any)` |rarr| :mini:`filter`
   Returns a filter for use in chained functions and iterators.


:mini:`type chainedfunction < function, iteratable`

:mini:`type chainedstate`

:mini:`meth ->(Iteratable: function, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>(Iteratable: iteratable, Function: function, Arg₃: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>(ChainedFunction: chainedfunction, Function: function, Arg₃: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->?(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>?(Iteratable: iteratable, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth ->?(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`meth =>?(ChainedFunction: chainedfunction, Function: function)` |rarr| :mini:`chainedfunction`

:mini:`type doublediterator < iteratable`

:mini:`type doublediteratorstate < state`

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


:mini:`fun reduce(Initial?: any, Iteratable: iteratable, Fn: function)` |rarr| :mini:`any` or :mini:`nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Iteratable`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Iteratable` is used.


:mini:`fun min(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.


:mini:`fun max(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the largest value (based on :mini:`>`) produced by :mini:`Iteratable`.


:mini:`fun sum(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the sum of the values (based on :mini:`+`) produced by :mini:`Iteratable`.


:mini:`fun prod(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns the product of the values (based on :mini:`*`) produced by :mini:`Iteratable`.


:mini:`meth :join(Iteratable: iteratable, Separator: string)` |rarr| :mini:`string`
   Joins the elements of :mini:`Iteratable` into a string using :mini:`Separator` between elements.


:mini:`fun reduce2(Iteratable: iteratable, Fn: function)` |rarr| :mini:`any` or :mini:`nil`

:mini:`fun min2(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns a tuple with the key and value of the smallest value (based on :mini:`<`) produced by :mini:`Iteratable`.


:mini:`fun max2(Iteratable: iteratable)` |rarr| :mini:`any` or :mini:`nil`
   Returns a tuple with the key and value of the largest value (based on :mini:`>`) produced by :mini:`Iteratable`.


:mini:`meth //(Iteratable: iteratable, Fn: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that produces :mini:`V₁`, :mini:`Fn(V₁, V₂)`, :mini:`Fn(Fn(V₁, V₂), V₃)`, ... .


:mini:`meth //(Iteratable: iteratable, Initial: any, Fn: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that produces :mini:`Initial`, :mini:`Fn(Initial, V₁)`, :mini:`Fn(Fn(Initial, V₁), V₂)`, ... .


:mini:`meth @(Value: any)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.


:mini:`meth @(Value: any, Update: function)` |rarr| :mini:`iteratable`
   Returns an iteratable that repeatedly produces :mini:`Value`.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.


:mini:`meth >>(Iteratable₁: iteratable, Iteratable₂: iteratable)` |rarr| :mini:`Iteratable`
   Returns an iteratable that produces the values from :mini:`Iteratable₁` followed by those from :mini:`Iteratable₂`.


:mini:`meth >>(Iteratable: iteratable)` |rarr| :mini:`Iteratable`
   Returns an iteratable that repeatedly produces the values from :mini:`Iteratable` (for use with :mini:`limit`).


:mini:`meth :limit(Iteratable: iteratable, Limit: integer)` |rarr| :mini:`iteratable`
   Returns an iteratable that produces at most :mini:`Limit` values from :mini:`Iteratable`.


:mini:`meth :skip(Iteratable: iteratable, Skip: integer)` |rarr| :mini:`iteratable`
   Returns an iteratable that skips the first :mini:`Skip` values from :mini:`Iteratable` and then produces the rest.


:mini:`fun unique(Iteratable: any)` |rarr| :mini:`iteratable`
   Returns an iteratable that returns the unique values produced by :mini:`Iteratable` (based on inserting into a :mini:`map`).


:mini:`fun zip(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable, Function: any)` |rarr| :mini:`iteratable`
   Returns a new iteratable that draws values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.


:mini:`fun pair(Iteratable₁: iteratable, Iteratable₂: iteratable)` |rarr| :mini:`iteratable`

:mini:`fun weave(Iteratable₁: iteratable, ...: iteratable, Iteratableₙ: iteratable)` |rarr| :mini:`iteratable`
   Returns a new iteratable that produces interleaved values :mini:`Vᵢ` from each of :mini:`Iteratableᵢ`.

   The iteratable stops produces values when any of the :mini:`Iteratableᵢ` stops.


:mini:`fun swap(Iteratable: iteratable)`
   Returns a new iteratable which swaps the keys and values produced by :mini:`Iteratable`.


:mini:`fun key(Iteratable: iteratable)`
   Returns a new iteratable which produces the keys of :mini:`Iteratable`.


:mini:`type iteratable`
   The base type for any iteratable value.


