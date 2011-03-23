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
    p_term *parent;
    p_term *var_list;
    p_term *clause_list;
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
    p_term *clauses_head;
    p_term *clauses_tail;
    p_term *global_object;
};

void _p_db_init(p_context *context);
void _p_db_init_builtins(p_context *context);
void _p_db_init_arith(p_context *context);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
