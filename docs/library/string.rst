.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

string
======

.. rst-class:: mini-api

Strings in Minilang can contain any sequence of bytes,  including :mini:`0` bytes.
Index and find methods however work on ``UTF-8`` characters,  byte sequences that are not valid ``UTF-8`` are handled gracefully but the results are probably not very useful.

Every :mini:`string` is also an :mini:`address` so address methods can also be used to work at the byte level if necessary.

Indexing a string starts at :mini:`1`,  with the last character at :mini:`String:length`. Negative indices are counted form the end,  :mini:`-1` is the last character and :mini:`-String:length` is the first character.

When creating a substring,  the first index is inclusive and second index is exclusive. The index :mini:`0` refers to just beyond the last character and can be used to take a substring to the end of a string.

:mini:`fun mlstringbuffercount()`
   *TBD*


:mini:`fun mlstringcheckcache()`
   *TBD*


:mini:`meth (Arg₁: any):sha256`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: any, Arg₃: function, ...)`
   *TBD*


:mini:`meth (Arg₁: buffer):put(Arg₂: string::buffer)`
   *TBD*


:mini:`type byte::order < enum`
   * :mini:`::LittleEndian`
   * :mini:`::BigEndian`


:mini:`meth (Arg₁: string::buffer):append(Arg₂: decimal)`
   *TBD*


:mini:`meth (N: integer) * (String: string): string`
   Returns :mini:`String` concatentated :mini:`N` times.

   .. code-block:: mini

      5 * "abc" :> "abcabcabcabcabc"


:mini:`meth (Codepoint: integer):char: string`
   Returns a UTF-8 string containing the character with unicode codepoint :mini:`Codepoint`.


:mini:`meth (Codepoint: integer):utf8: string`
   Returns a UTF-8 string containing the character with unicode codepoint :mini:`Codepoint`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: rational)`
   *TBD*


:mini:`type regex < function`
   A regular expression.


:mini:`fun regex(String: string): regex | error`
   Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.

   .. code-block:: mini

      regex("[0-9]+") :> /[0-9]+/
      regex("[0-9") :> error("RegexError", "Missing ']'")


