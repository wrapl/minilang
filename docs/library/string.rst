.. include:: <isonum.txt>

string
======

.. _fun-string:

:mini:`fun string(Value: any): string`
   Returns a general (type name only) representation of :mini:`Value` as a string.


.. _type-string:

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

:mini:`meth (String: string):code: integer`
   Returns the unicode codepoint of the first UTF-8 character of :mini:`String`.


:mini:`meth (Codepoint: integer):utf8: string`
   Returns a UTF-8 string containing the character with unicode codepoint :mini:`Codepoint`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: double, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: complex, Arg₃: string)`
   *TBD*

.. _fun-regex:

:mini:`fun regex(String: string): regex | error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.


.. _type-regex:

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


.. _fun-string-switch:

:mini:`fun string::switch(Cases: string|regex, ...)`
   Implements :mini:`switch` for string values. Case values must be strings or regular expressions.


.. _fun-string-buffer:

:mini:`fun string::buffer()`
   *TBD*

.. _type-string-buffer:

:mini:`type string::buffer`
   *TBD*

:mini:`meth (Arg₁: string::buffer):get`
   *TBD*

:mini:`meth (Arg₁: string::buffer):length`
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

:mini:`meth (String: string):length: integer`
   Returns the number of UTF-8 characters in :mini:`String`.


:mini:`meth (String: string):count: integer`
   Returns the number of UTF-8 characters in :mini:`String`.


:mini:`meth (String: string)[Index: integer]: string`
   Returns the substring of :mini:`String` of length 1 at :mini:`Index`.


:mini:`meth (String: string)[Start: integer, End: integer]: string`
   Returns the substring of :mini:`String` from :mini:`Start` to :mini:`End - 1` inclusively.


:mini:`meth (String: string)[Range: integer::range]: string`
   Returns the substring of :mini:`String` corresponding to :mini:`Range` inclusively.


:mini:`meth (String: string):limit(Length: integer): string`
   Returns the prefix of :mini:`String` limited to :mini:`Length`.


:mini:`meth (String: string):offset(Index: integer): integer`
   Returns the byte position of the :mini:`Index`-th character of :mini:`String`.


:mini:`meth (A: string) + (B: string): string`
   Returns :mini:`A` and :mini:`B` concatentated.


:mini:`meth (N: integer) * (String: string): string`
   Returns :mini:`String` concatentated :mini:`N` times.


:mini:`meth (String: string):trim: string`
   Returns a copy of :mini:`String` with whitespace removed from both ends.


:mini:`meth (String: string):trim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from both ends.


:mini:`meth (String: string):ltrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.


:mini:`meth (String: string):ltrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.


:mini:`meth (String: string):rtrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.


:mini:`meth (String: string):rtrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.


:mini:`meth (String: string):reverse: string`
   Returns a string with the characters in :mini:`String` reversed.


:mini:`meth (A: string) <> (B: string): integer`
   Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.


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


:mini:`meth (A: string) ~ (B: string): integer`
   Returns the edit distance between :mini:`A` and :mini:`B`.


:mini:`meth (A: string) ~> (B: string): integer`
   Returns an asymmetric edit distance from :mini:`A` to :mini:`B`.


:mini:`meth (String: string) / (Pattern: string): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.


:mini:`meth (String: string) / (Pattern: regex): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.

   If :mini:`Pattern` contains a subgroup then only the subgroup matches are removed from the output substrings.


:mini:`meth (String: string) / (Pattern: regex, Index: integer): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.

   Only the :mini:`Index` subgroup matches are removed from the output substrings.


:mini:`meth (String: string) /* (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.


:mini:`meth (String: string) /* (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.


:mini:`meth (String: string) */ (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.


:mini:`meth (String: string) */ (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.


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

:mini:`meth (String: string):replace(Replacements: map): string`
   Each key in :mini:`Replacements` can be either a string or a regex. Each value in :mini:`Replacements` can be either a string or a function.

   Returns a copy of :mini:`String` with each matching string or regex from :mini:`Replacements` replaced with the corresponding value. Functions are called with the matched string or regex subpatterns.


:mini:`meth (Arg₁: string):replace(Arg₂: regex, Arg₃: function)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: integer, Arg₃: string)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: integer, Arg₃: integer, Arg₄: string)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: integer, Arg₃: function)`
   *TBD*

:mini:`meth (Arg₁: string):replace(Arg₂: integer, Arg₃: integer, Arg₄: function)`
   *TBD*

:mini:`meth (Arg₁: string::buffer):append(Arg₂: regex)`
   *TBD*

