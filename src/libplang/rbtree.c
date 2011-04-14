/*
 * plang logic programming language
 * Copyright (C) 2011  Southern Storm Software, Pty Ltd.
 *
 * The plang package is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * The plang package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libcompiler library.  If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "rbtree-priv.h"
#include <string.h>

/* This file implements a red-black tree data structure that
 * maps type/name/arity references to a term value.  The algorithm
 * details are based on the description at:
 *
 *     http://en.wikipedia.org/wiki/Red-black_tree
 *
 * A full working implementation of the snippets from that Wikipedia
 * article can be found here:
 *
 *     http://en.literateprograms.org/Red-black_tree_(C)
 *
 * The key difference between our implementation and theirs is that
 * we unroll the implementation into a loop rather than rely upon
 * the compiler's support for tail recursion to optimize the algorithm.
 */

/* Convert a term into a key suitable for red-black tree lookup.
 * Returns zero if the term cannot be used as a key */
int _p_rbkey_init(p_rbkey *key, const p_term *term)
{
    term = p_term_deref(term);
    if (!term)
        return 0;
    key->type = term->header.type;
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        key->size = term->header.size;
        key->name = term->functor.functor_name;
        break;
    case P_TERM_LIST: {
        /* Index lists on the head as well.  This is useful for
         * indexing DCG terminal rules such as [a|T], [b|T], etc */
        const p_term *head = p_term_deref(term->list.head);
        if (head && head->header.type != P_TERM_LIST) {
            if (_p_rbkey_init(key, term->list.head)) {
                key->type |= P_TERM_LIST_OF;
            } else {
                key->type = P_TERM_LIST;
                key->size = 0;
                key->name = 0;
            }
        } else {
            key->size = 0;
            key->name = 0;
        }
        break; }
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_REAL:
        key->size = 0;
        key->name = term;
        break;
    case P_TERM_INTEGER:
#if defined(P_TERM_64BIT)
        key->size = term->header.size;
        key->name = 0;
#else
        key->size = 0;
        key->name = term;
#endif
        break;
    default: return 0;
    }
    return 1;
}

/* Compares a key against a red-black tree node */
P_INLINE int _p_rbkey_compare(const p_rbkey *key, const p_rbnode *node)
{
    if (key->type < node->type)
        return -1;
    else if (key->type > node->type)
        return 1;
    switch (key->type) {
    case P_TERM_FUNCTOR:
    case P_TERM_FUNCTOR | P_TERM_LIST_OF:
        if (key->size < node->size)
            return -1;
        else if (key->size > node->size)
            return 1;
        /* Fall through to the next case */
    case P_TERM_ATOM:
    case P_TERM_ATOM | P_TERM_LIST_OF:
        if (key->name < node->name)
            return -1;
        else if (key->name > node->name)
            return 1;
        break;
    case P_TERM_STRING:
    case P_TERM_STRING | P_TERM_LIST_OF:
        return p_term_strcmp(key->name, node->name);
    case P_TERM_REAL:
    case P_TERM_REAL | P_TERM_LIST_OF:
        if (key->name->real.value < node->name->real.value)
            return -1;
        else if (key->name->real.value > node->name->real.value)
            return 1;
        break;
    case P_TERM_INTEGER:
    case P_TERM_INTEGER | P_TERM_LIST_OF:
#if defined(P_TERM_64BIT)
        if (((int)(key->size)) < ((int)(node->size)))
            return -1;
        else if (((int)(key->size)) > ((int)(node->size)))
            return 1;
#else
        if (key->name->integer.value < node->name->integer.value)
            return -1;
        else if (key->name->integer.value > node->name->integer.value)
            return 1;
#endif
        break;
    default: break;
    }
    return 0;
}

/* Export key comparison, for use by the unit tests */
int _p_rbkey_compare_keys(const p_rbkey *key1, const p_rbkey *key2)
{
    p_rbnode node;
    memset(&node, 0, sizeof(node));
    node.type = key2->type;
    node.size = key2->size;
    node.name = key2->name;
    return _p_rbkey_compare(key1, &node);
}

/* Initialize a red-black tree structure */
void _p_rbtree_init(p_rbtree *tree)
{
    tree->root = 0;
}

/* Free a red-black tree structure */
static void _p_rbnode_free(p_rbnode *node)
{
    if (node) {
        _p_rbnode_free(node->left);
        _p_rbnode_free(node->right);
        GC_FREE(node);
    }
}
void _p_rbtree_free(p_rbtree *tree)
{
    _p_rbnode_free(tree->root);
    tree->root = 0;
}

/* Perform a lookup on a red-black tree.  Returns null if not found.
 * Otherwise returns a pointer to the node */
