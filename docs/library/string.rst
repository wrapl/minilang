string
======

:mini:`fun mlstring(Value: any): string`
   Returns a general (type name only) representation of :mini:`Value` as a string.


:mini:`type string < address, sequence`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: nil)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: some)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: integer, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: integer::range)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: real::range)`
   *TBD*

:mini:`meth (String: string):ord: integer`
   Returns the unicode codepoint of the first character of :mini:`String`.


:mini:`meth (Codepoint: integer):chr: string`
   Returns a string containing the character with unicode codepoint :mini:`Codepoint`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: double, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: complex, Arg₃: string)`
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

:mini:`meth (Arg₁: string::buffer):get`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: any, ...)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):write(Arg₂: any, ...)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: nil)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: some)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string)[Arg₂: integer]`
   *TBD*

:mini:`meth (Arg₁: string)[Arg₂: integer, Arg₃: integer]`
   *TBD*

:mini:`meth (Arg₁: string) + (Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):trim`
   *TBD*

:mini:`meth (Arg₁: string):trim(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):ltrim`
   *TBD*

:mini:`meth (Arg₁: string):ltrim(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):rtrim`
   *TBD*

:mini:`meth (Arg₁: string):rtrim(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):length`
   *TBD*

:mini:`meth (Arg₁: string):count`
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

:mini:`meth (Arg₁: string):lower`
   *TBD*

:mini:`meth (Arg₁: string):upper`
   *TBD*

:mini:`meth (Arg₁: string):escape`
   *TBD*

:mini:`meth (Arg₁: string):find(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):find2(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):find(Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string):find2(Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string):find(Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string):find2(Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string):find(Arg₂: regex, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string):find2(Arg₂: regex, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string) % (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string) ? (Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string):starts(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):starts(Arg₂: regex)`
   *TBD*

:mini:`meth (Arg₁: string):ends(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):after(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):after(Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string):before(Arg₂: string)`
   *TBD*

:mini:`meth (Arg₁: string):before(Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: string, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: regex, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: regex, Arg₃: function)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: map)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: regex)`
   *TBD*

