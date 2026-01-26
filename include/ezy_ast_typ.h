#if !defined(ezy_ast_typ_h)
#define ezy_ast_typ_h


enum ezy_ast_datatype_typ {
  ezy_ast_dt_invalid = 0,

  /* Special use data types */
  ezy_ast_dt_void, 
  ezy_ast_dt_null,
  ezy_ast_dt_var,
  ezy_ast_dt_infer,

  /* Standard data types */

  ezy_ast_dt_int8,
  ezy_ast_dt_uint8,
  ezy_ast_dt_int16,
  ezy_ast_dt_uint16,
  ezy_ast_dt_int32,
  ezy_ast_dt_uint32,
  ezy_ast_dt_int64,
  ezy_ast_dt_uint64,

  ezy_ast_dt_float32,
  ezy_ast_dt_float64,

  ezy_ast_dt_bool,
  ezy_ast_dt_char,
  ezy_ast_dt_string,

  ezy_ast_dt_array,
  ezy_ast_dt_struct,
  ezy_ast_dt_union,
  ezy_ast_dt_function,
};

enum ezy_ast_literal_typ {
  ezy_ast_literal_invalid = 0,
  ezy_ast_literal_int64,
  ezy_ast_literal_uint64,
  ezy_ast_literal_float64,
  ezy_ast_literal_char,
  ezy_ast_literal_string,
  ezy_ast_literal_bool,
  ezy_ast_literal_null,
  ezy_ast_literal_array,
  ezy_ast_literal_struct
};

enum ezy_ast_node_typ {
  ezy_ast_node_invalid = 0,
  ezy_ast_node_error,
  ezy_ast_node_datatype,
  ezy_ast_node_variable,
  ezy_ast_node_function,
  ezy_ast_node_struct,
  ezy_ast_node_union,
  ezy_ast_node_literal,
  ezy_ast_node_call,
  ezy_ast_node_stmt,
};

#endif // ezy_ast_typ_h
