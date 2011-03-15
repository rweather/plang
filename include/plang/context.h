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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p_context p_context;
typedef union p_term p_term;

p_context *p_context_create(void);
void p_context_free(p_context *context);

void *p_context_mark_trace(p_context *context);
void p_context_backtrack_trace(p_context *context, void *marker);

int p_context_consult_file(p_context *context, const char *filename);
int p_context_consult_string(p_context *context, const char *str);

typedef enum {
    P_RESULT_FAIL  = 0,
    P_RESULT_TRUE  = 1,
    P_RESULT_ERROR = 2
} p_goal_result;

p_goal_result p_context_execute_goal(p_context *context, p_term *goal);
p_goal_result p_context_reexecute_goal(p_context *context);
void p_context_abandon_goal(p_context *context);
p_term *p_context_uncaught_error(p_context *context);

#ifdef __cplusplus
};
#endif

#endif
