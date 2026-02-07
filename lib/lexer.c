#include <ezy_lexer.h>
#include <ezy_log.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#define ezylex_null_data ((union ezy_tkn_unit_t){.t_int64 = 0})
#define ezylex_blank_tok(t) ((ezy_tkn_t){.type = t, .data = ezylex_null_data})
#define ezylex_tknbuf_limit 16

static ezy_tkn_t ezylex_tknbuf[ezylex_tknbuf_limit];
static ezy_tkn_t ezylex_last_tok;
static const char *ezylex_tknbuf_headptr = NULL;
static size_t ezylex_tknbuf_head = 0;
static size_t ezylex_tknbuf_tail = 0;
static size_t ezylex_tknbuf_count = 0;

/* Line/column state */
static uint32_t ezylex_line = 1;
static uint32_t ezylex_col = 1;

/* Advance line/col state from start..end (end not included) */
static void ezylex_advance_pos(const char *start, const char *end)
{
  const char *s = start;
  while (s < end && *s)
  {
    if (*s == '\n')
    {
      ezylex_line += 1;
      ezylex_col = 1;
    }
    else if (*s != '\r')
    {
      ezylex_col++;
    }
    s++;
  }
}

ezy_tkn_t ezylex_peek_tkn_reverse(size_t n)
{
  if (n >= ezylex_tknbuf_count)
  {
    ezy_log_warn("Attempt to peek back %zu tokens, but only %zu tokens in buffer", n, ezylex_tknbuf_count);
    return ezylex_blank_tok(ezy_tkn_dummy);
  }
  size_t idx = (ezylex_tknbuf_head + ezylex_tknbuf_limit - 1 - n) % ezylex_tknbuf_limit;
  return ezylex_tknbuf[idx];
}

void ezylex_push_tkn(ezy_tkn_t tk)
{
  ezylex_tknbuf[ezylex_tknbuf_head] = tk;
  ezylex_tknbuf_head += 1;
  ezylex_tknbuf_head %= ezylex_tknbuf_limit;
  if (ezylex_tknbuf_count < ezylex_tknbuf_limit)
  {
    ezylex_tknbuf_count++;
  }
  else
  {
    ezylex_tknbuf_tail += 1;
    ezylex_tknbuf_tail %= ezylex_tknbuf_limit;
  }
}

void ezylex_start(const char *ptr)
{
  // reset state
  ezylex_line = 1;
  ezylex_col = 1;

  ezylex_tknbuf_head = 0;
  ezylex_tknbuf_tail = 0;
  ezylex_tknbuf_count = 0;
  ezylex_tknbuf_headptr = ptr;

  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_dummy);
  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = ptr;
  ezylex_push_tkn(tkn);
}

#define ezylex_str_startsw(s, pfx) (strncmp((s), (pfx), sizeof(pfx) - 1) == 0)

/* skip whitespace and update line/col state */
const char *ezylex_skip_ws(const char *ptr)
{
  while (*ptr)
  {
    if (*ptr == '\r')
      ptr++;

    if (*ptr == '\n')
    {
      ptr++;
      ezylex_line++;
      ezylex_col = 1;
    }
    else if (isspace((uint8_t)*ptr))
    {
      ptr++;
      ezylex_col++;
    }
    else
    {
      break;
    }
  };
  return ptr;
}

