#ifndef BST_H
#define BST_H

#include "complex_generic_item.h"
#include "arena.h"


typedef enum {
    BST_STRING = 1,
    BST_INT = 2
} BST_KEYTYPE;


typedef union {
    const char *string;
    int integer;
} BSTKey;

struct BSTNode {
    BSTKey key;
    GenericItem value;
    struct BSTNode *left;
    struct BSTNode *right;
};

struct BST {
    size_t size;
    BSTNode *root;
    BST_KEYTYPE type;
};


static inline BSTKey
bst_key_int(int x)
{
    return (BSTKey){ .integer = x };
}

static inline BSTKey
bst_key_str(const char *s)
{
    return (BSTKey){ .string = s };
}

#define BST_SET_KEY(item) _Generic((item),  \
    int: bst_key_int,                       \
    char *: bst_key_str,                    \
    const char *: bst_key_str               \
)(item)


BST *
bst_init(Arena *arena, BST_KEYTYPE type);

int
bst_insert_item(Arena *arena, BST *tree, BSTKey key, GenericItem value);

bool
bst_search_item(BST *tree, BSTKey key, GenericItem *ret);

bool
bst_pop_item(BST *tree, BSTKey key, GenericItem *ret);


void
bst_print_inorder(BST *tree);

void
bst_print_fmt(BST *tree);


#endif