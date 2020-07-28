iterator
========

.. include:: <isonum.txt>

**type** :mini:`chainediterator`

**type** :mini:`chainedfunction`
   :Parents: :mini:`function`


**method** :mini:`iteratable Arg₁ >> function Arg₂`

**method** :mini:`chainedfunction Arg₁ >> function Arg₂`

**function** :mini:`first(any Arg₁)`

**function** :mini:`first2(any Arg₁)`

**function** :mini:`last(any Arg₁)`

**function** :mini:`last2(any Arg₁)`

**function** :mini:`all(iteratable Arg₁)`

**method** :mini:`list(iteratable Arg₁)`

**function** :mini:`all2(iteratable Arg₁)`

**method** :mini:`map(iteratable Arg₁)`

**function** :mini:`count(any Arg₁)`

**function** :mini:`fold(any Arg₁, function Arg₂)`

**function** :mini:`min(any Arg₁)`

**function** :mini:`max(any Arg₁)`

**function** :mini:`sum(any Arg₁)`

**function** :mini:`prod(any Arg₁)`

**function** :mini:`fold2(any Arg₁, function Arg₂)`

**function** :mini:`min2(any Arg₁)`

**function** :mini:`max2(any Arg₁)`

**type** :mini:`folded`
   :Parents: :mini:`iteratable`


**method** :mini:`iteratable Arg₁ // function Arg₂`

**type** :mini:`limited`
   :Parents: :mini:`iteratable`


**method** :mini:`iteratable Arg₁:limit(integer Arg₂)`

**type** :mini:`skipped`
   :Parents: :mini:`iteratable`


**method** :mini:`iteratable Arg₁:skip(integer Arg₂)`

**type** :mini:`tasks`
   :Parents: :mini:`function`


**function** :mini:`tasks()`

**method** :mini:`tasks Arg₁:wait`

**function** :mini:`parallel(iteratable Arg₁, any Arg₂)`

**type** :mini:`unique`
   :Parents: :mini:`iteratable`


**function** :mini:`unique(any Arg₁)`

**type** :mini:`grouped`
   :Parents: :mini:`iteratable`


**function** :mini:`group(any Arg₁)`

**type** :mini:`repeated`
   :Parents: :mini:`iteratable`


**function** :mini:`repeat(any Arg₁)`

**type** :mini:`sequenced`
   :Parents: :mini:`iteratable`


**method** :mini:`iteratable Arg₁ || iteratable Arg₂`

**method** :mini:`|| iteratable Arg₁`

**type** :mini:`swapped`
   :Parents: :mini:`iteratable`


**method** :mini:`iteratable Arg₁:swap`

**type** :mini:`iteratable`
   The base type for any iteratable value.


