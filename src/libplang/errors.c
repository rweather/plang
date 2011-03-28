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

#include "errors-priv.h"
#include "term-priv.h"
#include "context-priv.h"

/* Wrap an error term with "error(Term, Name/Arity)" */
static p_term *p_wrap_error(p_context *context, p_term *term)
{
    p_term *goal;
    p_term *name;
    p_term *error;
    p_term *pred;
    int arity;
    if (context->current_node)
        goal = p_term_deref(context->current_node->goal);
    else
        goal = p_term_create_atom(context, "unknown");
    if (goal && goal->header.type == P_TERM_FUNCTOR) {
        name = goal->functor.functor_name;
        arity = (int)(goal->header.size);
    } else {
        name = goal;
        arity = 0;
    }
    error = p_term_create_functor
        (context, p_term_create_atom(context, "error"), 2);
    p_term_bind_functor_arg(error, 0, term);
    pred = p_term_create_functor(context, context->slash_atom, 2);
    p_term_bind_functor_arg(pred, 0, name);
    p_term_bind_functor_arg
        (pred, 1, p_term_create_integer(context, arity));
    p_term_bind_functor_arg(error, 1, pred);
    return error;
}

/* Create an "instantiation_error" term */
p_term *p_create_instantiation_error(p_context *context)
{
    return p_wrap_error
        (context, p_term_create_atom(context, "instantiation_error"));
}

/* Create a "type_error" term */
p_term *p_create_type_error
    (p_context *context, const char *name, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "type_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, term));
    return p_wrap_error(context, error);
}

/* Create a "domain_error" term */
p_term *p_create_domain_error
    (p_context *context, const char *name, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "domain_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, term));
    return p_wrap_error(context, error);
}

/* Create an "existence_error" term */
p_term *p_create_existence_error
    (p_context *context, const char *name, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "existence_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, term));
    return p_wrap_error(context, error);
}

/* Create a "permission_error" term */
p_term *p_create_permission_error
    (p_context *context, const char *name1, const char *name2, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "permission_error"), 3);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name1));
    p_term_bind_functor_arg
        (error, 1, p_term_create_atom(context, name2));
    p_term_bind_functor_arg(error, 2, p_term_clone(context, term));
    return p_wrap_error(context, error);
}

/* Create an "evaluation_error" term */
p_term *p_create_evaluation_error(p_context *context, const char *name)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "evaluation_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    return p_wrap_error(context, error);
}

/* Create a "system_error" term */
p_term *p_create_system_error(p_context *context)
{
    return p_wrap_error
        (context, p_term_create_atom(context, "system_error"));
}
