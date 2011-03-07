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
    p_term *atom_hash[P_CONTEXT_HASH_SIZE];

    p_trace *trace;
    int trace_top;
};

#define P_TRACE_SIZE 1020

struct p_trace
{
    p_term *bindings[P_TRACE_SIZE];
    p_trace *next;
};

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
