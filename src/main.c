#include <ezy_lexer.h>
#include <stdlib.h>
#include <ezy_log.h>
#include <ezy_parser.h>
#include <ezy_parser_arena.h>
#include <ezy_transpile_c.h>

void print_ast_node(ezy_ast_node_t* node, int indent) {
  for (int i = 0; i < indent * 2; i++) {
    ezy_log_raw("  ");
  }
  switch (node->type) {
    case ezy_ast_node_variable:
      ezy_log_raw("Variable(name: %.*s, type: %d", node->data.n_variable.name.len, node->data.n_variable.name.ptr, node->data.n_variable.typ.typ);
      if (node->data.n_variable.value) {
        ezy_log_raw(", value: \n");
        print_ast_node(node->data.n_variable.value, indent + 1);
        for (int i = 0; i < indent * 2; i++) {
          ezy_log_raw("  ");
        }
        ezy_log_raw(")\n");
      } else {
        ezy_log_raw(", value: NULL)\n");
      }
      break;
    case ezy_ast_node_literal:
      ezy_log_raw("Literal(");
      switch (node->data.n_literal.typ) {
        case ezy_ast_dt_int64:
          ezy_log_raw("int64: %lld", node->data.n_literal.value.t_int64);
          break;
        case ezy_ast_dt_string:
          ezy_log_raw("string: \"%.*s\"", node->data.n_literal.value.t_string.len, node->data.n_literal.value.t_string.ptr);
          break;
        case ezy_ast_dt_float64:
          ezy_log_raw("float64: %g", node->data.n_literal.value.t_float64);
          break;
        default:
          ezy_log_raw("other type=%d", node->data.n_literal.typ);
          break;
      }
      ezy_log_raw(")\n");
      break;
    case ezy_ast_node_variable_decl:
      ezy_log_raw("DeclVariable(isConst: %s, type: %d, value: \n", node->data.n_variable.typ.is_const ? "true" : "false", node->data.n_variable.typ.typ);
      print_ast_node(node->data.n_variable.value, indent + 1);
      break;
    case ezy_ast_node_function:
      ezy_log_raw("Function(");
      ezy_log_raw("name: %.*s, return_type: %d, params: ", node->data.n_function->name.len, node->data.n_function->name.ptr, node->data.n_function->return_typ.typ);
      for (int i = 0; i < node->data.n_function->param_count; i++) {
        ezy_log_raw("%d:", node->data.n_function->params[i].typ.typ);
        ezy_log_raw("%.*s ", node->data.n_function->params[i].name.len, node->data.n_function->params[i].name.ptr);
      }
      ezy_log_raw("):\n");
      print_ast_node(node->data.n_function->body, indent + 1);
      break;
    case ezy_ast_node_call:
      ezy_log_raw("FunctionCall(name: %.*s, args: \n", node->data.n_call->func_name.len, node->data.n_call->func_name.ptr);
      for (size_t i = 0; i < node->data.n_call->arg_count; i++) {
        print_ast_node(&node->data.n_call->args[i], indent + 1);
      }
      for (int i = 0; i < indent * 2; i++) {
        ezy_log_raw("  ");
      }
      ezy_log_raw(")\n");
      break;
    default:
      ezy_log("Other Node Type: %d\n", node->type);
      break;
  }
  if (node->next) {
    print_ast_node(node->next, indent);
  }
}

int main(int argc, const char** argv) {
  ezy_log("start of program");
  if ( argc < 2 ) {
    ezy_log_error("no input file specified");
    return 1;
  }
  const char* filename = argv[1];
  FILE* f = fopen(filename, "rb");
  if ( f == NULL ) {
    ezy_log_error("failed to open input file: %s", filename);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  char* buffer = (char*)malloc(fsize + 1);
  if (buffer == NULL) {
    ezy_log_error("failed to allocate memory for file buffer");
    fclose(f);
    return 1;
  }
  fread(buffer, 1, fsize, f);
  buffer[fsize] = '\0';
  fclose(f);

  ezy_log("file loaded: %s", filename);
  // now to test the parser...
  ezy_log("parsing...");
  ezy_ast_node_t* ast_root = ezyparse_parse(buffer);

  ezy_log("parsed\n");
  print_ast_node(ast_root, 0);

  ezy_log("transpiling to C...");
  ezy_cstr_t c_code = ezytranspile_c(ast_root);
  ezy_log("transpiled C code:\n%.*s", (int)c_code.len, c_code.ptr);

  ezyparse_arena_clear();
  free(buffer);
  return 0;
}