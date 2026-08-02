#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>

#include "arena.h"
#include "set.h"

size_t total_probe_blocks = 0;
size_t max_probe_blocks = 0;
size_t probe_blocks = 0;
size_t simd_insert_calls = 0;
size_t direct_inserts = 0;

#define UNREACHABLE                                           \
    do {                                                      \
        printf("UNREACHABLE REACHED at line: %d\n",__LINE__); \
        exit(-1);                                             \
    } while (0)

#define RESIZE_SET_THRESHOLD_WITH_TOMBSTONE(set) (((set)->size + (set)->tombstone) >= ((set)->capacity * 8) / 10)
#define RESIZE_SET_THRESHOLD(set) ((set)->size >= ((set)->capacity * 8) / 10)

#define SET_VALUES(set, index, key, fingerprint, hash)                          \
    do {                                                                        \
        (set)->keys[index] = (key);                                             \
        (set)->fingerprints[index] = (fingerprint);                             \
        if ((index) < 32) {                                                     \
            (set)->fingerprints[(set)->capacity + (index)] = (fingerprint);     \
        }                                                                       \
        (set)->hashes[index] = (hash);                                          \
    } while (0)


static inline bool
set_set_controls_to_empty(uint64_t *hashes, uint8_t *fingerprints, const size_t count)
{
    if (hashes == NULL || fingerprints == NULL) {
        return false;
    }

    memset(hashes, EMPTY, sizeof(uint64_t) * count);
    memset(fingerprints, EMPTY, sizeof(uint8_t) * (count + 32));

    return true;
}


#define SET_INITIAL_CAPACITY 32 // MUST BE 32

Set *
set_init(Arena *arena)
{
    Set *set = NULL;
    GenericItem *keys = NULL;
    
    uint8_t *fingerprints = NULL;
    uint64_t *hashes = NULL;


    set = arena_malloc(arena, sizeof(Set));
    if (set == NULL) {
        goto error;
    }

    keys = arena_malloc(arena, SET_INITIAL_CAPACITY * sizeof(GenericItem));
    if (keys == NULL) {
        goto error;
    }

    fingerprints = arena_malloc(arena, (SET_INITIAL_CAPACITY + 32) * sizeof(uint8_t));
    if (fingerprints == NULL) {
        goto error;
    }

    hashes = arena_malloc(arena, SET_INITIAL_CAPACITY * sizeof(uint64_t));
    if (hashes == NULL) {
        goto error;
    }

    if (!set_set_controls_to_empty(hashes, fingerprints, SET_INITIAL_CAPACITY)) {
        goto error;
    }

    set->size = 0;
    set->capacity = SET_INITIAL_CAPACITY;
    set->tombstone = 0;

    set->keys = keys;

    set->fingerprints = fingerprints;
    set->hashes = hashes;
    return set;

error:
    if (set != NULL) {
        try_free(arena, set, sizeof(Set));
    }

    if (keys != NULL) {
        try_free(arena, keys, sizeof(GenericItem) * SET_INITIAL_CAPACITY);
    }

    if (fingerprints != NULL) {
        try_free(arena, fingerprints, sizeof(uint8_t) * (SET_INITIAL_CAPACITY + 32));
    }

    if (hashes != NULL) {
        try_free(arena, hashes, sizeof(uint64_t) * SET_INITIAL_CAPACITY);
    }

    return NULL;
}

void
set_clear(Set *set)
{
    if (set == NULL) {
        return;
    }

    memset(set->fingerprints, EMPTY, sizeof(uint8_t) * (set->capacity + 32));
    memset(set->hashes, EMPTY, sizeof(uint64_t) * set->capacity);

    set->size = 0;
    set->tombstone = 0;
}


