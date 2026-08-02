#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include "arena.h"
#include "vector.h"

#define VECTOR_INITIAL_CAPACITY 8

Vector *
vector_init(Arena *arena)
{
    Vector *vector = arena_malloc(arena, sizeof(Vector));
    if (vector == NULL) {
        return NULL;
    }

    GenericItem *tmp = arena_malloc(arena, VECTOR_INITIAL_CAPACITY * sizeof(GenericItem));
    if (tmp == NULL) {
        try_free(arena, vector, sizeof(Vector));
        return NULL;
    }

    vector->array = tmp;
    vector->size = 0;
    vector->capacity = VECTOR_INITIAL_CAPACITY;
    return vector;
}

Vector *
vector_init_with_count(Arena *arena, const size_t count)
{
    Vector *vector = arena_malloc(arena, sizeof(Vector));
    if (vector == NULL) {
        return NULL;
    }

    GenericItem *tmp = arena_malloc(arena, count * sizeof(GenericItem));
    if (tmp == NULL) {
        try_free(arena, vector, sizeof(Vector));
        return NULL;
    }

    vector->array = tmp;
    vector->size = 0;
    vector->capacity = count;
    return vector;
}

bool
vector_destroy(Arena *arena, Vector *vector)
{
    if (arena == NULL || vector == NULL) {
        return false;
    }

    assert(vector->array != NULL);

    try_free(arena, vector->array, sizeof(GenericItem) * vector->capacity);
    try_free(arena, vector, sizeof(Vector));

    return true;
}

bool
resize_vector_array(Arena *arena, Vector *vector, const size_t new_count) 
{
    if (vector == NULL) {
        return false;
    }

    const size_t new_capacity = new_count == 0 ? 2 : new_count;

    GenericItem *tmp = arena_realloc(arena, vector->array, vector->capacity * sizeof(GenericItem), new_capacity * sizeof(GenericItem));
    if (tmp == NULL) {
        return false;
    }
    vector->capacity = new_capacity;
    vector->array = tmp;
    return true;
}

// if the array size is 3 times less than the array capacity, we reduce the capacity 
#define REDUCE_ARRAY_CAPACITY_THRESHOLD 3

static inline bool
reduce_vector_capacity_if_threshold(Arena *arena, Vector *vector)
{
    if (vector == NULL || vector->array == NULL) {
        return false;
    }

    if (vector->size * REDUCE_ARRAY_CAPACITY_THRESHOLD < vector->capacity) {
        return resize_vector_array(arena, vector, vector->size * 2);
    }
    return false;
}

bool
vector_append_item(Arena *arena, Vector *vector, const GenericItem item)
{
    if (vector == NULL) {
        return false;
    }

    if (vector->size == vector->capacity) {
        if (!resize_vector_array(arena, vector, vector->capacity * 2)) {
            return false;
        }
    }

    vector->array[vector->size] = item;
    vector->size++;
    return true;
}

bool
vector_pop_item(Arena *arena, Vector *vector, GenericItem *ret)
{
    if (vector == NULL || vector->array == NULL || vector->size == 0) {
        return false;
    }

    if (ret != NULL) {
        *ret = vector->array[vector->size - 1];
    }
    vector->size--;
    reduce_vector_capacity_if_threshold(arena, vector);
    return true;
}


bool
vector_pop_item_at_index(Arena *arena, Vector *vector, const size_t index, bool keep_order, GenericItem *ret)
{
    if (vector == NULL || vector->array == NULL || index >= vector->size) {
        return false;
    }

    if (ret != NULL) {
        GenericItem to_assign = vector->array[index];
        *ret = to_assign;
    }

    if (!keep_order) {
        vector->array[index] = vector->array[vector->size - 1];
    }
    else {
        memmove(&vector->array[index], &vector->array[index + 1], sizeof(GenericItem) * (vector->size - index - 1));
    }
    vector->size--;
    reduce_vector_capacity_if_threshold(arena, vector);
    return true;
}


bool
vector_insert_item(Arena *arena, Vector *vector, const size_t index, const GenericItem item)
{
    if (vector == NULL || vector->array == NULL || index > vector->size) {
        return false;
    }

    if (index == vector->size) {
        return vector_append_item(arena, vector, item);
    }

    if (vector->size == vector->capacity) {
        if (!resize_vector_array(arena, vector, vector->capacity * 2)) {
            return false;
        }
    }

    memmove(&vector->array[index + 1], &vector->array[index], sizeof(GenericItem) * (vector->size - index));
    vector->array[index] = item;
    vector->size++;
    return true;
}


bool
vector_find_item(Vector *vector, const GenericItem item, size_t *ret)
{
    if (vector == NULL) {
        return false;
    }

    for (size_t i = 0; i < vector->size; i++) {
        const GenericItem current_item = vector->array[i];
        if (item_compare(current_item, item)) {
            if (ret != NULL) {
                *ret = i;
            }
            return true;
        }
    }
    return false;
}


GenericItem *
vector_slice(Arena *arena, Vector *vector, const size_t start, const size_t stop)
{
    if (vector == NULL || start >= stop || stop > vector->size) {
        return NULL;
    }

    size_t raw_size = (stop - start) * sizeof(GenericItem);

    GenericItem *buffer = arena_malloc(arena, (stop - start) * sizeof(GenericItem));
    if (buffer == NULL) {
        return NULL;
    }

    GenericItem *array_start = vector->array + start;
    memcpy(buffer, array_start, raw_size);
    return buffer;
}

bool
vector_reverse(Vector *vector)
{
    if (vector == NULL || vector->size <= 1) {
        return false;
    }
    
    size_t j = vector->size - 1;
    for (size_t i = 0; i < j; i++, j--) {
        GenericItem j_value = vector->array[j];
        vector->array[j] = vector->array[i];
        vector->array[i] = j_value;
    }
    return true;
}


Vector *
vector_reverse_new(Arena *arena, const Vector *vector)
{
    if (vector == NULL) {
        return NULL;
    }

    Vector *new_vector = vector_init_with_count(arena, vector->size);
    if (new_vector == NULL) {
        return NULL;
    }
    
    size_t vector_size = vector->size;
    for (size_t i = 0; i < vector_size; i++) {
        new_vector->array[i] = vector->array[vector_size - i - 1];
    }
    new_vector->size = vector->size;
    return new_vector;
}


// -1 for error
// 0 and 1 for false and true respectively
int
vector_compare(const Vector *self, const Vector *other)
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

    for (size_t i = 0; i < self->size; i++) {
        GenericItem self_item = self->array[i];
        GenericItem other_item = other->array[i];
        if (!item_compare(self_item, other_item)) {
            return 0;
        }
    }
    return 1;
}


// note that if the vector holds a reference to itself (in anyway)
// this will print infinitely!!!
void
vector_print(const Vector *vector)
{
    if (vector == NULL) {
        return;
    }

    printf("[");
    for (size_t i = 0; i < vector->size; i++) {
        print_item(vector->array[i]); printf(", ");
    }
    printf("]");
}