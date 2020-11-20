string
======

.. include:: <isonum.txt>

:mini:`string`
   :Parents: :mini:`iteratable`


:mini:`stringshort`
   :Parents: :mini:`string`


:mini:`stringlong`
   :Parents: :mini:`string`, :mini:`buffer`


:mini:`meth +(Arg₁: stringshort, Arg₂: integer)`

:mini:`string`
   :Parents: :mini:`buffer`, :mini:`iteratable`


:mini:`meth string(Arg₁: nil)`

:mini:`meth string(Arg₁: some)`

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.


:mini:`regex`

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: any)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: any)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: nil)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: some)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: integer)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: real)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: string)`

:mini:`meth [](Arg₁: string, Arg₂: integer)`

:mini:`meth [](Arg₁: string, Arg₂: integer, Arg₃: integer)`

:mini:`meth +(Arg₁: string, Arg₂: string)`

:mini:`meth :trim(Arg₁: string)`

:mini:`meth :trim(Arg₁: string, Arg₂: string)`

:mini:`meth :ltrim(Arg₁: string)`

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`

:mini:`meth :rtrim(Arg₁: string)`

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`

:mini:`meth :length(Arg₁: string)`

:mini:`meth :count(Arg₁: string)`

:mini:`meth <>(Arg₁: string, Arg₂: string)`

:mini:`meth ~(Arg₁: string, Arg₂: string)`

:mini:`meth ~>(Arg₁: string, Arg₂: string)`

:mini:`meth /(Arg₁: string, Arg₂: string)`

:mini:`meth /(Arg₁: string, Arg₂: regex)`

:mini:`meth :lower(Arg₁: string)`

:mini:`meth :upper(Arg₁: string)`

:mini:`meth :find(Arg₁: string, Arg₂: string)`

:mini:`meth :find2(Arg₁: string, Arg₂: string)`

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`

:mini:`meth :find(Arg₁: string, Arg₂: regex)`

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`

:mini:`meth %(Arg₁: string, Arg₂: regex)`

:mini:`meth ?(Arg₁: string, Arg₂: regex)`

:mini:`meth :starts(Arg₁: string, Arg₂: string)`

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`

:mini:`meth :replace(Arg₁: string, Arg₂: map)`

:mini:`meth string(Arg₁: regex)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: regex)`

