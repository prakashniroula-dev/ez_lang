#include <ezy_ast.h>
#include <ezy_lexer.h>
#include <ezy_log.h>
#include <ezy_parser_arena.h>
#include <string.h>

// internal error struct
struct ezyparse_error
{
  const char *msg;
  ezy_tkn_t last_tkn;
};

// Declarations of helper functions
static inline bool ezyparse_expect(ezy_tkn_t tkn, enum ezy_tkn_typ type);
static inline bool ezyparse_match(ezy_tkn_t tkn, enum ezy_tkn_typ type);

// Declarations of parsing functions
static struct ezyparse_error ezyparse_parse_datatype(struct ezy_ast_datatype_t *dest);
static struct ezyparse_error ezyparse_parse_parameter_list(struct ezy_ast_args_t **dest_params, size_t *dest_count);
static struct ezyparse_error ezyparse_parse_expression(ezy_ast_node_t **dest);
static struct ezyparse_error ezyparse_parse_decl(ezy_ast_node_t **dest);
static struct ezyparse_error ezyparse_parse_statement(struct ezy_ast_function_t *func_node, ezy_ast_node_t **dest);
static struct ezyparse_error ezyparse_parse_function(ezy_ast_node_t **dest);
static ezy_ast_node_t* ezyparse_parse_pratt_expr(int min_prec);

// helper macros for token handling
#define tok(n) ezylex_peek_tkn(n)
#define consume(n) ezylex_consume_tkn(n)

// =========== Helper functions ===========

static inline bool ezyparse_expect(ezy_tkn_t tkn, enum ezy_tkn_typ type)
{
  if (tkn.type == type)
    return true;
  ezy_log_warn("(Expected token type %d but got %d at line %u, col %u)", type, tkn.type, tkn.line, tkn.col);
  return false;
}

static inline bool ezyparse_match(ezy_tkn_t tkn, enum ezy_tkn_typ type)
{
  return tkn.type == type;
}

// ========== Parsing functions ===========

static struct ezyparse_error ezyparse_parse_datatype(struct ezy_ast_datatype_t *dest)
{
  ezy_tkn_t tkn = tok(0);
  struct ezyparse_error err = {.last_tkn = tkn, .msg = NULL};
  if (tkn.type != ezy_tkn_identifier)
  {
    return (struct ezyparse_error){.msg = "Expected datatype identifier", .last_tkn = tkn};
  }

  // datatype table: longer datatype names first
  static const struct
  {
    const char *name;
    enum ezy_ast_datatype_typ type;
  } datatype_table[] = {
      {"int", ezy_ast_dt_int32},
      {"int8", ezy_ast_dt_int8},
      {"int16", ezy_ast_dt_int16},
      {"int32", ezy_ast_dt_int32},
      {"int64", ezy_ast_dt_int64},

      {"uint", ezy_ast_dt_uint32},
      {"uint8", ezy_ast_dt_uint8},
      {"uint16", ezy_ast_dt_uint16},
      {"uint32", ezy_ast_dt_uint32},
      {"uint64", ezy_ast_dt_uint64},

      {"float", ezy_ast_dt_float32},
      {"float32", ezy_ast_dt_float32},
      {"float64", ezy_ast_dt_float64},

      {"string", ezy_ast_dt_string},

      {"bool", ezy_ast_dt_bool},
      {"char", ezy_ast_dt_char},
      {"void", ezy_ast_dt_void},
  };

  bool matched = false;
  for (size_t i = 0; i < sizeof(datatype_table) / sizeof(datatype_table[0]); i++)
  {
    if (tkn.data.t_identifier.len == strlen(datatype_table[i].name) && strncmp(tkn.data.t_identifier.ptr, datatype_table[i].name, tkn.data.t_identifier.len) == 0)
    {
      dest->typ = datatype_table[i].type;
      dest->nullable = false;
      dest->is_ptr = false;
      dest->is_const = false;
      consume(1); // consume datatype identifier
      matched = true;
    }
  }

  if (!matched)
  {
    return (struct ezyparse_error){.msg = "Unrecognized datatype identifier", .last_tkn = tkn};
  }

  tkn = tok(0);

