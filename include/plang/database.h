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

#ifndef PLANG_DATABASE_H
#define PLANG_DATABASE_H

#include <plang/term.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    P_OP_NONE,
    P_OP_XF,
    P_OP_YF,
    P_OP_XFX,
    P_OP_XFY,
    P_OP_YFX,
    P_OP_FX,
    P_OP_FY
} p_op_specifier;

p_op_specifier p_db_operator_info(const p_term *name, int arity, int *priority);
void p_db_set_operator_info(p_term *name, p_op_specifier specifier, int priority);

typedef p_goal_result (*p_db_builtin)(p_context *context, p_term **args, p_term **error);
p_db_builtin p_db_builtin_predicate(const p_term *name, int arity);
void p_db_set_builtin_predicate(p_term *name, int arity, p_db_builtin builtin);

int p_db_clause_assert_first(p_context *context, p_term *clause);
int p_db_clause_assert_last(p_context *context, p_term *clause);
int p_db_clause_retract(p_context *context, p_term *clause);
int p_db_clause_abolish(p_context *context, const p_term *name, int arity);

p_term *p_db_global_object(p_context *context, p_term *name);
void p_db_set_global_object(p_context *context, p_term *name, p_term *value);

#ifdef __cplusplus
};
#endif

#endif
