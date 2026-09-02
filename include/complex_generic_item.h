#ifndef COMPLEX_GENERIC_ITEM
#define COMPLEX_GENERIC_ITEM

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

typedef struct Vector Vector;
typedef struct HashMap HashMap;
typedef struct Set Set;
typedef struct LinkedList LinkedList;
typedef struct Queue Queue;
typedef struct Node Node;
typedef struct MapEntry MapEntry;
typedef struct DLinkedList DLinkedList;
typedef struct DNode DNode;
typedef struct BST BST;
typedef struct BSTNode BSTNode;


typedef struct GenericItem GenericItem;


typedef enum {
    INTEGER = 0,
    BOOLEAN_TYPE = 1,
    CHARACTER = 2,
    FLOAT_TYPE = 3,
    DOUBLE_TYPE = 4,
    STRING = 5,
    POINTER = 6,

    // custom structs
    VECTOR = 7,
    MAP = 8,
    SET = 9,
    LINKED_LIST = 10,
    QUEUE = 11,
    DB_LINKED_LIST = 12,
} DataType;

struct GenericItem {
    union {
        char character;
        int integer;
        bool boolean;
        float ft;
        double db;
        char *string;
        void *pointer;

        // custom structs
        // these will all be pointers

        Vector *vector;
        HashMap *map;
        Set *set;
        LinkedList *l_list;
        Queue *queue;
        DLinkedList *db_list;
        BST *tree;
    } data;
    DataType tp;
};

static inline GenericItem
convert_int(const int value)
{
    return (GenericItem){.data.integer=value, .tp=INTEGER};
}

static inline GenericItem
convert_bool(const bool value)
{
    return (GenericItem){.data.boolean=value, .tp=BOOLEAN_TYPE};
}

static inline GenericItem
convert_char(const char value)
{
    return (GenericItem){.data.character=value, .tp=CHARACTER};
}

static inline GenericItem
convert_float(const float value)
{
    return (GenericItem){.data.ft=value, .tp=FLOAT_TYPE};
}

static inline GenericItem
convert_double(const double value)
{
    return (GenericItem){.data.db=value, .tp=DOUBLE_TYPE};
}

static inline GenericItem
convert_string(char *value)
{
    return (GenericItem){.data.string=value, .tp=STRING};
}

static inline GenericItem
convert_pointer(void *value)
{
    return (GenericItem){.data.pointer=value, .tp=POINTER};
}

static inline GenericItem
convert_vector(Vector *value)
{
    return (GenericItem){.data.vector=value, .tp=VECTOR};
}

static inline GenericItem
convert_hashmap(HashMap *value)
{
    return (GenericItem){.data.map=value, .tp=MAP};
}

static inline GenericItem
convert_set(Set *value)
{
    return (GenericItem){.data.set=value, .tp=SET};
}

static inline GenericItem
convert_linked_list(LinkedList *value)
{
    return (GenericItem){.data.l_list=value, .tp=LINKED_LIST};
}

static inline GenericItem
convert_queue(Queue *value)
{
    return (GenericItem){.data.queue=value, .tp=QUEUE};
}

static inline GenericItem
convert_db_linked_list(DLinkedList *value)
{
    return (GenericItem){.data.db_list=value, .tp=DB_LINKED_LIST};
}

// macro for easy conversion
#define convert_item(item) (_Generic((item), \
    int: convert_int,                        \
    bool: convert_bool,                      \
    char: convert_char,                      \
    float: convert_float,                    \
    double: convert_double,                  \
    char *: convert_string,                  \
    void *: convert_pointer,                 \
    Vector *: convert_vector,                \
    HashMap *: convert_hashmap,              \
    Set *: convert_set,                      \
    LinkedList *: convert_linked_list,       \
    Queue *: convert_queue,                  \
    DLinkedList *: convert_db_linked_list    \
)(item))



// ================================
//          HASH FUNCTIONS
// ================================

static inline uint64_t
hash_int(const GenericItem item)
{
    uint64_t x = (uint64_t)item.data.integer;
    x *= 0x9E3779B97F4A7C15ULL;
    return x;
}

static inline uint64_t
hash_char(const GenericItem item)
{
    return (unsigned char)item.data.character;
}

static inline uint64_t
hash_float_double(const GenericItem item)
{
    uint32_t bits;
    if (item.tp == FLOAT_TYPE) {
        memcpy(&bits, &item.data.ft, sizeof(bits));
    }
    else {
        memcpy(&bits, &item.data.db, sizeof(bits));
    }
    return (uint64_t)bits;
}

static inline uint64_t
hash_string(const GenericItem item)
{
    uint64_t hash = 1469598103934665603ULL; // offset basis
    
    const char *string = item.data.string;
    while (*string) {
        hash ^= (unsigned char)(*string++);
        hash *= 1099511628211ULL; // FNV prime
    }

    return hash;
}

static inline uint64_t
hash_pointer(const GenericItem item)
{
    return (uint64_t)(uintptr_t)item.data.pointer;
}

static inline bool
get_hash(const GenericItem item, uint64_t *hash)
{
    if (hash == NULL) {
        return false;
    }
    uint64_t res;
    switch (item.tp) {
        case INTEGER:
        case BOOLEAN_TYPE:
            res =  hash_int(item);
            break;

        case CHARACTER:
            res = hash_char(item);
            break;

        case FLOAT_TYPE:
        case DOUBLE_TYPE:
            res = hash_float_double(item);
            break;

        case STRING:
            res = hash_string(item);
            break;

        case POINTER:
            res = hash_pointer(item);
            break;

        case VECTOR:
        case MAP:
        case SET:
        case LINKED_LIST:
        case QUEUE:
        case DB_LINKED_LIST:
            return false;
        
        default:
            printf("Unknown type\n");
            return false;
    }
    *hash = res;
    return true;
}

int item_compare(const GenericItem item, const GenericItem other);

void print_item(const GenericItem item);


#endif