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

#ifdef __cplusplus
extern "C" {
#endif

typedef union p_term p_term;

enum {
    P_TERM_INVALID,
    P_TERM_FUNCTOR,
    P_TERM_LIST,
    P_TERM_ATOM,
    P_TERM_STRING,
    P_TERM_VARIABLE,
    P_TERM_TYPED_VARIABLE,
    P_TERM_MEMBER_VARIABLE,
    P_TERM_INTEGER,
    P_TERM_REAL,
    P_TERM_OBJECT
};

p_term *p_term_create_functor(p_context *context, p_term *name, int arg_count);
int p_term_bind_functor_arg(p_term *term, int index, p_term *value);
p_term *p_term_create_functor_with_args(p_context *context, p_term *name, p_term **args, int arg_count);

p_term *p_term_create_list(p_context *context, p_term *head, p_term *tail);
p_term *p_term_create_atom(p_context *context, const char *name);
p_term *p_term_create_string(p_context *context, const char *str);
p_term *p_term_create_variable(p_context *context);
p_term *p_term_create_named_variable(p_context *context, const char *name);
p_term *p_term_create_typed_variable(p_context *context, int type, p_term *functor_name, int arg_count, const char *variable_name);
p_term *p_term_create_member_variable(p_context *context, p_term *object, p_term *name);
p_term *p_term_create_integer(p_context *context, int value);
p_term *p_term_create_real(p_context *context, double value);

p_term *p_term_nil_atom(p_context *context);
p_term *p_term_prototype_atom(p_context *context);
p_term *p_term_class_name_atom(p_context *context);

p_term *p_term_deref(const p_term *term);

int p_term_type(const p_term *term);
int p_term_arg_count(const p_term *term);
const char *p_term_name(const p_term *term);
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
p_term *p_term_class_name(p_context *context, const p_term *term);
int p_term_is_instance(p_context *context, const p_term *term, const p_term *class_name);

#ifdef __cplusplus
};
#endif

#endif
