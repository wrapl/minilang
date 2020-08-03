string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1724 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 1735 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 1746 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 1774 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 1812 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2003 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2014 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2230 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2241 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2256 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2268 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2278 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2291 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2296 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2348 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2388 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2424 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2452 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2482 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2491 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2500 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2512 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2530 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2558 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: string)`
   *Defined at line 2591 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 2635 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 2668 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 2697 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 2719 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 2751 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 2803 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 2897 in src/ml_types.c*

