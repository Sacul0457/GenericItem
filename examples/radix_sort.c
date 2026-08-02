#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "arena.h"
#include "complex_generic_item.h"
#include "vector.h"

typedef struct {
    size_t size;
    Vector vectors[10];
} Bucket;


static inline Bucket *
bucket_init(Arena *arena)
{
    Bucket *bucket = arena_malloc(arena, sizeof(Bucket));
    if (bucket == NULL) {
        return NULL;
    }

    for (int i = 0; i < 10; i++){
        Vector *tmp = vector_init(arena);
        if (tmp == NULL) {
            arena_realloc(arena, bucket, sizeof(Bucket), 0);
            return NULL;
        }
        bucket->vectors[i] = *tmp;
    }
    bucket->size = 10;
    return bucket;
}


static inline bool
destroy_bucket(Arena *arena, Bucket *bucket)
{
    if (arena == NULL || bucket == NULL) {
        return false;
    }

    for (size_t i = 0; i < bucket->size; i++) {
        Vector *vector = &bucket->vectors[i];
        if (!vector_destroy(arena, vector)) {
            return false;
        }
    }
    return true;
}


static inline void
bucket_print(Bucket *bucket)
{
    if (bucket == NULL) {
        return;
    }
    printf("\n");
    for (size_t i = 0; i < bucket->size; i++) {
        vector_print(&bucket->vectors[i]);
    }
    printf("\n");
}


static inline bool
max(Vector *int_array, int *ret)
{
    if (int_array == NULL || int_array->size == 0 || ret == NULL) {
        return false;
    }

    GenericItem item = int_array->array[0];
    if (item.tp != INTEGER) {
        return false;
    }
    int largest = item.data.integer;
    for (size_t i = 1; i < int_array->size; i++) {
        GenericItem curr_item = int_array->array[i];
        if (curr_item.data.integer > largest) {
            largest = curr_item.data.integer;
        }
    }
    
    *ret = largest;
    return true;
}


#define INT_TO_STRING(num, buffer) (snprintf((buffer), sizeof((buffer)), "%d", (num)))

Vector *
radix_sort(Arena *arena, Vector *array)
{
    if (arena == NULL || array == NULL) {
        return NULL;
    }

    Bucket *bucket = bucket_init(arena);
    if (bucket == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < array->size; i++) {
        GenericItem item = array->array[i];
        assert(item != NULL && item.tp == INTEGER);

        int num = item.data.integer;
        Vector *vector = &bucket->vectors[num % 10];
        vector_append_item(arena, vector, item);
    }

    size_t place = 10;
    int k;
    if (!max(array, &k)) {
        return NULL;
    }
    char buffer[32];
    INT_TO_STRING(k, buffer);
    size_t n = strlen(buffer);

    Bucket *new_bucket;
    while (true) {
        new_bucket = bucket_init(arena);
        for (size_t i = 0; i < bucket->size; i++) {
            Vector *vector = &bucket->vectors[i];
            for (size_t j = 0; j < vector->size; j++) {
                GenericItem item = vector->array[j];
                assert(item != NULL && item.tp == INTEGER);
                Vector *curr_vector = &new_bucket->vectors[(item.data.integer / place) % 10];
                vector_append_item(arena, curr_vector, item);
            }
        }
        try_free(arena, bucket, sizeof(Bucket));
        bucket = new_bucket;
        place *= 10;
        if (n == 0) {
            break;
        }
        n -=1;
    }

    Vector *ret = vector_init(arena);
    for (size_t i = 0; i < new_bucket->size; i++) {
        Vector *curr_vector = &new_bucket->vectors[i];
        for (size_t j = 0; j < curr_vector->size; j++) {
            vector_append_item(arena, ret, curr_vector->array[j]);
        }
    }
    return ret;
}

static inline int 
random_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int main()
{
    Arena *arena = arena_init();
    if (arena == NULL) {
        return -1;
    }

    Vector *array = vector_init(arena);
    if (array == NULL) {
        goto cleanup;
    }

    srand((unsigned int)time(NULL));

    for (size_t i = 0; i < 30; i++) {
        int num = random_int(0, 100);
        vector_insert_item(arena, array, i, convert_item(num));
    }

    printf("Before: "); vector_print(array); printf("\n");
    printf("After: "); vector_print(radix_sort(arena, array));

cleanup:
    arena_destroy(arena);
    return 0;
}