bool
set_resize(Arena *arena, Set *set, const size_t count)
{
    if (arena == NULL || set == NULL) {
        return false;
    }
    assert(count >= set->size && set->capacity >= 32);


    GenericItem *new_keys = NULL;
    
    uint8_t *new_fingerprints = NULL;
    uint64_t *new_hashes = NULL;

    new_keys = arena_malloc(arena, sizeof(GenericItem) * count);
    if (new_keys == NULL) {
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

    if (!set_set_controls_to_empty(new_hashes, new_fingerprints, count)) {
        goto error;
    }

    const size_t old_capacity = set->capacity;
    const size_t old_size = set->size;
    const GenericItem *old_keys = set->keys;
    const uint64_t *old_hashes = set->hashes;
    const uint8_t *old_fingerprints = set->fingerprints;

    // reset
    set->size = 0;
    set->tombstone = 0;
    set->capacity = count;
    set->fingerprints = new_fingerprints;
    set->hashes = new_hashes;
    set->keys = new_keys;


    // we maintain another counter so that we need not
    // loop through all the entries

    const __m256i simd_ts = _mm256_set1_epi8(TOMBSTONE);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);
    const __m256i all_bits_set = _mm256_set1_epi8(0xFF);

    for (size_t block = 0; block < old_capacity && set->size < old_size; block += 32) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&old_fingerprints[block]);

        const __m256i eq_empty = _mm256_cmpeq_epi8(fingerprints, simd_empty);
        const __m256i eq_tomb  = _mm256_cmpeq_epi8(fingerprints, simd_ts);

        const __m256i invalid = _mm256_or_si256(eq_empty, eq_tomb);
        const __m256i valid = _mm256_andnot_si256(invalid, all_bits_set);

        uint32_t mask = _mm256_movemask_epi8(valid);

        while (mask != 0) {
            int lane = __builtin_ctz(mask);
            const size_t old_index = (block + lane) & (old_capacity - 1);
        
            const size_t hash = old_hashes[old_index];
            size_t new_index = hash & (set->capacity - 1);
            while (set->fingerprints[new_index] != EMPTY) {
                new_index = (new_index + 1) & (set->capacity - 1);
            }

            set->fingerprints[new_index] = old_fingerprints[old_index];
            set->hashes[new_index] = hash;
            set->keys[new_index] = old_keys[old_index];

            set->size++;

            mask &= mask - 1;
        }
    }

    assert(set->size == old_size);
    return true;

error:
    if (new_keys != NULL) {
        try_free(arena, new_keys, sizeof(GenericItem) * count);
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
_vectorised_insert_item(Set *set, GenericItem key, uint64_t hash, const uint8_t key_fingerprint, const size_t start_index)
{
    bool first_deleted_slot = 0;
    size_t first_deleted_slot_index = 0;

    const size_t start_block = (start_index) & (set->capacity - 1);
    size_t block = start_block;

    const __m256i simd_fp = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_ts = _mm256_set1_epi8(TOMBSTONE);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);

    //probe_blocks = 0;

    while (true) {
        //probe_blocks++;
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&set->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fp);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            size_t index = (block + lane) & (set->capacity - 1);
            if (IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
                return 0;
            }
            match_mask &= match_mask - 1;
        }

        if (!first_deleted_slot) {
            const __m256i tombstone_cmp = _mm256_cmpeq_epi8(fingerprints, simd_ts);
            const uint32_t tombstone_mask = (uint32_t)_mm256_movemask_epi8(tombstone_cmp);
            if (tombstone_mask != 0 ) {
                int lane = __builtin_ctz(tombstone_mask);
                size_t index = (block + lane) & (set->capacity - 1);
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
                set->tombstone--;
            }
            else {
                int lane = __builtin_ctz(empty_mask);
                index = (block + lane) & (set->capacity - 1);
            }
            SET_VALUES(set, index, key, key_fingerprint, hash);

            set->size++;
            return 1;
        }

        block = (block + 32) & (set->capacity - 1);
        if (block == start_block) {
            return -1;
        }
    }

    UNREACHABLE;
}


int
set_add_item(Arena *arena, Set *set, GenericItem key)
{   
    if (set == NULL) {
        return -1;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return -1;
    }

    const size_t index = hash & (set->capacity - 1);
    const uint8_t fingerprint = set->fingerprints[index];

    uint8_t key_fp = FINGERPRINT(hash);
    if (fingerprint == EMPTY) {
        SET_VALUES(set, index, key, FINGERPRINT(hash), hash);
        //direct_inserts++;
        set->size++;
    }
    else if (key_fp == fingerprint && IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
        return 1;
    }
    else {
        //simd_insert_calls++;
        int err = _vectorised_insert_item(set, key, hash, key_fp, index + 1);
        //total_probe_blocks += probe_blocks;
        //if (probe_blocks > max_probe_blocks) {
        //    max_probe_blocks = probe_blocks;
        //}
        if (!err) {
            return 1;
        }
    }

    if (RESIZE_SET_THRESHOLD(set)) {
        set_resize(arena, set, set->capacity * 2);
    }
    else if (RESIZE_SET_THRESHOLD_WITH_TOMBSTONE(set)) {
        set_resize(arena, set, set->capacity);
    }

    return 1;
}


