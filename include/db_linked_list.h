#ifndef DB_LINKED_LIST_H
#define DB_LINKED_LIST_H

#include "complex_generic_item.h"
#include "arena.h"

#define DNODE_VALUES_CAPACITY 32
#define DNODE_VALUES_CAPACITY_HALVED 16


struct DNode {
    GenericItem values[DNODE_VALUES_CAPACITY];
    size_t size;
    struct DNode *next;
    struct DNode *prev;
};

struct DLinkedList{
    struct DNode *head;
    struct DNode *tail;
    size_t size;
};


DLinkedList *
dlinked_list_init(Arena *arena);

bool 
dlinked_list_append_right(Arena *arena, DLinkedList *list, GenericItem value);

bool
dlinked_list_append_left(Arena *arena, DLinkedList *list, GenericItem value);

bool
dlinked_list_pop_right(DLinkedList *lis, GenericItem *ret);

bool
dlinked_list_pop_left(DLinkedList *list, GenericItem *ret);

bool
dlinked_list_search(const DLinkedList *list, const GenericItem value, size_t *dest);

bool
dlinked_list_index(const DLinkedList *list, size_t index, GenericItem *ret);

int
db_list_compare(const DLinkedList *self, const DLinkedList *other);

void
dlinked_list_print(const DLinkedList *list);

#endif