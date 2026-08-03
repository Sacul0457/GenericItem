#include "stdio.h"
#include "stddef.h"
#include "stdlib.h"
#include "mem.h"

#include "complex_generic_item.h"
#include "arena.h"
#include "db_linked_list.h"


DLinkedList *
dlinked_list_init(Arena *arena)
{
    if (arena == NULL) {
        return NULL;
    }

    DLinkedList *list = arena_malloc(arena, sizeof(DLinkedList));
    if (list == NULL) {
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

static inline DNode *
node_init(Arena *arena, GenericItem value)
{
    assert(arena != NULL);
    DNode *new_node = arena_malloc(arena, sizeof(DNode));
    if (new_node == NULL) {
        return false;
    }

    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->values[0] = value;
    new_node->size = 1;
    return new_node;
}


// this is called when the node is NOT full
static inline void
_move_node_values_and_insert(DNode *node, size_t index, GenericItem value)
{
    assert(node->size != DNODE_VALUES_CAPACITY);
    if (index == node->size) {
        node->values[index] = value;
    }
    else {
        memmove(&node->values[index + 1], &node->values[index], (node->size - index) * sizeof(GenericItem));
        node->values[index] = value;
    }
    node->size++;
}


// returns the newly created node
// this is only called when a node is full!
static inline DNode *
_split_node_and_insert(Arena *arena, DNode *node, size_t index, GenericItem value)
{
    assert(arena && node && node->size == DNODE_VALUES_CAPACITY);
    DNode *new_node = arena_malloc(arena, sizeof(DNode));
    if (new_node == NULL) {
        return NULL;
    }

    // the node is halved
    new_node->size = DNODE_VALUES_CAPACITY_HALVED;
    node->size = DNODE_VALUES_CAPACITY_HALVED;

    memmove(new_node->values, &node->values[DNODE_VALUES_CAPACITY_HALVED], DNODE_VALUES_CAPACITY_HALVED * sizeof(GenericItem));
    if (index >= DNODE_VALUES_CAPACITY_HALVED) {
        _move_node_values_and_insert(new_node, index % DNODE_VALUES_CAPACITY_HALVED, value);
    }
    else {
        _move_node_values_and_insert(node, index, value);
    }

    new_node->next = node->next;
    new_node->prev = node;
    node->next = new_node;
    return new_node;
}



// returns the node inserted
bool
dlinked_list_append_right(Arena *arena, DLinkedList *list, GenericItem value)
{
    if (arena == NULL || list == NULL) {
        return false;
    }

    if (list->size == 0) {
        DNode *new_node = node_init(arena, value);
        if (new_node == NULL) {
            return false;
        }
        new_node->prev = NULL;
        list->head = new_node;
        list->tail = new_node;
    }
    else {
        if (list->tail->size == DNODE_VALUES_CAPACITY) {
            DNode *new_node = node_init(arena, value);
            if (new_node == NULL) {
                return false;
            }
            new_node->prev = list->tail;
            list->tail->next = new_node;
            list->tail = new_node;
        }
        else {
            list->tail->values[list->tail->size] = value;
            list->tail->size++;
        }
    }
    list->size++;
    return true;
}

// returns the Node inserted
bool
dlinked_list_append_left(Arena *arena, DLinkedList *list, GenericItem value)
{
    if (arena == NULL || list == NULL) {
        return false;
    }

    if (list->size == 0) {
        DNode *new_node = node_init(arena, value);
        if (new_node == NULL) {
            return false;
        }
        new_node->prev = NULL;
        list->head = new_node;
        list->tail = new_node;
    }
    else {
        if (list->head->size == DNODE_VALUES_CAPACITY) {
            assert(_split_node_and_insert(arena, list->head, 0, value) != NULL);
        }
        else {  
            _move_node_values_and_insert(list->head, 0, value);
        }
    }
    list->size++;
    return true;
}

static inline GenericItem
_pop_value_at_index(DNode *node, const size_t index)
{
    assert(node != NULL);
    GenericItem ret = node->values[index];
    if (index != node->size) {
        memmove(&node->values[index], &node->values[index + 1], (node->size - index - 1) * sizeof(GenericItem));
    }
    node->size--;
    return ret;
}


bool
dlinked_list_pop_right(DLinkedList *list, GenericItem *ret)
{
    if (list == NULL || list->size == 0) {
        return false;
    }

    assert(list->head && list->tail);


    GenericItem ret_value;
    if (list->size == 1) {
        ret_value = list->head->values[0];
        list->head = NULL;
        list->tail = NULL;
    }
    else if (list->tail->size == 1) {
        ret_value = list->tail->values[0];
        list->tail = list->tail->prev;
        list->tail->next = NULL;
    }
    else {
        ret_value = list->tail->values[list->tail->size - 1];
        list->tail->size--;
    }

    if (ret != NULL) {
        *ret = ret_value;
    }

    list->size--;

    return ret;
}

bool
dlinked_list_pop_left(DLinkedList *list, GenericItem *ret)
{
    if (list == NULL || list->size == 0) {
        return NULL;
    }

    assert(list->head && list->tail);


    GenericItem ret_value;
    if (list->size == 1) {
        ret_value = list->head->values[0];
        list->head = NULL;
        list->tail = NULL;
    }
    else if (list->head->size == 1) {
        // we know that there's at least 2 nodes in the list
        // see the first check
        ret_value = list->head->values[0];
        list->head->next->prev = NULL;
        list->head = list->head->next;
    }
    else {
        ret_value = _pop_value_at_index(list->head, 0);
    }

    if (ret != NULL) {
        *ret = ret_value;
    }

    list->size--;

    return ret;
}


// Returns the Node with this value
// And the sets the index of where this node is at using 'dest'
bool
dlinked_list_search(const DLinkedList *list, const GenericItem value, size_t *dest)
{
    if (list == NULL || list->size == 0) {
        return false;
    }

    DNode *current = list->head;
    size_t total_index = 0;
    while (current) {
        for (size_t i = 0; i < current->size; i++) {
            if (item_compare(current->values[i], value)) {
                if (dest != NULL) {
                    *dest = total_index;
                }
                return true;
            }
            total_index += 1;
        }
        current = current->next;
    }

    return false;
}



bool
dlinked_list_index(const DLinkedList *list, const size_t index, GenericItem *ret)
{
    if (list == NULL || index >= list->size || ret == NULL) {
        return false;
    }

    const size_t mid = list->size / 2;
    if (index > mid) {
        size_t value_arr_index = list->size -1 - index;
        DNode *current = list->tail;
        while (true) {
            if (value_arr_index < current->size) {
                *ret = current->values[current->size - 1 - value_arr_index];
                return true;
            }
            value_arr_index -= current->size;
            current = current->prev;
        }
    }
    else {
        size_t value_arr_index = index;
        DNode *current = list->head;
        while (true) {
            if (value_arr_index < current->size) {
                *ret = current->values[value_arr_index];
                return true;
            }
            value_arr_index -= current->size;
            current = current->next;
        }
    }
    return false;
}


int
db_list_compare(const DLinkedList *self, const DLinkedList *other)
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

    DNode *self_node = self->head;
    DNode *other_node = other->head;
    while (self_node) {
        if (self_node->size != other_node->size) {
            return 0;
        }

        for (size_t i = 0; i < self_node->size; i++) {
            if (!item_compare(self_node->values[i], other_node->values[i])) {
                return 0;
            }
        }
        self_node = self_node->next;
        other_node = other_node->next;
    }
    return 0;
}


void
dlinked_list_print(const DLinkedList *list)
{
    if (list == NULL) {
        return;
    }

    assert(list->head != NULL && list->tail != NULL);

    DNode *current = list->head;
    printf("DLinkedList(");
    while (current) {
        for (size_t i = 0; i < current->size; i++) {
            print_item(current->values[i]); printf(" -> ");
        }
        current = current->next;
    }
    printf(")");
}