ezy_tkn_t ezylex_number(const char **ptr)
{
  const char *p = *ptr;
  const char *start = p;

  bool isFloat = false;
  bool isNegative = false;
  uint8_t base = 10;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);

  if (ezylex_str_startsw(p, "0b"))
  {
    p += 2;
    base = 2;
  }
  else if (ezylex_str_startsw(p, "0x"))
  {
    p += 2;
    base = 16;
  }

  if (*p == '-' || *p == '+')
  {
    if ( base != 10 )
    {
      // ezy_log_error("invalid leading '+' or '-' for non-decimal number literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
      tkn.data.t_string.ptr = "invalid leading '+' or '-' for non-decimal number literal";
      *ptr = p;
      return tkn;
    }
    isNegative = *p == '-';
    p++;
  }

  *ptr = p;

  while (*p)
  {
    if (base == 2 && (*p != '0' && *p != '1'))
      break;
    if (base == 10 && !isdigit((uint8_t)*p) && *p != '.')
      break;
    if (base == 16 && !isxdigit((uint8_t)*p))
      break;
    if (*p == '.')
    {
      if (isFloat)
      {
        // ezy_log_error("invalid number literal with multiple decimal points, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid number literal with multiple decimal points";
        *ptr = p;
        return tkn;
      }
      else if (base != 10)
      {
        // ezy_log_error("invalid float literal with non-decimal base, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid float literal with non-decimal base";
        *ptr = p;
        return tkn;
      }
      else if (!isdigit(*(p + 1)))
      {
        // ezy_log_error("invalid float literal with no digits after decimal point, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid float literal with no digits after decimal point";
        *ptr = p;
        return tkn;
      }
      isFloat = true;
    }
    p++;
  }

  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = *ptr;
  char *end = NULL;

  if (isFloat)
  {
    double v = strtod(*ptr, &end);
    if (isNegative)
    {
      v = -v;
    }
    if (end != p)
    {
      // ezy_log_error("invalid float literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
      tkn.data.t_string.ptr = "invalid float literal";
      *ptr = p;
      return tkn;
    }
    tkn = ezylex_blank_tok(ezy_tkn_float64);
    tkn.data.t_float64 = v;
  }
  else
  {
    if (base == 10)
    {
      // ezy_log("got to ptr = %s, isNegative: %d", *ptr, isNegative);
      uint64_t v = _strtoui64(*ptr, &end, 10);
      if ( isNegative && v > INT64_MAX + 1ULL )
      {
        ezy_log_error("invalid integer literal, value too small:" "%" PRIu64, v);
        tkn.data.t_string.ptr = "invalid integer literal, value too small to fit in int64";
        *ptr = p;
        return tkn;
      }
      if (end != p)
      {
        // ezy_log_error("invalid integer literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid integer literal";
        *ptr = p;
        return tkn;
      }
      tkn = ezylex_blank_tok(ezy_tkn_uint64);
      if ( isNegative ) {
        tkn.type = ezy_tkn_int64;
        tkn.data.t_int64 = -(int64_t)v;
      } else {
        tkn.data.t_uint64 = v;
      }
    }
    else
    {
      unsigned long long v = strtoull(*ptr, &end, base);
      if (end != p)
      {
        // ezy_log_error("invalid integer literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid integer literal";
        *ptr = p;
        return tkn;
      }
      tkn = ezylex_blank_tok(ezy_tkn_uint64);
      tkn.data.t_uint64 = v;
    }
  }
  /* update global position to end of token */
  ezylex_advance_pos(start, p);

  *ptr = p;
  return tkn;
}

#define ezylex_is_kw(str, len, kw) (strncmp((str), (kw), (len)) == 0)

ezy_tkn_t ezylex_identifier_or_kw(const char **ptr)
{
  const char *p = *ptr;
  const char *start = p;

  while (isalnum((uint8_t)*p) || (*p == '_'))
  {
    p++;
  }

  size_t len = p - *ptr;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);

  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = *ptr;

  // Keyword table : longest keywords first
  static const struct
  {
    const char *kw;
    enum ezy_kw_typ kw_type;
  } keywords[] = {
      {"return", ezy_kw_return},
      {"struct", ezy_kw_struct},
      {"const", ezy_kw_const},
      {"union", ezy_kw_union},
      {"type", ezy_kw_type},
      {"let", ezy_kw_let},
      {"fn", ezy_kw_fn},
  };

  // Check for keywords using the table
  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
  {
    if (ezylex_is_kw(*ptr, len, keywords[i].kw))
    {
      tkn.type = ezy_tkn_keyword;
      tkn.data.t_keyword = keywords[i].kw_type;
      break;
    }
  }

  // Not a keyword, treat as identifier
  if (tkn.type != ezy_tkn_keyword)
  {
    tkn.type = ezy_tkn_identifier;
    tkn.data.t_identifier.ptr = *ptr;
    tkn.data.t_identifier.len = len;
  }

  /* update global position to end of token */
  ezylex_advance_pos(start, p);

  *ptr = p;
  return tkn;
}

