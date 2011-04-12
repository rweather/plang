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

#include <plang/term.h>
#include <plang/errors.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "term-priv.h"
#include "database-priv.h"
#include "context-priv.h"
#include "parser-priv.h"

/* Validate a variable list that was passed to iostream::writeTerm() */
static int p_builtin_validate_var_list
    (p_context *context, p_term *vars, p_term **error)
{
    p_term *save_vars = vars;
    p_term *head;
    p_term *name;
    if (!vars || (vars->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return 0;
    }
    while (vars && vars->header.type == P_TERM_LIST) {
        head = p_term_deref(vars->list.head);
        if (!head || head->header.type != P_TERM_FUNCTOR ||
                head->header.size != 2 ||
                head->functor.functor_name != context->unify_atom) {
            *error = p_create_type_error
                (context, "variable_names", save_vars);
            return 0;
        }
        name = p_term_deref(p_term_arg(head, 0));
        if (!name || (name->header.type != P_TERM_ATOM &&
                      name->header.type != P_TERM_STRING)) {
            *error = p_create_type_error
                (context, "variable_names", save_vars);
            return 0;
        }
        vars = p_term_deref(vars->list.tail);
    }
    if (vars != context->nil_atom) {
        *error = p_create_type_error
            (context, "variable_names", save_vars);
        return 0;
    }
    return 1;
}

/* Implementations for stdout/stderr printing */
static p_goal_result p_builtin_print
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[1]);
    if (p_term_integer_value(args[0]) == 1) {
        p_term_print_with_vars
            (context, term, p_term_stdio_print_func, stdout, 0);
    } else {
        p_term_print_with_vars
            (context, term, p_term_stdio_print_func, stderr, 0);
    }
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_print_3
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[1]);
    p_term *vars = p_term_deref_member(context, args[2]);
    if (!p_builtin_validate_var_list(context, vars, error))
        return P_RESULT_ERROR;
    if (p_term_integer_value(args[0]) == 1) {
        p_term_print_with_vars
            (context, term, p_term_stdio_print_func, stdout, vars);
    } else {
        p_term_print_with_vars
            (context, term, p_term_stdio_print_func, stderr, vars);
    }
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_print_byte
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[1]);
    if (!term || (term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    } else if (term->header.type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "byte", term);
        return P_RESULT_ERROR;
    } else {
        int value = p_term_integer_value(term);
        if (value < 0 || value > 255) {
            *error = p_create_type_error(context, "byte", term);
            return P_RESULT_ERROR;
        }
        if (p_term_integer_value(args[0]) == 1)
            putc(value, stdout);
        else
            putc(value, stderr);
    }
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_print_flush
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_integer_value(args[0]) == 1)
        fflush(stdout);
    else
        fflush(stderr);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_print_string
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[1]);
    if (!term || (term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    } else if (term->header.type != P_TERM_STRING) {
        *error = p_create_type_error(context, "string", term);
        return P_RESULT_ERROR;
    } else if (p_term_integer_value(args[0]) == 1) {
        p_term_print_unquoted
            (context, term, p_term_stdio_print_func, stdout);
    } else {
        p_term_print_unquoted
            (context, term, p_term_stdio_print_func, stderr);
    }
    return P_RESULT_TRUE;
}

/** @cond */
struct p_write_term_data
{
    p_context *context;
    p_term *stream;
    char buffer[512];
    size_t buflen;
    p_term *error;
    p_term *writeString;
    p_goal_result result;
};
/** @endcond */

static void p_write_term_flush(struct p_write_term_data *data)
{
    p_term *str = p_term_create_string_n
        (data->context, data->buffer, data->buflen);
    p_term *call = p_term_create_functor
        (data->context, data->context->call_member_atom, 2);
    p_term *args = p_term_create_functor
        (data->context, data->context->call_args_atom, 2);
    p_term_bind_functor_arg(args, 0, data->stream);
    p_term_bind_functor_arg(args, 1, str);
    p_term_bind_functor_arg
        (call, 0, p_term_create_member_variable
            (data->context, data->stream, data->writeString, 0));
    p_term_bind_functor_arg(call, 1, args);
    data->buflen = 0;
    data->result = p_context_call_once
        (data->context, call, &(data->error));
}

