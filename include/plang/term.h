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

#ifndef PLANG_TERM_H
#define PLANG_TERM_H

#include <plang/context.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Update precedes_ordering in p_term_precedes if this list changes */
enum {
    P_TERM_INVALID,
    P_TERM_FUNCTOR,
    P_TERM_LIST,
    P_TERM_ATOM,
    P_TERM_STRING,
    P_TERM_INTEGER,
    P_TERM_REAL,
    P_TERM_OBJECT,
    P_TERM_PREDICATE,
    P_TERM_CLAUSE,

    P_TERM_VARIABLE         = 16,   /* Used as a flag for all vars */
    P_TERM_MEMBER_VARIABLE,
};

enum {
    P_BIND_DEFAULT          = 0x0000,
    P_BIND_NO_OCCURS_CHECK  = 0x0001,
    P_BIND_NO_RECORD        = 0x0002,
    P_BIND_RECORD_ONE_WAY   = 0x0004,
    P_BIND_EQUALITY         = 0x0008,
    P_BIND_ONE_WAY          = 0x0010
};

p_term *p_term_create_functor(p_context *context, p_term *name, int arg_count);
int p_term_bind_functor_arg(p_term *term, int index, p_term *value);
p_term *p_term_create_functor_with_args(p_context *context, p_term *name, p_term **args, int arg_count);

p_term *p_term_create_list(p_context *context, p_term *head, p_term *tail);
void p_term_set_tail(p_term *list, p_term *tail);
p_term *p_term_create_atom(p_context *context, const char *name);
p_term *p_term_create_atom_n(p_context *context, const char *name, size_t len);
p_term *p_term_create_string(p_context *context, const char *str);
p_term *p_term_create_string_n(p_context *context, const char *str, size_t len);
p_term *p_term_create_variable(p_context *context);
p_term *p_term_create_named_variable(p_context *context, const char *name);
p_term *p_term_create_member_variable(p_context *context, p_term *object, p_term *name, int auto_create);
p_term *p_term_create_integer(p_context *context, int value);
p_term *p_term_create_real(p_context *context, double value);

p_term *p_term_nil_atom(p_context *context);
p_term *p_term_prototype_atom(p_context *context);
p_term *p_term_class_name_atom(p_context *context);

p_term *p_term_deref(const p_term *term);
p_term *p_term_deref_member(p_context *context, p_term *term);
p_term *p_term_deref_own_member(p_context *context, p_term *term);

int p_term_type(const p_term *term);
int p_term_arg_count(const p_term *term);
const char *p_term_name(const p_term *term);
size_t p_term_name_length(const p_term *term);
size_t p_term_name_length_utf8(const p_term *term);
p_term *p_term_functor(const p_term *term);
p_term *p_term_arg(const p_term *term, int index);
int p_term_integer_value(const p_term *term);
double p_term_real_value(const p_term *term);
p_term *p_term_head(const p_term *term);
p_term *p_term_tail(const p_term *term);
p_term *p_term_object(const p_term *term);

p_term *p_term_create_object(p_context *context, p_term *prototype);
p_term *p_term_create_class_object(p_context *context, p_term *class_name, p_term *prototype);
int p_term_add_property(p_context *context, p_term *term, p_term *name, p_term *value);
p_term *p_term_property(p_context *context, const p_term *term, const p_term *name);
p_term *p_term_own_property(p_context *context, const p_term *term, const p_term *name);
int p_term_set_own_property(p_context *context, p_term *term, p_term *name, p_term *value);
int p_term_is_instance_object(p_context *context, const p_term *term);
int p_term_is_class_object(p_context *context, const p_term *term);
int p_term_inherits(p_context *context, const p_term *term1, const p_term *term2);
int p_term_is_instance_of(p_context *context, const p_term *term1, const p_term *term2);

p_term *p_term_create_predicate(p_context *context, p_term *name, int arg_count);
p_term *p_term_create_clause(p_context *context, p_term *head, p_term *body);
void p_term_add_clause_first(p_context *context, p_term *predicate, p_term *clause);
void p_term_add_clause_last(p_context *context, p_term *predicate, p_term *clause);

/** @cond */
typedef struct p_term_clause_iter p_term_clause_iter;
struct p_term_clause_iter
{
    struct p_term_clause *next1;
    struct p_term_clause *next2;
    struct p_term_clause *next3;
};
/** @endcond */
void p_term_clauses_begin(const p_term *predicate, const p_term *head, p_term_clause_iter *iter);
p_term *p_term_clauses_next(p_term_clause_iter *iter);
int p_term_clauses_has_more(const p_term_clause_iter *iter);

p_term *p_term_create_member_name(p_context *context, p_term *class_name, p_term *name);

int p_term_bind_variable(p_context *context, p_term *var, p_term *value, int flags);
int p_term_unify(p_context *context, p_term *term1, p_term *term2, int flags);

typedef void (*p_term_print_func)(void *data, const char *format, ...);
void p_term_stdio_print_func(void *data, const char *format, ...);

void p_term_print(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data);
void p_term_print_unquoted(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data);
void p_term_print_with_vars(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data, const p_term *vars);

int p_term_precedes(p_context *context, const p_term *term1, const p_term *term2);

int p_term_is_ground(const p_term *term);
p_term *p_term_clone(p_context *context, p_term *term);
p_term *p_term_unify_clause(p_context *context, p_term *term, p_term *clause);

int p_term_strcmp(const p_term *str1, const p_term *str2);
p_term *p_term_concat_string(p_context *context, p_term *str1, p_term *str2);

p_term *p_term_witness(p_context *context, p_term *term, p_term **subgoal);

p_term *p_term_expand_dcg(p_context *context, p_term *term);

enum {
    P_SORT_ASCENDING        = 0x0000,
    P_SORT_DESCENDING       = 0x0001,
    P_SORT_KEYED            = 0x0002,
    P_SORT_REVERSE_KEYED    = 0x0004,
    P_SORT_UNIQUE           = 0x0008
};

p_term *p_term_sort(p_context *context, p_term *list, int flags);

#ifdef __cplusplus
};
#endif

#endif
