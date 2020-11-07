string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1999 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 2010 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 2021 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 2049 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 2087 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2289 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2303 in src/ml_types.c*

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`
   *Defined at line 2337 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2557 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2568 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2583 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2595 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2605 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string)`
   *Defined at line 2618 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2627 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string)`
   *Defined at line 2639 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2648 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2660 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2665 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2717 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2757 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2793 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2821 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2857 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2866 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2875 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *Defined at line 2887 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2902 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2920 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2941 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *Defined at line 2964 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2990 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 3019 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 3051 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 3100 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *Defined at line 3132 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *Defined at line 3142 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 3174 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 3196 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 3233 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 3290 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 3390 in src/ml_types.c*