static void p_write_term_func(void *_data, const char *format, ...)
{
    struct p_write_term_data *data = (struct p_write_term_data *)_data;
    va_list va;
    va_start(va, format);
    while (*format != '\0') {
        if (data->result != P_RESULT_TRUE)
            break;
        if (*format == '%') {
            ++format;
            if (*format == '\0') {
                break;
            } else if (*format == 's') {
                const char *str = va_arg(va, const char *);
                size_t len = strlen(str);
                size_t temp_len;
                while (len > 0) {
                    if (data->buflen >= sizeof(data->buffer)) {
                        p_write_term_flush(data);
                        if (data->result != P_RESULT_TRUE)
                            break;
                    }
                    temp_len = sizeof(data->buffer) - data->buflen;
                    if (temp_len > len)
                        temp_len = len;
                    memcpy(data->buffer + data->buflen, str, temp_len);
                    str += temp_len;
                    len -= temp_len;
                    data->buflen += temp_len;
                }
            } else if (*format == 'c') {
                int ch = va_arg(va, int);
                if (data->buflen >= sizeof(data->buffer))
                    p_write_term_flush(data);
                data->buffer[(data->buflen)++] = (char)ch;
            } else {
                /* Assume this is some format like %d or %.10g and
                 * that it is the last and only format present */
                if (data->buflen >= (sizeof(data->buffer) - 64))
                    p_write_term_flush(data);
#if defined(HAVE_VSNPRINTF)
                vsnprintf(data->buffer + data->buflen,
                          sizeof(data->buffer) - data->buflen,
                          format - 1, va);
#elif defined(HAVE__VSNPRINTF)
                _vsnprintf(data->buffer + data->buflen,
                           sizeof(data->buffer) - data->buflen,
                           format - 1, va);
#else
                vsprintf(data->buffer + data->buflen, format - 1, va);
#endif
                data->buflen += strlen(data->buffer + data->buflen);
                va_end(va);
                return;
            }
        } else {
            if (data->buflen >= sizeof(data->buffer))
                p_write_term_flush(data);
            data->buffer[(data->buflen)++] = *format;
        }
        ++format;
    }
    va_end(va);
}

/* Writing terms to an iostream */
static p_goal_result p_builtin_iostream_writeTerm
    (p_context *context, p_term **args, p_term **error)
{
    p_term *stream = p_term_deref_member(context, args[0]);
    p_term *term = p_term_deref_member(context, args[1]);
    p_term *vars = p_term_deref_member(context, args[2]);
    struct p_write_term_data data;
    if (!p_builtin_validate_var_list(context, vars, error))
        return P_RESULT_ERROR;
    data.context = context;
    data.stream = stream;
    data.buflen = 0;
    data.error = 0;
    data.writeString = p_term_create_atom(context, "writeString");
    data.result = P_RESULT_TRUE;
    p_term_print_with_vars
        (context, term, p_write_term_func, &data, vars);
    if (data.buflen > 0 && data.result == P_RESULT_TRUE)
        p_write_term_flush(&data);
    *error = data.error;
    return data.result;
}

static p_term *p_input_concat_string
    (p_context *context, p_term *str, const char *buf, size_t len)
{
    p_term *str2 = p_term_create_string_n(context, buf, len);
    if (!str)
        return str2;
    return p_term_concat_string(context, str, str2);
}

