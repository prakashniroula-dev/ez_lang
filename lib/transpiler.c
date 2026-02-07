#include <ezy_ast.h>
#include <ezy_lexer.h>
#include <ezy_log.h>
#include <ezy_parser_arena.h>
#include <string.h>

void ezytranspile_c_node(ezy_ast_node_t *node, char **out, size_t *used, size_t *capacity)
{
  if (node == NULL || out == NULL || used == NULL || capacity == NULL)
  {
    return;
  }
  if (node->type == ezy_ast_node_function)
  {
    const char *return_type = NULL;
    struct ezy_ast_function_t *fn = node->data.n_function;
    switch (node->data.n_function->return_typ.typ)
    {
    case ezy_ast_dt_int64:
      return_type = "int64_t";
      break;
    case ezy_ast_dt_float64:
      return_type = "double";
      break;
    case ezy_ast_dt_string:
      return_type = "const char*";
      break;
    default:
      return_type = "void";
      break;
    }
    int n = snprintf(*out + *used, *capacity - *used, "%s %.*s(", return_type,
                     (int)fn->name.len,
                     fn->name.ptr);
    *used += n;
    for (size_t i = 0; i < fn->param_count; i++)
    {
      const char *param_type = NULL;
      switch (fn->params[i].typ.typ)
      {
      case ezy_ast_dt_int64:
        param_type = "int64_t";
        break;
      case ezy_ast_dt_int32:
        param_type = "int32_t";
        break;
      case ezy_ast_dt_string:
        param_type = "const char*";
        break;
      case ezy_ast_dt_bool:
        param_type = "bool";
        break;
      case ezy_ast_dt_float32:
        param_type = "float";
        break;
      case ezy_ast_dt_float64:
        param_type = "double";
        break;
      default:
        param_type = "void*";
        break;
      }
      n = snprintf(*out + *used, *capacity - *used, "%s %.*s%s",
                   param_type,
                   (int)fn->params[i].name.len,
                   fn->params[i].name.ptr,
                   (i < fn->param_count - 1) ? ", " : "");
      *used += n;
    }
    n = snprintf(*out + *used, *capacity - *used, ")");
    *used += n;
    if (fn->body != NULL)
    {
      n = snprintf(*out + *used, *capacity - *used, " {\n");
      *used += n;
      ezy_ast_node_t *body_node = fn->body;
      while (body_node != NULL)
      {
        ezytranspile_c_node(body_node, out, used, capacity);
        body_node = body_node->next;
      }
      n = snprintf(*out + *used, *capacity - *used, "}\n");
      *used += n;
    }
    else
    {
      n = snprintf(*out + *used, *capacity - *used, ";\n");
      *used += n;
    }
  }

  if (node->type == ezy_ast_node_variable_decl)
  {
    const char *var_type = NULL;
    struct ezy_ast_variable_t var = node->data.n_variable;
    switch (var.typ.typ)
    {
    case ezy_ast_dt_int32:
      var_type = "int32_t";
      break;
    case ezy_ast_dt_float64:
      var_type = "double";
      break;
    case ezy_ast_dt_string:
      var_type = "const char*";
      break;
    default:
      var_type = "void*";
      break;
    }
    int n = snprintf(*out + *used, *capacity - *used, "%s %.*s", var_type, (int)var.name.len, var.name.ptr);
    *used += n;
    if (var.value != NULL)
    {
      n = snprintf(*out + *used, *capacity - *used, " = ");
      *used += n;
      ezytranspile_c_node(var.value, out, used, capacity);
    }
    n = snprintf(*out + *used, *capacity - *used, ";\n");
    *used += n;
  }

  if (node->type == ezy_ast_node_literal)
  {
    struct ezy_ast_literal_t lit = node->data.n_literal;
    int n = 0;
    switch (lit.typ)
    {
    case ezy_ast_dt_int64:
      n = snprintf(*out + *used, *capacity - *used, "%lld", lit.value.t_int64);
      *used += n;
      break;
    case ezy_ast_dt_uint64:
      n = snprintf(*out + *used, *capacity - *used, "%llu", lit.value.t_uint64);
      *used += n;
      break;
    case ezy_ast_dt_float64:
      n = snprintf(*out + *used, *capacity - *used, "%g", lit.value.t_float64);
      *used += n;
      break;
    case ezy_ast_dt_string:
      n = snprintf(*out + *used, *capacity - *used, "\"%.*s\"", (int)lit.value.t_string.len, lit.value.t_string.ptr);
      *used += n;
      break;
    case ezy_ast_dt_bool:
      n = snprintf(*out + *used, *capacity - *used, "%s", lit.value.t_uint64 ? "true" : "false");
      *used += n;
      break;
    case ezy_ast_dt_char:
      n = snprintf(*out + *used, *capacity - *used, "'%c'", (char)lit.value.t_char);
      *used += n;
      break;
    default:
      n = snprintf(*out + *used, *capacity - *used, "/* unsupported literal type %d */", lit.typ);
      *used += n;
      break;
    }
  }

  if (node->type == ezy_ast_node_call)
  {
    struct ezy_ast_call_t *call = node->data.n_call;
    if (strncmp(call->func_name.ptr, "print", call->func_name.len) == 0)
    {
      // special handling for print function
      int n = snprintf(*out + *used, *capacity - *used, "printf(\"");
      *used += n;
      // one pass for type format string
      for (size_t i = 0; i < call->arg_count; i++)
      {
        struct ezy_ast_node_t *arg = &call->args[i];
        if (arg->type == ezy_ast_node_literal)
        {
          struct ezy_ast_literal_t lit = arg->data.n_literal;
          if (lit.typ == ezy_ast_dt_string)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%s");
            *used += n;
          }
          else if (lit.typ == ezy_ast_dt_int64)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%lld");
            *used += n;
          }
          else if (lit.typ == ezy_ast_dt_float64)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%g");
            *used += n;
          }
          else if (lit.typ == ezy_ast_dt_char)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%c");
            *used += n;
          }
          else if (lit.typ == ezy_ast_dt_bool)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%s");
            *used += n;
          }
          else if (lit.typ == ezy_ast_dt_uint64)
          {
            n = snprintf(*out + *used, *capacity - *used, "%%llu");
            *used += n;
          }
          else
          {
            n = snprintf(*out + *used, *capacity - *used, "/* unsupported literal type %d */", lit.typ);
            *used += n;
          }
        }
        if (i < call->arg_count - 1)
        {
          n = snprintf(*out + *used, *capacity - *used, " ");
          *used += n;
        }
      }
      n = snprintf(*out + *used, *capacity - *used, "\", ");
      *used += n;
      // second pass for argument values
      for (size_t i = 0; i < call->arg_count; i++)
      {
        struct ezy_ast_node_t *arg = &call->args[i];
        ezytranspile_c_node(arg, out, used, capacity);
        if (i < call->arg_count - 1)
        {
          n = snprintf(*out + *used, *capacity - *used, ", ");
          *used += n;
        }
      }
      n = snprintf(*out + *used, *capacity - *used, ");\n");
      *used += n;
      return;
    }
    int n = snprintf(*out + *used, *capacity - *used, "%.*s(", (int)call->func_name.len, call->func_name.ptr);
    *used += n;
    for (size_t i = 0; i < call->arg_count; i++)
    {
      ezytranspile_c_node(&call->args[i], out, used, capacity);
      if (i < call->arg_count - 1)
      {
        n = snprintf(*out + *used, *capacity - *used, ", ");
        *used += n;
      }
    }
    n = snprintf(*out + *used, *capacity - *used, ");\n");
    *used += n;
  }

  if (node->type == ezy_ast_node_binop)
  {
    struct ezy_ast_binop_t *binop = &node->data.n_binop;
    ezytranspile_c_node(binop->left, out, used, capacity);
    int n = snprintf(*out + *used, *capacity - *used, " %d ", binop->operator);
    *used += n;
    ezytranspile_c_node(binop->right, out, used, capacity);
  }

}

ezy_cstr_t ezytranspile_c(ezy_ast_node_t *node)
{
  size_t cap = 65535;
  char *out = ezyparse_arena_alloc(cap); // allocate 64KB for output
  size_t used = 0;
  while (node != NULL && out != NULL && used < cap)
  {
    ezytranspile_c_node(node, &out, &used, &cap);
    node = node->next;
  }
  ezy_cstr_t result = {out, used};
  return result;
}