  if (ezyparse_match(tkn, ezy_tkn_operator) && tkn.data.t_operator == ezy_op_qn)
  {
    dest->nullable = true;
    consume(1); // consume '?'
  }
  else if (ezyparse_match(tkn, ezy_tkn_operator) && tkn.data.t_operator == ezy_op_asterisk)
  {
    dest->is_ptr = true;
    consume(1); // consume '*'
  }

  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

static struct ezyparse_error ezyparse_parse_parameter_list(struct ezy_ast_args_t **dest_params, size_t *dest_count)
{
  size_t alloc_count = 16; // max 16 params to start with
  size_t param_count = 0;

  struct ezy_ast_args_t *params = ezyparse_arena_alloc(sizeof(struct ezy_ast_args_t) * alloc_count);

  
  ezy_tkn_t tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_l)
  {
    return (struct ezyparse_error){.msg = "Expected '(' at start of parameter list", .last_tkn = tkn};
  }
  consume(1); // consume '('

  tkn = tok(0);
  while (!(ezyparse_match(tkn, ezy_tkn_operator) && tkn.data.t_operator == ezy_op_brac_small_r))
  {
    bool is_dt_const = false;

    if (param_count > 0)
    {
      if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_comma)
      {
        return (struct ezyparse_error){.msg = "Expected ',' between parameters", .last_tkn = tkn};
      }
      consume(1); // consume ','
    }

    tkn = tok(0);
    // this block is here because const in function parameters has different syntax
    if (tkn.type == ezy_tkn_identifier && strncmp(tkn.data.t_identifier.ptr, "const", tkn.data.t_identifier.len) == 0)
    {
      is_dt_const = true;
      consume(1); // consume 'const'
    }

    struct ezyparse_error err = ezyparse_parse_datatype(&params[param_count].typ);
    params[param_count].typ.is_const = is_dt_const;

    if (err.msg != NULL)
    {
      return err;
    }

    tkn = tok(0);
    if (!ezyparse_expect(tkn, ezy_tkn_identifier))
      return (struct ezyparse_error){.msg = "Expected parameter name identifier", .last_tkn = tkn};

    consume(1); // consume parameter name

    if (param_count + 1 >= alloc_count)
    {
      void *old_params = (void *)params;

      // attempt to backtrack previous allocation
      ezyparse_arena_backtrack(sizeof(struct ezy_ast_args_t) * alloc_count, old_params);

      params = ezyparse_arena_alloc(sizeof(struct ezy_ast_args_t) * alloc_count);
      memmove(params, old_params, sizeof(struct ezy_ast_args_t) * param_count);
      
      alloc_count *= 2;
    }

    params[param_count].name = tkn.data.t_identifier;
    param_count++;

    tkn = tok(0);
  }

  if (alloc_count > param_count)
  {
    // attempt to backtrack previous allocation & shrink size
    void *old_params = (void *)params;
    if (ezyparse_arena_backtrack(sizeof(struct ezy_ast_args_t) * alloc_count, old_params)) {
      params = ezyparse_arena_alloc(sizeof(struct ezy_ast_args_t) * param_count);
      memmove(params, old_params, sizeof(struct ezy_ast_args_t) * param_count);
    }
  }

  consume(1); // consume ')'

  *dest_count = param_count;
  *dest_params = params;

  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

static struct ezyparse_error ezyparse_parse_call_args(struct ezy_ast_node_t **dest, size_t *dest_count) {
  size_t alloc_count = 8; // max 8 args to start with
  size_t arg_count = 0;

  ezy_ast_node_t *args = ezyparse_arena_alloc(sizeof(ezy_ast_node_t) * alloc_count);

  ezy_tkn_t tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_l)
  {
    return (struct ezyparse_error){.msg = "Expected '(' at start of call argument list", .last_tkn = tkn};
  }
  consume(1); // consume '('

  
  tkn = tok(0);
  while (!(ezyparse_match(tkn, ezy_tkn_operator) && tkn.data.t_operator == ezy_op_brac_small_r))
  {
    if (arg_count > 0)
    {
      if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_comma)
      {
        return (struct ezyparse_error){.msg = "Expected ',' between call arguments", .last_tkn = tkn};
      }
      consume(1); // consume ','
    }

    ezy_ast_node_t* node;
    
    // ezy_log("Call args -> expression: token type %d", tkn.type);
    struct ezyparse_error err = ezyparse_parse_expression(&node);

    args[arg_count] = *node;
    if (err.msg != NULL)
    {
      return err;
    }

    arg_count++;

    if (arg_count >= alloc_count)
    {
      void *old_args = (void *)args;

      // attempt to backtrack previous allocation
      ezyparse_arena_backtrack(sizeof(ezy_ast_node_t) * alloc_count, old_args);
      alloc_count *= 2;

      args = ezyparse_arena_alloc(sizeof(ezy_ast_node_t) * alloc_count);
      memmove(args, old_args, sizeof(ezy_ast_node_t) * arg_count);
    }

    tkn = tok(0);
  }

  if (alloc_count > arg_count)
  {
    // attempt to backtrack previous allocation & shrink size
    void *old_args = (void *)args;
    if (ezyparse_arena_backtrack(sizeof(ezy_ast_node_t) * alloc_count, old_args)) {
      args = ezyparse_arena_alloc(sizeof(ezy_ast_node_t) * arg_count);
      memmove(args, old_args, sizeof(ezy_ast_node_t) * arg_count);
    }
  }

  *dest = args;
  *dest_count = arg_count;
  consume(1); // consume ')'
  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

