#ifndef SET_H
#define SET_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "arena.h"
#include "hash.h"
#include "complex_generic_item.h"


struct Set {
    size_t size;
    size_t capacity;
    size_t tombstone;

    uint8_t *fingerprints; // first 7 bits of the hash
    uint64_t *hashes;

    GenericItem *keys;
};

extern size_t total_probe_blocks;
extern size_t max_probe_blocks;
extern size_t probe_blocks;
extern size_t simd_insert_calls;
extern size_t direct_inserts;

// Api Functions

Set *
set_init(Arena *arena);

void
set_clear(Set *set);

bool
set_resize(Arena *arena, Set *set, size_t count);

int
set_add_item(Arena *arena, Set *set, GenericItem key);

bool
set_get_item(Set *set, const GenericItem key, GenericItem *ret);

bool
pop_item(Arena *arena, Set *set, const GenericItem key, GenericItem *ret);

int
set_compare(Set *self, Set *other);

void
set_print(Set *set);

#endif