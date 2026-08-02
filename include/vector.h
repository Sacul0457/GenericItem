#ifndef VECTOR_H
#define VECTOR_H

#include "arena.h"
#include "complex_generic_item.h"

struct Vector {
    GenericItem *array;
    size_t size;
    size_t capacity;
};


Vector *
vector_init(Arena *arena);

Vector *
vector_init_with_count(Arena *arena, const size_t count);

bool
vector_destroy(Arena *arena, Vector *vector);

bool
resize_vector_array(Arena *arena, Vector *vector, const size_t new_count);

bool
vector_append_item(Arena *arena, Vector *vector, const GenericItem item);

bool
vector_pop_item(Arena *arena, Vector *vector, GenericItem *ret);

bool
vector_pop_item_at_index(Arena *arena, Vector *vector, const size_t index, bool keep_order, GenericItem *ret);

bool
vector_insert_item(Arena *arena, Vector *vector, const size_t index, const GenericItem item);

bool
vector_find_item(Vector *vector, const GenericItem item, size_t *ret);

GenericItem *
vector_slice(Arena *arena, Vector *vector, const size_t start, const size_t stop);

bool
vector_reverse(Vector *vector);

Vector *
vector_reverse_new(Arena *arena, const Vector *vector);

int vector_compare(const Vector *self, const Vector *other);

void
vector_print(const Vector *vector);

#endif