static enum ezy_pratt_prec {
  ezy_pratt_prec_invalid = 0,
  ezy_pratt_prec_lowest,
  ezy_pratt_prec_assignment,    // =
  ezy_pratt_prec_conditional,   // ?:
  ezy_pratt_prec_sum,           // + -
  ezy_pratt_prec_product,       // * /
  ezy_pratt_prec_prefix,        // -X !X
  ezy_pratt_prec_call,          // myFunction(X)
};

static enum ezy_pratt_prec ezyparse_get_token_prec(ezy_tkn_t tkn)
{
  if (tkn.type == ezy_tkn_operator)
  {
    switch (tkn.data.t_operator)
    {
    case ezy_op_assign:
      return ezy_pratt_prec_assignment;
    case ezy_op_plus:
    case ezy_op_minus:
      return ezy_pratt_prec_sum;
    case ezy_op_asterisk:
    case ezy_op_divide:
      return ezy_pratt_prec_product;
    case ezy_op_brac_small_l:
      return ezy_pratt_prec_call;
    default:
      return ezy_pratt_prec_lowest - 1;
    }
  }
  return ezy_pratt_prec_lowest - 1;
}

static ezy_ast_node_t* ezyparse_parse_pratt_prefix()
{
  ezy_tkn_t tkn = tok(0);
  if ( tkn.type == ezy_tkn_int64 ) {
    ezy_ast_node_t *lit_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
    lit_node->type = ezy_ast_node_literal;

    uint64_t v = tkn.data.t_int64 < 0 ? -(uint64_t)tkn.data.t_int64 : (uint64_t)tkn.data.t_int64;
    enum ezy_ast_datatype_typ typ = v > INT32_MAX ? ezy_ast_dt_int64 : ezy_ast_dt_int32;

    lit_node->data.n_literal.typ = typ;
    lit_node->data.n_literal.value.t_int64 = tkn.data.t_int64;
    consume(1); // consume literal token
    return lit_node;
  }

  if ( tkn.type == ezy_tkn_uint64 ) {
    ezy_ast_node_t *lit_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));

    uint64_t v = tkn.data.t_uint64;
    enum ezy_ast_datatype_typ typ = v > UINT32_MAX ? ezy_ast_dt_uint64 : ezy_ast_dt_uint32;

    lit_node->type = ezy_ast_node_literal;
    lit_node->data.n_literal.typ = typ;
    lit_node->data.n_literal.value.t_uint64 = tkn.data.t_uint64;
    consume(1); // consume literal token
    return lit_node;
  }

  if ( tkn.type == ezy_tkn_string ) {
    ezy_ast_node_t *lit_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
    lit_node->type = ezy_ast_node_literal;
    lit_node->data.n_literal.typ = ezy_ast_dt_string;
    lit_node->data.n_literal.value.t_string = tkn.data.t_string;
    consume(1); // consume literal token
    return lit_node;
  }

  if ( tkn.type == ezy_tkn_float64 ) {
    ezy_ast_node_t *lit_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
    lit_node->type = ezy_ast_node_literal;
    lit_node->data.n_literal.typ = ezy_ast_dt_float64;
    lit_node->data.n_literal.value.t_float64 = tkn.data.t_float64;
    consume(1); // consume literal token
    return lit_node;
  }

  // if ( tkn.type == ezy_tkn_uint64 ) {
  //   ezy_ast_node_t *lit_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
  //   lit_node->type = ezy_ast_node_literal;
  //   lit_node->data.n_literal.typ = ezy_ast_dt_uint64;
  //   lit_node->data.n_literal.value.t_uint64 = tkn.data.t_uint64;
  //   consume(1); // consume literal token
  //   return lit_node;
  // }

  if ( tkn.type == ezy_tkn_identifier ) {
    ezy_ast_node_t *var_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
    ezy_tkn_t tkn1 = tok(1);
    // peek token to check if it's a function call
    bool isFuncCall = ( tkn1.type == ezy_tkn_operator && tkn1.data.t_operator == ezy_op_brac_small_l );
    if ( isFuncCall ) {
      struct ezy_ast_call_t *call_data = ezyparse_arena_alloc(sizeof(struct ezy_ast_call_t));
      call_data->func_name = tkn.data.t_identifier;
      var_node->type = ezy_ast_node_call;
      var_node->data.n_call = call_data;
    } else {
      var_node->type = ezy_ast_node_variable;
      var_node->data.n_variable.name = tkn.data.t_identifier;
    }
    consume(1); // consume identifier token
    return var_node;
  }

  if ( tkn.type == ezy_tkn_operator && tkn.data.t_operator == ezy_op_brac_small_l ) {
    consume(1); // consume '('
    ezy_ast_node_t *expr = ezyparse_parse_pratt_expr(ezy_pratt_prec_lowest);
    tkn = tok(0);
    if ( !ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_r ) {
      ezy_log_warn("Expected ')' after expression");
      return NULL;
    }
    consume(1); // consume ')'
    return expr;
  }

  ezy_log_warn("Unsupported prefix token type %d, %d", tkn.type, tkn.data.t_operator);
  return NULL;
}