p_rbnode *_p_rbtree_lookup(const p_rbtree *tree, const p_rbkey *key)
{
    const p_rbnode *node = tree->root;
    int cmp;
    while (node != 0) {
        cmp = _p_rbkey_compare(key, node);
        if (cmp == 0)
            return (p_rbnode *)node;
        else if (cmp < 0)
            node = node->left;
        else
            node = node->right;
    }
    return 0;
}

/* Replace one node with another */
P_INLINE void _p_rbtree_replace_node
    (p_rbtree *tree, p_rbnode *node, p_rbnode *replace)
{
    p_rbnode *parent = node->parent;
    if (!parent)
        tree->root = replace;
    else if (parent->left == node)
        parent->left = replace;
    else
        parent->right = replace;
    if (replace)
        replace->parent = parent;
}

/* Rotate a red-black tree left around a specific node */
P_INLINE void _p_rbtree_rotate_left(p_rbtree *tree, p_rbnode *node)
{
    p_rbnode *right = node->right;
    _p_rbtree_replace_node(tree, node, right);
    node->right = right->left;
    if (right->left)
        right->left->parent = node;
    right->left = node;
    node->parent = right;
}

/* Rotate a red-black tree right around a specific node */
P_INLINE void _p_rbtree_rotate_right(p_rbtree *tree, p_rbnode *node)
{
    p_rbnode *left = node->left;
    _p_rbtree_replace_node(tree, node, left);
    node->left = left->right;
    if (left->right)
        left->right->parent = node;
    left->right = node;
    node->parent = left;
}

/* Inserts a key into a red-black tree.  Returns a pointer to the node.
 * If the key does not exist in the tree, then a new node is created.
 * Otherwise a pointer to the previous node is returned */
p_rbnode *_p_rbtree_insert(p_rbtree *tree, const p_rbkey *key)
{
    p_rbnode *node = tree->root;
    p_rbnode *parent;
    p_rbnode *grand_parent;
    p_rbnode *uncle;
    p_rbnode *orig_node;
    int cmp;

    /* If the root is null, then create the first node and return */
    if (!node) {
        node = GC_NEW(p_rbnode);
        if (!node)
            return 0;
        node->type = key->type;
        node->red = 0;              /* Root node must be black */
        node->size = key->size;
        node->name = key->name;
        tree->root = node;
        return node;
    }

    /* Search for an existing node, or the best insertion point */
    parent = 0;
    grand_parent = 0;
    do {
        cmp = _p_rbkey_compare(key, node);
        if (cmp == 0)
            return node;            /* Found an existing node */
        grand_parent = parent;
        parent = node;
        if (cmp < 0)
            node = node->left;
        else
            node = node->right;
    } while (node != 0);

    /* Add a new node to the tree */
    orig_node = node = GC_NEW(p_rbnode);
    if (!node)
        return 0;
    node->type = key->type;
    node->red = 1;                  /* New nodes start as red */
    node->size = key->size;
    node->name = key->name;
    node->parent = parent;
    if (cmp < 0)
        parent->left = node;
    else
        parent->right = node;

    for (;;) {
        /* If the parent is black, then we're done */
        if (!parent->red)
            return orig_node;

        /* Find the uncle node */
        if (grand_parent) {
            if (parent == grand_parent->left)
                uncle = grand_parent->right;
            else
                uncle = grand_parent->left;
        } else {
            uncle = 0;
        }

        /* If the uncle is red, then repaint the parent and uncle
         * black, and the grand-parent red.  We then move up the tree
         * to repaint or rotate around the grand-parent node */
        if (uncle && uncle->red) {
            parent->red = 0;
            uncle->red = 0;
            grand_parent->red = 1;
            node = grand_parent;
            parent = node->parent;
            if (!parent) {
                /* At the root - repaint it black and stop */
                node->red = 0;
                return orig_node;
            }
            grand_parent = parent->parent;
        } else {
            break;
        }
    }

    /* Perform rotations to rebalance the tree */
    if (node == parent->right && parent == grand_parent->left) {
        _p_rbtree_rotate_left(tree, parent);
        node = parent;
        parent = node->parent;
    } else if (node == parent->left && parent == grand_parent->right) {
        _p_rbtree_rotate_right(tree, parent);
        node = parent;
        parent = node->parent;
    }
    parent->red = 0;
    grand_parent->red = 1;
    if (tree->root == grand_parent)
        tree->root = parent;
    if (node == parent->left && parent == grand_parent->left)
        _p_rbtree_rotate_right(tree, grand_parent);
    else
        _p_rbtree_rotate_left(tree, grand_parent);

    /* Return the pointer to the original node's value */
    return orig_node;
}

/* Safe way to ask if a node's color is red, even if it is null */
#define _p_rbnode_is_red(node) ((node) ? (node)->red : 0)

