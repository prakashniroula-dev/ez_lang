#if !defined(ez_ezylexer_h)
#define ez_ezylexer_h

#include <ezy_tkn_typ.h>
#include <ezy_typ.h>

union ezy_tkn_unit_t
{
  enum ezy_kw_typ t_keyword;
  enum ezy_op_typ t_operator;
  ezy_cstr_t t_identifier;

  int64_t t_int64;
  uint64_t t_uint64;
  double t_float64;
  uint8_t t_char;
  ezy_cstr_t  t_string;
};

typedef struct
{
  enum ezy_tkn_typ type;
  union ezy_tkn_unit_t data;

  uint32_t line;
  uint32_t col;
  const char* ptr;
} ezy_tkn_t;

void ezylex_start(const char*);
ezy_tkn_t ezylex_peek_tkn(size_t);
void ezylex_consume_tkn(size_t);
void ezylex_consume_all_tkn();

#endif // ez_ezylexer_h