static ezy_ast_node_t* ezyparse_parse_pratt_postfix(ezy_ast_node_t* left, int prec) {
  ezy_tkn_t tkn = tok(0);

  if ( tkn.type == ezy_tkn_operator && tkn.data.t_operator == ezy_op_brac_small_l ) {
    // function call is already allocated in prefix parse
    ezy_ast_node_t *call_node = left;
    struct ezy_ast_call_t *call_data = call_node->data.n_call;
    struct ezyparse_error err = ezyparse_parse_call_args(&call_data->args, &call_data->arg_count);
    ezy_log("Parsed function call (name = %.*s, args = ", (int)call_data->func_name.len, call_data->func_name.ptr);
    for (size_t i = 0; i < call_data->arg_count; i++) {
      ezy_log_raw("%d: ", call_data->args[i].type);
      switch (call_data->args[i].data.n_literal.typ) {
        case ezy_ast_dt_uint64:
          ezy_log_raw("uint64(%llu)", (unsigned long long)call_data->args[i].data.n_literal.value.t_uint64);
          break;
        case ezy_ast_dt_string:
          ezy_log_raw("string(%.*s)", (int)call_data->args[i].data.n_literal.value.t_string.len, call_data->args[i].data.n_literal.value.t_string.ptr);
          break;
        case ezy_ast_dt_float64:
          ezy_log_raw("float64(%f)", call_data->args[i].data.n_literal.value.t_float64);
          break;
        default:
          ezy_log_raw("unknown");
          break;
      }
      if (i + 1 < call_data->arg_count) {
        ezy_log_raw(", ");
      }
    }
    ezy_log_raw(")\n");
    if (err.msg != NULL)
    {
      ezy_log_warn("Error parsing function call arguments: %s", err.msg);
      return NULL;
    }      

    return call_node;
  }

  // binary operator
  
  ezy_ast_node_t *node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
  node->type = ezy_ast_node_binop;
  node->data.n_binop.operator = tkn.data.t_operator;
  node->data.n_binop.left = left;
  
  consume(1); // consume operator token

  ezy_ast_node_t *right = ezyparse_parse_pratt_expr(prec + 1);
  node->data.n_binop.right = right;
  return node;
}