ezy_tkn_t ezylex_char(const char **ptr)
{
  const char *p = *ptr;
  const char *start = p;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);
  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = *ptr;
  p++; // skip opening '

  char c = *p;
  char c1 = *(p + 1);

  if (c == '\'')
  {
    // ezy_log_error("empty char literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
    tkn.data.t_string.ptr = "empty char literal";
    *ptr = p;
    return tkn;
  }

  // simple char without escape
  if (c != '\\' && c1 == '\'')
  {
    tkn.type = ezy_tkn_char;
    tkn.data.t_char = (uint8_t)(*p);
    p += 2; // skip char and closing '
    ezylex_advance_pos(start, p);
    *ptr = p;
    return tkn;
  }

  uint8_t escChar = 0;
  p++; // skip backslash

  c = *p;
  c1 = *(p + 1);
  if (c == '\\')
    escChar = '\\';
  else if (c == 'n')
    escChar = '\n';
  else if (c == 'r')
    escChar = '\r';
  else if (c == 't')
    escChar = '\t';
  else if (c == 'a')
    escChar = '\a';
  else if (c == '\'')
    escChar = '\'';
  else if (c == '"')
    escChar = '"';
  else if (c == '0')
    escChar = '\0';
  else
  {
    // try for octal or hex escape sequences
    char *end = NULL;
    if (c >= '0' && c <= '7')
    {
      size_t val = strtoull(p, &end, 8);
      if (end == p || val > 0xFF)
      {
        // ezy_log_error("invalid escape character in char literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid escape character in char literal";
        *ptr = p;
        return tkn;
      }
      escChar = (uint8_t)val;
      p = end;
    }
    else if (c == 'x')
    {
      p++;
      size_t val = strtoull(p, &end, 16);
      if (end == p || val > 0xFF)
      {
        // ezy_log_error("invalid escape character in char literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "invalid escape character in char literal";
        *ptr = p;
        return tkn;
      }
      escChar = (uint8_t)val;
      p = end;
    }
    else
    {
      // ezy_log_error("invalid escape character in char literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
      tkn.data.t_string.ptr = "invalid escape character in char literal";
      *ptr = p;
      return tkn;
    }
  }

  p++;

  // expect closing '
  if (*p != '\'')
  {
    // ezy_log_error("unterminated char literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
    tkn.data.t_string.ptr = "unterminated char literal";
    *ptr = p;
    return tkn;
  }

  p++; // skip closing '

  tkn.type = ezy_tkn_char;
  tkn.data.t_char = (uint8_t)escChar;
  ezylex_advance_pos(start, p);
  *ptr = p;
  return tkn;
}

// string literal lexer (escaped sequence stored without decoding)
ezy_tkn_t ezylex_string(const char **ptr)
{
  const char *p = *ptr;
  const char *start = p;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);
  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = *ptr;
  p++; // skip opening "

  const char *str_start = p;
  size_t str_len = 0;

  while (*p && *p != '"' && *p != '\n')
  {
    if (*p == '\\')
    {
      p++; // skip escape backslash
      str_len++;
      if (*p == 0)
      {
        // ezy_log_error("unterminated string literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
        tkn.data.t_string.ptr = "unterminated string literal";
        *ptr = p;
        return tkn;
      }
    }
    p++;
    str_len++;
  }

  if (*p != '"')
  {
    // ezy_log_error("unterminated string literal, (line: %u, col: %u)", ezylex_line, ezylex_col);
    tkn.data.t_string.ptr = "unterminated string literal";
    *ptr = p;
    return tkn;
  }

  // Now p points to closing "
  tkn.type = ezy_tkn_string;
  tkn.data.t_string.ptr = str_start;
  tkn.data.t_string.len = str_len;

  p++; // skip closing "

  ezylex_advance_pos(start, p);
  *ptr = p;
  return tkn;
}

ezy_tkn_t ezylex_operator(const char **ptr)
{
  const char *p = *ptr;
  const char *start = p;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);
  tkn.line = ezylex_line;
  tkn.col = ezylex_col;
  tkn.ptr = *ptr;

  enum ezy_op_typ op = ezy_op_invalid;

  // operator table : longer operators first
  static const struct
  {
    const char *op;
    enum ezy_op_typ type;
  } ops[] = {
      {"<<=", ezy_op_bw_lshift_eq},
      {">>=", ezy_op_bw_rshift_eq},
      {"+=", ezy_op_plus_eq},
      {"-=", ezy_op_minus_eq},
      {"/=", ezy_op_divide_eq},
      {"*=", ezy_op_times_eq},
      {"%=", ezy_op_modulo_eq},
      {"++", ezy_op_increment},
      {"--", ezy_op_decrement},
      {"==", ezy_op_cond_eq},
      {"!=", ezy_op_cond_neq},
      {"&=", ezy_op_bw_and_eq},
      {"|=", ezy_op_bw_or_eq},
      {"^=", ezy_op_bw_xor_eq},
      {"&&", ezy_op_cond_and},
      {"||", ezy_op_cond_or},
      {"<<", ezy_op_bw_lshift},
      {">>", ezy_op_bw_rshift},

      {";", ezy_op_semicolon},
      {",", ezy_op_comma},
      {"(", ezy_op_brac_small_l},
      {")", ezy_op_brac_small_r},
      {"{", ezy_op_brac_curly_l},
      {"}", ezy_op_brac_curly_r},
      {"[", ezy_op_brac_big_l},
      {"]", ezy_op_brac_big_r},
      {"+", ezy_op_plus},
      {"-", ezy_op_minus},
      {"*", ezy_op_asterisk},
      {"/", ezy_op_divide},
      {"%", ezy_op_modulo},
      {"=", ezy_op_assign},
      {"!", ezy_op_cond_not},
      {"<", ezy_op_cond_lessthan},
      {">", ezy_op_cond_morethan},
      {"&", ezy_op_bw_and},
      {"|", ezy_op_bw_or},
      {"^", ezy_op_bw_xor},
      {"~", ezy_op_bw_not},
      {"?", ezy_op_qn},
      {".", ezy_op_dot},
  };

  // match operators serially with the help of the table
  for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++)
  {
    size_t oplen = strlen(ops[i].op);
    if (strncmp(p, ops[i].op, oplen) == 0)
    {
      op = ops[i].type;
      p += oplen;
      break;
    }
  }

  if (op == ezy_op_invalid)
  {
    // ezy_log_error("unrecognized operator starting with '%c', (line: %u, col: %u)", *p, ezylex_line, ezylex_col);
    tkn.data.t_string.ptr = "unrecognized operator starting with '%c'";
    *ptr = p;
    return tkn;
  }

  tkn.type = ezy_tkn_operator;
  tkn.data.t_operator = op;

  ezylex_advance_pos(start, p);
  *ptr = p;
  return tkn;
}

