#include <stdio.h>
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <string.h>

#include "arena.h"


Arena *
arena_init()
{
    Arena *arena = malloc(sizeof(Arena));
    if (arena == NULL) {
        return NULL;
    }
    arena->size = 0;
    arena->current = NULL;
    arena->head = NULL;

    for (int i = 0; i < DEFAULT_ARENA_SIZE; i++) {
        if (pool_init(arena) == NULL) {
            arena_destroy(arena);
            return NULL;
        }
    }
    return arena;
}

static inline RET_SIGN _pool_destroy(Pool *pool);

RET_SIGN
arena_destroy(Arena *arena)
{
    if (arena == NULL) {
        return RET_NULLPTR;
    }

    if (arena->head) {
        Pool *current = arena->head;
        while (current) {
            Pool *next = current->next;
            _pool_destroy(current);
            current = next;
        }
    }
    
    free(arena);
    return RET_OK;
}

// reset all the pools in the arenas
// and reset the number of pools to default size
RET_SIGN
arena_reset(Arena *arena)
{
    if (arena == NULL || arena->head == NULL) {
        return RET_NULLPTR;
    }

    Pool *pool = arena->head;

    while (pool) {
        Pool *next = pool->next;
        if (arena->size > DEFAULT_ARENA_SIZE) {
            pool_destroy(arena, pool);
        }
        else {
            pool_reset(pool);
        }
        pool = next;
    }

    assert(arena->size == DEFAULT_ARENA_SIZE);
    arena->current = arena->head;
    return RET_OK;
}

#define POOL_IS_FULL(pool) ((pool)->used >= (pool)->capacity)

Pool *
pool_init(Arena *arena)
{
    if (!arena) {
        return NULL;
    }

    Pool *new_pool = malloc(sizeof(Pool));
    if (new_pool == NULL) {
        return NULL;
    }

    void *base = malloc(POOL_CAPACITY);
    if (base == NULL) {
        free(new_pool);
        return NULL;
    }

    new_pool->base = base;
    new_pool->used = 0;
    new_pool->live_bytes = 0;
    new_pool->capacity = POOL_CAPACITY;
    new_pool->next = NULL;

    // link the arena
    if (arena->size == 0) {
        arena->head = new_pool;
        arena->current = new_pool;
    }
    else {
        Pool *curr_pool = arena->head;
        for (size_t i = 0; i < arena->size - 1; i++) {
            curr_pool = curr_pool->next;
        }
        assert(curr_pool != NULL);
        curr_pool->next = new_pool;

        if (POOL_IS_FULL(curr_pool)) {
            arena->current = new_pool;
        }
    }

    arena->size++;
    return new_pool;
}


// public api
// auto connects the linked lists
RET_SIGN
pool_destroy(Arena *arena, Pool *pool)
{
    if (arena == NULL || pool == NULL || arena->head == NULL) {
        return RET_NULLPTR;
    }

    Pool *current = arena->head;
    while (current->next) {
        if (current->next == pool) {
            break;
        }
        current = current->next;
    }
    assert(current != NULL);
    if (current->next != pool) {
        return RET_FAIL;
    }

    current->next = current->next->next;
    free(pool->base);
    free(pool);
    arena->size--;
    return RET_OK;
}


RET_SIGN
pool_reset(Pool *pool)
{
    if (pool == NULL) {
        return RET_NULLPTR;
    }

    pool->used = 0;
    pool->live_bytes = 0;
    return RET_OK;
}


// private api
// does not connect the pools together
static inline RET_SIGN
_pool_destroy(Pool *pool)
{
    if (!pool) {
        return RET_NULLPTR;
    }

    free(pool->base);
    free(pool);
    return RET_OK;
}

#define GET_ALIGNED_PTR_VALUE(ptr_value) ((ptr_value) + ((alignof(max_align_t)) - (ptr_value % (alignof(max_align_t)))) % (alignof(max_align_t)))

static inline size_t
get_aligned_size(void *base, size_t used)
{   
    uintptr_t base_ptr = (uintptr_t)base;
    uintptr_t used_ptr_value = base_ptr + used;
    // get new aligned ptr value
    uintptr_t aligned_used_ptr = GET_ALIGNED_PTR_VALUE(used_ptr_value);
    // new aligned ptr value - the base ptr value to get the size_t
    return aligned_used_ptr - base_ptr;
}

static inline void *
_arena_malloc(Arena *arena, Pool *pool, const size_t size)
{
    if (pool == NULL) {
        return NULL;
    }

    size_t aligned_used = get_aligned_size(pool->base, pool->used);
    size_t new_used = aligned_used + size;
    if (new_used > pool->capacity) {
        return NULL;
    }

    void *ret = (char *)pool->base + aligned_used;

    pool->live_bytes += size; // we de not include the alignmente
    pool->used = new_used;

    if (POOL_IS_FULL(pool)) {
        if (pool->next == NULL) {
            pool_init(arena);
        }
        arena->current = pool->next;
    }

    return ret;
}

