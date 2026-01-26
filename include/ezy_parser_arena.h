#if !defined(ezy_parser_arena_h)
#define ezy_parser_arena_h
#include <stdint.h>

struct ezyparse_arena {
  uint8_t* mainbuf;
  size_t size;
  size_t used;
  struct ezyparse_arena* next;
};

#define ezyparse_arena_size 65536

void* ezyparse_arena_alloc(size_t size);
void ezyparse_arena_backtrack(size_t size, void* final_ptr);
void ezyparse_arena_clear();

#endif // ezy_parser_arena_h
