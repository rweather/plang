/*
 * plang logic programming language
 * Copyright (C) 2011,2012  Southern Storm Software, Pty Ltd.
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

#ifndef PLANG_RBTREE_PRIV_H
#define PLANG_RBTREE_PRIV_H

#include "term-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

struct p_rbnode
{
#if defined(P_TERM_64BIT)
    unsigned int type : 31;
    unsigned int red  : 1;
    unsigned int size;
#else
    unsigned int type : 7;
    unsigned int red  : 1;
    unsigned int size : 24;
#endif
    const p_term *name;
    union {
        p_term *value;
        struct p_term_clause_list clauses;
    };
    p_rbnode *parent;
    p_rbnode *left;
    p_rbnode *right;
};

typedef struct p_rbkey p_rbkey;
struct p_rbkey
{
    unsigned int type;
    unsigned int size;
    const p_term *name;
};

int _p_rbkey_init(p_rbkey *key, const p_term *term);
int _p_rbkey_compare_keys(const p_rbkey *key1, const p_rbkey *key2);

void _p_rbtree_init(p_rbtree *tree);
void _p_rbtree_free(p_rbtree *tree);

p_rbnode *_p_rbtree_lookup(const p_rbtree *tree, const p_rbkey *key);
p_rbnode *_p_rbtree_insert(p_rbtree *tree, const p_rbkey *key);
p_term *_p_rbtree_remove(p_rbtree *tree, const p_rbkey *key);

p_rbnode *_p_rbtree_visit_all(p_rbtree *tree, p_rbnode *last);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
