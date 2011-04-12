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

#ifndef PLANG_PARSER_PRIV_H
#define PLANG_PARSER_PRIV_H

#include <plang/context.h>
#include <plang/term.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

struct p_input_var
{
    p_term *name;
    p_term *var;
    unsigned int count;
};

typedef struct p_input_stream p_input_stream;

typedef int (*p_input_read_func)
    (p_input_stream *stream, char *buf, size_t max_size);

struct p_input_stream
{
    p_context *context;
    FILE *stream;
    const char *filename;
    const char *buffer;
    size_t buffer_len;
    p_input_read_func read_func;
    int close_stream;
    int error_count;
    int warning_count;
    p_term *declarations;
    struct p_input_var *variables;
    size_t num_variables;
    size_t max_variables;
    p_term *filename_string;
    p_term *class_name;
    p_term *read_term;
    p_term *vars;
    int generate_vars;
};

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE p_input_stream *
#endif

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
