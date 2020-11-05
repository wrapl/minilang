string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`

   *Defined at line 1998 in src/ml_types.c*

:mini:`stringshort`
   :Parents: :mini:`string`

   *Defined at line 2009 in src/ml_types.c*

:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`

   *Defined at line 2020 in src/ml_types.c*

:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`
   *Defined at line 2048 in src/ml_types.c*

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`

   *Defined at line 2086 in src/ml_types.c*

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   *Defined at line 2288 in src/ml_types.c*

:mini:`regex`
   *Defined at line 2302 in src/ml_types.c*

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`
   *Defined at line 2336 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer)`
   *Defined at line 2556 in src/ml_types.c*

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`
   *Defined at line 2567 in src/ml_types.c*

:mini:`meth +(Arg₁: string, Arg₂: string)`
   *Defined at line 2582 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string)`
   *Defined at line 2594 in src/ml_types.c*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *Defined at line 2604 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string)`
   *Defined at line 2617 in src/ml_types.c*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2626 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string)`
   *Defined at line 2638 in src/ml_types.c*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *Defined at line 2647 in src/ml_types.c*

:mini:`meth :length(Arg₁: string)`
   *Defined at line 2659 in src/ml_types.c*

:mini:`meth <>(Arg₁: string, Arg₂: string)`
   *Defined at line 2664 in src/ml_types.c*

:mini:`meth ~(Arg₁: string, Arg₂: string)`
   *Defined at line 2716 in src/ml_types.c*

:mini:`meth ~>(Arg₁: string, Arg₂: string)`
   *Defined at line 2756 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: string)`
   *Defined at line 2792 in src/ml_types.c*

:mini:`meth /(Arg₁: string, Arg₂: regex)`
   *Defined at line 2820 in src/ml_types.c*

:mini:`meth :lower(Arg₁: string)`
   *Defined at line 2856 in src/ml_types.c*

:mini:`meth :upper(Arg₁: string)`
   *Defined at line 2865 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *Defined at line 2874 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *Defined at line 2886 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2901 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *Defined at line 2919 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *Defined at line 2940 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *Defined at line 2963 in src/ml_types.c*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 2989 in src/ml_types.c*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *Defined at line 3018 in src/ml_types.c*

:mini:`meth %(Arg₁: string, Arg₂: regex)`
   *Defined at line 3050 in src/ml_types.c*

:mini:`meth ?(Arg₁: string, Arg₂: regex)`
   *Defined at line 3099 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *Defined at line 3131 in src/ml_types.c*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *Defined at line 3141 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *Defined at line 3173 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *Defined at line 3195 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *Defined at line 3232 in src/ml_types.c*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *Defined at line 3289 in src/ml_types.c*

:mini:`meth string(Arg₁: regex)`
   *Defined at line 3389 in src/ml_types.c*

