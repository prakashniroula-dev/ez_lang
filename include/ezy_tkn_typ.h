#if !defined(ezy_tkn_typ_h)
#define ezy_tkn_typ_h

#include <stdint.h>

/* Keyword types */
enum ezy_kw_typ
{
  ezy_kw_invalid = 0,
  ezy_kw_let,
  ezy_kw_const,
  ezy_kw_type,
  ezy_kw_struct,
  ezy_kw_union,
  ezy_kw_void,
  ezy_kw_fn,
  ezy_kw_return
};

/* Operator types */
enum ezy_op_typ
{
  ezy_op_invalid = 0,

  /* Basic */
  ezy_op_semicolon,
  ezy_op_comma,

  /* Brackets */
  ezy_op_brac_big_l,
  ezy_op_brac_big_r,
  ezy_op_brac_small_l,
  ezy_op_brac_small_r,
  ezy_op_brac_curly_l,
  ezy_op_brac_curly_r,

  /* Arithmetic */
  ezy_op_assign,
  ezy_op_plus,
  ezy_op_minus,
  ezy_op_divide,
  ezy_op_asterisk,
  ezy_op_modulo,

  ezy_op_plus_eq,
  ezy_op_minus_eq,
  ezy_op_divide_eq,
  ezy_op_times_eq,
  ezy_op_modulo_eq,
  ezy_op_increment,
  ezy_op_decrement,

  /* Conditional operators */
  ezy_op_cond_not,
  ezy_op_cond_and,
  ezy_op_cond_or,
  ezy_op_cond_lessthan,
  ezy_op_cond_morethan,
  ezy_op_cond_eq,
  ezy_op_cond_neq,

  /* Bitwise operators */
  ezy_op_bw_not,
  ezy_op_bw_and,
  ezy_op_bw_and_eq,
  ezy_op_bw_or,
  ezy_op_bw_or_eq,
  ezy_op_bw_lshift,
  ezy_op_bw_lshift_eq,
  ezy_op_bw_rshift,
  ezy_op_bw_rshift_eq,
  ezy_op_bw_xor,
  ezy_op_bw_xor_eq,

  /* Others */
  ezy_op_qn, // '?'
  ezy_op_dot,
};

enum ezy_tkn_typ
{
  ezy_tkn_dummy = -2,
  ezy_tkn_eof = -1,
  ezy_tkn_invalid = 0,

  ezy_tkn_keyword,
  ezy_tkn_identifier,
  ezy_tkn_operator,

  ezy_tkn_int64,
  ezy_tkn_uint64,
  ezy_tkn_float64,
  ezy_tkn_char,
  ezy_tkn_string,
};

#endif // ezy_tkn_typ_h
