#ifndef QUEUE_H
#define QUEUE_H

#include "complex_generic_item.h"
#include "arena.h"

struct Queue {
    size_t size;
    size_t capacity;
    size_t front;
    GenericItem *array;
};

Queue *
init_queue(Arena *arena);

bool
append(Arena *arena, Queue *queue, const GenericItem item);

GenericItem *
pop(Queue *queue);

GenericItem *
peek_last(Queue *queue);

GenericItem *
peek_first(Queue *queue);

GenericItem *
queue_index(Queue *queue, const size_t index);

int
queue_compare(Queue *self, Queue *other);

void
queue_print(Queue *queue);

#endif