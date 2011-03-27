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

/* Create an "instantiation_error" term */
p_term *p_create_instantiation_error(p_context *context)
{
    return p_term_create_atom(context, "instantiation_error");
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
    return error;
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
    return error;
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
    return error;
}

/* Create an "evaluation_error" term */
p_term *p_create_evaluation_error(p_context *context, const char *name)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "evaluation_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    return error;
}
