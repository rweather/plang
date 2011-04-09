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

#include "term-priv.h"
#include "context-priv.h"

/** @cond */
struct p_term_expand_info
{
    p_term *or_atom;
    p_term *compound_atom;
    p_term *unify_atom;
};
/** @endcond */

static p_term *p_term_expand_head
    (p_context *context, p_term *term, p_term *in_var, p_term *out_var)
{
    p_term *new_term;
    if (term->header.type == P_TERM_ATOM) {
        new_term = p_term_create_functor(context, term, 2);
        p_term_bind_functor_arg(new_term, 0, in_var);
        p_term_bind_functor_arg(new_term, 1, out_var);
    } else {
        unsigned int index;
        new_term = p_term_create_functor
            (context, term->functor.functor_name,
             p_term_arg_count(term) + 2);
        for (index = 0; index < term->header.size; ++index) {
            p_term_bind_functor_arg
                (new_term, (int)index, term->functor.arg[index]);
        }
        p_term_bind_functor_arg(new_term, term->header.size, in_var);
        p_term_bind_functor_arg(new_term, term->header.size + 1, out_var);
    }
    return new_term;
}

P_INLINE p_term *p_term_create_binary
    (p_context *context, p_term *name, p_term *term1, p_term *term2)
{
    p_term *term = p_term_create_functor(context, name, 2);
    p_term_bind_functor_arg(term, 0, term1);
    p_term_bind_functor_arg(term, 1, term2);
    return term;
}

static p_term *p_term_expand_body
    (p_context *context, p_term *term, p_term *in_var,
     p_term *out_var, struct p_term_expand_info *info, int *first)
{
    p_term *list;
    p_term *left;
    p_term *right;
    p_term *middle_var;
    term = p_term_deref(term);
    if (!term)
        return term;
    switch (term->header.type) {
    case P_TERM_ATOM:
        if (term == context->nil_atom) {
            /* Empty list should be converted into (In = Out).
             * If we are still at the first term, then we can
             * unify the variables now.  Otherwise we must do
             * the unification at runtime */
            if (*first) {
                p_term_unify
                    (context, in_var, out_var, P_BIND_NO_RECORD);
                return context->true_atom;
            } else {
                return p_term_create_binary
                    (context, info->unify_atom, in_var, out_var);
            }
        } else if (term == context->cut_atom) {
            /* Cut operator for commiting to the current rule.
             * Convert it into (!, In = Out) */
            *first = 0;
            right = p_term_create_binary
                (context, info->unify_atom, in_var, out_var);
            return p_term_create_binary
                (context, context->comma_atom, term, right);
        } else {
            /* Expand the atom into an arity-2 rule predicate */
            *first = 0;
            return p_term_expand_head(context, term, in_var, out_var);
        }
    case P_TERM_FUNCTOR:
        if (term->functor.functor_name == info->or_atom &&
                term->header.size == 2) {
            /* OR'ed alternatives separated by "||" */
            *first = 0;
            left = p_term_expand_body
                (context, term->functor.arg[0], in_var,
                 out_var, info, first);
            right = p_term_expand_body
                (context, term->functor.arg[1], in_var,
                 out_var, info, first);
            return p_term_create_binary
                (context, info->or_atom, left, right);
        } else if (term->functor.functor_name == context->comma_atom &&
                   term->header.size == 2) {
            /* Sequence of DCG terms separated by "," */
            middle_var = p_term_create_variable(context);
            left = p_term_expand_body
                (context, term->functor.arg[0], in_var,
                 middle_var, info, first);
            right = p_term_expand_body
                (context, term->functor.arg[1], middle_var,
                 out_var, info, first);
            if (left == context->true_atom)
                return right;
            else if (right == context->true_atom)
                return left;
            return p_term_create_binary
                (context, context->comma_atom, left, right);
        } else if (term->functor.functor_name == context->cut_atom &&
                   term->header.size == 1) {
            /* Logical negation of a DCG term.  Convert it
             * into (!expand(Arg), In = Out) */
            *first = 0;
            middle_var = p_term_create_variable(context);
            term = p_term_expand_body
                (context, term->functor.arg[0], in_var,
                 middle_var, info, first);
            left = p_term_create_functor
                (context, context->cut_atom, 1);
            p_term_bind_functor_arg(left, 0, term);
            right = p_term_create_binary
                (context, info->unify_atom, in_var, out_var);
            return p_term_create_binary
                (context, context->comma_atom, left, right);
        } else if (term->functor.functor_name == info->compound_atom &&
                   term->header.size == 1) {
            /* Compound statement.  Convert it into (Stmt, In = Out) */
            *first = 0;
            right = p_term_create_binary
                (context, info->unify_atom, in_var, out_var);
            term = p_term_deref(term->functor.arg[0]);
            if (term == context->true_atom)
                return right;
            return p_term_create_binary
                (context, context->comma_atom, term, right);
        }
        *first = 0;
        return p_term_expand_head(context, term, in_var, out_var);
    case P_TERM_LIST:
        /* Convert [Members] into (In = [Members|Out]) */
        list = p_term_create_list(context, term->list.head, 0);
        left = p_term_deref(term->list.tail);
        right = list;
        while (left && left->header.type == P_TERM_LIST) {
            middle_var = p_term_create_list
                (context, left->list.head, 0);
            p_term_set_tail(right, middle_var);
            right = middle_var;
            left = p_term_deref(left->list.tail);
        }
        p_term_set_tail(right, out_var);
        if (*first) {
            /* Keep the "first" indication in case there are more
             * literals following.  We can incorporate them into a
             * single list unification */
            p_term_unify(context, in_var, list, P_BIND_NO_RECORD);
            return context->true_atom;
        } else {
            return p_term_create_binary
                (context, info->unify_atom, in_var, list);
        }
        break;
    case P_TERM_STRING:
        /* Strings should be converted into singleton list matchers */
        if (*first) {
            list = p_term_create_list(context, term, out_var);
            p_term_unify(context, in_var, list, P_BIND_NO_RECORD);
            return context->true_atom;
        } else {
            list = p_term_create_list(context, term, out_var);
            return p_term_create_binary
                (context, info->unify_atom, in_var, list);
        }
        break;
    default: break;
    }
    return term;
}

