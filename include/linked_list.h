#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "arena.h"
#include "complex_generic_item.h"


#define NODE_VALUES_CAPACITY 32
#define NODE_VALUES_CAPACITY_HALVED 16

struct Node {
    struct Node *next;
    GenericItem values[NODE_VALUES_CAPACITY];
    size_t size;
};

struct LinkedList {
    size_t size;
    struct Node *head;
    struct Node *tail;
};


LinkedList *
linked_list_init(Arena *arena);

bool
linked_list_append_item(Arena *arena, LinkedList *list, GenericItem value);

bool
linked_list_insert_item(Arena *arena, LinkedList *list, const size_t index, GenericItem value);

bool
linked_list_remove_item(LinkedList *list, const GenericItem value);

bool
linked_list_pop_at_index(LinkedList *list, const size_t index, GenericItem *ret);

bool
linked_list_search(const LinkedList *list, const GenericItem value, size_t *dest);

bool
linked_list_index(const LinkedList *list, const size_t index, GenericItem *ret);

int
linked_list_compare(const LinkedList *self, const LinkedList *other);

void
linked_list_print(const LinkedList *list);

#endif