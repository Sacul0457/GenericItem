#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "arena.h"
#include "linked_list.h"

LinkedList *
linked_list_init(Arena *arena)
{
    LinkedList *list = arena_malloc(arena, sizeof(LinkedList));
    if (list == NULL) {
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}


static inline Node *
node_init(Arena *arena, GenericItem value)
{
    assert(arena != NULL);
    Node *new_node = arena_malloc(arena, sizeof(Node));
    if (new_node == NULL) {
        return false;
    }

    new_node->next = NULL;
    new_node->values[0] = value;
    new_node->size = 1;
    return new_node;
}

bool
linked_list_append_item(Arena *arena, LinkedList *list, GenericItem value)
{
    if (list == NULL) {
        return false;
    }


    if (list->size == 0) {
        Node *new_node = node_init(arena, value);
        if (new_node == NULL) {
            return false;
        }
        list->head = new_node;
        list->tail = new_node;
    }
    else {
        if (list->tail->size == NODE_VALUES_CAPACITY) {
            Node *new_node = node_init(arena, value);
            if (new_node == NULL) {
                return false;
            }
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

// this is called when the node is NOT full
static inline void
_move_node_values_and_insert(Node *node, size_t index, GenericItem value)
{
    assert(node->size != NODE_VALUES_CAPACITY);
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
// this is only called when the node is full!
static inline Node *
_split_node_and_insert(Arena *arena, Node *node, size_t index, GenericItem value)
{
    assert(arena && node && node->size == NODE_VALUES_CAPACITY);
    Node *new_node = arena_malloc(arena, sizeof(Node));
    if (new_node == NULL) {
        return NULL;
    }

    // the node is halved
    new_node->size = NODE_VALUES_CAPACITY_HALVED;
    node->size = NODE_VALUES_CAPACITY_HALVED;

    memmove(new_node->values, &node->values[NODE_VALUES_CAPACITY_HALVED], NODE_VALUES_CAPACITY_HALVED * sizeof(GenericItem));
    if (index >= NODE_VALUES_CAPACITY_HALVED) {
        _move_node_values_and_insert(new_node, index % NODE_VALUES_CAPACITY_HALVED, value);
    }
    else {
        _move_node_values_and_insert(node, index, value);
    }

    new_node->next = node->next;
    node->next = new_node;
    return new_node;
}


bool
linked_list_insert_item(Arena *arena, LinkedList *list, const size_t index, GenericItem value)
{
    if (list == NULL || index > list->size) {
        return false;
    }

    if (index == list->size) {
        return linked_list_append_item(arena, list, value);
    }

    Node *current = list->head;
    size_t value_arr_index = index;
    while (true) {
        if (value_arr_index < current->size) {
            break;
        }
        value_arr_index -= current->size;
        current = current->next;
    }
    
    // we do not need to handle updating the tail
    // since we already checked for that at (index == list->size)
    if (current->size == NODE_VALUES_CAPACITY) {
        Node *new_node = _split_node_and_insert(arena, current, value_arr_index, value);
        assert(new_node != NULL);
        if (current == list->tail) {
            list->tail = new_node;
        }
    }
    else {
        _move_node_values_and_insert(current, value_arr_index, value);
    }
    list->size++;
    return true;
}


static inline GenericItem
_pop_value_at_index(Node *node, const size_t index)
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
linked_list_remove_item(LinkedList *list, const GenericItem value)
{
    if (list == NULL || list->size == 0) {
        return false;
    }


    Node *current = list->head;
    Node *prev = NULL;
    while (current) {
        for (size_t j = 0; j < current->size; j++) {
            if (!item_compare(current->values[j], value)) {
                continue;
            }
            // also decrements node->size
            _pop_value_at_index(current, j);
            list->size--;
            if (current->size == 0) {
                if (current == list->head) {
                    list->head = current->next;
                }
                if (current == list->tail) {
                    list->tail = prev;
                }
                if (prev != NULL) {
                    prev->next = current->next;
                }
                current->next = NULL;
            }
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}


bool
linked_list_pop_at_index(LinkedList *list, const size_t index, GenericItem *ret)
{
    if (list == NULL || list->size == 0 || index >= list->size) {
        return false;
    }

    Node *current = list->head;
    Node *prev = NULL;

    size_t value_arr_index = index;
    while (true) {
        if (value_arr_index < current->size) {
            break;
        }
        prev = current;
        value_arr_index -= current->size;
        current = current->next;
    }

    GenericItem value = _pop_value_at_index(current, value_arr_index);
    if (ret != NULL) {
        *ret = value;
    }
    list->size--;
    if (current->size == 0) {
        if (current == list->head) {
            list->head = current->next;
        }
        if (current == list->tail) {
            list->tail = prev;
        }
        if (prev != NULL) {
            prev->next = current->next;
        }
        current->next = NULL;
    }
    return true;
}


// Returns the Node with this value
// And the sets the index of where this node is at using 'dest'
bool
linked_list_search(const LinkedList *list, const GenericItem value, size_t *dest)
{
    if (list == NULL || list->size == 0) {
        return false;
    }

    Node *current = list->head;
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
linked_list_index(const LinkedList *list, const size_t index, GenericItem *ret)
{
    if (list == NULL || index >= list->size || ret == NULL) {
        return false;
    }

    Node *current = list->head;
    size_t value_arr_index = index;
    while (true) {
        if (value_arr_index < current->size) {
            *ret = current->values[value_arr_index];
            return true;
        }
        value_arr_index -= current->size;
        current = current->next;
    }
    return false;
}


int
linked_list_compare(const LinkedList *self, const LinkedList *other)
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

    assert(self->head && other->head);

    Node *self_node = self->head;
    Node *other_node = other->head;

    while (self_node) {
        if (self_node->size != other_node->size) {
            return false;
        }

        for (size_t i = 0; i < self_node->size; i++) {
            if (!item_compare(self_node->values[i], other_node->values[i])) {
                return 0;
            }
        }
        self_node = self_node->next;
        other_node = other_node->next;
    }
    return 1;
}


void
linked_list_print(const LinkedList *list)
{
    if (list == NULL) {
        return;
    }

    assert(list->head != NULL && list->tail != NULL);

    Node *current = list->head;
    printf("LinkedList(");
    while (current) {
        for (size_t i = 0; i < current->size; i++) {
            print_item(current->values[i]); printf(" -> ");
        }
        current = current->next;
    }
    printf(")");
}