string
======

.. include:: <isonum.txt>

:mini:`type string < buffer, sequence`

:mini:`meth MLSequenceCount(Arg₁: string)`

:mini:`meth string(Arg₁: nil)`

:mini:`meth string(Arg₁: some)`

:mini:`meth string(Arg₁: integer, Arg₂: string)`

:mini:`meth string(Arg₁: double, Arg₂: string)`

:mini:`meth string(Arg₁: complex, Arg₂: string)`

:mini:`fun regex(String: string)` |rarr| :mini:`regex` or :mini:`error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.


:mini:`type regex`

:mini:`meth <>(Arg₁: regex, Arg₂: regex)`

:mini:`fun stringbuffer()`

:mini:`type stringbuffer`

:mini:`meth :get(Arg₁: stringbuffer)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: any)`

:mini:`meth :write(Arg₁: stringbuffer, Arg₂: any)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: nil)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: some)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: integer)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: double)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: string)`

:mini:`meth (Arg₁: string)[Arg₂: integer]`

:mini:`meth (Arg₁: string)[Arg₂: integer, Arg₃: integer]`

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

:mini:`meth /(Arg₁: string, Arg₂: regex, Arg₃: integer)`

:mini:`meth /*(Arg₁: string, Arg₂: string)`

:mini:`meth /*(Arg₁: string, Arg₂: regex)`

:mini:`meth */(Arg₁: string, Arg₂: string)`

:mini:`meth */(Arg₁: string, Arg₂: regex)`

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

:mini:`meth :ends(Arg₁: string, Arg₂: string)`

:mini:`meth :after(Arg₁: string, Arg₂: string)`

:mini:`meth :after(Arg₁: string, Arg₂: string, Arg₃: integer)`

:mini:`meth :before(Arg₁: string, Arg₂: string)`

:mini:`meth :before(Arg₁: string, Arg₂: string, Arg₃: integer)`

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`

:mini:`meth :replace(Arg₁: string, Arg₂: map)`

:mini:`meth string(Arg₁: regex)`

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: regex)`