static ezy_ast_node_t* ezyparse_parse_pratt_expr(int min_prec)
{
  ezy_tkn_t tkn = tok(0);
  if ( tkn.type == ezy_tkn_operator && (tkn.data.t_operator == ezy_op_semicolon || tkn.data.t_operator == ezy_op_comma ) ) {
    return NULL;
  }
  
  ezy_log("prec = %d, parse prefix tkn type: %d", min_prec, tkn.type);
  if ( tkn.type == ezy_tkn_operator ) {
    ezy_log("Operator token: %d", tkn.data.t_operator);
  } else if ( tkn.type == ezy_tkn_identifier ) {
    ezy_log("Identifier token: %.*s", tkn.data.t_identifier.len, tkn.data.t_identifier.ptr);
  } else if ( tkn.type == ezy_tkn_int64 ) {
    ezy_log("Int64 literal token: %lld", tkn.data.t_int64);
  } else if ( tkn.type == ezy_tkn_uint64 ) {
    ezy_log("Uint64 literal token: %llu", (unsigned long long)tkn.data.t_uint64);
  } else if ( tkn.type == ezy_tkn_string ) {
    ezy_log("String literal token: \"%.*s\"", tkn.data.t_string.len, tkn.data.t_string.ptr);
  } else if ( tkn.type == ezy_tkn_float64 ) {
    ezy_log("Float64 literal token: %f", tkn.data.t_float64);
  }
  ezy_ast_node_t* left = ezyparse_parse_pratt_prefix();
  tkn = tok(0);
  while (true)
  {
    enum ezy_pratt_prec prec = ezyparse_get_token_prec(tkn);
    ezy_log("on the way to postfix, token type: %d, operator: %d, precedence: %d", tkn.type, tkn.data.t_operator, prec);
    if (prec < min_prec || min_prec < ezy_pratt_prec_lowest) break;
    
    ezy_log("prec = %d, min_prec = %d, parse postfix tkn type: %d", prec, min_prec, tkn.type);
    if ( tkn.type == ezy_tkn_operator ) {
      ezy_log("Operator token: %d", tkn.data.t_operator);
    } else if ( tkn.type == ezy_tkn_identifier ) {
      ezy_log("Identifier token: %.*s", tkn.data.t_identifier.len, tkn.data.t_identifier.ptr);
    } else if ( tkn.type == ezy_tkn_int64 ) {
      ezy_log("Int64 literal token: %lld", tkn.data.t_int64);
    } else if ( tkn.type == ezy_tkn_uint64 ) {
      ezy_log("Uint64 literal token: %llu", (unsigned long long)tkn.data.t_uint64);
    } else if ( tkn.type == ezy_tkn_string ) {
      ezy_log("String literal token: \"%.*s\"", tkn.data.t_string.len, tkn.data.t_string.ptr);
    } else if ( tkn.type == ezy_tkn_float64 ) {
      ezy_log("Float64 literal token: %f", tkn.data.t_float64);
    }
    left = ezyparse_parse_pratt_postfix(left, prec);
    tkn = tok(0);
  }

  return left;
}

static struct ezyparse_error ezyparse_parse_expression(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = tok(0);
  ezy_log("Initial token type : %d", tkn.type);
  ezy_ast_node_t* expr = ezyparse_parse_pratt_expr(ezy_pratt_prec_lowest);
  if (expr == NULL) {
    return (struct ezyparse_error){.msg = "Failed to parse expression", .last_tkn = tok(0)};
  }
  *dest = expr;
  return (struct ezyparse_error){.msg = NULL, .last_tkn = tok(0)};
}

static struct ezyparse_error ezyparse_parse_decl(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_keyword) || tkn.data.t_keyword != ezy_kw_let && tkn.data.t_keyword != ezy_kw_const)
  {
    return (struct ezyparse_error){.msg = "Expected 'let' or 'const' keyword", .last_tkn = tkn};
  }
  
  bool isConst = (tkn.data.t_keyword == ezy_kw_const);
  
  consume(1); // consume 'let' or 'const' keyword
  
  tkn = tok(0);
  
  ezy_ast_node_t *dest_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
  *dest = dest_node;

  dest_node->data.n_variable.typ.typ = ezy_ast_dt_infer; // default to infer
  
  ezy_log("before parsing datatype, tok type=%d (%.*s)", tkn.type, tkn.data.t_identifier.len, tkn.data.t_identifier.ptr);
  ezyparse_parse_datatype(&dest_node->data.n_variable.typ);
  ezy_log("Parsed datatype for declaration, type=%d", dest_node->data.n_variable.typ.typ);

  tkn = tok(0);

  dest_node->type = ezy_ast_node_variable_decl;
  dest_node->data.n_variable.typ.is_const = isConst;

  if (!ezyparse_expect(tkn, ezy_tkn_identifier))
    return (struct ezyparse_error){.msg = "Expected variable / type name identifier", .last_tkn = tkn};

    
  dest_node->data.n_variable.name = tkn.data.t_identifier;
  ezy_log("Decl -> identifier: token type %d, name: %.*s", tkn.type, tkn.data.t_identifier.len, tkn.data.t_identifier.ptr);
  
  tkn = tok(1); // lookahead for '='

  bool assign = tkn.type == ezy_tkn_operator && tkn.data.t_operator == ezy_op_assign;

  if ( isConst && !assign )
  {
    return (struct ezyparse_error){.msg = "Const declarations must be immediately assigned", .last_tkn = tkn};
  }

  consume(1); // consume identifier

  // check for immediate assignment
  if (assign)
  {
    consume(1); // consume '='
    // ezy_log("Decl -> expression: token type %d", tkn.type);
    tkn = tok(0);
    // ezy_log("Before passing to parse, token type = %d", tkn.type);
    struct ezyparse_error err = ezyparse_parse_expression(&dest_node->data.n_variable.value);
    if (err.msg != NULL)
    {
      return err;
    }
  } else {
    // mark as nullable if not assigned
    dest_node->data.n_variable.typ.nullable = true;
  }

  tkn = tok(0);

  if (tkn.type != ezy_tkn_operator || tkn.data.t_operator != ezy_op_semicolon)
  {
    return (struct ezyparse_error){.msg = "Expected ';' at end of declaration", .last_tkn = tkn};
  }

  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

