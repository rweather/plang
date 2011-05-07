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
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

#define P_CONTEXT_HASH_SIZE     511

/* Internal result code that indicates that a builtin predicate
 * has modified the search tree */
#define P_RESULT_TREE_CHANGE    ((p_goal_result)(P_RESULT_HALT + 5))

typedef struct p_trail p_trail;

struct p_path_list
{
    char **paths;
    size_t num_paths;
    size_t max_paths;
};

typedef struct p_exec_node p_exec_node;
typedef struct p_exec_fail_node p_exec_fail_node;
typedef struct p_exec_clause_node p_exec_clause_node;
typedef struct p_exec_catch_node p_exec_catch_node;
typedef void (*p_exec_fail_func)
    (p_context *context, p_exec_fail_node *node);

struct p_exec_node
{
    p_term *goal;
    p_exec_node *success_node;
    p_exec_fail_node *cut_node;
    p_exec_fail_func fail_func;
};
struct p_exec_fail_node
{
    p_exec_node parent;
    void *fail_marker;
    double confidence;
    p_exec_catch_node *catch_node;
};
struct p_exec_clause_node
{
    p_exec_fail_node parent;
    p_term *next_clause;
};
struct p_exec_catch_node
{
    p_exec_fail_node parent;
    p_exec_catch_node *catch_parent;
};

typedef void (*p_library_entry_func)(p_context *context);
typedef struct p_library p_library;
struct p_library
{
    void *handle;
    p_library_entry_func shutdown_func;
    p_library *next;
};

struct p_context
{
    p_term *nil_atom;
    p_term *prototype_atom;
    p_term *class_name_atom;
    p_term *dot_atom;
    p_term *clause_atom;
    p_term *comma_atom;
    p_term *line_atom;
    p_term *if_atom;
    p_term *in_atom;
    p_term *slash_atom;
    p_term *true_atom;
    p_term *fail_atom;
    p_term *commit_atom;
    p_term *call_member_atom;
    p_term *call_args_atom;
    p_term *unify_atom;
    p_term *atom_hash[P_CONTEXT_HASH_SIZE];

    p_trail *trail;
    int trail_top;

    int fail_on_unknown : 1;
    int debug : 1;

    int goal_active;
    void *goal_marker;
    p_exec_node *current_node;
    p_exec_fail_node *fail_node;
    p_exec_catch_node *catch_node;
    void *fail_marker;
    double confidence;

    int allow_test_goals;
    p_term *test_goal;

    struct p_path_list user_imports;
    struct p_path_list system_imports;
    struct p_path_list user_libs;
    struct p_path_list system_libs;
    struct p_path_list loaded_files;

    int unique_num;

    p_library *libraries;

    unsigned int random_seed;
};

#define P_TRACE_SIZE 1020

struct p_trail
{
    void **bindings[P_TRACE_SIZE];
    p_trail *next;
};

int _p_context_record_in_trail(p_context *context, p_term *var);
int _p_context_record_contents_in_trail(p_context *context, void **location, void *prev_value);

void _p_context_basic_fail_func
    (p_context *context, p_exec_fail_node *node);
void _p_context_clause_fail_func
    (p_context *context, p_exec_fail_node *node);
void _p_context_init_fail_node
    (p_context *context, p_exec_fail_node *node,
     p_exec_fail_func fail_func);

p_goal_result p_goal_call_from_parser(p_context *context, p_term *goal);

p_goal_result _p_context_load_library(p_context *context, p_term *name, p_term **error);

#define p_context_add_path(list,name)   \
    do { \
        if ((list).num_paths >= (list).max_paths) { \
            size_t new_max = (list).max_paths * 2; \
            if (new_max < 8) \
                new_max = 8; \
            (list).paths = (char **)GC_REALLOC((list).paths, new_max * sizeof(char *)); \
            (list).max_paths = new_max; \
        } \
        (list).paths[((list).num_paths)++] = GC_STRDUP(name); \
    } while (0)

/* Determine what kind of Win32 system we are running on */
#if defined(__CYGWIN__) || defined(__CYGWIN32__)
#define	P_WIN32         1
#define	P_WIN32_CYGWIN  1
#elif defined(_WIN32) || defined(WIN32)
#define	P_WIN32         1
#define	P_WIN32_NATIVE  1
#endif

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