/**
 * \brief Expands the DCG rule in \a term to a full clause
 * definition for \a context.
 *
 * The \a term must have the functor \ref syntax_dcg "(-->)/2".
 * The returned term will have the functor \ref clause_op_2 "(:-)/2".
 *
 * \ingroup term
 */
p_term *p_term_expand_dcg(p_context *context, p_term *term)
{
    struct p_term_expand_info info;
    p_term *head;
    p_term *body;
    p_term *clause;
    p_term *in_var;
    p_term *out_var;
    int first = 1;

    /* Create atoms for the main DCG control structures */
    info.or_atom = p_term_create_atom(context, "||");
    info.compound_atom = p_term_create_atom(context, "$$compound");
    info.unify_atom = p_term_create_atom(context, "=");

    /* Verify that the rule has the form "name(args) --> body" */
    term = p_term_deref(term);
    if (!term || term->header.type != P_TERM_FUNCTOR ||
            term->header.size != 2 ||
            term->functor.functor_name !=
                    p_term_create_atom(context, "-->"))
        return 0;
    head = p_term_deref(term->functor.arg[0]);
    if (!head || (head->header.type != P_TERM_FUNCTOR &&
                  head->header.type != P_TERM_ATOM))
        return 0;

    /* Convert the head term by adding two extra variable arguments */
    in_var = p_term_create_variable(context);
    out_var = p_term_create_variable(context);
    head = p_term_expand_head(context, head, in_var, out_var);

    /* Convert the body term */
    body = p_term_expand_body
        (context, term->functor.arg[1], in_var, out_var, &info, &first);

    /* Create the final clause */
    clause = p_term_create_functor(context, context->clause_atom, 2);
    p_term_bind_functor_arg(clause, 0, head);
    p_term_bind_functor_arg(clause, 1, body);
    return clause;
}
