#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>

#include "arena.h"
#include "hashmap.h"


#define MAP_INITIAL_CAPACITY 32
#define RESIZE_MAP_THRESHOLD(map) (((map)->size >= ((map)->capacity * 7) / 8))
#define RESIZE_MAP_THRESHOLD_WITH_TOMBSTONE(map) (((map)->size + (map)->tombstone) >= ((map)->capacity * 7) / 8)


#define UNREACHABLE                                           \
    do {                                                      \
        printf("UNREACHABLE REACHED at line: %d\n",__LINE__); \
        exit(-1);                                             \
    } while (0)

#define SET_VALUES(map, index, key, value, fingerprint, hash)                   \
    do {                                                                        \
        (map)->keys[index] = (key);                                             \
        (map)->values[index] = (value);                                         \
        (map)->fingerprints[index] = (fingerprint);                             \
        if ((index) < 32) {                                                     \
            (map)->fingerprints[(map)->capacity + (index)] = (fingerprint);     \
        }                                                                       \
        (map)->hashes[index] = (hash);                                          \
    } while (0)


static inline void
map_set_controls_to_empty(uint64_t *hashes, uint8_t *fingerprints, size_t count)
{
    assert(hashes && fingerprints);

    memset(hashes, EMPTY, sizeof(uint64_t) * count);
    memset(fingerprints, EMPTY, sizeof(uint8_t) * (count + 32));
    return;
}

// Helper Functions

HashMap *
map_init(Arena *arena)
{
    HashMap *map = NULL;
    GenericItem *keys = NULL;
    GenericItem *values = NULL;
    
    uint8_t *fingerprints = NULL;
    uint64_t *hashes = NULL;


    map = arena_malloc(arena, sizeof(HashMap));
    if (map == NULL) {
        goto error;
    }

    keys = arena_malloc(arena, MAP_INITIAL_CAPACITY * sizeof(GenericItem));
    if (keys == NULL) {
        goto error;
    }

    values = arena_malloc(arena, MAP_INITIAL_CAPACITY * sizeof(GenericItem));
    if (keys == NULL) {
        goto error;
    }


    fingerprints = arena_malloc(arena, (MAP_INITIAL_CAPACITY + 32) * sizeof(uint8_t));
    if (fingerprints == NULL) {
        goto error;
    }

    hashes = arena_malloc(arena, MAP_INITIAL_CAPACITY * sizeof(uint64_t));
    if (hashes == NULL) {
        goto error;
    }

    map_set_controls_to_empty(hashes, fingerprints, MAP_INITIAL_CAPACITY);

    map->size = 0;
    map->capacity = MAP_INITIAL_CAPACITY;
    map->tombstone = 0;

    map->keys = keys;
    map->values = values;

    map->fingerprints = fingerprints;
    map->hashes = hashes;
    return map;

error:
    if (map != NULL) {
        try_free(arena, map, sizeof(HashMap));
    }

    if (keys != NULL) {
        try_free(arena, keys, sizeof(GenericItem) * MAP_INITIAL_CAPACITY);
    }

    if (values != NULL) {
        try_free(arena, values, sizeof(GenericItem) * MAP_INITIAL_CAPACITY);
    }

    if (fingerprints != NULL) {
        try_free(arena, fingerprints, sizeof(uint8_t) * (MAP_INITIAL_CAPACITY + 32));
    }

    if (hashes != NULL) {
        try_free(arena, hashes, sizeof(uint64_t) * MAP_INITIAL_CAPACITY);
    }

    return NULL;
}


bool
map_clear(HashMap *map)
{
    if (map == NULL) {
        return false;
    }

    // set hashes and fingerprints to EMPTY
    map_set_controls_to_empty(map->hashes, map->fingerprints, map->capacity);

    map->size = 0;
    map->tombstone = 0;
    return true;
}


static inline void
_vectorised_resize_insert_item(HashMap *map, GenericItem key, GenericItem value,
                        uint64_t hash, const uint8_t key_fingerprint)
{

    const size_t start_block = (hash & (map->capacity - 1)) & ~(size_t)31;
    size_t block = start_block;

    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);

    while (true) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&map->fingerprints[block]);

        const __m256i empty_cmp = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const uint32_t empty_mask = (uint32_t)_mm256_movemask_epi8(empty_cmp);
        if (empty_mask != 0) {

            int lane = __builtin_ctz(empty_mask);
            size_t index = (block + lane) & (map->capacity - 1);
            SET_VALUES(map, index, key, value, key_fingerprint, hash);

            map->size++;
            return;
        }

        block = (block + 32) & (map->capacity - 1);

        assert(block != start_block);
    }

    UNREACHABLE;
}



