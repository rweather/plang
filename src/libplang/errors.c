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

#include <plang/errors.h>
#include "term-priv.h"
#include "context-priv.h"

/**
 * \defgroup errors Native C API - Error Creation
 *
 * Functions in this module assist with the creation of error terms
 * for builtin predicates.  In the predicate documentation, errors
 * are described as follows:
 *
 * \li <tt>instantiation_error</tt> - \em Arg is a variable.
 * \li <tt>type_error(number, \em Value)</tt> - \em Value is
 *     not a number.
 * \li ...
 *
 * When the error is generated, the effect is to \ref throw_1 "throw/1"
 * a term of the form <tt>error(\em ErrorTerm, \em Name / \em Arity)</tt>
 * where:
 *
 * \li \em ErrorTerm is a cloned copy of the
 *     <tt>instantiation_error</tt>, <tt>type_error</tt>, etc term.
 *     The term must be cloned so that it will survive backtracking
 *     when searching for a \ref catch_3 "catch/3" goal to handle
 *     the error.
 * \li \em Name / \em Arity is the name of the predicate that
 *     generated the error.
 *
 * Because errors terms can be quite complex, the functions below
 * are provided to assist with the process of creating them
 * from native C code.
 *
 * Note: \ref standard "Standard Prolog" specifies the second argument
 * to <b>error/2</b> as "implementation-defined".  In Plang we have
 * chosen to provide the name of the predicate throwing the error.
 * This may be changed in later versions of Plang; e.g. to include
 * filename and line number information for the call site that
 * generated the error.  If that happens, the implementation of these
 * error creation functions will be modified to generate the
 * new form of error term.
 */
/*\@{*/

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

/**
 * \brief Creates a new instantiation error term within \a context.
 *
 * The returned term will have the form
 * <tt>error(instantiation_error, \em Name / \em Arity)</tt>.
 *
 * \ingroup errors
 */
p_term *p_create_instantiation_error(p_context *context)
{
    return p_wrap_error
        (context, p_term_create_atom(context, "instantiation_error"));
}

/**
 * \brief Creates a new type error term from \a expected_type
 * and \a culprit within \a context.
 *
 * The returned term will have the form
 * <tt>error(type_error(\em expected_type, \em culprit),
 * \em Name / \em Arity)</tt>.  The \a culprit term will
 * be cloned so that it will survive back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_type_error
    (p_context *context, const char *expected_type, p_term *culprit)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "type_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, expected_type));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, culprit));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new domain error term from \a expected_domain
 * and \a culprit within \a context.
 *
 * The returned term will have the form
 * <tt>error(domain_error(\em expected_domain, \em culprit),
 * \em Name / \em Arity)</tt>.  The \a culprit term will
 * be cloned so that it will survive back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_domain_error
    (p_context *context, const char *expected_domain, p_term *culprit)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "domain_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, expected_domain));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, culprit));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new existence error term from \a object_type
 * and \a culprit within \a context.
 *
 * The returned term will have the form
 * <tt>error(existence_error(\em object_type, \em culprit),
 * \em Name / \em Arity)</tt>.  The \a culprit term will
 * be cloned so that it will survive back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_existence_error
    (p_context *context, const char *object_type, p_term *culprit)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "existence_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, object_type));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, culprit));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new permission error term from \a operation,
 * \a permission_type, and \a culprit within \a context.
 *
 * The returned term will have the form
 * <tt>error(permission_error(\em operation, \em permission_type,
 * \em culprit), \em Name / \em Arity)</tt>.  The \a culprit term
 * will be cloned so that it will survive back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_permission_error
    (p_context *context, const char *operation,
     const char *permission_type, p_term *culprit)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "permission_error"), 3);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, operation));
    p_term_bind_functor_arg
        (error, 1, p_term_create_atom(context, permission_type));
    p_term_bind_functor_arg(error, 2, p_term_clone(context, culprit));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new representation error term from \a flag
 * within \a context.
 *
 * The returned term will have the form
 * <tt>error(representation_error(\em flag), \em Name / \em Arity)</tt>.
 *
 * \ingroup errors
 */
p_term *p_create_representation_error(p_context *context, const char *flag)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "representation_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, flag));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new evaluation error term from \a eval_type
 * within \a context.
 *
 * The returned term will have the form
 * <tt>error(evaluation_error(\em eval_type), \em Name / \em Arity)</tt>.
 *
 * \ingroup errors
 */
p_term *p_create_evaluation_error(p_context *context, const char *eval_type)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "evaluation_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, eval_type));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new resource error term from \a resource
 * within \a context.
 *
 * The returned term will have the form
 * <tt>error(resource_error(\em resource), \em Name / \em Arity)</tt>.
 * The \a resource term will be cloned so that it will survive
 * back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_resource_error(p_context *context, p_term *resource)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "resource_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_clone(context, resource));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new syntax error term from \a term
 * within \a context.
 *
 * The returned term will have the form
 * <tt>error(syntax_error(\em term), \em Name / \em Arity)</tt>.
 * The \a term will be cloned so that it will survive back-tracking.
 *
 * \ingroup errors
 */
p_term *p_create_syntax_error(p_context *context, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "syntax_error"), 1);
    p_term_bind_functor_arg
        (error, 0, p_term_clone(context, term));
    return p_wrap_error(context, error);
}

/**
 * \brief Creates a new system error term within \a context.
 *
 * The returned term will have the form
 * <tt>error(system_error, \em Name / \em Arity)</tt>.
 *
 * \ingroup errors
 */
p_term *p_create_system_error(p_context *context)
{
    return p_wrap_error
        (context, p_term_create_atom(context, "system_error"));
}

/**
 * \brief Creates a new generic error term from \a term
 * within \a context.
 *
 * The returned term will have the form
 * <tt>error(\em term, \em Name / \em Arity)</tt>.
 * The \a term will be cloned so that it will survive back-tracking.
 *
 * This function is intended for new error types that are not
 * handled by the other functions in this module.
 *
 * \ingroup errors
 */
p_term *p_create_generic_error(p_context *context, p_term *term)
{
    return p_wrap_error(context, p_term_clone(context, term));
}

/*\@}*/