/* Rebalance the "tree" to account for the removal of "node" */
static void _p_rbtree_rebalance(p_rbtree *tree, p_rbnode *node)
{
    p_rbnode *parent;
    p_rbnode *sibling;

    for (;;) {
        /* Nothing to do if we're at the root */
        parent = node->parent;
        if (!parent)
            return;

        /* If our sibling is red, then rotate about the parent
         * to bring the sibling up into the parent's position */
        if (parent->left == node)
            sibling = parent->right;
        else
            sibling = parent->left;
        if (sibling && sibling->red) {
            parent->red = 1;
            sibling->red = 0;
            if (node == parent->left)
                _p_rbtree_rotate_left(tree, parent);
            else
                _p_rbtree_rotate_right(tree, parent);
            parent = node->parent;
            if (parent->left == node)
                sibling = parent->right;
            else
                sibling = parent->left;
        }

        /* If our parent, sibling, and sibling's children are all black
         * then paint the sibling red and repeat with the parent */
        if (!parent->red && !_p_rbnode_is_red(sibling) &&
                !_p_rbnode_is_red(sibling->left) &&
                !_p_rbnode_is_red(sibling->right)) {
            sibling->red = 1;
            node = parent;
        } else {
            break;
        }
    }

    /* If our parent is red, but our sibling and sibling's children
     * are black, then swap the parent and sibling colors and stop */
    if (parent->red && !_p_rbnode_is_red(sibling) &&
            !_p_rbnode_is_red(sibling->left) &&
            !_p_rbnode_is_red(sibling->right)) {
        sibling->red = 1;
        parent->red = 0;
        return;
    }

    /* Perform the final rotations */
    if (node == parent->left) {
        if (!_p_rbnode_is_red(sibling) &&
                _p_rbnode_is_red(sibling->left) &&
                !_p_rbnode_is_red(sibling->right)) {
            sibling->red = 1;
            sibling->left->red = 0;
            _p_rbtree_rotate_right(tree, sibling);
        }
    } else {
        if (!_p_rbnode_is_red(sibling) &&
                !_p_rbnode_is_red(sibling->left) &&
                _p_rbnode_is_red(sibling->right)) {
            sibling->red = 1;
            sibling->right->red = 0;
            _p_rbtree_rotate_left(tree, sibling);
        }
    }
    parent = node->parent;
    if (node == parent->left)
        sibling = parent->right;
    else
        sibling = parent->left;
    sibling->red = parent->red;
    parent->red = 0;
    if (node == parent->left) {
        sibling->right->red = 0;
        _p_rbtree_rotate_left(tree, parent);
    } else {
        sibling->left->red = 0;
        _p_rbtree_rotate_right(tree, parent);
    }
}

/* Removes a key and its value from a red-black tree.  Returns
 * the value that was associated with the key, or null if the
 * key is not present in the tree */
p_term *_p_rbtree_remove(p_rbtree *tree, const p_rbkey *key)
{
    p_rbnode *node = tree->root;
    p_rbnode *child;
    p_term *value;
    int cmp;

    /* Locate the key within the tree */
    while (node != 0) {
        cmp = _p_rbkey_compare(key, node);
        if (cmp == 0)
            break;
        else if (cmp < 0)
            node = node->left;
        else
            node = node->right;
    }
    if (!node)
        return 0;
    value = node->value;

    /* If the node has two non-null children, then copy the maximum
     * value in the left sub-tree into this node.  We keep the same
     * red/black state in the node so that the tree stays sound.
     * Then we delete the maximum left child node instead */
    if (node->left && node->right) {
        child = node->left;
        while (child->right != 0)
            child = child->right;
        node->type = child->type;
        node->size = child->size;
        node->name = child->name;
        node->value = child->value;
        node = child;
    }

    /* The node now has either zero or one children, never two */
    if (node->left)
        child = node->left;
    else
        child = node->right;
    if (!node->red) {
        node->red = child ? child->red : 0;
        _p_rbtree_rebalance(tree, node);
    }

    /* Replace the node with its child and then color it
     * black if it is at the root */
    _p_rbtree_replace_node(tree, node, child);
    if (child && child == tree->root)
        child->red = 0;
    GC_FREE(node);

    /* Node has been removed - we're done */
    return value;
}

/* Visit all of the nodes in the tree.  The "last" parameter should
 * be initialized to NULL to start the search.  Each time the next
 * node in preorder is returned.  The function returns NULL once the
 * tree has been exhausted */
p_rbnode *_p_rbtree_visit_all(p_rbtree *tree, p_rbnode *last)
{
    p_rbnode *parent;

    /* Handle the easy cases first */
    if (!last)
        return tree->root;
    else if (last->left)
        return last->left;
    else if (last->right)
        return last->right;

    /* Search up the tree for the next node to the right of this one */
    parent = last->parent;
    while (parent != 0) {
        if (parent->left == last && parent->right)
            return parent->right;
        last = parent;
        parent = parent->parent;
    }
    return 0;
}
