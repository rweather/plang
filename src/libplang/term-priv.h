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

#ifndef PLANG_TERM_PRIV_H
#define PLANG_TERM_PRIV_H

#include <plang/term.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    P_TERM_FUNCTOR,
    P_TERM_LIST,
    P_TERM_NIL,
    P_TERM_ATOM,
    P_TERM_STRING,
    P_TERM_VARIABLE,
    P_TERM_TYPED_VARIABLE,
    P_TERM_INTEGER,
    P_TERM_REAL,
    P_TERM_OBJECT
};

#if defined(__WORDSIZE) && __WORDSIZE == 64
#define P_TERM_64BIT    1
#endif

struct p_term_header {
#if defined(P_TERM_64BIT)
    unsigned int type;
    unsigned int size;
#else
    unsigned int type : 8;
    unsigned int size : 24;
#endif
};

struct p_term_functor {
    struct p_term_header header;
    p_term *functor_name;                 /* Must be an atom */
    p_term *arg[1];
};

struct p_term_list {
    struct p_term_header header;
    p_term *head;
    p_term *tail;
};

struct p_term_atom {
    struct p_term_header header;
    char name[1];
};

struct p_term_var {
    struct p_term_header header;
    p_term *value;
};

/* Can only be unified with a p_term that matches the p_term type
   information in "constraint" and "functor_name" */
struct p_term_typed_var {
    struct p_term_header header;
    struct p_term_header constraint;
    p_term *functor_name;                 /* Must be an atom or null */
    p_term *value;
};

struct p_term_integer {
    struct p_term_header header;
    int value;
};

struct p_term_real {
    struct p_term_header header;
    double value;
};

struct p_term_property {
    p_term *name;
    p_term *value;
};

struct p_term_object {
    struct p_term_header header;
    p_term *next;
    struct p_term_property properties[1];
};

union p_term {
    struct p_term_header    header;
    struct p_term_functor   functor;
    struct p_term_list      list;
    struct p_term_atom      atom;
    struct p_term_atom      string;
    struct p_term_var       var;
    struct p_term_typed_var typed_var;
    struct p_term_integer   integer;
    struct p_term_real      real;
    struct p_term_object    object;
};

#ifdef __cplusplus
};
#endif

#endif
