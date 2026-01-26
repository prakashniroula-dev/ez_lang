
/* 

typedef struct
{
  size_t argc;

  ezy_tkn_typ ret_typ;
  union ezy_tkn_unit *ret;

  ezy_tkn_typ *args_typelist;
  union ezy_tkn_unit *args_list;
} ezy_func_t;

typedef struct
{
  size_t count;
  enum ezy_tkn_typ *typelist;
} ezy_union_t;

typedef struct
{
  size_t count;
  enum ezy_tkn_typ *typelist;
  union ezy_tkn_unit *datalist;
} ezy_struct_t;
 */

/* == Datatypes == */
// enum ezy_dt_typ
// {
//   ezy_dt_invalid = 0,

//   /* Boolean */
//   ezy_dt_bool,

//   /* int family */
//   ezy_dt_byte,
//   ezy_dt_int8,
//   ezy_dt_int16,
//   ezy_dt_int32,
//   ezy_dt_int64,
//   ezy_dt_int128,

//   /* uint family */
//   ezy_dt_ubyte,
//   ezy_dt_uint8,
//   ezy_dt_uint16,
//   ezy_dt_uint32,
//   ezy_dt_uint64,
//   ezy_dt_uint128,

//   /* float family */
//   ezy_dt_float8,
//   ezy_dt_float16,
//   ezy_dt_float32,
//   ezy_dt_float64,
//   ezy_dt_float128,

//   /* others */
//   ezy_dt_char,
//   ezy_dt_var,
//   ezy_dt_null,
//   ezy_dt_void,

//   /* arrays */
//   ezy_dt_arr_fixed,
//   ezy_dt_arr_dynamic,

//   ezy_dt_union,
//   ezy_dt_struct,

//   ezy_dt_func,
// };

/* #define ezylex_typelist(...) \
  (enum ezy_tkn_typ[]){__VA_ARGS__}, \
  (sizeof((enum ezy_tkn_typ[]){__VA_ARGS__}) / sizeof(enum ezy_tkn_typ)) */

/* 
static bool ezylex_expect(ezy_tkn_t tkn, enum ezy_tkn_typ type) {
  if ( tkn.type == type ) return true;
  ezy_log_error("()");
  return false;
}

static inline bool ezylex_match(ezy_tkn_t tkn, enum ezy_tkn_typ type) {
  return tkn.type == type;
}

static inline bool ezylex_match_oneof(ezy_tkn_t tkn, const enum ezy_tkn_typ typlist[], int n) {
  for ( int i = 0; i < n; i++) {
    if ( tkn.type == typlist[i] ) return true;
  }
  return false;
}
   */