bool
map_resize(Arena *arena, HashMap *map, const size_t count)
{
    if (arena == NULL || map == NULL) {
        return false;
    }
    assert(count >= map->size && map->capacity >= 32);


    GenericItem *new_keys = NULL;
    GenericItem *new_values = NULL;
    
    uint8_t *new_fingerprints = NULL;
    uint64_t *new_hashes = NULL;

    new_keys = arena_malloc(arena, sizeof(GenericItem) * count);
    if (new_keys == NULL) {
        return false;
    }
    
    new_values = arena_malloc(arena, sizeof(GenericItem) * count);
    if (new_values == NULL) {
        return false;
    }

    new_fingerprints = arena_malloc(arena, sizeof(uint8_t) * (count + 32));
    if (new_fingerprints == NULL) {
        goto error;
    }

    new_hashes = arena_malloc(arena, sizeof(uint64_t) * count);
    if (new_hashes == NULL) {
        goto error;
    }

    map_set_controls_to_empty(new_hashes, new_fingerprints, count);

    // we maintain another counter so that we need not
    // loop through all the entries

    HashMap temp_map = {
        .capacity = count,
        .size = 0,
        .tombstone = 0,
        .fingerprints = new_fingerprints,
        .hashes = new_hashes,
        .keys = new_keys,
        .values = new_values
    };


    size_t block = 0;
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);
    const __m256i simd_ts = _mm256_set1_epi8(TOMBSTONE);

    while (temp_map.size < map->size) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&map->fingerprints[block]);

        const __m256i empty_cmp = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const __m256i ts_cmp = _mm256_cmpeq_epi8(fingerprints, simd_ts);

        const __m256i has_either = _mm256_or_si256(empty_cmp, ts_cmp);
        const __m256i neither = _mm256_xor_si256(has_either, _mm256_set1_epi8(-1));
        uint32_t neither_mask = (uint32_t)_mm256_movemask_epi8(neither);
        while (neither_mask != 0) {
            int lane = __builtin_ctz(neither_mask);
            const size_t index = (block + lane) & (map->capacity - 1);
            _vectorised_resize_insert_item(&temp_map, 
                                            map->keys[index],
                                            map->values[index],
                                            map->hashes[index],
                                            map->fingerprints[index]);
            neither_mask &= neither_mask - 1;
        }

        block = (block + 32) & (map->capacity - 1);
    }

    assert(temp_map.size == map->size);

    map->hashes = new_hashes;
    map->fingerprints = new_fingerprints;

    map->keys = new_keys;
    map->values = new_values;

    map->tombstone = 0;
    map->size = temp_map.size;
    map->capacity = count;
    return true;

error:
    if (new_keys != NULL) {
        try_free(arena, new_keys, sizeof(GenericItem) * count);
    }

    if (new_values != NULL) {
        try_free(arena, new_values, sizeof(GenericItem) * count);
    }

    if (new_fingerprints != NULL) {
        try_free(arena, new_fingerprints, sizeof(uint8_t) * (count + 32));
    }

    if (new_hashes != NULL) {
        try_free(arena, new_hashes, sizeof(uint64_t) * count);
    }

    return false;  
}


static inline int
_vectorised_insert_item(HashMap *map, GenericItem key, GenericItem value,
                        uint64_t hash, const uint8_t key_fingerprint, const size_t start_index)
{
    bool first_deleted_slot = 0;
    size_t first_deleted_slot_index = 0;

    const size_t start_block = start_index & ~(size_t)31;
    size_t block = start_block;

    const __m256i simd_fp = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_ts = _mm256_set1_epi8(TOMBSTONE);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);

    //probe_blocks = 0;

    while (true) {
        //probe_blocks++;
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&map->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fp);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            size_t index = (block + lane) & (map->capacity - 1);
            if (IS_MATCHING_KEY(key, map->keys[index], hash, map->hashes[index])) {
                return 0;
            }
            match_mask &= match_mask - 1;
        }

        if (!first_deleted_slot) {
            const __m256i tombstone_cmp = _mm256_cmpeq_epi8(fingerprints, simd_ts);
            const uint32_t tombstone_mask = (uint32_t)_mm256_movemask_epi8(tombstone_cmp);
            if (tombstone_mask != 0 ) {
                int lane = __builtin_ctz(tombstone_mask);
                size_t index = (block + lane) & (map->capacity - 1);
                first_deleted_slot = true;
                first_deleted_slot_index = index;
            }
        }

        const __m256i empty_cmp = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const uint32_t empty_mask = (uint32_t)_mm256_movemask_epi8(empty_cmp);
        if (empty_mask != 0) {
            size_t index;
            if (first_deleted_slot) {
                index = first_deleted_slot_index;
                map->tombstone--;
            }
            else {
                int lane = __builtin_ctz(empty_mask);
                index = (block + lane) & (map->capacity - 1);
            }
            SET_VALUES(map, index, key, value, key_fingerprint, hash);

            map->size++;
            return 1;
        }

        block = (block + 32) & (map->capacity - 1);
        if (block == start_block) {
            UNREACHABLE;
        }
    }
    UNREACHABLE;
}


