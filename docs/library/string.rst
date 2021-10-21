string
======

:mini:`type string < address, sequence`
   *TBD*

:mini:`meth string(Arg₁: nil)`
   *TBD*

:mini:`meth string(Arg₁: some)`
   *TBD*

:mini:`meth string(Arg₁: integer, Arg₂: string)`
   *TBD*

:mini:`meth string(Arg₁: integer::range)`
   *TBD*

:mini:`meth string(Arg₁: real::range)`
   *TBD*

:mini:`meth :ord(String: string): integer`
   Returns the unicode codepoint of the first character of :mini:`String`.


:mini:`meth :chr(Codepoint: integer): string`
   Returns a string containing the character with unicode codepoint :mini:`Codepoint`.


:mini:`meth string(Arg₁: double, Arg₂: string)`
   *TBD*

:mini:`meth string(Arg₁: complex, Arg₂: string)`
   *TBD*

:mini:`fun regex(String: string): regex | error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.


:mini:`type regex`
   *TBD*

:mini:`meth (Arg₁: regex) <> (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: regex) = (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: regex) != (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: regex) < (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: regex) > (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: regex) <= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: regex) >= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.


:mini:`fun string::switch(Cases...: string|regex)`
   Implements :mini:`switch` for string values. Case values must be strings or regular expressions.


:mini:`fun string::buffer()`
   *TBD*

:mini:`type string::buffer`
   *TBD*

:mini:`meth :get(Arg₁: string::buffer)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: any, ...)`
   *TBD*

:mini:`meth :write(Arg₁: string::buffer, Arg₂: any, ...)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: nil)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: some)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: integer)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: double)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string)[Arg₂: integer]`
   *TBD*

:mini:`meth (Arg₁: string)[Arg₂: integer, Arg₃: integer]`
   *TBD*

:mini:`meth (Arg₁: string) + (Arg₂: string)`
   *TBD*

:mini:`meth :trim(Arg₁: string)`
   *TBD*

:mini:`meth :trim(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :ltrim(Arg₁: string)`
   *TBD*

:mini:`meth :ltrim(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :rtrim(Arg₁: string)`
   *TBD*

:mini:`meth :rtrim(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :length(Arg₁: string)`
   *TBD*

:mini:`meth :count(Arg₁: string)`
   *TBD*

:mini:`meth (Arg₁: string) <> (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) = (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) != (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) < (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) > (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) <= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) >= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: string) ~ (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) ~> (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) / (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) / (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string) / (Arg₂: regex, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string) /* (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) /* (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string) */ (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string) */ (Arg₂: regex)`
   *TBD*

:mini:`meth :lower(Arg₁: string)`
   *TBD*

:mini:`meth :upper(Arg₁: string)`
   *TBD*

:mini:`meth :find(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :find2(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :find(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :find2(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :find(Arg₁: string, Arg₂: regex)`
   *TBD*

:mini:`meth :find2(Arg₁: string, Arg₂: regex)`
   *TBD*

:mini:`meth :find(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *TBD*

:mini:`meth :find2(Arg₁: string, Arg₂: regex, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string) % (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string) ? (Arg₂: regex)`
   *TBD*

:mini:`meth :starts(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :starts(Arg₁: string, Arg₂: regex)`
   *TBD*

:mini:`meth :ends(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :after(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :after(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :before(Arg₁: string, Arg₂: string)`
   *TBD*

:mini:`meth :before(Arg₁: string, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :replace(Arg₁: string, Arg₂: string, Arg₃: string)`
   *TBD*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: string)`
   *TBD*

:mini:`meth :replace(Arg₁: string, Arg₂: regex, Arg₃: function)`
   *TBD*

:mini:`meth :replace(Arg₁: string, Arg₂: map)`
   *TBD*

:mini:`meth string(Arg₁: regex)`
   *TBD*

:mini:`meth :append(Arg₁: string::buffer, Arg₂: regex)`
   *TBD*

