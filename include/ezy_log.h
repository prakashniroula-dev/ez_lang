#if !defined(ezy_log_h)
#define ezy_log_h

#include <stdio.h>

#define ezy__log__makestr(x) #x
#define ezy__log_makestr(x) ezy__log__makestr(x)
#define ezy__log_expand_line() __LINE__
#define ezy__log_expand_file() __FILE__ 

#define ezy__log_info() \
  "(file: \"" ezy__log_expand_file() "\"; " \
  "line: " ezy__log_makestr(ezy__log_expand_line()) "): " \

#define ezy_log(...) fprintf(stderr, \
  "\n[ezy_log] " ezy__log_info() \
  __VA_ARGS__ \
)

#define ezy_log_warn(...) fprintf(stderr, \
  "\n![ezy_warn] " ezy__log_info() \
  __VA_ARGS__ \
)

#define ezy_log_error(...) fprintf(stderr, \
  "\n(!!)[ezy_error] " ezy__log_info() \
  __VA_ARGS__ \
)

#endif // ezy_log_h