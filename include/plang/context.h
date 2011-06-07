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

#ifndef PLANG_CONTEXT_H
#define PLANG_CONTEXT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p_context p_context;
typedef union p_term p_term;

p_context *p_context_create(void);
void p_context_free(p_context *context);

void *p_context_mark_trail(p_context *context);
void p_context_backtrack_trail(p_context *context, void *marker);

typedef enum {
    P_CONSULT_DEFAULT   = 0,
    P_CONSULT_ONCE      = 1
} p_consult_option;

int p_context_consult_file
    (p_context *context, const char *filename, p_consult_option option);
int p_context_consult_string(p_context *context, const char *str);

typedef enum {
    P_RESULT_FAIL       = 0,
    P_RESULT_TRUE       = 1,
    P_RESULT_ERROR      = 2,
    P_RESULT_HALT       = 3
} p_goal_result;

p_goal_result p_context_execute_goal
    (p_context *context, p_term *goal, p_term **error);
p_goal_result p_context_reexecute_goal
    (p_context *context, p_term **error);
void p_context_abandon_goal(p_context *context);

p_goal_result p_context_call_predicate
    (p_context *context, p_term *name, p_term **args,
     int num_args, p_term **error);
p_goal_result p_context_call_member_predicate
    (p_context *context, p_term *object, p_term *name,
     p_term **args, int num_args, p_term **error);
p_goal_result p_context_new_object
    (p_context *context, p_term *name, p_term **args, int num_args,
     p_term **object, p_term **error);

double p_context_fuzzy_confidence(p_context *context);
void p_context_set_fuzzy_confidence(p_context *context, double value);

p_goal_result p_context_call_once
    (p_context *context, p_term *goal, p_term **error);

int p_context_is_debug(p_context *context);
void p_context_set_debug(p_context *context, int debug);

void p_context_add_import_path(p_context *context, const char *path);
void p_context_add_library_path(p_context *context, const char *path);

void *p_context_gc_malloc(p_context *context, size_t size);
void p_context_gc_free(p_context *context, void *ptr);

#ifdef __cplusplus
};
#endif

#endif
