string
======

.. include:: <isonum.txt>

**method** :mini:`boolean(string String)` |rarr| :mini:`boolean` or :mini:`error`
   Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).

   Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).

   Otherwise returns an error.


**type** :mini:`string`
   :Parents: :mini:`iteratable`


**type** :mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`


**method** :mini:`integer(string Arg₁)`

**method** :mini:`integer(string Arg₁, integer Arg₂)`

**method** :mini:`real(string Arg₁)`

**type** :mini:`regex`

**function** :mini:`regex(string String)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.


**method** :mini:`string Arg₁[integer Arg₂]`

**method** :mini:`string Arg₁[integer Arg₂, integer Arg₃]`

**method** :mini:`string Arg₁ + string Arg₂`

**method** :mini:`string Arg₁:trim`

**method** :mini:`string Arg₁:trim(string Arg₂)`

**method** :mini:`string Arg₁:length`

**method** :mini:`string Arg₁ <> string Arg₂`

**method** :mini:`string Arg₁ ~ string Arg₂`

**method** :mini:`string Arg₁ ~> string Arg₂`

**method** :mini:`string Arg₁ / string Arg₂`

**method** :mini:`string Arg₁ / regex Arg₂`

**method** :mini:`string Arg₁:lower`

**method** :mini:`string Arg₁:upper`

**method** :mini:`string Arg₁:find(string Arg₂)`

**method** :mini:`string Arg₁:find(string Arg₂, integer Arg₃)`

**method** :mini:`string Arg₁:find(regex Arg₂)`

**method** :mini:`string Arg₁:find(regex Arg₂, integer Arg₃)`

**method** :mini:`string Arg₁ % string Arg₂`

**method** :mini:`string Arg₁ % regex Arg₂`

**method** :mini:`string Arg₁ ? regex Arg₂`

**method** :mini:`string Arg₁:replace(string Arg₂, string Arg₃)`

**method** :mini:`string Arg₁:replace(regex Arg₂, string Arg₃)`

**method** :mini:`string Arg₁:replace(regex Arg₂, function Arg₃)`

**method** :mini:`string Arg₁:replace(map Arg₂)`

**method** :mini:`method(string Arg₁)`

