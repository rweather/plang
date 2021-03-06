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

#ifndef PLANG_DATABASE_PRIV_H
#define PLANG_DATABASE_PRIV_H

#include <plang/database.h>
#include "term-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

/* Information about a class */
typedef struct p_class_info p_class_info;
struct p_class_info
{
    p_term *class_object;
    p_class_info *parent;
    p_term *var_list;
};

/* Information that is attached to an atom to provide information
 * about the operators and predicates with that name */
struct p_database_info
{
    p_database_info *next;
    unsigned int arity;
    unsigned int flags : 8;
    unsigned int op_specifier : 8;
    unsigned int op_priority : 16;
    p_db_builtin builtin_func;
    p_db_arith arith_func;
    p_class_info *class_info;
    p_term *predicate;
};

struct p_builtin
{
    const char *name;
    int arity;
    p_db_builtin func;
};

struct p_arith
{
    const char *name;
    int arity;
    p_db_arith arith_func;
};

void _p_db_register_builtins(p_context *context, const struct p_builtin *builtins);
void _p_db_register_ariths(p_context *context, const struct p_arith *ariths);
void _p_db_register_sources(p_context *context, const char * const *sources);

void _p_db_init(p_context *context);
void _p_db_init_builtins(p_context *context);
void _p_db_init_arith(p_context *context);
void _p_db_init_io(p_context *context);
void _p_db_init_fuzzy(p_context *context);
void _p_db_init_sort(p_context *context);

p_database_info *_p_db_find_arity(const p_term *atom, unsigned int arity);
p_database_info *_p_db_create_arity(p_term *atom, unsigned int arity);

p_term *_p_db_clause_assert_last(p_context *context, p_term *clause);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
