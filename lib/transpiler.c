#include <ezy_ast.h>
#include <ezy_lexer.h>
#include <ezy_log.h>
#include <ezy_parser_arena.h>
#include <string.h>
#include <inttypes.h>

// Declare main transpilation function
ezy_multistr_t *ezytranspile_c(ezy_ast_node_t *node);

// helper functions
void ezytranspile_top_level(ezy_ast_node_t *node, ezy_multistr_t **out);
static inline void ezyt_append_buf(ezy_multistr_t **out, const char *src, size_t n);

// Transpilation functions for specific node types
bool ezytranspile_function(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_variable_decl(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_literal(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_call(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_stmt(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_expression(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_binop(ezy_ast_node_t *node, ezy_multistr_t **out);
bool ezytranspile_datatype(struct ezy_ast_datatype_t *datatype, ezy_multistr_t **out);

static char ezyt_tmp_buf[1024];
#define ezyt_max_cstr_size 65536

// ================ Helper functions to append buffer ================

static inline void ezyt_append_buf(ezy_multistr_t **out, const char *src, size_t n)
{
  ezy_multistr_t *buf = *out;
  if (n > ezyt_max_cstr_size)
  {
    ezy_log_warn("append too large for chunk, rejected");
    return;
  }
  if (buf->str.len + n >= ezyt_max_cstr_size)
  {
    ezy_multistr_t *newbuf = ezyparse_arena_alloc(sizeof(ezy_multistr_t));
    char *ptr = ezyparse_arena_alloc(ezyt_max_cstr_size);
    if (newbuf == NULL || ptr == NULL)
    {
      ezy_log_warn("Memory allocation failed");
      return;
    }
    newbuf->str.ptr = ptr;
    newbuf->str.len = 0;
    newbuf->next = NULL;
    buf->next = newbuf;
    *out = newbuf;
    buf = newbuf;
  }
  memcpy(buf->str.ptr + buf->str.len, src, n);
  buf->str.len += n;
}

// helper macro to append formatted string to output buffer
#define ezyt_append(out, ...)                                          \
  do                                                                   \
  {                                                                    \
    int n = snprintf(ezyt_tmp_buf, sizeof(ezyt_tmp_buf), __VA_ARGS__); \
    if (n >= sizeof(ezyt_tmp_buf))                                     \
    {                                                                  \
      n = sizeof(ezyt_tmp_buf) - 1;                                    \
      ezy_log_warn("Formatted string truncated in ezyt_append");       \
    }                                                                  \
    if (n > 0)                                                         \
    {                                                                  \
      ezyt_append_buf(out, ezyt_tmp_buf, (size_t)n);                   \
    }                                                                  \
    else                                                               \
    {                                                                  \
      ezy_log_warn("Formatting error in ezyt_append");                 \
    }                                                                  \
  } while (0)

// =============== Transpilation functions for specific nodes ================

// Transpile a datatype to C type and append to output buffer
bool ezytranspile_datatype(struct ezy_ast_datatype_t *datatype, ezy_multistr_t **out)
{
  // make a static table mapping
  static const struct
  {
    enum ezy_ast_datatype_typ typ;
    const char *c_type;
  } type_mapping[] = {
      {ezy_ast_dt_int8, "int8_t"},
      {ezy_ast_dt_uint8, "uint8_t"},
      {ezy_ast_dt_int16, "int16_t"},
      {ezy_ast_dt_uint16, "uint16_t"},
      {ezy_ast_dt_int32, "int32_t"},
      {ezy_ast_dt_uint32, "uint32_t"},
      {ezy_ast_dt_int64, "int64_t"},
      {ezy_ast_dt_uint64, "uint64_t"},
      {ezy_ast_dt_float32, "float"},
      {ezy_ast_dt_float64, "double"},
      {ezy_ast_dt_bool, "bool"},
      {ezy_ast_dt_char, "char"},
      {ezy_ast_dt_string, "char*"},
      {ezy_ast_dt_void, "void"},
  };

  if ( datatype->is_const ) {
    ezyt_append(out, "const ");
  }

  for (size_t i = 0; i < sizeof(type_mapping) / sizeof(type_mapping[0]); i++)
  {
    if (datatype->typ == type_mapping[i].typ)
    {
      ezyt_append(out, "%s", type_mapping[i].c_type);
      return true;
    }
  }

  if ( datatype->is_ptr ) {
    ezyt_append(out, " *");
  }

  return false;
}

void ezyt_append_char_literal(char c, ezy_multistr_t **out)
{
  ezyt_append(out, "\'");
  switch (c)
  {
  case '\n':
    ezyt_append(out, "\\n");
    break;
  case '\t':
    ezyt_append(out, "\\t");
    break;
  case '\\':
    ezyt_append(out, "\\\\");
    break;
  case '\'':
    ezyt_append(out, "\\\'");
    break;
  default:
    if (c >= 32 && c <= 126)
    {
      ezyt_append(out, "%c", c);
    }
    else
    {
      ezyt_append(out, "\\x%02x", (unsigned char)c);
    }
    break;
  }
  ezyt_append(out, "\'");
}

void ezyt_append_str_literal(ezy_cstr_t str, ezy_multistr_t **out)
{
  // handle escaping
  ezyt_append(out, "\"");
  bool has_escape = false;
  for (size_t i = 0; i < str.len; i++)
  {
    char c = str.ptr[i];
    if (has_escape)
    {
      has_escape = false;
      switch (c)
      {
      case 'n':
        ezyt_append(out, "\\n");
        break;
      case 't':
        ezyt_append(out, "\\t");
        break;
      case '\\':
        ezyt_append(out, "\\\\");
        break;
      case '"':
        ezyt_append(out, "\\\"");
        break;
      case '\'':
        ezyt_append(out, "\\\'");
        break;
      default:
        ezy_log_warn("Unsupported escape sequence \\%c in string literal", c);
        if (c >= 32 && c <= 126)
          ezyt_append(out, "<?\\%c>", c);
        else
          ezyt_append(out, "<?\\x%02x>", (unsigned char)c);
        break;
      }
      continue;
    }
    if (c == '\\')
    {
      has_escape = true;
      continue;
    }
    if (c >= 32 && c <= 126)
    {
      ezyt_append(out, "%c", c);
    }
    else
    {
      ezyt_append(out, "\\x%02x", (unsigned char)c);
    }
  }
  if (has_escape) {
    ezy_log_warn("String ends with trailing backslash");
    ezyt_append(out, "<?\\>");
  }
  ezyt_append(out, "\"");
}

bool ezytranspile_binop_single_v(struct ezy_ast_node_t* left_or_right, ezy_multistr_t **out)
{
  ezy_ast_node_t *node = left_or_right;
  if ( node->type == ezy_ast_node_literal )
  {
    struct ezy_ast_literal_t lit = node->data.n_literal;
    if (!ezytranspile_literal(node, out))
    {
      ezy_log_warn("Unsupported literal type %d in binary operand", lit.typ);
      return false;
    }
  } else if ( node->type == ezy_ast_node_variable ) {
    struct ezy_ast_variable_t var = node->data.n_variable;
    ezyt_append(out, "%.*s", var.name.len, var.name.ptr);
  } else if ( node->type == ezy_ast_node_call ) {
    if (!ezytranspile_call(node, out))
    {
      ezy_log_warn("Unsupported call node in binary operand");
      return false;
    }
  }
  else
  {
    ezy_log_warn("Unsupported node type %d in binary operand", node->type);
    return false;
  }

  return true;
}

bool ezytranspile_binop(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  struct ezy_ast_binop_t binop = node->data.n_binop;
  ezy_ast_node_t *left = binop.left;
  ezy_ast_node_t *right = binop.right;

  if (left && left->type == ezy_ast_node_binop)
  {
    ezyt_append(out, "(");
    if (!ezytranspile_binop(left, out))
    return false;
    ezyt_append(out, ")");
  } else {
    if (!ezytranspile_binop_single_v(left, out))
      return false;
  }
  
  static const struct {
    enum ezy_op_typ op;
    const char *c_op;
  } op_mapping[] = {
      {ezy_op_plus, "+"},
      {ezy_op_minus, "-"},
      {ezy_op_asterisk, "*"},
      {ezy_op_divide, "/"},
      {ezy_op_modulo, "%"},
  };

  const char *c_op = NULL;
  for (size_t i = 0; i < sizeof(op_mapping) / sizeof(op_mapping[0]); i++)
  {
    if (binop.operator == op_mapping[i].op)
    {
      c_op = op_mapping[i].c_op;
      break;
    }
  }
  if (c_op == NULL)
  {
    ezy_log_warn("Unsupported binary operator %d", binop.operator);
    return false;
  }

  ezyt_append(out, " %s ", c_op);

  if ( right && right->type == ezy_ast_node_binop ) {
    ezyt_append(out, "(");
    if (!ezytranspile_binop(right, out))
      return false;
    ezyt_append(out, ")");
    return true;
  } else {
    if (!ezytranspile_binop_single_v(right, out))
      return false;
    return true;
  }

  ezy_log_warn("Unsupported node type %d in binary operator right operand", right->type);
  return false; // should not reach here
}

bool ezytranspile_variable_decl(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  struct ezy_ast_variable_t var = node->data.n_variable;
  // variable type can be inferred from initializer if type is infer
  if (var.typ.typ == ezy_ast_dt_infer)
  {
    if (var.value == NULL)
    {
      ezy_log_warn("Cannot infer type of variable '%.*s' without initializer", var.name.len, var.name.ptr);
      return false;
    }
    // only support inferring from literals for now
    if (var.value->type == ezy_ast_node_literal)
    {
      var.typ.typ = var.value->data.n_literal.typ;
    }
    else
    {
      ezy_log_warn("Cannot infer type of variable '%.*s' from non-literal initializer", var.name.len, var.name.ptr);
      return false;
    }
  };

  if (!ezytranspile_datatype(&var.typ, out))
  {
    ezy_log_warn("Unsupported variable type %d", var.typ.typ);
    return false;
  }

  ezyt_append(out, " %.*s", var.name.len, var.name.ptr);
  if (var.value != NULL)
  {
    ezyt_append(out, " = ");
    if (var.value->type == ezy_ast_node_literal)
    {
      if (!ezytranspile_literal(var.value, out))
      {
        ezy_log_warn("Unsupported literal type %d for variable initializer", var.value->data.n_literal.typ);
        return false;
      }
    }
    else if ( var.value->type == ezy_ast_node_binop )
    {
      if (!ezytranspile_binop(var.value, out))
      {
        ezy_log_warn("Unsupported binary operator in variable initializer");
        return false;
      }
    }
    else
    {
      ezy_log_warn("Unsupported variable initializer node type %d", var.value->type);
      return false;
    }
  }
  return true;
}

const char* ezytranspile_dt_cfmt(enum ezy_ast_datatype_typ dt) {
  switch (dt) {
    case ezy_ast_dt_int8: return "%" PRId8;
    case ezy_ast_dt_uint8: return "%" PRIu8;
    case ezy_ast_dt_int16: return "%" PRId16;
    case ezy_ast_dt_uint16: return "%" PRIu16;
    case ezy_ast_dt_int32: return "%" PRId32;
    case ezy_ast_dt_uint32: return "%" PRIu32;
    case ezy_ast_dt_int64: return "%" PRId64;
    case ezy_ast_dt_uint64: return "%" PRIu64;
    case ezy_ast_dt_float32: return "%g";
    case ezy_ast_dt_float64: return "%g";
    case ezy_ast_dt_bool: return "%s";
    case ezy_ast_dt_char: return "%c";
    case ezy_ast_dt_string: return "%s";
    default: return "/* unsupported type */ %s";
  }
}

bool ezytranspile_literal(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  struct ezy_ast_literal_t lit = node->data.n_literal;
  const char* fmt = ezytranspile_dt_cfmt(lit.typ);
  switch (lit.typ)
  {
  case ezy_ast_dt_int64:
  case ezy_ast_dt_int32:
  case ezy_ast_dt_int16:
  case ezy_ast_dt_int8:
    ezyt_append(out, fmt, lit.value.t_int64);
    break;

  case ezy_ast_dt_uint64:
  case ezy_ast_dt_uint32:
  case ezy_ast_dt_uint16:
  case ezy_ast_dt_uint8:
    ezyt_append(out, fmt, lit.value.t_uint64);
    break;

  case ezy_ast_dt_float32:
  case ezy_ast_dt_float64:
    ezyt_append(out, fmt, lit.value.t_float64);
    break;

  case ezy_ast_dt_string:
    ezyt_append_str_literal(lit.value.t_string, out);
    break;

  case ezy_ast_dt_bool:
    ezyt_append(out, "%s", lit.value.t_uint64 ? "true" : "false");
    break;

  case ezy_ast_dt_char:
    ezyt_append_char_literal(lit.value.t_char, out);
    break;
  default:
    ezyt_append(out, "/* unsupported literal type %d */", lit.typ);
    return false;
  }
  return true;
}

bool ezytranspile_call(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  struct ezy_ast_call_t *call_data = node->data.n_call;
  // check for built-in functions
  if ( strncmp(call_data->func_name.ptr, "print", call_data->func_name.len) == 0)
  {
    ezyt_append(out, "printf(\"");

    // first pass : format string
    for (size_t i = 0; i < call_data->arg_count; i++)
    {
      switch (call_data->args[i].type)
      {
        case ezy_ast_node_literal:
          ezyt_append(out, "%s", ezytranspile_dt_cfmt(call_data->args[i].data.n_literal.typ));
          break;
        case ezy_ast_node_variable:
          ezyt_append(out, "/* todo: variable */");
          break;
        case ezy_ast_node_call:
          ezyt_append(out, "/* todo: nested call */");
          break;
        case ezy_ast_node_binop:
          ezyt_append(out, "/* todo: binop result */");
          break;
        default:
          ezyt_append(out, "/* unsupported arg type %d */", call_data->args[i].type);
          break;
      }
      if (i < call_data->arg_count - 1)
      {
        ezyt_append(out, " ");
      }
    }

    ezyt_append(out, "\", ");

    // second pass : arguments
    for (size_t i = 0; i < call_data->arg_count; i++)
    {
      switch (call_data->args[i].type)
      {
        case ezy_ast_node_literal:
          struct ezy_ast_datatype_t dt = { .typ = call_data->args[i].data.n_literal.typ };
          // explicitly typecast non-string/char/bool literals to ensure correct printf formatting
          if ( dt.typ != ezy_ast_dt_string && dt.typ != ezy_ast_dt_char && dt.typ != ezy_ast_dt_bool ) {
            ezyt_append(out, "(");
            ezytranspile_datatype(&dt, out);
            ezyt_append(out, ")");
          }
          ezytranspile_literal(&call_data->args[i], out);
          break;
        case ezy_ast_node_variable:
          ezyt_append(out, "/* todo: variable */");
          continue;
        case ezy_ast_node_call:
          ezyt_append(out, "/* todo: nested call */");
          continue;
        case ezy_ast_node_binop:
          ezyt_append(out, "/* todo: binop result */");
          continue;
        default:
          ezyt_append(out, "/* unsupported arg type %d */", call_data->args[i].type);
          continue;
      }
      if (i < call_data->arg_count - 1)
      {
        ezyt_append(out, ", ");
      }
    }
    
    ezyt_append(out, ")");
    return true;
  }

  // other function calls
  ezyt_append(out, "%.*s(", call_data->func_name.len, call_data->func_name.ptr);
  for (size_t i = 0; i < call_data->arg_count; i++)
  {
    if (!ezytranspile_expression(&call_data->args[i], out))
    {
      ezy_log_warn("Unsupported argument type %d in function call", call_data->args[i].type);
      ezyt_append(out, "/* unsupported arg type %d */", call_data->args[i].type);
      continue;
    }
    if (i < call_data->arg_count - 1)
    {
      ezyt_append(out, ", ");
    }
  }
  ezyt_append(out, ")");

  return true;
}

bool ezytranspile_expression(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  bool res = false;

  if (node->type == ezy_ast_node_call)
    res = ezytranspile_call(node, out);

  else if (node->type == ezy_ast_node_binop)
    res = ezytranspile_binop(node, out);

  else if (node->type == ezy_ast_node_literal)
    res = ezytranspile_literal(node, out);

  else if (node->type == ezy_ast_node_variable)
  {
    struct ezy_ast_variable_t var = node->data.n_variable;
    ezyt_append(out, "%.*s", var.name.len, var.name.ptr);
    res = true;
  }

  return res;
}

bool ezytranspile_stmt(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  bool res = false;

  if (node->type == ezy_ast_node_variable_decl)
    res = ezytranspile_variable_decl(node, out);
  else
    res = ezytranspile_expression(node, out);

  if (res)
  {
    ezyt_append(out, ";\n");
  }
  else
  {
    ezyt_append(out, "/* failed to transpile statement of type %d */\n", node->type);
  }

  return res;
}

bool ezytranspile_function(ezy_ast_node_t *node, ezy_multistr_t **out)
{

  const char *return_type = NULL;
  struct ezy_ast_function_t *fn = node->data.n_function;

  if (!ezytranspile_datatype(&fn->return_typ, out))
  {
    ezy_log_warn("Unsupported return type for function %.*s", (int)fn->name.len, fn->name.ptr);
    return false;
  }

  ezyt_append(out, " %.*s(", (int)fn->name.len, fn->name.ptr);

  for (size_t i = 0; i < fn->param_count; i++)
  {
    const char *param_type = "";
    if (!ezytranspile_datatype(&fn->params[i].typ, out))
    {
      ezy_log_warn("Unsupported parameter type for function %.*s", (int)fn->name.len, fn->name.ptr);
      ezyt_append(out, "/* unsupported param type */ void* %.*s", (int)fn->params[i].name.len, fn->params[i].name.ptr);
    }
    else
    {
      // leading space is required for correct formatting
      ezyt_append(out, " %.*s", (int)fn->params[i].name.len, fn->params[i].name.ptr);
    }
    if (i < fn->param_count - 1)
    {
      ezyt_append(out, ", ");
    }
  }

  ezyt_append(out, ")");

  if (fn->body != NULL)
  {
    ezyt_append(out, " {\n");
    ezy_ast_node_t *body_node = fn->body;
    while (body_node != NULL)
    {
      ezytranspile_stmt(body_node, out);
      body_node = body_node->next;
    }
    ezyt_append(out, "}\n");
  }
  else
  {
    ezyt_append(out, ";\n");
  }

  return true;
}

// top level
void ezytranspile_top_level(ezy_ast_node_t *node, ezy_multistr_t **out)
{
  if (node == NULL)
    return;

  switch (node->type)
  {
  case ezy_ast_node_function:
    ezytranspile_function(node, out);
    break;
  case ezy_ast_node_variable_decl:
    ezytranspile_variable_decl(node, out);
    break;
  default:
    ezy_log_warn("Unsupported AST node type %d in transpilation", node->type);
    break;
  }
}

static const char *c_biolerplate =
    "#include <stdint.h>\n"
    "#include <stdbool.h>\n"
    "\n";

ezy_multistr_t *ezytranspile_c(ezy_ast_node_t *node)
{
  ezy_multistr_t *head = ezyparse_arena_alloc(sizeof(ezy_multistr_t));
  char *ptr = ezyparse_arena_alloc(ezyt_max_cstr_size); // allocate 64KB for output
  if (ptr == NULL || head == NULL)
  {
    ezy_log_warn("Memory allocation failed in ezytranspile_c");
    return NULL;
  }
  head->str.ptr = ptr;
  head->str.len = 0;
  head->next = NULL;
  ezy_multistr_t *current = head;

  // Initial boilerplate for C output
  ezyt_append_buf(&current, c_biolerplate, strlen(c_biolerplate));

  while (node != NULL)
  {
    ezytranspile_top_level(node, &current);
    node = node->next;
  }
  return head;
}