// parse a single statement
static struct ezyparse_error ezyparse_parse_statement(struct ezy_ast_function_t *func_node, ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = tok(0);
  bool isBlock = false;

  ezy_ast_node_t *first_node = NULL;
  ezy_ast_node_t *cur_node = NULL;

  
  if (ezyparse_match(tkn, ezy_tkn_keyword) && (tkn.data.t_keyword == ezy_kw_let || tkn.data.t_keyword == ezy_kw_const))
  {
    struct ezyparse_error err = ezyparse_parse_decl(cur_node == NULL ? &cur_node : &cur_node->next);
    if (err.msg != NULL)
    {
      return err;
    }
    if ( first_node == NULL ) {
      first_node = cur_node;
    }
  }

  else if (ezyparse_match(tkn, ezy_tkn_keyword) && tkn.data.t_keyword == ezy_kw_return)
  {
    // To do : parse return statement
  }

  else
  {
    // ezy_log("Statement -> expression: token type %d", tkn.type);
    struct ezyparse_error err = ezyparse_parse_expression(cur_node == NULL ? &cur_node : &cur_node->next);
    if (err.msg != NULL)
    {
      return err;
    }
    if ( first_node == NULL ) {
      first_node = cur_node;
    }
  }

  tkn = tok(0);

  if (!ezyparse_expect(tkn, ezy_tkn_operator) || (!isBlock && tkn.data.t_operator != ezy_op_semicolon))
  {
    return (struct ezyparse_error){.msg = "Expected ';' at end of a non-block statement", .last_tkn = tkn};
  }

  consume(1); // consume ';'
  *dest = first_node;

  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

static struct ezyparse_error ezyparse_parse_block(struct ezy_ast_function_t *func_node, ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = tok(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_curly_l)
  {
    return (struct ezyparse_error){.msg = "Expected '{' at start of block", .last_tkn = tkn};
  }
  consume(1); // consume '{'

  ezy_ast_node_t *block_node = NULL;
  ezy_ast_node_t *curr_stmt = NULL;

  tkn = tok(0);
  while (!ezyparse_match(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_curly_r)
  {
    ezy_ast_node_t *stmt_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
    err = ezyparse_parse_statement(func_node, &stmt_node);
    if (err.msg != NULL)
    {
      return err;
    }

    if (block_node == NULL)
    {
      block_node = stmt_node;
      curr_stmt = block_node;
    }
    else
    {
      curr_stmt->next = stmt_node;
      curr_stmt = stmt_node;
    }

    tkn = tok(0);
  }

  consume(1); // consume '}'

  *dest = block_node;
  return (struct ezyparse_error){.msg = NULL, .last_tkn = tkn};
}

static struct ezyparse_error ezyparse_parse_function(ezy_ast_node_t **dest)
{

  ezy_tkn_t tkn = tok(0);

  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_fn)
  {
    return (struct ezyparse_error){.msg = "Expected 'fn' keyword", .last_tkn = tkn};
  }
  ezy_ast_node_t *func_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
  struct ezy_ast_function_t *func_data = ezyparse_arena_alloc(sizeof(struct ezy_ast_function_t));
  
  func_node->type = ezy_ast_node_function;
  
  func_data->return_typ.typ = ezy_ast_dt_infer;
  
  consume(1); // consume 'fn' keyword

  // parse return type if specified
  ezyparse_parse_datatype(&func_data->return_typ);
  
  tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_identifier))
    return (struct ezyparse_error){.msg = "Expected function name / type identifier", .last_tkn = tkn};

  func_data->name = tkn.data.t_identifier;
  consume(1); // consume function name
  
  tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_l)
    return (struct ezyparse_error){.msg = "Expected '(' after function name", .last_tkn = tkn};

  // ezy_log("Parsed fn name = %.*s", (int)func_data->name.len, func_data->name.ptr);
  
  tkn = tok(1); // lookahead for parameter list
  
  if (tkn.type == ezy_tkn_operator && tkn.data.t_operator == ezy_op_brac_small_r)
  {
    // ezy_log("fn no params");
    func_data->param_count = 0;
    func_data->params = NULL;
    consume(2); // consume '(' and ')'
  }
  else
  {
    // ezy_log("fn with params");
    struct ezyparse_error err = ezyparse_parse_parameter_list(&func_data->params, &func_data->param_count);
    if (err.msg != NULL)
    {
      return err;
    }
  }
  
  func_node->data.n_function = func_data;
  *dest = func_node;
  // parse function body
  return ezyparse_parse_block(func_data, &func_data->body);
}

