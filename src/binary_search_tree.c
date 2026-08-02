#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>


#include "arena.h"
#include "binary_search_tree.h"


BST *
bst_init(Arena *arena, BST_KEYTYPE type)
{
    if (arena == NULL) {
        return NULL;
    }

    BST *tree = arena_malloc(arena, sizeof(BST));
    if (tree == NULL) {
        return NULL;
    }

    tree->size = 0;
    tree->root = NULL;
    tree->type = type;
    return tree;
}

bool
bst_clear(BST *tree)
{
    if (tree == NULL) {
        return false;
    }

    tree->size = 0;
    tree->root = NULL;
    return true;
}

static inline int
_bst_cmp_key(BST_KEYTYPE type, BSTKey self, BSTKey other)
{

    if (type == BST_INT) {
        if (self.integer > other.integer) {
            return 1;
        }
        else if (self.integer < other.integer) {
            return -1;
        }
        else {
            return 0;
        }
    }
    else {
        return strcmp(self.string, other.string);
    }
}


int
bst_insert_item(Arena *arena, BST *tree, BSTKey key, GenericItem value)
{
    if (arena == NULL || tree == NULL) {
        return -1;
    }

    BSTNode *new_node = arena_malloc(arena, sizeof(BSTNode));
    if (new_node == NULL) {
        return -1;
    }

    new_node->key = key;
    new_node->value = value;
    new_node->left = NULL;
    new_node->right = NULL;

    if (tree->root == NULL) {
        tree->root = new_node;
    }
    else {
        BSTNode *parent = NULL;
        BSTNode *node = tree->root;
        while (node) {
            parent = node;
            int cmp = _bst_cmp_key(tree->type, key, node->key);
            if (cmp > 0) {
                node = node->right;
            }
            else if (cmp < 0) {
                node = node->left;
            }
            else {
                try_free(arena, new_node, sizeof(BSTNode));
                return 0;
            }
        }
        assert(parent != NULL);

        int cmp = _bst_cmp_key(tree->type, key, parent->key);
        if (cmp > 0) {
            parent->right = new_node;
        }
        else {
            parent->left = new_node;
        }
    }

    tree->size++;
    return true;
}

bool
bst_search_item(BST *tree, BSTKey key, GenericItem *ret)
{
    if (tree == NULL || tree->size == 0) {
        return false;
    }

    BSTNode *current = tree->root;
    while (current) {
        int cmp = _bst_cmp_key(tree->type, key, current->key);
        if (cmp > 0) {
            current = current->right;
        }
        else if (cmp < 0) {
            current = current->left;
        }
        else {
            if (ret != NULL) {
                *ret = current->value;
            }
            return true;
        }
    }
    return false;
}


static inline void
replace_child(
    BST *tree,
    BSTNode *parent,
    BSTNode *child,
    BSTNode *replacement)
{
    if (parent == NULL)
        tree->root = replacement;
    else if (parent->left == child)
        parent->left = replacement;
    else
        parent->right = replacement;
}


bool
bst_pop_item(BST *tree, BSTKey key, GenericItem *ret)
{
    if (tree == NULL || tree->size == 0) {
        return false;
    }

    BSTNode *parent = NULL;
    BSTNode *node = tree->root;
    while (node) {
        int cmp = _bst_cmp_key(tree->type, key, node->key);
        if (cmp > 0) {
            parent = node;
            node = node->right;
        }
        else if (cmp < 0) {
            parent = node;
            node = node->left;
        }
        else {
            break;
        }
    }
    if (node == NULL) {
        // not found
        return false;
    }

    if (node->left == NULL && node->right == NULL) {
        replace_child(tree, parent, node, NULL);
    }   
    else if (node->right == NULL) {
        replace_child(tree, parent, node, node->left);
    }
    else if (node->left == NULL) {
        replace_child(tree, parent, node, node->right);
    }
    else {
        BSTNode *successor = node->right;
        BSTNode *parent_successor = node;
        while (successor->left != NULL) {
            parent_successor = successor;
            successor = successor->left;
        }

        if (parent_successor == node) {
            successor->left = node->left;
        }
        else {  
            parent_successor->left = successor->right;
            successor->left = node->left;
            successor->right = node->right;
        }
        
        replace_child(tree, parent, node, successor);
    }

    if (ret != NULL) {
        *ret = node->value;
    }
    tree->size--;
    return true;
}

#define _BST_PRINT_KEY(key, type)           \
    do {                                    \
        if ((type) == BST_INT) {            \
            printf("%d ", (key).integer);   \
        }                                   \
        else {                              \
            printf("%s ", (key).string);    \
        }                                   \
    } while (0)


#define _BST_PRINT_KEY_FMT(key, type, depth)                      \
    do {                                                          \
        if ((type) == BST_INT) {                                  \
            printf("%*s%d\n", (depth) * 4, "", (key).integer);    \
        }                                                         \
        else {                                                    \
            printf("%*s%s\n", (depth) * 4, "", (key).string);     \
        }                                                         \
    } while (0)


static inline void 
_bst_print_inorder(BSTNode *node, BST_KEYTYPE type) {
    if (node == NULL) return;

    _bst_print_inorder(node->left, type);
    _BST_PRINT_KEY(node->key, type);
    _bst_print_inorder(node->right, type);
}

void
bst_print_inorder(BST *tree)
{
    if (tree == NULL) {
        return;
    }
    _bst_print_inorder(tree->root, tree->type);
}

static inline void
_bst_print_fmt(BSTNode *node, BST_KEYTYPE type, int depth) {
    if (node == NULL) return;

    _bst_print_fmt(node->right, type, depth + 1);

    _BST_PRINT_KEY_FMT(node->key, type, depth);

    _bst_print_fmt(node->left, type, depth + 1);
}

void
bst_print_fmt(BST *tree)
{
    if (tree == NULL) {
        return;
    }
    _bst_print_fmt(tree->root, tree->type, 0);
}