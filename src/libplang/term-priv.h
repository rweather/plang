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

#ifndef PLANG_TERM_PRIV_H
#define PLANG_TERM_PRIV_H

#include <plang/term.h>
#include <limits.h>
#include <config.h>
#ifdef HAVE_GC_GC_H
#include <gc/gc.h>
#elif defined(HAVE_GC_H)
#include <gc.h>
#else
#error "libgc is required to build plang"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__cplusplus)
#define P_INLINE inline
#elif defined(__GNUC_GNU_INLINE__) && !defined(__NO_INLINE__)
#define P_INLINE static __inline__
#else
#define P_INLINE static
#endif

#if defined(__WORDSIZE) && __WORDSIZE == 64
#define P_TERM_64BIT    1
#endif

/** @cond */

enum {
    P_TERM_RENAME       = P_TERM_INVALID
};

/* Forward declarations for rbtree-priv.h */
typedef struct p_rbnode p_rbnode;
typedef struct p_rbtree p_rbtree;
struct p_rbtree
{
    p_rbnode *root;
};

struct p_term_header {
#if defined(P_TERM_64BIT)
    unsigned int type;
    unsigned int size;
#else
    unsigned int type : 8;
    unsigned int size : 24;
#endif
};

struct p_term_functor {
    struct p_term_header header;
    p_term *functor_name;               /* Must be an atom */
    p_term *arg[1];
};

struct p_term_list {
    struct p_term_header header;
    p_term *head;
    p_term *tail;
};

typedef struct p_database_info p_database_info;

struct p_term_atom {
    struct p_term_header header;
    p_term *next;
    p_database_info *db_info;
    char name[1];
};

struct p_term_string {
    struct p_term_header header;
    char name[1];
};

struct p_term_var {
    struct p_term_header header;
    p_term *value;
};

struct p_term_member_var {
    struct p_term_header header;
    p_term *value;
    p_term *object;
    p_term *name;                       /* Must be an atom */
};

struct p_term_integer {
    struct p_term_header header;
#if !defined(P_TERM_64BIT)
    int value;
#endif
};

struct p_term_real {
    struct p_term_header header;
    double value;
};

struct p_term_property {
    p_term *name;
    p_term *value;
};

#define P_TERM_MAX_PROPS    8

struct p_term_object {
    struct p_term_header header;
    p_term *next;
    struct p_term_property properties[P_TERM_MAX_PROPS];
};

#define P_TERM_INDEX_TRIGGER    4

struct p_term_clause_list
{
    struct p_term_clause *head;
    struct p_term_clause *tail;
};

struct p_term_predicate {
    struct p_term_header header;
    p_term *name;                       /* Must be an atom */
    struct p_term_clause_list clauses;
    struct p_term_clause_list var_clauses;
    unsigned int clause_count;
    unsigned int index_arg : 30;
    unsigned int is_indexed : 1;
    unsigned int dont_index : 1;
    p_rbtree index;
};

#if defined(P_TERM_64BIT)
#define P_TERM_DEFAULT_CLAUSE_NUM   (((unsigned int)1) << 31)
#else
#define P_TERM_DEFAULT_CLAUSE_NUM   (((unsigned int)1) << 23)
#endif

struct p_term_clause {
    struct p_term_header header;
    struct p_term_clause *next_clause;
    struct p_term_clause *next_index;
    p_term *head;
    p_term *body;
};

struct p_term_rename {
    struct p_term_header header;
    p_term *var;
};

union p_term {
    struct p_term_header        header;
    struct p_term_functor       functor;
    struct p_term_list          list;
    struct p_term_atom          atom;
    struct p_term_string        string;
    struct p_term_var           var;
    struct p_term_member_var    member_var;
    struct p_term_integer       integer;
    struct p_term_real          real;
    struct p_term_object        object;
    struct p_term_predicate     predicate;
    struct p_term_clause        clause;
    struct p_term_rename        rename;
};

#define p_term_malloc(context, type, size)  ((type *)GC_MALLOC((size)))
#define p_term_new(context, type)           (GC_NEW(type))

int _p_term_next_utf8(const char *str, size_t len, size_t *size);

int _p_term_retract_clause
    (p_context *context, p_term *predicate,
     struct p_term_clause *clause, p_term *clause2);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