/* Standard input routines */
static p_goal_result p_builtin_stdin_read_byte
    (p_context *context, p_term **args, p_term **error)
{
    int ch = getc(stdin);
    if (ch == EOF)
        return P_RESULT_FAIL;
    if (p_term_unify(context, args[0],
                     p_term_create_integer(context, ch),
                     P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}
static p_goal_result p_builtin_stdin_read_bytes
    (p_context *context, p_term **args, p_term **error)
{
    int len = p_term_integer_value(args[1]);
    char buffer[512];
    size_t buflen = 0;
    p_term *str = 0;
    int ch;
    if (!len)
        str = p_term_create_string(context, "");
    while (len > 0) {
        ch = getc(stdin);
        if (ch == EOF)
            break;
        buffer[buflen++] = (char)ch;
        if (buflen >= sizeof(buffer)) {
            str = p_input_concat_string
                (context, str, buffer, buflen);
            buflen = 0;
        }
        --len;
    }
    if (ch == EOF && !str && buflen == 0)
        return P_RESULT_FAIL;
    str = p_input_concat_string(context, str, buffer, buflen);
    if (p_term_unify(context, args[0], str, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}
static p_goal_result p_builtin_stdin_read_line
    (p_context *context, p_term **args, p_term **error)
{
    char buffer[512];
    size_t buflen = 0;
    p_term *str = 0;
    int ch;
    while ((ch = getc(stdin)) != EOF) {
        if (ch == '\n') {
            break;
        } else if (ch != '\r') {
            buffer[buflen++] = (char)ch;
            if (buflen >= sizeof(buffer)) {
                str = p_input_concat_string
                    (context, str, buffer, buflen);
                buflen = 0;
            }
        }
    }
    if (ch == EOF && !str && buflen == 0)
        return P_RESULT_FAIL;
    str = p_input_concat_string(context, str, buffer, buflen);
    if (p_term_unify(context, args[0], str, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

int p_context_consult(p_context *context, p_input_stream *stream);
int p_string_read_func(p_input_stream *stream, char *buf, size_t max_size);

/** @cond */
struct p_read_term_stream
{
    p_input_stream parent;
    p_term *stream;
    p_term *error;
    p_term *lines;
    p_term *readLine;
};
/** @endcond */

static p_goal_result p_read_term_line
    (p_context *context, struct p_read_term_stream *stream,
     p_term **line)
{
    p_term *call = p_term_create_functor
        (context, context->call_member_atom, 2);
    p_term *args = p_term_create_functor
        (context, context->call_args_atom, 2);
    *line = p_term_create_variable(context);
    p_term_bind_functor_arg(args, 0, stream->stream);
    p_term_bind_functor_arg(args, 1, *line);
    p_term_bind_functor_arg
        (call, 0, p_term_create_member_variable
            (context, stream->stream, stream->readLine, 0));
    p_term_bind_functor_arg(call, 1, args);
    return p_context_call_once(context, call, &(stream->error));
}

static int p_ends_in_dot(const p_term *term)
{
    const char *name = p_term_name(term);
    size_t len = p_term_name_length(term);
    while (len > 0) {
        --len;
        if (name[len] == '.')
            return 1;
        else if (name[len] != ' ' && name[len] != '\t' &&
                 name[len] != '\f' && name[len] != '\v')
            break;
    }
    return len == 0 ? -1 : 0;
}

static p_goal_result p_builtin_read_term
    (p_context *context, struct p_read_term_stream *stream)
{
    p_goal_result result;
    int first = 1;
    int end;
    p_term *line;
    p_term *nl = p_term_create_string(context, "\n");

    /* Read lines from the input stream until we find one that
     * ends in a "." character */
    stream->lines = p_term_create_string(context, "\?\?- ");
    while ((result = p_read_term_line(context, stream, &line))
                == P_RESULT_TRUE) {
        stream->lines = p_term_concat_string
            (context, stream->lines, line);
        stream->lines = p_term_concat_string
            (context, stream->lines, nl);
        end = p_ends_in_dot(line);
        if (end > 0)
            break;
        else if (!end)
            first = 0;
    }
    if (result == P_RESULT_FAIL && !first) {
        /* We got at least one non-empty line but no "." */
        stream->error = p_create_syntax_error
            (context, p_term_create_string
                (context, "eof reached; expecting '.' to terminate term"));
        return P_RESULT_ERROR;
    }
    if (result != P_RESULT_TRUE)
        return result;

    /* Consult the contents of the string */
    stream->parent.buffer = p_term_name(stream->lines);
    stream->parent.buffer_len = p_term_name_length(stream->lines);
    if (p_context_consult(context, &(stream->parent)) != 0) {
        stream->error = p_create_syntax_error
            (context, p_term_create_string
                (context, "syntax error while reading term"));
        return P_RESULT_ERROR;
    }
    return P_RESULT_TRUE;
}

/* Read terms from an iostream */
static p_goal_result p_builtin_iostream_readTerm
    (p_context *context, p_term **args, p_term **error)
{
    struct p_read_term_stream stream;
    p_goal_result result;
    memset(&stream, 0, sizeof(stream));
    stream.parent.context = context;
    stream.parent.read_func = p_string_read_func;
    stream.stream = p_term_deref_member(context, args[0]);
    stream.readLine = p_term_create_atom(context, "readLine");
    result = p_builtin_read_term(context, &stream);
    if (stream.error)
        *error = stream.error;
    if (result == P_RESULT_TRUE) {
        if (!p_term_unify
                (context, args[1], stream.parent.read_term,
                 P_BIND_DEFAULT))
            return P_RESULT_FAIL;
    }
    return result;
}
static p_goal_result p_builtin_iostream_readTerm_3
    (p_context *context, p_term **args, p_term **error)
{
    struct p_read_term_stream stream;
    p_goal_result result;
    memset(&stream, 0, sizeof(stream));
    stream.parent.context = context;
    stream.parent.read_func = p_string_read_func;
    stream.parent.generate_vars = 1;
    stream.stream = p_term_deref_member(context, args[0]);
    stream.readLine = p_term_create_atom(context, "readLine");
    result = p_builtin_read_term(context, &stream);
    if (stream.error)
        *error = stream.error;
    if (result == P_RESULT_TRUE) {
        if (!p_term_unify
                (context, args[1], stream.parent.read_term,
                 P_BIND_DEFAULT))
            return P_RESULT_FAIL;
        if (!p_term_unify
                (context, args[2], stream.parent.vars,
                 P_BIND_DEFAULT))
            return P_RESULT_FAIL;
    }
    return result;
}

void _p_db_init_io(p_context *context)
{
    static struct p_builtin const builtins[] = {
        {"$$iostream_readTerm", 2, p_builtin_iostream_readTerm},
        {"$$iostream_readTerm", 3, p_builtin_iostream_readTerm_3},
        {"$$iostream_writeTerm", 3, p_builtin_iostream_writeTerm},
        {"$$print", 2, p_builtin_print},
        {"$$print", 3, p_builtin_print_3},
        {"$$print_byte", 2, p_builtin_print_byte},
        {"$$print_flush", 1, p_builtin_print_flush},
        {"$$print_string", 2, p_builtin_print_string},
        {"$$stdin_read_byte", 1, p_builtin_stdin_read_byte},
        {"$$stdin_read_bytes", 2, p_builtin_stdin_read_bytes},
        {"$$stdin_read_line", 1, p_builtin_stdin_read_line},
        {0, 0, 0}
    };
    _p_db_register_builtins(context, builtins);
}
