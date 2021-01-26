/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -LANSI-C -C -c -t -KName -Zkeyword -Hhash -Nlookup -m 100 keywords.lst  */
/* Computed positions: -k'1-2' */

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

#define TOTAL_KEYWORDS 32
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 6
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 32
/* maximum key range = 32, duplicates = 0 */

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
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33,  0, 33, 25, 33, 33,
      24,  2,  8, 33, 13, 11, 33, 33,  4, 26,
       0,  0, 33, 33, 22, 10,  2, 11,  2,  0,
      14, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
      33, 33, 33, 33, 33, 33
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

const struct keyword_t *
lookup (register const char *str, register size_t len)
{
  static const struct keyword_t wordlist[] =
    {
      {""},
#line 34 "keywords.lst"
      {"_", MLT_BLANK},
#line 25 "keywords.lst"
      {"on", MLT_ON},
#line 29 "keywords.lst"
      {"not", MLT_NOT},
#line 15 "keywords.lst"
      {"to", MLT_TO},
#line 7 "keywords.lst"
      {"end", MLT_END},
#line 12 "keywords.lst"
      {"next", MLT_NEXT},
#line 30 "keywords.lst"
      {"old", MLT_OLD},
#line 8 "keywords.lst"
      {"loop", MLT_LOOP},
#line 32 "keywords.lst"
      {"let", MLT_LET},
#line 6 "keywords.lst"
      {"else", MLT_ELSE},
#line 13 "keywords.lst"
      {"for", MLT_FOR},
#line 5 "keywords.lst"
      {"elseif", MLT_ELSEIF},
#line 16 "keywords.lst"
      {"in", MLT_IN},
#line 26 "keywords.lst"
      {"nil", MLT_NIL},
#line 23 "keywords.lst"
      {"with", MLT_WITH},
#line 10 "keywords.lst"
      {"until", MLT_UNTIL},
#line 18 "keywords.lst"
      {"when", MLT_WHEN},
#line 9 "keywords.lst"
      {"while", MLT_WHILE},
#line 4 "keywords.lst"
      {"then", MLT_THEN},
#line 11 "keywords.lst"
      {"exit", MLT_EXIT},
#line 3 "keywords.lst"
      {"if", MLT_IF},
#line 19 "keywords.lst"
      {"fun", MLT_FUN},
#line 17 "keywords.lst"
      {"is", MLT_IS},
#line 28 "keywords.lst"
      {"or", MLT_OR},
#line 21 "keywords.lst"
      {"susp", MLT_SUSP},
#line 24 "keywords.lst"
      {"do", MLT_DO},
#line 20 "keywords.lst"
      {"ret", MLT_RET},
#line 27 "keywords.lst"
      {"and", MLT_AND},
#line 31 "keywords.lst"
      {"def", MLT_DEF},
#line 33 "keywords.lst"
      {"var", MLT_VAR},
#line 14 "keywords.lst"
      {"each", MLT_EACH},
#line 22 "keywords.lst"
      {"meth", MLT_METH}
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
