.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

string
======

:mini:`meth (N: integer) * (String: string): string`
   Returns :mini:`String` concatentated :mini:`N` times.



:mini:`meth (Codepoint: integer):utf8: string`
   Returns a UTF-8 string containing the character with unicode codepoint :mini:`Codepoint`.



.. _type-regex:

:mini:`type regex`
   A regular expression.



.. _fun-regex:

:mini:`fun regex(String: string): regex | error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.



:mini:`meth (Arg₁: regex) != (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: regex) < (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: regex) <= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (A: regex) <> (B: regex): integer`
   Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.



:mini:`meth (Arg₁: regex) = (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: regex) > (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: regex) >= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: string::buffer):append(Arg₂: regex)`
   *TBD*


.. _type-string:

:mini:`type string < address, sequence`
   A string of characters in UTF-8 encoding.



.. _fun-string:

:mini:`fun string(Value: any): string`
   Returns a general (type name only) representation of :mini:`Value` as a string.



:mini:`meth (Arg₁: string) != (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.



:mini:`meth (String: string) % (Pattern: regex): tuple[string] | nil`
   Matches :mini:`String` with :mini:`Pattern` returning a tuple of the matched components,  or :mini:`nil` if the pattern does not match.



:mini:`meth (String: string) */ (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.



:mini:`meth (String: string) */ (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.



:mini:`meth (A: string) + (B: string): string`
   Returns :mini:`A` and :mini:`B` concatentated.



:mini:`meth (String: string) / (Pattern: regex, Index: integer): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.

   Only the :mini:`Index` subgroup matches are removed from the output substrings.



:mini:`meth (String: string) / (Pattern: regex): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.

   If :mini:`Pattern` contains a subgroup then only the subgroup matches are removed from the output substrings.



:mini:`meth (String: string) / (Pattern: string): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.



:mini:`meth (String: string) /* (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.



:mini:`meth (String: string) /* (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.



:mini:`meth (String: string):after(Delimiter: string): string | nil`
   Returns the portion of :mini:`String` after the 1st occurence of :mini:`Delimiter`,  or :mini:`nil` if no occurence if found.



:mini:`meth (String: string):after(Delimiter: string, N: integer): string | nil`
   Returns the portion of :mini:`String` after the :mini:`N`-th occurence of :mini:`Delimiter`,  or :mini:`nil` if no :mini:`N`-th occurence if found.

   If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.



:mini:`meth (String: string):before(Delimiter: string): string | nil`
   Returns the portion of :mini:`String` before the 1st occurence of :mini:`Delimiter`,  or :mini:`nil` if no occurence if found.



:mini:`meth (String: string):before(Delimiter: string, N: integer): string | nil`
   Returns the portion of :mini:`String` before the :mini:`N`-th occurence of :mini:`Delimiter`,  or :mini:`nil` if no :mini:`N`-th occurence if found.

   If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.



:mini:`meth (String: string):code: integer`
   Returns the unicode codepoint of the first UTF-8 character of :mini:`String`.



:mini:`meth (String: string):count: integer`
   Returns the number of UTF-8 characters in :mini:`String`.



:mini:`meth (String: string):ends(Suffix: string): string | nil`
   Returns :mini:`String` if it ends with :mini:`Suffix` and :mini:`nil` otherwise.



:mini:`meth (String: string):escape: string`
   Returns :mini:`String` with white space,  quotes and backslashes replaced by escape sequences.



:mini:`meth (Haystack: string):find(Needle: string): integer | nil`
   Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find(Needle: string, Start: integer): integer | nil`
   Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find(Pattern: regex): integer | nil`
   Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find(Pattern: regex, Start: integer): integer | nil`
   Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find2(Needle: string): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find2(Needle: string, Start: integer): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find2(Pattern: regex): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.



:mini:`meth (Haystack: string):find2(Pattern: regex, Start: integer): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.



:mini:`meth (String: string):length: integer`
   Returns the number of UTF-8 characters in :mini:`String`.



:mini:`meth (String: string):limit(Length: integer): string`
   Returns the prefix of :mini:`String` limited to :mini:`Length`.



:mini:`meth (String: string):lower: string`
   Returns :mini:`String` with each character converted to lower case.



:mini:`meth (String: string):ltrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.



:mini:`meth (String: string):ltrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.



:mini:`meth (String: string):offset(Index: integer): integer`
   Returns the byte position of the :mini:`Index`-th character of :mini:`String`.



:mini:`meth (String: string):replace(I: integer, Function: function): string`
   Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Function(String[I])`.



:mini:`meth (String: string):replace(I: integer, J: integer, Replacement: string): string`
   Returns a copy of :mini:`String` with the :mini:`String[I,  J]` is replaced by :mini:`Replacement`.



:mini:`meth (String: string):replace(Pattern: regex, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.



:mini:`meth (String: string):replace(I: integer, Function: integer, Arg₄: function): string`
   Returns a copy of :mini:`String` with the :mini:`String[I,  J]` is replaced by :mini:`Function(String[I,  J])`.



:mini:`meth (String: string):replace(Pattern: string, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.



:mini:`meth (String: string):replace(Replacements: map): string`
   Each key in :mini:`Replacements` can be either a string or a regex. Each value in :mini:`Replacements` can be either a string or a function.

   Returns a copy of :mini:`String` with each matching string or regex from :mini:`Replacements` replaced with the corresponding value. Functions are called with the matched string or regex subpatterns.



:mini:`meth (String: string):replace(Pattern: regex, Function: function): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Function(Match)` where :mini:`Match` is the actual matched text.



:mini:`meth (String: string):replace(I: integer, Replacement: string): string`
   Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Replacement`.



:mini:`meth (String: string):reverse: string`
   Returns a string with the characters in :mini:`String` reversed.



:mini:`meth (String: string):rtrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.



:mini:`meth (String: string):rtrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.



:mini:`meth (String: string):starts(Prefix: string): string | nil`
   Returns :mini:`String` if it starts with :mini:`Prefix` and :mini:`nil` otherwise.



:mini:`meth (String: string):starts(Pattern: regex): string | nil`
   Returns :mini:`String` if it starts with :mini:`Pattern` and :mini:`nil` otherwise.



:mini:`meth (String: string):trim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from both ends.



:mini:`meth (String: string):trim: string`
   Returns a copy of :mini:`String` with whitespace removed from both ends.



:mini:`meth (String: string):upper: string`
   Returns :mini:`String` with each character converted to upper case.



:mini:`meth (Arg₁: string) < (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: string) <= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (A: string) <> (B: string): integer`
   Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.



:mini:`meth (Arg₁: string) = (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ == Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: string) > (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.



:mini:`meth (Arg₁: string) >= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.



:mini:`meth (String: string) ? (Pattern: regex): string | nil`
   Returns :mini:`String` if it matches :mini:`Pattern` and :mini:`nil` otherwise.



:mini:`meth (String: string)[Index: integer]: string`
   Returns the substring of :mini:`String` of length 1 at :mini:`Index`.



:mini:`meth (String: string)[Start: integer, End: integer]: string`
   Returns the substring of :mini:`String` from :mini:`Start` to :mini:`End - 1` inclusively.



:mini:`meth (String: string)[Range: integer::range]: string`
   Returns the substring of :mini:`String` corresponding to :mini:`Range` inclusively.



:mini:`meth (A: string) ~ (B: string): integer`
   Returns the edit distance between :mini:`A` and :mini:`B`.



:mini:`meth (A: string) ~> (B: string): integer`
   Returns an asymmetric edit distance from :mini:`A` to :mini:`B`.



:mini:`meth (Buffer: string::buffer):append(Value: string)`
   Appends :mini:`Value` to :mini:`Buffer`.



.. _type-string-buffer:

:mini:`type string::buffer`
   A string buffer that automatically grows and shrinks as required.



.. _fun-string-buffer:

:mini:`fun string::buffer(): string::buffer`
   Returns a new :mini:`string::buffer`



:mini:`meth (Buffer: string::buffer):get: string`
   Returns the contents of :mini:`Buffer` as a string.

   .. deprecated:: 2.5.0

      Use :mini:`Buffer:rest` instead.



:mini:`meth (Buffer: string::buffer):length: integer`
   Returns the number of bytes currently available in :mini:`Buffer`.



:mini:`meth (Buffer: string::buffer):rest: string`
   Returns the contents of :mini:`Buffer` as a string.



:mini:`meth (Buffer: string::buffer):write(Value₁, ..., Valueₙ: any): integer`
   Writes each :mini:`Valueᵢ` in turn to :mini:`Buffer`.



.. _fun-string-switch:

:mini:`fun string::switch(Cases: string|regex, ...)`
   Implements :mini:`switch` for string values. Case values must be strings or regular expressions.



