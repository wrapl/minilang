string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1817 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 1828 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 1839 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 1867 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 1905 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2107 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2121 in src/ml_types.c*

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`
   *Defined at line 2155 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2375 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2386 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2401 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2413 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2423 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string)`
   *Defined at line 2436 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2445 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string)`
   *Defined at line 2457 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2466 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2478 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2483 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2535 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2575 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2611 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2639 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2675 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2684 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2693 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *Defined at line 2705 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2720 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2738 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2759 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *Defined at line 2782 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2808 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2837 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 2869 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 2918 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *Defined at line 2950 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *Defined at line 2960 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 2992 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 3014 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 3051 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 3108 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 3208 in src/ml_types.c*

