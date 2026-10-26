#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"
BSTNode *bst_insert(BSTNode *root, const char *key, int symbol_index)
{
    int cmp;
    if (!root) {
        BSTNode *node = (BSTNode *)calloc(1, sizeof(*node));
        if (!node) return NULL;
        snprintf(node->key, sizeof(node->key), "%s", key);
        node->symbol_index = symbol_index;
        return node;
    }
    cmp = strcmp(key, root->key);
    if (cmp < 0) {
        BSTNode *child = bst_insert(root->left, key, symbol_index);
        if (child) root->left = child;
    } else if (cmp > 0) {
        BSTNode *child = bst_insert(root->right, key, symbol_index);
        if (child) root->right = child;
    }
    return root; /* duplicate keys keep first index */
}
int bst_find(const BSTNode *root, const char *key)
{
    while (root) {
        int cmp = strcmp(key, root->key);
        if (!cmp) return root->symbol_index;
        root = cmp < 0 ? root->left : root->right;
    }
    return -1;
}
void bst_free(BSTNode *root)
{
    if (!root) return;
    bst_free(root->left); bst_free(root->right); free(root);
}
