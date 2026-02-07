#if !defined(ezy_typ_h)
#define ezy_typ_h

#include <stdlib.h>

typedef struct
{
  const char *ptr;
  size_t len;
} ezy_cstr_t;

typedef struct ezy_multistr_t {
  ezy_cstr_t str;
  struct ezy_multistr_t *next;
} ezy_multistr_t;

#endif // ezy_typ_h