:mini:`meth (Arg₁: regex) != (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" != r"[A-Za-z0-9_]+" :> /[A-Za-z0-9_]+/
      r"[A-Za-z0-9_]+" != r"[0-9]+" :> /[0-9]+/
      r"[0-9]+" != r"[0-9]+" :> nil


:mini:`meth (Arg₁: regex) < (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" < r"[A-Za-z0-9_]+" :> /[A-Za-z0-9_]+/
      r"[A-Za-z0-9_]+" < r"[0-9]+" :> nil
      r"[0-9]+" < r"[0-9]+" :> nil


:mini:`meth (Arg₁: regex) <= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" <= r"[A-Za-z0-9_]+" :> /[A-Za-z0-9_]+/
      r"[A-Za-z0-9_]+" <= r"[0-9]+" :> nil
      r"[0-9]+" <= r"[0-9]+" :> /[0-9]+/


:mini:`meth (A: regex) <> (B: regex): integer`
   Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively. Mainly for using regular expressions as keys in maps.

   .. code-block:: mini

      r"[0-9]+" <> r"[A-Za-z0-9_]+" :> -1
      r"[A-Za-z0-9_]+" <> r"[0-9]+" :> 1
      r"[0-9]+" <> r"[0-9]+" :> 0


:mini:`meth (Arg₁: regex) = (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ = Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" = r"[A-Za-z0-9_]+" :> nil
      r"[A-Za-z0-9_]+" = r"[0-9]+" :> nil
      r"[0-9]+" = r"[0-9]+" :> /[0-9]+/


:mini:`meth (Arg₁: regex) > (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" > r"[A-Za-z0-9_]+" :> nil
      r"[A-Za-z0-9_]+" > r"[0-9]+" :> /[0-9]+/
      r"[0-9]+" > r"[0-9]+" :> nil


:mini:`meth (Arg₁: regex) >= (Arg₂: regex): regex | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      r"[0-9]+" >= r"[A-Za-z0-9_]+" :> nil
      r"[A-Za-z0-9_]+" >= r"[0-9]+" :> /[0-9]+/
      r"[0-9]+" >= r"[0-9]+" :> /[0-9]+/


:mini:`meth (Regex: regex):pattern: string`
   Returns the pattern used to create :mini:`Regex`.

   .. code-block:: mini

      r"[0-9]+":pattern :> "[0-9]+"


:mini:`meth (Buffer: string::buffer):append(Value: regex)`
   Appends a representation of :mini:`Value` to :mini:`Buffer`.


:mini:`type string < address, sequence`
   A string of characters in UTF-8 encoding.


:mini:`fun string(Value: any): string`
   Returns a representation of :mini:`Value` as a string.

   .. code-block:: mini

      string(100) :> "100"
      string(nil) :> "nil"
      string("Hello world!\n") :> "Hello world!\n"
      string([1, 2, 3]) :> "[1, 2, 3]"


:mini:`fun regex::escape(String: string): string`
   Escapes characters in :mini:`String` that are treated specially in regular expressions.

   .. code-block:: mini

      regex::escape("Word (?)\n") :> "Word \\(\\?\\)\\n"


:mini:`fun string::escape(String: string): string`
   Escapes characters in :mini:`String`.

   .. code-block:: mini

      string::escape("\'Hello\nworld!\'")
      :> "\\\'Hello\\nworld!\\\'"


:mini:`meth (Arg₁: string) != (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ != Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" != "World" :> "World"
      "World" != "Hello" :> "Hello"
      "Hello" != "Hello" :> nil
      "abcd" != "abc" :> "abc"
      "abc" != "abcd" :> "abcd"


:mini:`meth (String: string) !? (Pattern: regex): string | nil`
   Returns :mini:`String` if it does not match :mini:`Pattern` and :mini:`nil` otherwise.

   .. code-block:: mini

      "2022-03-08" !? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)" :> nil
      "Not a date" !? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
      :> "Not a date"


:mini:`meth (String: string) % (Pattern: regex): tuple[string] | nil`
   Matches :mini:`String` with :mini:`Pattern` returning a tuple of the matched components,  or :mini:`nil` if the pattern does not match.

   .. code-block:: mini

      "2022-03-08" % r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
      :> (2022-03-08, 2022, 03, 08)
      "Not a date" % r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)" :> nil


:mini:`meth (String: string) */ (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.

   .. code-block:: mini

      "2022/03/08" */ r"[/-]" :> (2022/03, 08)
      "2022-03-08" */ r"[/-]" :> (2022-03, 08)


:mini:`meth (String: string) */ (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.

   .. code-block:: mini

      "2022/03/08" */ "/" :> (2022/03, 08)


:mini:`meth (A: string) + (B: string): string`
   Returns :mini:`A` and :mini:`B` concatentated.

   .. code-block:: mini

      "Hello" + " " + "world" :> "Hello world"


:mini:`meth (Start: string) .. (Limit: string): string::interval`
   Returns a interval from the first character of :mini:`Start` to the first character of :mini:`Limit` (inclusive).


:mini:`meth (String: string) / (Pattern: string): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`. Adjacent occurences of :mini:`Pattern` do not create empty strings.

   .. code-block:: mini

      "The cat snored  as he slept" / " "
      :> ["The", "cat", "snored", "as", "he", "slept"]
      "2022/03/08" / "/" :> ["2022", "03", "08"]


:mini:`meth (String: string) /* (Pattern: regex): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.

   .. code-block:: mini

      "2022/03/08" /* r"[/-]" :> (2022, 03/08)
      "2022-03-08" /* r"[/-]" :> (2022, 03-08)


:mini:`meth (String: string) /* (Pattern: string): tuple[string,  string]`
   Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.

   .. code-block:: mini

      "2022/03/08" /* "/" :> (2022, 03/08)


:mini:`meth (Arg₁: string) < (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ < Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" < "World" :> "World"
      "World" < "Hello" :> nil
      "Hello" < "Hello" :> nil
      "abcd" < "abc" :> nil
      "abc" < "abcd" :> "abcd"


:mini:`meth (Arg₁: string) <= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ <= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" <= "World" :> "World"
      "World" <= "Hello" :> nil
      "Hello" <= "Hello" :> "Hello"
      "abcd" <= "abc" :> nil
      "abc" <= "abcd" :> "abcd"


:mini:`meth (A: string) <> (B: string): integer`
   Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`,  :mini:`0` or :mini:`1` respectively.

   .. code-block:: mini

      "Hello" <> "World" :> -1
      "World" <> "Hello" :> 1
      "Hello" <> "Hello" :> 0
      "abcd" <> "abc" :> 1
      "abc" <> "abcd" :> -1


:mini:`meth (Arg₁: string) = (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ = Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" = "World" :> nil
      "World" = "Hello" :> nil
      "Hello" = "Hello" :> "Hello"
      "abcd" = "abc" :> nil
      "abc" = "abcd" :> nil


:mini:`meth (Arg₁: string) > (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ > Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" > "World" :> nil
      "World" > "Hello" :> "Hello"
      "Hello" > "Hello" :> nil
      "abcd" > "abc" :> "abc"
      "abc" > "abcd" :> nil


:mini:`meth (Arg₁: string) >= (Arg₂: string): string | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ >= Arg₂` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello" >= "World" :> nil
      "World" >= "Hello" :> "Hello"
      "Hello" >= "Hello" :> "Hello"
      "abcd" >= "abc" :> "abc"
      "abc" >= "abcd" :> nil


:mini:`meth (String: string) ? (Pattern: regex): string | nil`
   Returns :mini:`String` if it matches :mini:`Pattern` and :mini:`nil` otherwise.

   .. code-block:: mini

      "2022-03-08" ? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
      :> "2022-03-08"
      "Not a date" ? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)" :> nil


:mini:`meth (String: string)[Interval: integer::interval]: string`
   Returns the substring of :mini:`String` corresponding to :mini:`Interval` inclusively.


:mini:`meth (String: string)[Index: integer]: string | nil`
   Returns the substring of :mini:`String` of length 1 at :mini:`Index`.

   .. code-block:: mini

      let S := "λ:😀 → 😺"
      map(-7 .. 7 => (2, 2 -> S[_]))
      :> {-7 is "λ", -6 is ":", -5 is "😀", -4 is " ", -3 is "→", -2 is " ", -1 is "😺", 0, 1 is "λ", 2 is ":", 3 is "😀", 4 is " ", 5 is "→", 6 is " ", 7 is "😺"}


:mini:`meth (String: string)[Start: integer, End: integer]: string | nil`
   Returns the substring of :mini:`String` from :mini:`Start` to :mini:`End - 1` inclusively.


:mini:`meth (String: string):after(Delimiter: string): string | nil`
   Returns the portion of :mini:`String` after the 1st occurence of :mini:`Delimiter`,  or :mini:`nil` if no occurence if found.

   .. code-block:: mini

      "2022/03/08":after("/") :> "03/08"


:mini:`meth (String: string):after(Delimiter: string, N: integer): string | nil`
   Returns the portion of :mini:`String` after the :mini:`N`-th occurence of :mini:`Delimiter`,  or :mini:`nil` if no :mini:`N`-th occurence if found.
   If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.

   .. code-block:: mini

      "2022/03/08":after("/", 2) :> "08"


:mini:`meth (String: string):before(Delimiter: string): string | nil`
   Returns the portion of :mini:`String` before the 1st occurence of :mini:`Delimiter`,  or :mini:`nil` if no occurence if found.

   .. code-block:: mini

      "2022/03/08":before("/") :> "2022"


:mini:`meth (String: string):before(Delimiter: string, N: integer): string | nil`
   Returns the portion of :mini:`String` before the :mini:`N`-th occurence of :mini:`Delimiter`,  or :mini:`nil` if no :mini:`N`-th occurence if found.
   If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.

   .. code-block:: mini

      "2022/03/08":before("/", 2) :> "2022/03"


:mini:`meth (Arg₁: string):cname`
   *TBD*


:mini:`meth (String: string):code: integer`
   Returns the unicode codepoint of the first UTF-8 character of :mini:`String`.

   .. code-block:: mini

      "A":code :> 65
      "😀️":code :> 128512


:mini:`meth (Haystack: string):contains(Pattern: regex): string | nil`
   Returns the :mini:`Haystack` if it contains :mini:`Pattern` or :mini:`nil` otherwise.

   .. code-block:: mini

      "The cat snored as he slept":contains(r"[a-z]{3}")
      :> "The cat snored as he slept"
      "The cat snored as he slept":contains(r"[0-9]+") :> nil


:mini:`meth (Haystack: string):contains(Needle: string): string | nil`
   Returns the :mini:`Haystack` if it contains :mini:`Pattern` or :mini:`nil` otherwise.

   .. code-block:: mini

      "The cat snored as he slept":contains("cat")
      :> "The cat snored as he slept"
      "The cat snored as he slept":contains("dog") :> nil


:mini:`meth (String: string):count: integer`
   Returns the number of UTF-8 characters in :mini:`String`. Use :mini:`:size` to get the number of bytes.

   .. code-block:: mini

      "Hello world":count :> 11
      "Hello world":size :> 11
      "λ:😀 → 😺":count :> 7
      "λ:😀 → 😺":size :> 16


:mini:`meth (String: string):ctype: string::ctype`
   Returns the unicode type of the first character of :mini:`String`.

   .. code-block:: mini

      map("To €2 á\n" => (2, 2 -> :ctype))
      :> {"T" is Lu, "o" is Ll, " " is Zs, "€" is Sc, "2" is Nd, "á" is Ll, "\n" is Cc}


:mini:`meth (String: string):ends(Suffix: string): string | nil`
   Returns :mini:`String` if it ends with :mini:`Suffix` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello world":ends("world") :> "Hello world"
      "Hello world":ends("cake") :> nil


:mini:`meth (String: string):escape: string`
   Returns :mini:`String` with white space,  quotes and backslashes replaced by escape sequences.

   .. code-block:: mini

      "\t\"Text\"\r\n":escape :> "\\t\\\"Text\\\"\\r\\n"


:mini:`meth (Haystack: string):find(Pattern: regex): integer | nil`
   Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find(r"[a-z]{3}") :> 5
      "The cat snored as he slept":find(r"[0-9]+") :> nil


:mini:`meth (Haystack: string):find(Pattern: regex, Start: integer): integer | nil`
   Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find(r"s[a-z]+", 1) :> 9
      "The cat snored as he slept":find(r"s[a-z]+", 10) :> 22
      "The cat snored as he slept":find(r"s[a-z]+", -6) :> 22


:mini:`meth (Haystack: string):find(Needle: string): integer | nil`
   Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find("cat") :> 5
      "The cat snored as he slept":find("dog") :> nil


:mini:`meth (Haystack: string):find(Needle: string, Start: integer): integer | nil`
   Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find("s", 1) :> 9
      "The cat snored as he slept":find("s", 10) :> 17
      "The cat snored as he slept":find("s", -6) :> 22


:mini:`meth (Haystack: string):find2(Pattern: regex): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2(r"[a-z]{3}")
      :> (5, cat)
      "The cat snored as he slept":find2(r"[0-9]+") :> nil


:mini:`meth (Haystack: string):find2(Pattern: regex, Start: integer): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2(r"s[a-z]+", 1)
      :> (9, snored)
      "The cat snored as he slept":find2(r"s[a-z]+", 10)
      :> (22, slept)
      "The cat snored as he slept":find2(r"s[a-z]+", -6)
      :> (22, slept)


:mini:`meth (Haystack: string):find2(Pattern: regex, Start: tuple::integer::string): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2(r"s[a-z]+", 1)
      :> (9, snored)
      "The cat snored as he slept":find2(r"s[a-z]+", 10)
      :> (22, slept)
      "The cat snored as he slept":find2(r"s[a-z]+", -6)
      :> (22, slept)


:mini:`meth (Haystack: string):find2(Needle: string): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2("cat") :> (5, cat)
      "The cat snored as he slept":find2("dog") :> nil


:mini:`meth (Haystack: string):find2(Needle: string, Start: integer): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2("s", 1) :> (9, s)
      "The cat snored as he slept":find2("s", 10) :> (17, s)
      "The cat snored as he slept":find2("s", -6) :> (22, s)


:mini:`meth (Haystack: string):find2(Needle: string, Start: tuple::integer::string): tuple[integer, string] | nil`
   Returns :mini:`(Index,  Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`,  or :mini:`nil` if no occurence is found.

   .. code-block:: mini

      "The cat snored as he slept":find2("s", 1) :> (9, s)
      "The cat snored as he slept":find2("s", 10) :> (17, s)
      "The cat snored as he slept":find2("s", -6) :> (22, s)


:mini:`meth (String: string):length: integer`
   Returns the number of UTF-8 characters in :mini:`String`. Use :mini:`:size` to get the number of bytes.

   .. code-block:: mini

      "Hello world":length :> 11
      "Hello world":size :> 11
      "λ:😀 → 😺":length :> 7
      "λ:😀 → 😺":size :> 16


:mini:`meth (String: string):limit(Length: integer): string`
   Returns the prefix of :mini:`String` limited to :mini:`Length`.

   .. code-block:: mini

      "Hello world":limit(5) :> "Hello"
      "Cake":limit(5) :> "Cake"


:mini:`meth (String: string):lower: string`
   Returns :mini:`String` with each character converted to lower case.

   .. code-block:: mini

      "Hello World":lower :> "hello world"


:mini:`meth (String: string):ltrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.

   .. code-block:: mini

      " \t Hello \n":ltrim :> "Hello \n"


:mini:`meth (String: string):ltrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.

   .. code-block:: mini

      " \t Hello \n":trim(" \n") :> "\t Hello"


:mini:`meth (A: string):max(B: string): integer`
   Returns :mini:`max(A,  B)`

   .. code-block:: mini

      "Hello":max("World") :> "World"
      "World":max("Hello") :> "World"
      "abcd":max("abc") :> "abcd"
      "abc":max("abcd") :> "abcd"


:mini:`meth (A: string):min(B: string): integer`
   Returns :mini:`min(A,  B)`

   .. code-block:: mini

      "Hello":min("World") :> "Hello"
      "World":min("Hello") :> "Hello"
      "abcd":min("abc") :> "abc"
      "abc":min("abcd") :> "abc"


:mini:`meth (String: string):normalize(Norm: string::norm): string`
   Returns a normalized copy of :mini:`String` using the normalizer specified by :mini:`Norm`.

   .. code-block:: mini

      let S := "𝕥𝕖𝕩𝕥" :> "𝕥𝕖𝕩𝕥"
      S:normalize(string::norm::NFD) :> "𝕥𝕖𝕩𝕥"


:mini:`meth (String: string):offset(Index: integer): integer`
   Returns the byte position of the :mini:`Index`-th character of :mini:`String`.

   .. code-block:: mini

      let S := "λ:😀 → 😺"
      list(1 .. S:length, S:offset(_))
      :> [0, 2, 3, 7, 8, 11, 12]


:mini:`meth (String: string):precount: integer`
   Returns the number of UTF-8 characters in :mini:`String`. Use :mini:`:size` to get the number of bytes.

   .. code-block:: mini

      "Hello world":count :> 11
      "Hello world":size :> 11
      "λ:😀 → 😺":count :> 7
      "λ:😀 → 😺":size :> 16


:mini:`meth (String: string):replace(I: integer, Fn: function): string`
   Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Fn(String[I])`.

   .. code-block:: mini

      "hello world":replace(1, :upper) :> "Hello world"


:mini:`meth (String: string):replace(I: integer, Fn: integer, Arg₄: function): string`
   Returns a copy of :mini:`String` with the :mini:`String[I,  J]` is replaced by :mini:`Fn(String[I,  J])`.

   .. code-block:: mini

      "hello world":replace(1, 6, :upper) :> "HELLO world"


:mini:`meth (String: string):replace(I: integer, J: integer, Replacement: string): string`
   Returns a copy of :mini:`String` with the :mini:`String[I,  J]` is replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace(1, 6, "Goodbye") :> "Goodbye world"
      "Hello world":replace(-6, 0, ", how are you?")
      :> "Hello, how are you?"


:mini:`meth (String: string):replace(I: integer, Replacement: string): string`
   Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace(6, "_") :> "Hello_world"


:mini:`meth (String: string):replace(Replacements: map): string`
   Each key in :mini:`Replacements` can be either a string or a regex. Each value in :mini:`Replacements` can be either a string or a function.
   Returns a copy of :mini:`String` with each matching string or regex from :mini:`Replacements` replaced with the corresponding value. Functions are called with the matched string or regex subpatterns.

   .. code-block:: mini

      "the dog snored as he slept":replace({
         r" ([a-z])" is fun(Match, A) '-{A:upper}',
         "nor" is "narl"
      }) :> "the-Dog-Snarled-As-He-Slept"


:mini:`meth (String: string):replace(Pattern: regex, Fn: function): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Fn(Match,  Sub₁,  ...,  Subₙ)` where :mini:`Match` is the actual matched text and :mini:`Subᵢ` are the matched subpatterns.

   .. code-block:: mini

      "the cat snored as he slept":replace(r" ([a-z])", fun(Match, A) '-{A:upper}')
      :> "the-Cat-Snored-As-He-Slept"


:mini:`meth (String: string):replace(Pattern: regex, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace(r"l+", "bb") :> "Hebbo worbbd"


:mini:`meth (String: string):replace(Pattern: string, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace("l", "bb") :> "Hebbbbo worbbd"


:mini:`meth (String: string):replace2(Replacements: map): string`
   Each key in :mini:`Replacements` can be either a string or a regex. Each value in :mini:`Replacements` can be either a string or a function.
   Returns a copy of :mini:`String` with each matching string or regex from :mini:`Replacements` replaced with the corresponding value. Functions are called with the matched string or regex subpatterns.

   .. code-block:: mini

      "the dog snored as he slept":replace2({
         r" ([a-z])" is fun(Match, A) '-{A:upper}',
         "nor" is "narl"
      }) :> (the-Dog-Snarled-As-He-Slept, 6)


:mini:`meth (String: string):replace2(Pattern: regex, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace2(r"l+", "bb") :> (Hebbo worbbd, 2)


:mini:`meth (String: string):replace2(Pattern: string, Replacement: string): string`
   Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.

   .. code-block:: mini

      "Hello world":replace2("l", "bb") :> (Hebbbbo worbbd, 3)


:mini:`meth (String: string):reverse: string`
   Returns a string with the characters in :mini:`String` reversed.

   .. code-block:: mini

      "Hello world":reverse :> "dlrow olleH"


:mini:`meth (String: string):rtrim: string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.

   .. code-block:: mini

      " \t Hello \n":rtrim :> " \t Hello"


:mini:`meth (String: string):rtrim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.

   .. code-block:: mini

      " \t Hello \n":rtrim(" \n") :> " \t Hello"


:mini:`meth (String: string):starts(Pattern: regex): string | nil`
   Returns :mini:`String` if it starts with :mini:`Pattern` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello world":starts(r"[A-Z]") :> "Hello world"
      "Hello world":starts(r"[0-9]") :> nil


:mini:`meth (String: string):starts(Prefix: string): string | nil`
   Returns :mini:`String` if it starts with :mini:`Prefix` and :mini:`nil` otherwise.

   .. code-block:: mini

      "Hello world":starts("Hello") :> "Hello world"
      "Hello world":starts("cake") :> nil


:mini:`meth stringtable(Arg₁: string)`
   *TBD*


:mini:`meth (String: string):title: string`
   Returns :mini:`String` with the first character and each character after whitespace converted to upper case and each other case converted to lower case.

   .. code-block:: mini

      "hello world":title :> "Hello World"
      "HELLO WORLD":title :> "Hello World"


:mini:`meth (String: string):trim: string`
   Returns a copy of :mini:`String` with whitespace removed from both ends.

   .. code-block:: mini

      " \t Hello \n":trim :> "Hello"


:mini:`meth (String: string) / (Pattern: regex, Index: integer): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.
   Only the :mini:`Index` subgroup matches are removed from the output substrings.

   .. code-block:: mini

      "<A>-<B>-<C>" / (r">(-)<", 1) :> ["<A>", "<B>", "<C>"]


:mini:`meth (String: string) / (Pattern: regex): list`
   Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.
   If :mini:`Pattern` contains subgroups then only the subgroup matches are removed from the output substrings.

   .. code-block:: mini

      "2022/03/08" / r"[/-]" :> ["2022", "03", "08"]
      "2022-03-08" / r"[/-]" :> ["2022", "03", "08"]


:mini:`meth (String: string):trim(Chars: string): string`
   Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from both ends.

   .. code-block:: mini

      " \t Hello \n":trim(" \n") :> "\t Hello"


:mini:`meth (String: string):upper: string`
   Returns :mini:`String` with each character converted to upper case.

   .. code-block:: mini

      "Hello World":upper :> "HELLO WORLD"


:mini:`meth (Arg₁: string):utf8`
   *TBD*


:mini:`meth (A: string) ~ (B: string): integer`
   Returns the edit distance between :mini:`A` and :mini:`B`.

   .. code-block:: mini

      "cake" ~ "cat" :> 2
      "yell" ~ "hello" :> 2
      "say" ~ "goodbye" :> 6
      "goodbye" ~ "say" :> 6
      "λ:😀 → Y" ~ "λ:X → 😺" :> 2


:mini:`meth (A: string) ~> (B: string): integer`
   Returns an asymmetric edit distance from :mini:`A` to :mini:`B`.

   .. code-block:: mini

      "cake" ~> "cat" :> 1
      "yell" ~> "hello" :> 2
      "say" ~> "goodbye" :> 6
      "goodbye" ~> "say" :> 3
      "λ:😀 → Y" ~> "λ:X → 😺" :> 4


:mini:`meth (Buffer: string::buffer):append(Value: string)`
   Appends :mini:`Value` to :mini:`Buffer`.


:mini:`meth (Arg₁: string::buffer):append(Arg₂: string, Arg₃: string)`
   *TBD*


:mini:`type string::buffer`
   A string buffer that automatically grows and shrinks as required.


:mini:`meth string::buffer(): string::buffer`
   Returns a new :mini:`string::buffer`


:mini:`meth string::buffer(Arg₁: address): string::buffer`
   Returns a new :mini:`string::buffer`


:mini:`meth (Buffer: string::buffer):get: string`
   Returns the contents of :mini:`Buffer` as a string and clears :mini:`Buffer`.
   .. deprecated:: 2.5.0
   
      Use :mini:`Buffer:rest` instead.

   .. code-block:: mini

      let B := string::buffer()
      B:write("Hello world")
      B:get :> "Hello world"
      B:get :> ""


:mini:`meth (Arg₁: string::buffer):grow(Arg₂: sequence)`
   *TBD*


:mini:`meth (Buffer: string::buffer):length: integer`
   Returns the number of bytes currently available in :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer()
      B:write("Hello world")
      B:length :> 11


:mini:`meth (Buffer: string::buffer):read16: integer`
   Reads a signed 16-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read16 :> 25928


:mini:`meth (Buffer: string::buffer):read16(Arg₂: byte::order): integer`
   Reads a signed 16-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read16(address::LE) :> 25928
      B:read16(address::BE) :> 27756


:mini:`meth (Buffer: string::buffer):read16(Arg₂: byte::order): integer`
   Reads a signed 16-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu16(address::LE)
      :> error("MethodError", "no method found for readu16(string::buffer, address::byteorder)")
      B:readu16(address::BE)
      :> error("MethodError", "no method found for readu16(string::buffer, address::byteorder)")


:mini:`meth (Buffer: string::buffer):read32: integer`
   Reads a signed 32-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read32 :> 1819043144


:mini:`meth (Buffer: string::buffer):read32(Arg₂: byte::order): integer`
   Reads a signed 32-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read32(address::LE) :> 1819043144
      B:read32(address::BE) :> 1864398703


:mini:`meth (Buffer: string::buffer):read32(Arg₂: byte::order): integer`
   Reads a signed 32-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu32(address::LE)
      :> error("MethodError", "no method found for readu32(string::buffer, address::byteorder)")
      B:readu32(address::BE)
      :> error("MethodError", "no method found for readu32(string::buffer, address::byteorder)")


:mini:`meth (Buffer: string::buffer):read64: integer`
   Reads a signed 64-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read64 :> 8031924123371070792


:mini:`meth (Buffer: string::buffer):read64(Arg₂: byte::order): integer`
   Reads a signed 64-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:read64(address::LE) :> 8031924123371070792
      B:read64(address::BE) :> nil


:mini:`meth (Buffer: string::buffer):read64(Arg₂: byte::order): integer`
   Reads a signed 64-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu64(address::LE)
      :> error("MethodError", "no method found for readu64(string::buffer, address::byteorder)")
      B:readu64(address::BE)
      :> error("MethodError", "no method found for readu64(string::buffer, address::byteorder)")


:mini:`meth (Buffer: string::buffer):read8: integer`
   Reads a signed 8-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("\xCC") :> <string::buffer>
      B:read8 :> -52


:mini:`meth (Buffer: string::buffer):readu16: integer`
   Reads a signed 16-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu16 :> 25928


:mini:`meth (Buffer: string::buffer):readu32: integer`
   Reads a signed 32-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu32 :> 1819043144


:mini:`meth (Buffer: string::buffer):readu64: integer`
   Reads a signed 64-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("Hello world!\n")
      :> <string::buffer>
      B:readu64 :> 8031924123371070792


:mini:`meth (Buffer: string::buffer):readu8: integer`
   Reads a signed 8-bit value from :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer("\xCC") :> <string::buffer>
      B:readu8 :> 204


:mini:`meth (Buffer: string::buffer):rest: string`
   Returns the contents of :mini:`Buffer` as a string and clears :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer()
      B:write("Hello world")
      B:rest :> "Hello world"
      B:rest :> ""


:mini:`meth (Buffer: string::buffer):unread(Bytes: address): string::buffer`
   Inserts the contents of :mini:`Bytes` at the start of :mini:`Buffer`.


:mini:`meth (Buffer: string::buffer):write(Value₁, : any, ...): integer`
   Writes each :mini:`Valueᵢ` in turn to :mini:`Buffer`.

   .. code-block:: mini

      let B := string::buffer()
      B:write("1 + 1 = ", 1 + 1)
      B:rest :> "1 + 1 = 2"


:mini:`meth (Buffer: string::buffer):write16(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 16-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write16(12345) :> <string::buffer>
      B:rest :> "90"


:mini:`meth (Buffer: string::buffer):write16(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 16-bit signed value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write16(12345, address::LE) :> <string::buffer>
      B:rest :> "90"
      B:write16(12345, address::BE) :> <string::buffer>
      B:rest :> "09"


:mini:`meth (Buffer: string::buffer):write32(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 32-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write32(12345) :> <string::buffer>
      B:rest :> "90\0\0"


:mini:`meth (Buffer: string::buffer):write32(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 32-bit signed value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write32(12345, address::LE) :> <string::buffer>
      B:rest :> "90\0\0"
      B:write32(12345, address::BE) :> <string::buffer>
      B:rest :> "\0\009"


:mini:`def MLStringProperties: string::properties`
   *TBD*


:mini:`meth (Buffer: string::buffer):write64(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 64-bit signed value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write64(12345) :> <string::buffer>
      B:rest :> "90\0\0\0\0\0\0"


:mini:`meth (Buffer: string::buffer):write64(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 64-bit signed value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write64(12345, address::LE) :> <string::buffer>
      B:rest :> "90\0\0\0\0\0\0"
      B:write64(12345, address::BE) :> <string::buffer>
      B:rest :> "\0\0\0\0\0\009"


:mini:`meth (Buffer: string::buffer):write8(Value: integer): buffer`
   Write :mini:`Value` to :mini:`Buffer` as an 8-bit signed value.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:write8(120) :> <string::buffer>
      B:rest :> "x"


:mini:`meth (Buffer: string::buffer):writeu16(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 16-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu16(12345) :> <string::buffer>
      B:rest :> "90"


:mini:`meth (Buffer: string::buffer):writeu16(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 16-bit unsigned value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu16(12345, address::LE) :> <string::buffer>
      B:rest :> "90"
      B:writeu16(12345, address::BE) :> <string::buffer>
      B:rest :> "09"


:mini:`meth (Buffer: string::buffer):writeu32(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 32-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu32(12345) :> <string::buffer>
      B:rest :> "90\0\0"


:mini:`meth (Buffer: string::buffer):writeu32(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 32-bit unsigned value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu32(12345, address::LE) :> <string::buffer>
      B:rest :> "90\0\0"
      B:writeu32(12345, address::BE) :> <string::buffer>
      B:rest :> "\0\009"


:mini:`meth (Buffer: string::buffer):writeu64(Value: integer): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 64-bit unsigned value. Uses the platform byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu64(12345) :> <string::buffer>
      B:rest :> "90\0\0\0\0\0\0"


:mini:`meth (Buffer: string::buffer):writeu64(Value: integer, Arg₃: byte::order): buffer`
   Writes :mini:`Value` to :mini:`Buffer` as an 64-bit unsigned value. Uses the given byte order.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu64(12345, address::LE) :> <string::buffer>
      B:rest :> "90\0\0\0\0\0\0"
      B:writeu64(12345, address::BE) :> <string::buffer>
      B:rest :> "\0\0\0\0\0\009"


:mini:`meth (Buffer: string::buffer):writeu8(Value: integer): buffer`
   Write :mini:`Value` to :mini:`Buffer` as an 8-bit signed value.

   .. code-block:: mini

      let B := string::buffer() :> <string::buffer>
      B:writeu8(120) :> <string::buffer>
      B:rest :> "x"


:mini:`type string::charset < sequence`
   *TBD*


:mini:`meth (Arg₁: string::charset)[Arg₂: integer]`
   *TBD*


:mini:`meth (Arg₁: string::charset):count`
   *TBD*


:mini:`type string::ctype < enum`
   * :mini:`::Cn` - General Other Types
   * :mini:`::Lu` - Uppercase Letter
   * :mini:`::Ll` - Lowercase Letter
   * :mini:`::Lt` - Titlecase Letter
   * :mini:`::Lm` - Modifier Letter
   * :mini:`::Lo` - Other Letter
   * :mini:`::Mn` - Non Spacing Mark
   * :mini:`::Me` - Enclosing Mark
   * :mini:`::Mc` - Combining Spacing Mark
   * :mini:`::Nd` - Decimal Digit Number
   * :mini:`::Nl` - Letter Number
   * :mini:`::No` - Other Number
   * :mini:`::Zs` - Space Separator
   * :mini:`::Zl` - Line Separator
   * :mini:`::Zp` - Paragraph Separator
   * :mini:`::Cc` - Control Char
   * :mini:`::Cf` - Format Char
   * :mini:`::Co` - Private Use Char
   * :mini:`::Cs` - Surrogate
   * :mini:`::Pd` - Dash Punctuation
   * :mini:`::Ps` - Start Punctuation
   * :mini:`::Pe` - End Punctuation
   * :mini:`::Pc` - Connector Punctuation
   * :mini:`::Po` - Other Punctuation
   * :mini:`::Sm` - Math Symbol
   * :mini:`::Sc` - Currency Symbol
   * :mini:`::Sk` - Modifier Symbol
   * :mini:`::So` - Other Symbol
   * :mini:`::Pi` - Initial Punctuation
   * :mini:`::Pf` - Final Punctuation


:mini:`type string::interval < integer::interval`
   *TBD*


:mini:`type string::norm < enum`
   * :mini:`::NFC`
   * :mini:`::NFD`
   * :mini:`::NFKC`
   * :mini:`::NFKD`


:mini:`meth (Arg₁: string::properties) :: (Arg₂: string)`
   *TBD*


:mini:`type string::property < sequence`
   *TBD*


:mini:`meth (Arg₁: string::property) :: (Arg₂: string)`
   *TBD*


:mini:`meth (Arg₁: string::property)[Arg₂: integer]`
   *TBD*


:mini:`meth (Arg₁: string::property)[Arg₂: string]`
   *TBD*


:mini:`meth (Arg₁: string::property):max`
   *TBD*


:mini:`meth (Arg₁: string::property):min`
   *TBD*


:mini:`meth stringcharset(Arg₁: string::property::value)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: string::property::value)`
   *TBD*


:mini:`type string::table`
   *TBD*


:mini:`meth (Arg₁: string::table):describe`
   *TBD*


:mini:`meth (Arg₁: string::table):splice(Arg₂: integer, Arg₃: integer, Arg₄: string)`
   *TBD*


:mini:`meth (Arg₁: string::buffer):append(Arg₂: string::table)`
   *TBD*


:mini:`meth stringtable()`
   *TBD*


:mini:`fun string::switch(Cases: string|regex, ...)`
   Implements :mini:`switch` for string values. Case values must be strings or regular expressions.

   .. code-block:: mini

      for Pet in ["cat", "dog", "mouse", "fox"] do
         switch Pet: string
            case "cat" do
               print("Meow!\n")
            case "dog" do
               print("Woof!\n")
            case "mouse" do
               print("Squeak!\n")
            else
               print("???!")
            end
      end :> nil

   .. code-block:: console

      Meow!
      Woof!
      Squeak!
      ???!

:mini:`meth (Arg₁: visitor):const(Arg₂: buffer)`
   *TBD*


:mini:`meth (Arg₁: visitor):copy(Arg₂: buffer)`
   *TBD*


