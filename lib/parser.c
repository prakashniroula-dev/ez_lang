#include <ezy_ast.h>
#include <ezy_lexer.h>
#include <ezy_log.h>
#include <ezy_parser_arena.h>
#include <string.h>

static struct ezyparse_error
{
  const char *msg;
  ezy_tkn_t last_tkn;
  size_t tkn_consumed;
};

#define ezyparse_typelist(...)       \
  (enum ezy_tkn_typ[]){__VA_ARGS__}, \
      (sizeof((enum ezy_tkn_typ[]){__VA_ARGS__}) / sizeof(enum ezy_tkn_typ)) * /

// for easy parsing.. define easy way to peek token
#define tok(n) ezylex_peek_tkn(n)
#define consume(n) ezylex_consume_tkn(n)

static bool ezyparse_expect(ezy_tkn_t tkn, enum ezy_tkn_typ type)
{
  if (tkn.type == type)
    return true;
  ezy_log_error("(Expected token type %d but got %d at line %u, col %u)", type, tkn.type, tkn.line, tkn.col);
  return false;
}

static inline bool ezyparse_match(ezy_tkn_t tkn, enum ezy_tkn_typ type)
{
  return tkn.type == type;
}

static inline bool ezyparse_match_oneof(ezy_tkn_t tkn, const enum ezy_tkn_typ typlist[], int n)
{
  for (int i = 0; i < n; i++)
  {
    if (tkn.type == typlist[i])
      return true;
  }
  return false;
}

static struct ezyparse_error ezyparse_parse_expression(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = tok(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type == ezy_tkn_eof)
  {
    return err;
  }
  // To do : parse expressions
}

static struct ezyparse_error ezyparse_parse_statements(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = tok(0);
  struct ezyparse_error err = {NULL, 0, 0, 0};
  if (tkn.type == ezy_tkn_eof)
  {
    return err;
  }
  // To do : parse statements
}

static struct ezyparse_error ezyparse_parse_datatype(struct ezy_ast_datatype_t *dest)
{
  ezy_tkn_t tkn = tok(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_identifier)
  {
    return (struct ezyparse_error){"Expected datatype identifier", tkn, 0};
  }

  // datatype table: longer datatype names first
  static const struct
  {
    const char *name;
    enum ezy_ast_datatype_typ type;
  } datatype_table[] = {
      {"float32", ezy_ast_dt_float32},
      {"float64", ezy_ast_dt_float64},

      {"uint64", ezy_ast_dt_uint64},
      {"uint32", ezy_ast_dt_uint32},
      {"uint16", ezy_ast_dt_uint16},

      {"string", ezy_ast_dt_string},

      {"uint8", ezy_ast_dt_uint8},

      {"int64", ezy_ast_dt_int64},
      {"int16", ezy_ast_dt_int16},
      {"int32", ezy_ast_dt_int32},

      {"float", ezy_ast_dt_float32},

      {"int8", ezy_ast_dt_int8},
      {"uint", ezy_ast_dt_uint32},

      {"bool", ezy_ast_dt_bool},
      {"char", ezy_ast_dt_char},
      {"int", ezy_ast_dt_int32},
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
    return (struct ezyparse_error){"Unrecognized datatype identifier", tkn, 0};
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

  return (struct ezyparse_error){NULL, 0, 1};
}

static struct ezyparse_error ezyparse_parse_function(ezy_ast_node_t *dest)
{

  ezy_tkn_t tkn = tok(0);

  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_fn)
  {
    return (struct ezyparse_error){"Expected 'fn' keyword", tkn, 0};
  }
  ezy_ast_node_t *func_node = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
  struct ezy_ast_function_t *func_data = ezyparse_arena_alloc(sizeof(struct ezy_ast_function_t));

  func_node->type = ezy_ast_node_function;
  func_node->data.n_function = func_data;

  func_data->return_typ.typ = ezy_ast_dt_infer;

  consume(1); // consume 'fn' keyword

  // parse return type if specified
  ezyparse_parse_datatype(&func_data->return_typ);

  tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_identifier))
    return (struct ezyparse_error){"Expected function name / type identifier", tkn, 0};

  func_data->name = tkn.data.t_identifier;
  consume(1); // consume function name

  tkn = tok(0);
  if (!ezyparse_expect(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_l)
    return (struct ezyparse_error){"Expected '(' after function name", tkn, 0};

  consume(1); // consume '('

  tkn = tok(0);
  size_t param_count = 0;

  func_data->param_names = ezyparse_arena_alloc(sizeof(ezy_cstr_t) * 16); // max 16 params initially
  func_data->param_typlist = ezyparse_arena_alloc(sizeof(struct ezy_ast_datatype_t) * 16);

  size_t alloc_count = 16;

  while (!ezyparse_match(tkn, ezy_tkn_operator) || tkn.data.t_operator != ezy_op_brac_small_r)
  {
    if ( param_count > 0 )
    {
      ezyparse_expect(tkn, ezy_tkn_operator);
      if (tkn.data.t_operator != ezy_op_comma)
      {
        return (struct ezyparse_error){"Expected ',' between parameters", tkn, 0};
      }
      consume(1); // consume ','
    }

    struct ezyparse_error err = ezyparse_parse_datatype(&func_data->param_typlist[param_count]);
    if (err.msg != NULL)
    {
      return err;
    }

    tkn = tok(0);
    if (!ezyparse_expect(tkn, ezy_tkn_identifier))
      return (struct ezyparse_error){"Expected parameter name identifier", tkn, 0};

    consume(1); // consume parameter name

    if (param_count + 1 >= alloc_count)
    {
      void* old_param_names = func_data->param_names;
      void* old_param_typlist = func_data->param_typlist;
      
      ezyparse_arena_backtrack(sizeof(struct ezy_ast_datatype_t) * alloc_count, old_param_typlist);
      ezyparse_arena_backtrack(sizeof(ezy_cstr_t) * alloc_count, old_param_names);

      alloc_count *= 2;

      func_data->param_names = ezyparse_arena_alloc(sizeof(ezy_cstr_t) * alloc_count);
      func_data->param_typlist = ezyparse_arena_alloc(sizeof(struct ezy_ast_datatype_t) * alloc_count);

      memmove(func_data->param_names, old_param_names, sizeof(ezy_cstr_t) * param_count);
      memmove(func_data->param_typlist, old_param_typlist, sizeof(struct ezy_ast_datatype_t) * param_count);
    }
    func_data->param_names[param_count] = tkn.data.t_identifier;
    param_count++;

    tkn = tok(0);
  }

  ezyparse_expect(tkn, ezy_tkn_operator);
  if (tkn.data.t_operator != ezy_op_brac_small_r)
  {
    return (struct ezyparse_error){"Expected ')' at end of parameter list", tkn, 0};
  }
  consume(1); // consume ')'

  func_data->param_count = param_count;

  // clear unused allocated param space in arena
  void* old_param_names = func_data->param_names;
  void* old_param_typlist = func_data->param_typlist;

  ezyparse_arena_backtrack(sizeof(struct ezy_ast_datatype_t) * alloc_count, old_param_typlist);
  ezyparse_arena_backtrack(sizeof(ezy_cstr_t) * alloc_count, old_param_names);

  func_data->param_names = ezyparse_arena_alloc(sizeof(ezy_cstr_t) * param_count);
  func_data->param_typlist = ezyparse_arena_alloc(sizeof(struct ezy_ast_datatype_t) * param_count);

  memmove(func_data->param_names, old_param_names, sizeof(ezy_cstr_t) * param_count);
  memmove(func_data->param_typlist, old_param_typlist, sizeof(struct ezy_ast_datatype_t) * param_count);

  // To do : parse function body
}

static struct ezyparse_error ezyparse_parse_struct(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_struct)
  {
    return (struct ezyparse_error){"Expected 'struct' keyword", tkn, 0};
  }
  // To do : parse structs
}

static struct ezyparse_error ezyparse_parse_union(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || tkn.data.t_keyword != ezy_kw_union)
  {
    return (struct ezyparse_error){"Expected 'union' keyword", tkn, 0};
  }
  // To do : parse unions
}

static struct ezyparse_error ezyparse_parse_global_decl(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);
  struct ezyparse_error err = {NULL, 0, 0};
  if (tkn.type != ezy_tkn_keyword || (tkn.data.t_keyword != ezy_kw_let && tkn.data.t_keyword != ezy_kw_const))
  {
    return (struct ezyparse_error){"Expected 'let' or 'const' keyword", tkn, 0};
  }
  // To do : parse global declarations
}

static struct ezyparse_error ezyparse_parse_program(ezy_ast_node_t *dest)
{
  ezy_tkn_t tkn = ezylex_peek_tkn(0);

  if (tkn.type != ezy_tkn_keyword)
  {
    return (struct ezyparse_error){"Unexpected token, expected keyword", tkn, 0};
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
    return (struct ezyparse_error){"Unexpected keyword", tkn, 0};
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
    if (tkn.type == ezy_tkn_invalid)
    {
      ezy_log_error("lexer error: %s\n\t at line %u, col %u", tkn.data.t_string.ptr, tkn.line, tkn.col);
      break;
    }
    if (tkn.type == ezy_tkn_eof)
    {
      break;
    }
    struct ezyparse_error err = ezyparse_parse_program(curr);
    if (err.msg != NULL)
    {
      ezy_log_error("parser error: %s\n\t at line %u, col %u", err.msg, err.last_tkn.line, err.last_tkn.col);
      ezylex_consume_tkn(1); // consume the problematic token
      if (curr == NULL)
      {
        root = ezyparse_arena_alloc(sizeof(ezy_ast_node_t));
        curr = root;
      }
      curr->type = ezy_ast_node_error;
      curr->data.n_error.msg = err.msg;
      curr->data.n_error.err_token = err.last_tkn;
    }
  }
  return root;
}

#undef tok
#undef consume