string
======

.. include:: <isonum.txt>

**type** :mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1724 in src/ml_types.c*

**type** :mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 1735 in src/ml_types.c*

**type** :mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 1746 in src/ml_types.c*

**method** :mini:`stringshort Arg₁ + integer Arg₂`
   *Defined at line 1774 in src/ml_types.c*

**type** :mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 1812 in src/ml_types.c*

**function** :mini:`regex(string String)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2003 in src/ml_types.c*

**type** :mini:`regex`
   *Defined at line 2014 in src/ml_types.c*

**method** :mini:`string Arg₁[integer Arg₂]`
   *Defined at line 2230 in src/ml_types.c*

**method** :mini:`string Arg₁[integer Arg₂, integer Arg₃]`
   *Defined at line 2241 in src/ml_types.c*

**method** :mini:`string Arg₁ + string Arg₂`
   *Defined at line 2256 in src/ml_types.c*

**method** :mini:`string Arg₁:trim`
   *Defined at line 2268 in src/ml_types.c*

**method** :mini:`string Arg₁:trim(string Arg₂)`
   *Defined at line 2278 in src/ml_types.c*

**method** :mini:`string Arg₁:length`
   *Defined at line 2291 in src/ml_types.c*

**method** :mini:`string Arg₁ <> string Arg₂`
   *Defined at line 2296 in src/ml_types.c*

**method** :mini:`string Arg₁ ~ string Arg₂`
   *Defined at line 2348 in src/ml_types.c*

**method** :mini:`string Arg₁ ~> string Arg₂`
   *Defined at line 2388 in src/ml_types.c*

**method** :mini:`string Arg₁ / string Arg₂`
   *Defined at line 2424 in src/ml_types.c*

**method** :mini:`string Arg₁ / regex Arg₂`
   *Defined at line 2452 in src/ml_types.c*

**method** :mini:`string Arg₁:lower`
   *Defined at line 2482 in src/ml_types.c*

**method** :mini:`string Arg₁:upper`
   *Defined at line 2491 in src/ml_types.c*

**method** :mini:`string Arg₁:find(string Arg₂)`
   *Defined at line 2500 in src/ml_types.c*

**method** :mini:`string Arg₁:find(string Arg₂, integer Arg₃)`
   *Defined at line 2512 in src/ml_types.c*

**method** :mini:`string Arg₁:find(regex Arg₂)`
   *Defined at line 2530 in src/ml_types.c*

**method** :mini:`string Arg₁:find(regex Arg₂, integer Arg₃)`
   *Defined at line 2558 in src/ml_types.c*

**method** :mini:`string Arg₁ % string Arg₂`
   *Defined at line 2591 in src/ml_types.c*

**method** :mini:`string Arg₁ % regex Arg₂`
   *Defined at line 2635 in src/ml_types.c*

**method** :mini:`string Arg₁ ? regex Arg₂`
   *Defined at line 2668 in src/ml_types.c*

**method** :mini:`string Arg₁:replace(string Arg₂, string Arg₃)`
   *Defined at line 2697 in src/ml_types.c*

**method** :mini:`string Arg₁:replace(regex Arg₂, string Arg₃)`
   *Defined at line 2719 in src/ml_types.c*

**method** :mini:`string Arg₁:replace(regex Arg₂, function Arg₃)`
   *Defined at line 2751 in src/ml_types.c*

**method** :mini:`string Arg₁:replace(map Arg₂)`
   *Defined at line 2803 in src/ml_types.c*

**method** :mini:`string(regex Arg₁)`
   *Defined at line 2897 in src/ml_types.c*

