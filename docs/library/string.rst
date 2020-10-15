string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1818 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 1829 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 1840 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 1868 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 1906 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2108 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2122 in src/ml_types.c*

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`
   *Defined at line 2156 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2376 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2387 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2402 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2414 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2424 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string)`
   *Defined at line 2437 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2446 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string)`
   *Defined at line 2458 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2467 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2479 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2484 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2536 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2576 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2612 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2640 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2676 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2685 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2694 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *Defined at line 2706 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2721 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2739 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2760 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *Defined at line 2783 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2809 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2838 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 2870 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 2919 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *Defined at line 2951 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *Defined at line 2961 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 2993 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 3015 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 3052 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 3109 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 3209 in src/ml_types.c*