void *
arena_malloc(Arena *arena, const size_t size)
{
    if (arena == NULL || arena->current == NULL) {
        return NULL;
    }

    if (size > POOL_CAPACITY) {
        printf("ERROR: %zu > POOL_CAPACITY (%d)\n", size, POOL_CAPACITY);
        return NULL;
    }

    void *ret = NULL;
    Pool *curr_pool = arena->current;
    while (curr_pool) {
        ret = _arena_malloc(arena, curr_pool, size);
        if (ret) {
            return ret;
        }
        curr_pool = curr_pool->next;
    }
    
    // all other pools have not enough space
    // allocate new pool
    Pool *new_pool = pool_init(arena);
    if (!new_pool) {
        return NULL;
    }

    ret = _arena_malloc(arena, new_pool, size);
    return ret;
}

// frees the pool if the number of pools
// in the arena is more than the default size
// If not, sets the 'used' to 0
static inline void
cleanup_pool_if_needed(Arena *arena, Pool *pool)
{
    assert(arena && pool);

    if (pool->live_bytes == 0) {
        if (arena->size > DEFAULT_ARENA_SIZE) {
            pool_destroy(arena, pool);
        }
        else {
            pool->used = 0;
        }
    }
}

void *
arena_realloc(Arena *arena, void *src, const size_t old_size, const size_t new_size)
{
    if (arena == NULL || src == NULL) {
        return NULL;
    }

    if (old_size == new_size) {
        return src;
    }

    if (new_size == 0) {
        try_free(arena, src, old_size);
        return src;
    }

    Pool *current_pool = arena->current;
    uintptr_t pool_used_ptr = (uintptr_t)((char *)current_pool->base + current_pool->used);

    uintptr_t src_base_ptr = (uintptr_t)src;
    uintptr_t src_end_ptr = (uintptr_t)((char *)src + old_size);

    if (pool_used_ptr == src_end_ptr) {
        if (new_size > old_size) {
            size_t curr_aligned_used = get_aligned_size(current_pool->base, current_pool->used);
            size_t new_used = curr_aligned_used + (new_size - old_size);
            if (new_used <= current_pool->capacity) {
                // we make sure lives_bytes does **not** account for the
                // alignment
                current_pool->live_bytes += (new_size - old_size);

                current_pool->used = new_used;
                return src;
            }
        }
        else {
            current_pool->used -= (old_size - new_size);
            current_pool->live_bytes -= (old_size - new_size);
            cleanup_pool_if_needed(arena, current_pool);
            return src;
        }
    }
    else if (new_size < old_size) {
        if (src_base_ptr >= (uintptr_t)current_pool->base && src_end_ptr < pool_used_ptr) {
            current_pool->live_bytes -= (old_size - new_size);
            cleanup_pool_if_needed(arena, current_pool);
        }
        return src;
    }

    void *buffer = arena_malloc(arena, new_size);
    if (buffer != NULL) {
        memmove(buffer, src, old_size);
    }
    try_free(arena, src, old_size); // try to free the old buffer
    return buffer;
}

static inline bool
_try_free(Arena *arena, Pool *pool, void *src, const size_t curr_size)
{
    if (arena == NULL || pool == NULL || curr_size == 0 || curr_size > pool->live_bytes) {
        return false;
    }

    uintptr_t pool_used_ptr = (uintptr_t)((char *)pool->base + pool->used); // already aligned

    uintptr_t src_base_ptr = (uintptr_t)src;
    uintptr_t src_end_ptr = (uintptr_t)((char *)src + curr_size);

    if (pool_used_ptr == src_end_ptr) {
        pool->used -= curr_size;
        pool->live_bytes -= curr_size;
        cleanup_pool_if_needed(arena, pool);
        return true;
    }
    
    if (src_base_ptr >= (uintptr_t)pool->base && src_end_ptr < pool_used_ptr) {
        pool->live_bytes -= curr_size;
        cleanup_pool_if_needed(arena, pool);
        return true;
    }

    return false;
}


// attemtps to free the object in the **current** pool
bool
try_free_fast(Arena *arena, void *src, const size_t curr_size)
{
    return _try_free(arena, arena->current, src, curr_size);
}


// Attemtps to free the object in the all the pools in the arena
// not just the current one
// If you want to only free it in the current one
// use 'try_free_fast'
bool
try_free(Arena *arena, void *src, const size_t curr_size)
{
    Pool *pool = arena->head;
    while (pool) {
        if (_try_free(arena, pool, src, curr_size)) {
            return true;
        }
        pool = pool->next;
    }
    return false;
}

void
arena_print(Arena *arena)
{
    if (arena == NULL) {
        return;
    }
    size_t total_used = 0;
    size_t total_live_bytes = 0;
    Pool *curr = arena->head;
    while (curr) {
        total_used += curr->used;
        total_live_bytes += curr->live_bytes;
        curr = curr->next;
    }

    printf("<Arena, size=%zu, head=%p, current=%p, total_used=%zu, total_live_bytes=%zu>\n", arena->size, arena->head, arena->current, total_used, total_live_bytes);
}


void
pool_print(Pool *pool)
{
    if (pool == NULL) {
        return;
    }
    printf("<Pool, used=%zu, live_bytes=%zu capacity=%zu, base=%p, next=%p>\n", pool->used, pool->live_bytes, pool->capacity, pool->base, pool->next);
}