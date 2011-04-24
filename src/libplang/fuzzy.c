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

#include <plang/database.h>
#include <plang/errors.h>
#include "context-priv.h"
#include "database-priv.h"

p_goal_result p_arith_eval
    (p_context *context, p_arith_value *result,
     p_term *expr, p_term **error);

static p_goal_result p_builtin_fuzzy_1
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value;
    p_goal_result result;
    double fuzzy;
    p_term *expr = p_term_deref_member(context, args[0]);
    if (p_term_type(expr) & P_TERM_VARIABLE) {
        if (p_term_unify(context, expr,
                         p_term_create_real
                            (context, context->confidence),
                         P_BIND_DEFAULT))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    } else {
        result = p_arith_eval(context, &value, expr, error);
        if (result != P_RESULT_TRUE)
            return result;
        if (value.type == P_TERM_INTEGER) {
            fuzzy = (double)(value.integer_value);
        } else if (value.type == P_TERM_REAL) {
            fuzzy = value.real_value;
        } else {
            *error = p_create_type_error(context, "number", expr);
            return P_RESULT_ERROR;
        }
        if (fuzzy <= 0.0)
            return P_RESULT_FAIL;
        if (fuzzy < context->confidence)
            context->confidence = fuzzy;
        return P_RESULT_TRUE;
    }
}

static p_goal_result p_builtin_set_fuzzy
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value;
    p_goal_result result;
    double fuzzy;
    result = p_arith_eval(context, &value, args[0], error);
    if (result != P_RESULT_TRUE)
        return result;
    if (value.type == P_TERM_INTEGER) {
        fuzzy = (double)(value.integer_value);
    } else if (value.type == P_TERM_REAL) {
        fuzzy = value.real_value;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
    if (fuzzy <= 0.0)
        return P_RESULT_FAIL;
    if (fuzzy > 1.0)
        fuzzy = 1.0;
    context->confidence = fuzzy;
    return P_RESULT_TRUE;
}

static p_goal_result p_builtin_register_fuzzy
    (p_context *context, p_term **args, p_term **error)
{
    static struct p_builtin const builtins[] = {
        {"fuzzy", 1, p_builtin_fuzzy_1},
        {"set_fuzzy", 1, p_builtin_set_fuzzy},
        {0, 0, 0}
    };
    _p_db_register_builtins(context, builtins);
    return P_RESULT_TRUE;
}

void _p_db_init_fuzzy(p_context *context)
{
    static struct p_builtin const builtins[] = {
        {"$$fuzzy", 1, p_builtin_fuzzy_1},
        {"$$register_fuzzy_builtins", 0, p_builtin_register_fuzzy},
        {"$$set_fuzzy", 1, p_builtin_set_fuzzy},
        {0, 0, 0}
    };
    _p_db_register_builtins(context, builtins);
}
