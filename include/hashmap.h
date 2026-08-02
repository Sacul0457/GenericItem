#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>


#include "complex_generic_item.h"
#include "arena.h"
#include "hash.h"



struct HashMap {
    size_t size;
    size_t capacity;
    size_t tombstone;

    uint8_t *fingerprints;
    uint64_t *hashes;

    GenericItem *keys;
    GenericItem *values;
};

// Api Functions


HashMap *
map_init(Arena *arena);

bool
map_clear(HashMap *map);


bool
map_resize(Arena *arena, HashMap *map, size_t count);

int
map_insert_item(Arena *arena, HashMap *map, GenericItem key, GenericItem value);

bool
map_get_item(HashMap *map, const GenericItem key, GenericItem *ret);

bool
map_pop_item(Arena *arena, HashMap *map, const GenericItem key, GenericItem *ret);

int
map_compare(HashMap *self, HashMap *other);

void
map_print(HashMap *map);


#endif