int
map_insert_item(Arena *arena, HashMap *map, GenericItem key, GenericItem value)
{   
    if (map == NULL) {
        return -1;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return -1;
    }

    const size_t index = hash & (map->capacity - 1);
    int err = _vectorised_insert_item(map, key, value, hash, FINGERPRINT(hash), index);

    if (!err) {
        return err;
    }

    if (RESIZE_MAP_THRESHOLD(map)) {
        map_resize(arena, map, map->capacity * 2);
    }
    else if (RESIZE_MAP_THRESHOLD_WITH_TOMBSTONE(map)) {
        map_resize(arena, map, map->capacity);
    }

    return 1;
}


static inline bool
_vectorised_search_item(HashMap *map, const GenericItem key, const uint64_t hash, const uint8_t key_fingerprint, const size_t start_index, GenericItem *ret)
{
    assert(map != NULL);

    const size_t start_block = start_index & ~(size_t)31;
    size_t block = start_block;

    const __m256i simd_fingerprint = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);
    while (true) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&map->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fingerprint);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            const size_t index = (block + lane) & (map->capacity - 1);
            if (IS_MATCHING_KEY(key, map->keys[index], hash, map->hashes[index])) {
                if (ret != NULL) {
                    *ret = map->values[index];
                }
                return true;
            }
            match_mask &= match_mask - 1;
        }
        
        // if it's empty, we return immediately
        const __m256i empty_cmp = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const uint32_t empty_mask = (uint32_t)_mm256_movemask_epi8(empty_cmp);
        if (empty_mask != 0) {
            return false;
        }

        block = (block + 32) & (map->capacity - 1);
        if (block == start_block) {
            return false;
        }
    }

    UNREACHABLE;
}


bool
map_get_item(HashMap *map, const GenericItem key, GenericItem *ret)
{
    if (map == NULL) {
        return false;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return false;
    }

    const size_t index = hash & (map->capacity - 1);
    return _vectorised_search_item(map, key, hash, FINGERPRINT(hash), index, ret);
}


static inline bool
_vectorised_pop_item(HashMap *map, const GenericItem key, const uint64_t hash, const uint8_t key_fingerprint, const size_t start_index, GenericItem *ret)
{
    assert(map != NULL);

    const size_t start_block = start_index & ~(size_t)31;
    size_t block = start_block;

    const __m256i simd_fingerprint = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);

    while (true) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&map->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fingerprint);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            const size_t index = (block + lane) & (map->capacity - 1);

            if (IS_MATCHING_KEY(key, map->keys[index], hash, map->hashes[index])) {
                map->fingerprints[index] = TOMBSTONE;
                if (index < 32) {
                    map->fingerprints[map->capacity + index] = TOMBSTONE;
                }
                map->tombstone++;
                map->size--;
                
                if (ret != NULL) {
                    *ret = map->values[index];
                }
                return true;
            }
            match_mask &= match_mask - 1;
        }
        
        // if it's empty, we return immediately
        const __m256i empty_cmp = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const uint32_t empty_mask = (uint32_t)_mm256_movemask_epi8(empty_cmp);
        if (empty_mask != 0) {
            return false;
        }

        block = (block + 32) & (map->capacity - 1);
        if (block == start_block) {
            return false;
        }
    }

    UNREACHABLE;
}


bool
map_pop_item(Arena *arena, HashMap *map, const GenericItem key, GenericItem *ret)
{
    if (arena == NULL || map == NULL) {
        return false;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return false;
    }
    
    const size_t index = hash & (map->capacity - 1);
    bool err = _vectorised_pop_item(map, key, hash, FINGERPRINT(hash), index, ret);

    if (map->tombstone * 5 >= map->capacity * 4) {
        map_resize(arena, map, map->capacity);
    }

    return err;
}


int
map_compare(HashMap *self, HashMap *other)
{
    if (self == NULL || other == NULL) {
        return -1;
    }
    if (self == other) {
        return 1;
    }

    if (self->size != other->size) {
        return 0;
    }

    for (size_t i = 0, j = 0; j < self->capacity; i++) {
        uint8_t fingerprint = self->fingerprints[i];
        if (fingerprint == EMPTY || fingerprint == TOMBSTONE) {
            continue;
        }
        // we know the key is not NULL
        if (!map_get_item(other, self->keys[i], NULL)) {
            return 0;
        }
        j++;
    }
    return 1;
}


void
map_print(HashMap *map)
{
    if (map == NULL) {
        return;
    }

    printf("{");
    for (size_t i = 0, j = 0; j < (map->size - map->tombstone); i++) {
        uint8_t fingerprint = map->fingerprints[i];
        if (fingerprint == EMPTY || fingerprint == TOMBSTONE) {
            continue;
        }
        print_item(map->keys[i]); printf(": "); print_item(map->values[i]); printf(", ");
        j++;
    }
    printf("}");
}