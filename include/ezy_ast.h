
// AST types

// Datatypes
// Function types
 // built-in & user-defined 

// Array types
// Struct types
// Union types
// Variables


#if !defined(ezy_ast_h)
#define ezy_ast_h
#include <ezy_lexer.h>
#include <stdlib.h>
#include <ezy_ast_typ.h>
#include <ezy_typ.h>
#include <stdbool.h>
#include <stdint.h>

struct ezy_ast_datatype_t {
  enum ezy_ast_datatype_typ typ;
  bool nullable;
  bool is_ptr;
  bool is_const;
  union {
    struct ezy_ast_array_t* array_t;
    struct ezy_ast_struct_t* struct_t;
    struct ezy_ast_union_t* union_t;
    struct ezy_ast_function_t* function_t;
  } ext;
};

struct ezy_ast_array_t {
  bool dynamic;
  struct ezy_ast_datatype_t typ;
  size_t length;
  size_t max_length; // 0 means unlimited (for dynamic arrays)
};

struct ezy_ast_struct_t {
  size_t count;
  struct ezy_ast_datatype_t *mem_typlist;
  ezy_cstr_t *mem_names;
};

struct ezy_ast_union_t {
  size_t count;
  struct ezy_ast_datatype_t *mem_typlist;
};

struct ezy_ast_function_t {
  ezy_cstr_t name;
  struct ezy_ast_datatype_t return_typ;
  size_t param_count;
  ezy_cstr_t *param_names;
  struct ezy_ast_datatype_t *param_typlist;
  struct ezy_ast_node_t *body;
};

struct ezy_ast_variable_t {
  ezy_cstr_t name;
  struct ezy_ast_datatype_t typ;
};

struct ezy_ast_literal_t {
  enum ezy_ast_literal_typ typ;
  union {
    int64_t t_int64;
    uint64_t t_uint64;
    double t_float64;
    uint8_t t_char;
    ezy_cstr_t t_string;
    struct {
      size_t count;
      struct ezy_ast_literal_t* elements;
    } t_array;
    struct {
      size_t count;
      ezy_cstr_t* mem_names;
      struct ezy_ast_literal_t* mem_values;
    } t_struct;
  } value;
};

struct ezy_ast_call_t {
  ezy_cstr_t func_name;
  size_t arg_count;
  struct ezy_ast_node_t* args;
};

struct ezy_ast_node_error_t {
  const char* msg;
  ezy_tkn_t err_token;
};

typedef struct ezy_ast_node_t {
  enum ezy_ast_node_typ type;
  union {
    // smaller.. so keep by value
    struct ezy_ast_datatype_t n_datatype;
    struct ezy_ast_variable_t n_variable;
    struct ezy_ast_literal_t n_literal; 
    struct ezy_ast_node_error_t n_error;

    // larger.. so keep by pointer
    struct ezy_ast_function_t* n_function;
    struct ezy_ast_struct_t* n_struct;
    struct ezy_ast_union_t* n_union;
    struct ezy_ast_call_t* n_call;
  } data;
  ezy_ast_node_t* next;
} ezy_ast_node_t;

#endif // ezy_ast_h