ezy_tkn_t ezylex_next_tkn()
{
  
  if (ezylex_tknbuf_headptr == NULL)
  {
    ezy_log_error("ezylex_next_tkn() called without ezylex_start() being called.");
    return ezylex_blank_tok(ezy_tkn_dummy);
  }

  const char *p = ezylex_skip_ws(ezylex_tknbuf_headptr);
  ezylex_tknbuf_headptr = p;
  ezy_tkn_t tkn = ezylex_blank_tok(ezy_tkn_invalid);

  char c = *p;
  char c1 = *(p + 1);

  if (c == 0)
    return ezylex_blank_tok(ezy_tkn_eof);

  bool isNum = isdigit((uint8_t)c) || (c == '.' && isdigit((uint8_t)c1));
  // handle leading '+' or '-' for numbers
  // (only if they are unary operators, i.e., preceded by another operator)
  isNum |= c == '-' && isdigit(c1) && ezylex_last_tok.type == ezy_tkn_operator;
  isNum |= c == '+' && isdigit(c1) && ezylex_last_tok.type == ezy_tkn_operator;

  ezy_log("isNum: %d, c: '%c', c1: '%c'", isNum, c, c1);

  if (isNum)
  {
    tkn = ezylex_number(&p);
    ezylex_tknbuf_headptr = p;
    return tkn;
  };

  if (isalpha((uint8_t)c) || c == '_')
  {
    tkn = ezylex_identifier_or_kw(&p);
    ezylex_tknbuf_headptr = p;
    return tkn;
  }

  if (c == '/' && c1 == '/')
  {
    // single-line comment
    p += 2;
    while (*p && *p != '\n')
      p++;
    ezylex_advance_pos(ezylex_tknbuf_headptr, p);
    ezylex_tknbuf_headptr = p;
    return ezylex_next_tkn();
  }

  if (c == '/' && c1 == '*')
  {
    // multi-line comment
    p += 2;
    while (*p && !(*p == '*' && *(p + 1) == '/'))
      p++;
    if (*p)
      p += 2;
    ezylex_advance_pos(ezylex_tknbuf_headptr, p);
    ezylex_tknbuf_headptr = p;
    return ezylex_next_tkn();
  }

  if (c == '\'' && c1 != '\'')
  {
    tkn = ezylex_char(&p);
    ezylex_tknbuf_headptr = p;
    return tkn;
  }

  if (c == '"')
  {
    tkn = ezylex_string(&p);
    ezylex_tknbuf_headptr = p;
    return tkn;
  }

  tkn = ezylex_operator(&p);
  ezylex_tknbuf_headptr = p;

  ezylex_last_tok = tkn;

  return tkn;
}

// peek token in relative position (0 = current token)
ezy_tkn_t ezylex_peek_tkn(size_t pos)
{
  while (pos >= ezylex_tknbuf_count)
  {
    ezylex_push_tkn(ezylex_next_tkn());
  }
  size_t index = (ezylex_tknbuf_tail + pos) % ezylex_tknbuf_limit;
  return ezylex_tknbuf[index];
}

void ezylex_consume_tkn(size_t count)
{
  if (count > ezylex_tknbuf_count)
  {
    ezy_log_error("ezylex_consume_tkn(size) exceeds available token count.");
    return;
  }
  for (size_t i = 0; i < count; i++)
  {
    ezylex_tknbuf_tail += 1;
    ezylex_tknbuf_tail %= ezylex_tknbuf_limit;
    ezylex_tknbuf_count--;
  }
}

void ezylex_consume_all_tkn()
{
  ezylex_tknbuf_head = 0;
  ezylex_tknbuf_tail = 0;
  ezylex_tknbuf_count = 0;
}