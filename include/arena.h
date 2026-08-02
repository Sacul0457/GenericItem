#ifndef ARENA
#define ARENA

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    RET_FAIL = -1,
    RET_NULLPTR = -2,
    RET_OK = 1,
} RET_SIGN;


// =========================
//     Pools & Arenas
// =========================

typedef struct Pool {
    size_t live_bytes;
    size_t used;
    size_t capacity;
    void *base;
    struct Pool *next;
} Pool;

typedef struct Arena {
    // the number of pools
    size_t size;

    Pool *head;
    Pool *current;
} Arena;


#define DEFAULT_ARENA_SIZE 5
#define ARENA_CAPACITY 10

#define POOL_CAPACITY 600000000

Arena *arena_init();
RET_SIGN arena_reset(Arena *arena);
RET_SIGN arena_destroy(Arena *arena);

Pool *pool_init(Arena *arena);
RET_SIGN pool_destroy(Arena *arena, Pool *pool);
RET_SIGN pool_reset(Pool *pool);

void *arena_malloc(Arena *arena, const size_t size);
void *arena_realloc(Arena *arena, void *src, const size_t old_size, const size_t new_size);

bool try_free_fast(Arena *arena, void *src, const size_t curr_size);
bool try_free(Arena *arena, void *src, const size_t curr_size);

void arena_print(Arena *arena);
void pool_print(Pool *pool);

#endif
