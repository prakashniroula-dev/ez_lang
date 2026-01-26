
#include <stdlib.h>
#include <ezy_log.h>
#include <ezy_parser_arena.h>

static uint8_t ezyparse_arena[ezyparse_arena_size];
static struct ezyparse_arena arena_head = {
  .mainbuf = ezyparse_arena,
  .size = ezyparse_arena_size,
  .used = 0,
  .next = NULL,
};

/* Allocate memory from parser arena */
void* ezyparse_arena_alloc(size_t size)
{
  struct ezyparse_arena* arena = &arena_head;
  while ( arena->used + size > arena->size )
  {
    if ( arena->next == NULL )
    {
      ezy_log_warn("ezyparse_arena_alloc: arena out of memory, allocating new arena block.");
      struct ezyparse_arena* new_arena = (struct ezyparse_arena*)malloc(sizeof(struct ezyparse_arena));
      new_arena->mainbuf = (uint8_t*)malloc(ezyparse_arena_size);
      new_arena->size = ezyparse_arena_size;
      new_arena->used = 0;
      new_arena->next = NULL;
      arena->next = new_arena;
    }
    arena = arena->next;
  }
  void* ptr = arena->mainbuf + arena->used;
  arena->used += size;
  return ptr;
}

void ezyparse_arena_backtrack(size_t size, void* final_ptr)
{
  struct ezyparse_arena* arena = &arena_head;
  while ( arena != NULL )
  {
    if ((uint8_t*)final_ptr + size == (uint8_t*)arena->mainbuf + arena->used ) {
      arena->used -= size;
      return;
    }
    arena = arena->next;
  }
}

/* Reset parser arena */
void ezyparse_arena_clear()
{
  struct ezyparse_arena* arena = arena_head.next;
  while ( arena != NULL )
  {
    struct ezyparse_arena* next = arena->next;
    free(arena->mainbuf);
    free(arena);
    arena = next;
  }
  arena_head.used = 0;
  arena_head.next = NULL;
}