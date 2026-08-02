#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "arena.h"
#include "queue.h"

// MUST BE MULTIPLE OF 2
#define QUEUE_INITIAL_CAPACITY 8

Queue *
init_queue(Arena *arena)
{
    assert(QUEUE_INITIAL_CAPACITY % 2 == 0);
    Queue *queue = arena_malloc(arena, sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }

    GenericItem *tmp = arena_malloc(arena, sizeof(GenericItem) * QUEUE_INITIAL_CAPACITY);
    if (tmp == NULL) {
        try_free(arena, queue, sizeof(Queue));
        return NULL;
    }

    queue->size = 0;
    queue->capacity = QUEUE_INITIAL_CAPACITY;
    queue->front = 0;
    queue->array = tmp;
    return queue;
}


bool
queue_destroy(Arena *arena, Queue *queue)
{
    if (arena == NULL || queue == NULL) {
        return false;
    }

    try_free(arena, queue->array, sizeof(GenericItem) * queue->capacity);
    try_free(arena, queue, sizeof(Queue));
    return true;
}


static inline bool
resize_queue(Arena *arena, Queue *queue)
{
    if (queue == NULL) {
        return false;
    }

    size_t new_capacity = (queue->capacity == 0) ? 2 : queue->size * 2;
    assert(new_capacity % 2 == 0);
    GenericItem *tmp = arena_realloc(arena, queue->array, queue->capacity * sizeof(GenericItem), new_capacity * sizeof(GenericItem));
    if (tmp == NULL) {
        return false;
    }

    if (queue->front != 0) {
        memmove(tmp + queue->capacity, tmp, queue->front * sizeof(GenericItem));
    }

    queue->array = tmp;
    queue->capacity = new_capacity;
    return true;
}


bool
append(Arena *arena, Queue *queue, const GenericItem item)
{
    if (queue == NULL  || arena == NULL) {
        return false;
    }

    if (queue->size == queue->capacity) {
        if (!resize_queue(arena, queue)) {
            return false;
        }
    }

    size_t index = (queue->front + queue->size) & (queue->capacity -1);

    queue->array[index] = item;
    queue->size++;
    return true;
}


GenericItem *
pop(Queue *queue)
{
    if (queue == NULL || queue->size == 0) {
        return NULL; 
    }

    GenericItem *ret = &queue->array[queue->front];
    queue->front = (queue->front + 1) & (queue->capacity - 1);

    queue->size--;
    return ret;
}

GenericItem *
peek_last(Queue *queue)
{
    if (queue == NULL || queue->size == 0) {
        return NULL;
    }

    size_t index = (queue->front + queue->size -1) & (queue->capacity - 1);
    return &queue->array[index];
}


GenericItem *
peek_first(Queue *queue)
{
    if (queue == NULL || queue->size == 0) {
        return NULL;
    }

    return &queue->array[queue->front];
}


GenericItem *
queue_index(Queue *queue, const size_t index)
{
    if (queue == NULL || index >= queue->size) {
        return NULL;
    }
    
    return &queue->array[(queue->front + index) & (queue->capacity - 1)];
}

int
queue_compare(Queue *self, Queue *other)
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
    
    for (size_t i = self->front; i < self->front + self->size; i++) {
        GenericItem *self_item = queue_index(self, i);
        GenericItem *other_item = queue_index(other, i);

        assert(self_item && other_item);

        if (!item_compare(*self_item, *other_item)) {
            return 0;
        }
    }
    return true;
}

void
queue_print(Queue *queue)
{
    if (queue == NULL || queue->array == NULL) {
        return;
    }

    printf("(Queue[");
    for (size_t i = queue->front; i < queue->front + queue->size; i++) {
        print_item(queue->array[i % queue->capacity]); printf(", ");
    }
    printf("])");
}