static struct ezyparse_error ezyparse_parse_struct(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_struct)
  {
    return (struct ezyparse_error){.msg = "Expected 'struct' keyword", .last_tkn = tkn};
  }
  // To do : parse structs
}

static struct ezyparse_error ezyparse_parse_union(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_union)
  {
    return (struct ezyparse_error){.msg = "Expected 'union' keyword", .last_tkn = tkn};
  }
  // To do : parse unions
}

static struct ezyparse_error ezyparse_parse_global_decl(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || (tkn.data.t_keyword != ezy_kw_let && tkn.data.t_keyword != ezy_kw_const))
  {
    return (struct ezyparse_error){.msg = "Expected 'let' or 'const' keyword", .last_tkn = tkn};
  }
  // To do : parse global declarations
}

static struct ezyparse_error ezyparse_parse_program(ezy_ast_node_t **dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);

  if (tkn.type != ezy_tkn_keyword)
  {
    return (struct ezyparse_error){.msg = "Unexpected token, expected keyword", .last_tkn = tkn};
  }

  switch (tkn.data.t_keyword)
  {
  case ezy_kw_fn:
    return ezyparse_parse_function(dest);
  case ezy_kw_struct:
    return ezyparse_parse_struct(dest);
  case ezy_kw_union:
    return ezyparse_parse_union(dest);
  case ezy_kw_let:
  case ezy_kw_const:
    return ezyparse_parse_global_decl(dest);
  default:
    return (struct ezyparse_error){.msg = "Unexpected keyword", .last_tkn = tkn};
  }
}

ezy_ast_node_t *ezyparse_parse(const char *src)
{
  ezy_ast_node_t *root = NULL;
  ezy_ast_node_t *curr = root;
  ezylex_start(src);
  while (true)
  {
    ezy_tkn_t tkn = ezylex_peek_tkn(0);
    if ( tkn.type == ezy_tkn_dummy ) {
      consume(1); // consume dummy token
      continue;
    }
    if (tkn.type == ezy_tkn_invalid)
    {
      ezy_log_error("lexer error: %s\n\t at line %u, col %u", tkn.data.t_string.ptr, tkn.line, tkn.col);
      break;
    }
    if (tkn.type == ezy_tkn_eof)
    {
      break;
    }
    ezy_ast_node_t* node;
    struct ezyparse_error err = ezyparse_parse_program(&node);
    if (root == NULL)
    {
      root = node;
      curr = root;
    } else {
      curr->next = node;
      curr = node;
    }
    if (err.msg != NULL)
    {
      ezy_log_error("parser error: %s\n\t at line %u, col %u", err.msg, err.last_tkn.line, err.last_tkn.col);
      ezylex_consume_tkn(1); // consume the problematic token
      curr->type = ezy_ast_node_error;
      curr->data.n_error.msg = err.msg;
      curr->data.n_error.err_token = err.last_tkn;
    }
  }
  return root;
}

#undef tok
#undef consume