static inline bool
_vectorised_search_item(Set *set, const GenericItem key, const uint64_t hash, const uint8_t key_fingerprint, const size_t start_index, GenericItem *ret)
{
    assert(set != NULL);

    const size_t start_block = (start_index) & (set->capacity - 1);
    size_t block = start_block;

    const __m256i simd_fingerprint = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);
    while (true) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&set->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fingerprint);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            const size_t index = (block + lane) & (set->capacity - 1);
            if (IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
                if (ret != NULL) {
                    *ret = set->keys[index];
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

        block = (block + 32) & (set->capacity - 1);
        if (block == start_block) {
            return false;
        }
    }

    UNREACHABLE;
}

bool
set_get_item(Set *set, const GenericItem key, GenericItem *ret)
{
    if (set == NULL) {
        return false;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return NULL;
    }

    size_t index = hash & (set->capacity - 1);
    uint8_t fingerprint = set->fingerprints[index];
    if (fingerprint == EMPTY) {
        return false;
    }

    // this is the fingerprint of the key to pop
    // not necessarily the fingerprint that is stored in set->fingerprints
    uint8_t to_get_fp = FINGERPRINT(hash);
    if (fingerprint == to_get_fp && IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
        if (ret != NULL) {
            *ret = set->keys[index];
        }
        return true;
    }
    else {
        return _vectorised_search_item(set, key, hash, to_get_fp, index + 1, ret);
    }
}


static inline bool
_vectorised_pop_item(Set *set, const GenericItem key, const uint64_t hash, const uint8_t key_fingerprint, const size_t start_index, GenericItem *ret)
{
    assert(set != NULL);

    const size_t start_block = (start_index) & (set->capacity - 1);
    size_t block = start_block;

    const __m256i simd_fingerprint = _mm256_set1_epi8(key_fingerprint);
    const __m256i simd_empty = _mm256_set1_epi8(EMPTY);

    while (true) {
        const __m256i fingerprints = _mm256_loadu_si256((const __m256i *)&set->fingerprints[block]);

        const __m256i match_cmp = _mm256_cmpeq_epi8(fingerprints, simd_fingerprint);
        uint32_t match_mask = (uint32_t)_mm256_movemask_epi8(match_cmp);

        while (match_mask != 0) {
            int lane = __builtin_ctz(match_mask);
            const size_t index = (block + lane) & (set->capacity - 1);

            if (IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
                set->fingerprints[index] = TOMBSTONE;
                if (index < 32) {
                    set->fingerprints[set->capacity + index] = TOMBSTONE;
                }
                set->tombstone++;
                set->size--;
                
                if (ret != NULL) {
                    *ret = set->keys[index];
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

        block = (block + 32) & (set->capacity - 1);
        if (block == start_block) {
            return false;
        }
    }

    UNREACHABLE;
}


bool
pop_item(Arena *arena, Set *set, const GenericItem key, GenericItem *ret)
{
    if (set == NULL) {
        return false;
    }

    uint64_t hash;
    if (!get_hash(key, &hash)) {
        return false;
    }
    
    const size_t index = hash & (set->capacity - 1);
    const uint8_t fingerprint = set->fingerprints[index];
    if (fingerprint == EMPTY) {
        return false;
    }

    // this is the fingerprint of the key to pop
    // not necessarily the fingerprint that is stored in set->fingerprints
    uint8_t to_get_fp = FINGERPRINT(hash);

    bool err;
    if (to_get_fp == fingerprint && IS_MATCHING_KEY(key, set->keys[index], hash, set->hashes[index])) {
        set->fingerprints[index] = TOMBSTONE;
        if (index < 32) {
            set->fingerprints[set->capacity + index] = TOMBSTONE;
        }

        if (ret != NULL) {
            *ret = set->keys[index];
        }

        set->tombstone++;
        set->size--;
        err = true;
    }   
    else {
        err = _vectorised_pop_item(set, key, hash, to_get_fp, index + 1, ret);
    }

    if (set->tombstone >= (set->capacity * 0.8)) {
        set_resize(arena, set, set->capacity);
    }

    return err;
}


int
set_compare(Set *self, Set *other)
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

    for (size_t i = 0; i < self->capacity; i++) {
        uint8_t fingerprint = self->fingerprints[i];
        if (fingerprint == EMPTY || fingerprint == TOMBSTONE || 
            set_get_item(other, self->keys[i], NULL)) {
            continue;
        }
        return 0;
    }
    return 1;
}


void
set_print(Set *set)
{
    if (set == NULL) {
        return;
    }

    printf("{");
    for (size_t i = 0; i < set->capacity; i++) {
        uint8_t fingerprint = set->fingerprints[i];
        if (fingerprint == EMPTY || fingerprint == TOMBSTONE) {
            continue;
        }
        print_item(set->keys[i]); printf(", ");
    }
    printf("}");
}
