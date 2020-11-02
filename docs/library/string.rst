string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1950 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 1961 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 1972 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 2000 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 2038 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2240 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2254 in src/ml_types.c*

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`
   *Defined at line 2288 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2508 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2519 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2534 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2546 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2556 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string)`
   *Defined at line 2569 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2578 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string)`
   *Defined at line 2590 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2599 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2611 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2616 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2668 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2708 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2744 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2772 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2808 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2817 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2826 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *Defined at line 2838 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2853 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2871 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2892 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *Defined at line 2915 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2941 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2970 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 3002 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 3051 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *Defined at line 3083 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *Defined at line 3093 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 3125 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 3147 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 3184 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 3241 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 3341 in src/ml_types.c*

