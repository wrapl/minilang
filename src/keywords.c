/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -LANSI-C -C -c -t -KName -Zkeyword -Hhash -Nlookup -m 100 keywords.lst  */
/* Computed positions: -k'1,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "keywords.lst"
struct keyword_t {const char *Name; int Token;};

#define TOTAL_KEYWORDS 34
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 6
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 44
/* maximum key range = 44, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45,  0,  0, 45,  3, 45,
      26,  5,  1, 11, 11, 45,  3, 45, 28, 24,
       9,  0, 15, 45,  8,  8,  0, 15,  0, 18,
       5, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
      45, 45, 45, 45, 45, 45, 45
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]+1];
}

const struct keyword_t *
lookup (register const char *str, register size_t len)
{
  static const struct keyword_t wordlist[] =
    {
      {""},
#line 36 "keywords.lst"
      {"_", MLT_BLANK},
      {""},
#line 30 "keywords.lst"
      {"not", MLT_NOT},
#line 12 "keywords.lst"
      {"next", MLT_NEXT},
#line 11 "keywords.lst"
      {"exit", MLT_EXIT},
#line 3 "keywords.lst"
      {"if", MLT_IF},
#line 25 "keywords.lst"
      {"do", MLT_DO},
#line 5 "keywords.lst"
      {"elseif", MLT_ELSEIF},
#line 32 "keywords.lst"
      {"def", MLT_DEF},
#line 6 "keywords.lst"
      {"else", MLT_ELSE},
#line 20 "keywords.lst"
      {"ret", MLT_RET},
#line 34 "keywords.lst"
      {"ref", MLT_REF},
#line 17 "keywords.lst"
      {"is", MLT_IS},
#line 16 "keywords.lst"
      {"in", MLT_IN},
#line 9 "keywords.lst"
      {"while", MLT_WHILE},
#line 14 "keywords.lst"
      {"each", MLT_EACH},
#line 15 "keywords.lst"
      {"to", MLT_TO},
#line 18 "keywords.lst"
      {"when", MLT_WHEN},
#line 21 "keywords.lst"
      {"susp", MLT_SUSP},
#line 24 "keywords.lst"
      {"with", MLT_WITH},
#line 22 "keywords.lst"
      {"debug", MLT_DEBUG},
#line 13 "keywords.lst"
      {"for", MLT_FOR},
#line 19 "keywords.lst"
      {"fun", MLT_FUN},
#line 23 "keywords.lst"
      {"meth", MLT_METH},
#line 29 "keywords.lst"
      {"or", MLT_OR},
#line 26 "keywords.lst"
      {"on", MLT_ON},
#line 33 "keywords.lst"
      {"let", MLT_LET},
#line 4 "keywords.lst"
      {"then", MLT_THEN},
#line 35 "keywords.lst"
      {"var", MLT_VAR},
#line 7 "keywords.lst"
      {"end", MLT_END},
#line 27 "keywords.lst"
      {"nil", MLT_NIL},
#line 28 "keywords.lst"
      {"and", MLT_AND},
#line 10 "keywords.lst"
      {"until", MLT_UNTIL},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 8 "keywords.lst"
      {"loop", MLT_LOOP},
#line 31 "keywords.lst"
      {"old", MLT_OLD}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].Name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
