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

#ifndef PLANG_CONTEXT_PRIV_H
#define PLANG_CONTEXT_PRIV_H

#include <plang/context.h>
#include <plang/term.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

#define P_CONTEXT_HASH_SIZE     511

typedef struct p_trace p_trace;

struct p_context
{
    p_term *nil_atom;
    p_term *prototype_atom;
    p_term *class_name_atom;
    p_term *clause_atom;
    p_term *comma_atom;
    p_term *line_atom;
    p_term *if_atom;
    p_term *slash_atom;
    p_term *atom_hash[P_CONTEXT_HASH_SIZE];

    p_trace *trace;
    int trace_top;

    int fail_on_unknown : 1;
    int debug : 1;

    int goal_active;
    p_term *goal;
    void *goal_marker;

    int allow_test_goals;
    p_term *test_goal;
};

#define P_TRACE_SIZE 1020

struct p_trace
{
    void **bindings[P_TRACE_SIZE];
    p_trace *next;
};

int _p_context_record_in_trace(p_context *context, p_term *var);
int _p_context_record_contents_in_trace(p_context *context, void **location);

p_goal_result p_goal_call(p_context *context, p_term *goal, p_term **error);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
