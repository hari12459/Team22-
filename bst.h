#ifndef SOURCESCOPE_BST_H
#define SOURCESCOPE_BST_H
#include "common.h"
typedef struct BSTNode {
    char key[SS_MAX_NAME];
    int symbol_index;
    struct BSTNode *left, *right;
} BSTNode;
BSTNode *bst_insert(BSTNode *root, const char *key, int symbol_index);
int bst_find(const BSTNode *root, const char *key);
void bst_free(BSTNode *root);
#endif
