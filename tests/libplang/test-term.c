/*
 * plang logic programming language
 * Copyright (C) 2011,2012  Southern Storm Software, Pty Ltd.
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

#include "testcase.h"
#include <plang/term.h>
#include <config.h>
#ifdef HAVE_GC_GC_H
#include <gc/gc.h>
#elif defined(HAVE_GC_H)
#include <gc.h>
#else
#error "libgc is required to build plang"
#endif
#include <stdarg.h>

P_TEST_DECLARE();

enum Token
{
    T_Eof,
    T_Atom,
    T_Variable,
    T_String,
    T_Integer,
    T_Real,
    T_LParen,
    T_RParen,
    T_LSquare,
    T_RSquare,
    T_Comma,
    T_Bar
};

#define MAX_VARS    256

struct parse_state
{
    p_term *name;
    p_term *var_names[MAX_VARS];
    p_term *var_values[MAX_VARS];
    int num_vars;
};

static const char *buffer;
static enum Token token;
static struct parse_state *state = 0;

static void next_token()
{
    int len, ch;
    char *buf;
    while (*buffer == ' ' || *buffer == '\t')
        ++buffer;
    if (*buffer == '\0') {
        token = T_Eof;
        return;
    }
    if (*buffer == '(') {
        token = T_LParen;
        ++buffer;
    } else if (*buffer == ')') {
        token = T_RParen;
        ++buffer;
    } else if (*buffer == '[') {
        if (buffer[1] == ']') {
            token = T_Atom;
            state->name = p_term_nil_atom(context);
            buffer += 2;
        } else {
            token = T_LSquare;
            ++buffer;
        }
    } else if (*buffer == ']') {
        token = T_RSquare;
        ++buffer;
    } else if (*buffer == ',') {
        token = T_Comma;
        ++buffer;
    } else if (*buffer == '|') {
        token = T_Bar;
        ++buffer;
    } else if (*buffer >= 'a' && *buffer <= 'z') {
        len = 1;
        while ((ch = buffer[len]) != '\0') {
            if (ch >= 'a' && ch <= 'z')
                ++len;
            else if (ch >= 'A' && ch <= 'Z')
                ++len;
            else if (ch >= '0' && ch <= '9')
                ++len;
            else if (ch == '_')
                ++len;
            else
                break;
        }
        buf = (char *)malloc(len + 1);
        memcpy(buf, buffer, len);
        buf[len] = '\0';
        buffer += len;
        state->name = p_term_create_atom(context, buf);
        free(buf);
        token = T_Atom;
    } else if (*buffer >= 'A' && *buffer <= 'Z') {
        len = 1;
        while ((ch = buffer[len]) != '\0') {
            if (ch >= 'a' && ch <= 'z')
                ++len;
            else if (ch >= 'A' && ch <= 'Z')
                ++len;
            else if (ch >= '0' && ch <= '9')
                ++len;
            else if (ch == '_')
                ++len;
            else
                break;
        }
        buf = (char *)malloc(len + 1);
        memcpy(buf, buffer, len);
        buf[len] = '\0';
        buffer += len;
        state->name = p_term_create_atom(context, buf);
        free(buf);
        token = T_Variable;
    } else if (*buffer == '"') {
        ++buffer;
        len = 0;
        while (buffer[len] != '\0' && buffer[len] != '"')
            ++len;
        buf = (char *)malloc(len + 1);
        memcpy(buf, buffer, len);
        buf[len] = '\0';
        buffer += len;
        if (*buffer == '"')
            ++buffer;
        state->name = p_term_create_string(context, buf);
        free(buf);
        token = T_String;
    } else if (*buffer == '-' || (*buffer >= '0' && *buffer <= '9')) {
        int is_real = 0;
        len = 1;
        while ((ch = buffer[len]) != '\0') {
            if (ch == '.' || ch == 'e' || ch == 'E' || ch == '-')
                is_real = 1;
            else if (ch < '0' || ch > '9')
                break;
            ++len;
        }
        buf = (char *)malloc(len + 1);
        memcpy(buf, buffer, len);
        buf[len] = '\0';
        buffer += len;
        if (is_real) {
            token = T_Real;
            state->name = p_term_create_real(context, atof(buf));
        } else {
            token = T_Integer;
            state->name = p_term_create_integer(context, atoi(buf));
        }
        free(buf);
    } else {
        P_FAIL("parse error - invalid token");
    }
}

static p_term *parse_expression()
{
    p_term *result = 0;
    switch (token) {
    case T_Atom:
        result = state->name;
        next_token();
        if (token == T_LParen) {
            p_term *args[MAX_VARS];
            int num_args = 0;
            next_token();
            while (token != T_RParen) {
                P_VERIFY(num_args < MAX_VARS);
                args[num_args++] = parse_expression();
                if (token == T_Comma)
                    next_token();
            }
            next_token();
            result = p_term_create_functor_with_args
                (context, result, args, num_args);
        }
        break;
    case T_Variable: {
        int index;
        for (index = 0; index < state->num_vars; ++index) {
            if (state->var_names[index] == state->name) {
                result = state->var_values[index];
                break;
            }
        }
        if (!result) {
            P_VERIFY(state->num_vars < MAX_VARS);
            result = p_term_create_named_variable
                (context, p_term_name(state->name));
            state->var_names[state->num_vars] = state->name;
            state->var_values[state->num_vars] = result;
            ++(state->num_vars);
        }
        next_token();
        break; }
    case T_String:
    case T_Integer:
    case T_Real:
        result = state->name;
        next_token();
        break;
    case T_LSquare: {
        p_term *head;
        p_term *tail = 0;
        next_token();
        while (token != T_RSquare && token != T_Bar) {
            head = parse_expression();
            if (token == T_Comma)
                next_token();
            if (tail) {
                p_term_set_tail
                    (tail, p_term_create_list(context, head, 0));
                tail = p_term_tail(tail);
            } else {
                result = p_term_create_list(context, head, 0);
                tail = result;
            }
        }
        P_VERIFY(result != 0);
        if (token == T_Bar) {
            next_token();
            p_term_set_tail(tail, parse_expression());
        } else {
            p_term_set_tail(tail, p_term_nil_atom(context));
        }
        P_VERIFY(token == T_RSquare);
        next_token();
        break; }
    default:
        P_FAIL("parse error - expecting an identifier or list");
        break;
    }
    return result;
}

static void clear_parse_state()
{
    if (state) {
        GC_FREE(state);
        state = 0;
    }
}

static p_term *parse_term(const char *str)
{
    p_term *result;
    if (!state)
        state = GC_MALLOC_UNCOLLECTABLE(sizeof(struct parse_state));
    buffer = str;
    next_token();
    if (token == T_Eof)
        P_FAIL("parse error - missing expression");
    result = parse_expression();
    if (token != T_Eof)
        P_FAIL("parse error - expecting eof");
    return result;
}

struct print_string
{
    char *buf;
    size_t len;
    size_t max_len;
};

static void term_print(void *_data, const char *format, ...)
{
    struct print_string *data = (struct print_string *)_data;
    va_list va;
    int size;
    va_start(va, format);
    size = vsnprintf(data->buf + data->len, data->max_len - data->len,
                     format, va);
    if (((size_t)size) >= (data->max_len - data->len)) {
        size_t new_len = (data->len + (size_t)size + 1024) & ~1023;
        data->buf = (char *)GC_REALLOC(data->buf, new_len);
        data->max_len = new_len;
        size = vsnprintf(data->buf + data->len,
                         data->max_len - data->len, format, va);
    }
    data->len += (size_t)size;
    va_end(va);
}

static char *term_to_string(p_term *term)
{
    struct print_string data;
    data.buf = (char *)GC_MALLOC(1024);;
    data.len = 0;
    data.max_len = 1024;
    p_term_print(context, term, term_print, &data);
    return data.buf;
}

static void test_atom()
{
    p_term *atom1;
    p_term *atom2;
    p_term *atom3;
    p_term *atom4;
    int value;
    char name[64];

    atom1 = p_term_create_atom(context, "foo");
    P_VERIFY(atom1 != 0);
    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_COMPARE(p_term_type(atom1), P_TERM_ATOM);

    atom2 = p_term_create_atom(context, "foo");
    P_VERIFY(atom1 == atom2);

    atom3 = p_term_create_atom(context, "bar");
    P_VERIFY(atom3 != 0);
    P_VERIFY(atom3 != atom1);
    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(atom3), "bar") == 0);

    atom4 = p_term_create_atom(context, 0);
    P_VERIFY(strcmp(p_term_name(atom4), "") == 0);
    P_VERIFY(p_term_create_atom(context, "") == atom4);
    P_VERIFY(p_term_create_atom(context, 0) == atom4);

    /* Load up the hash table to check overflow handling */
    for (value = 0; value < 1024; ++value) {
        sprintf(name, "%d", value);
        p_term_create_atom(context, name);
    }
    for (value = 0; value < 1024; ++value) {
        sprintf(name, "%d", value);
        atom4 = p_term_create_atom(context, name);
        P_VERIFY(strcmp(p_term_name(atom4), name) == 0);
        P_COMPARE(p_term_type(atom4), P_TERM_ATOM);
    }

    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(atom3), "bar") == 0);
}

static void test_standard_atoms()
{
    p_term *nil_atom = p_term_nil_atom(context);
    p_term *prototype_atom = p_term_prototype_atom(context);
    p_term *class_name_atom = p_term_class_name_atom(context);

    P_VERIFY(nil_atom != 0);
    P_VERIFY(prototype_atom != 0);
    P_VERIFY(class_name_atom != 0);

    P_VERIFY(strcmp(p_term_name(nil_atom), "[]") == 0);
    P_VERIFY(strcmp(p_term_name(prototype_atom), "prototype") == 0);
    P_VERIFY(strcmp(p_term_name(class_name_atom), "className") == 0);

    P_VERIFY(p_term_nil_atom(context) == nil_atom);
    P_VERIFY(p_term_prototype_atom(context) == prototype_atom);
    P_VERIFY(p_term_class_name_atom(context) == class_name_atom);
}

static void test_string()
{
    p_term *string1;
    p_term *string2;
    p_term *string3;
    p_term *string4;

    string1 = p_term_create_string(context, "foo");
    P_VERIFY(string1 != 0);
    P_VERIFY(strcmp(p_term_name(string1), "foo") == 0);
    P_COMPARE(p_term_type(string1), P_TERM_STRING);
    P_COMPARE(p_term_name_length(string1), 3);

    string2 = p_term_create_string(context, "foo");
    P_VERIFY(string1 != string2);   /* Strings are not hashed */
    P_VERIFY(strcmp(p_term_name(string2), "foo") == 0);
    P_COMPARE(p_term_name_length(string2), 3);

    string3 = p_term_create_string(context, "bar");
    P_VERIFY(string3 != 0);
    P_VERIFY(string3 != string1);
    P_VERIFY(string3 != string2);
    P_VERIFY(strcmp(p_term_name(string1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(string2), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(string3), "bar") == 0);
    P_COMPARE(p_term_name_length(string3), 3);

    string4 = p_term_create_string(context, 0);
    P_VERIFY(strcmp(p_term_name(string4), "") == 0);
    P_COMPARE(p_term_name_length(string4), 0);

    string4 = p_term_create_string(context, "");
    P_VERIFY(strcmp(p_term_name(string4), "") == 0);
    P_COMPARE(p_term_name_length(string4), 0);

    string4 = p_term_create_string_n(context, "foo\0bar", 7);
    P_VERIFY(memcmp(p_term_name(string4), "foo\0bar", 7) == 0);
    P_COMPARE(p_term_name_length(string4), 7);
}

static void test_integer()
{
    p_term *int1;
    p_term *int2;
    p_term *int3;
    p_term *int4;
    p_term *int5;
    p_term *var;

    int1 = p_term_create_integer(context, 0);
    P_VERIFY(int1 != 0);
    P_COMPARE(p_term_integer_value(int1), 0);
    P_COMPARE(p_term_type(int1), P_TERM_INTEGER);

    int2 = p_term_create_integer(context, 124);
    P_COMPARE(p_term_integer_value(int2), 124);

    int3 = p_term_create_integer(context, -124);
    P_COMPARE(p_term_integer_value(int3), -124);

    int4 = p_term_create_integer(context, 0x7fffffff);
    P_COMPARE(p_term_integer_value(int4), 0x7fffffff);

    int5 = p_term_create_integer(context, (int)(-0x7fffffff - 1));
    P_COMPARE(p_term_integer_value(int5), (int)(-0x7fffffff - 1));

    P_COMPARE(p_term_integer_value(int1), 0);
    P_COMPARE(p_term_integer_value(int2), 124);
    P_COMPARE(p_term_integer_value(int3), -124);
    P_COMPARE(p_term_integer_value(int4), 0x7fffffff);
    P_COMPARE(p_term_integer_value(int5), (int)(-0x7fffffff - 1));

    P_COMPARE(p_term_integer_value(0), 0);

    var = p_term_create_variable(context);
    P_COMPARE(p_term_integer_value(var), 0);

    P_VERIFY(p_term_bind_variable(context, var, int2, P_BIND_DEFAULT));
    P_COMPARE(p_term_integer_value(var), 124);
}

static void test_real()
{
    p_term *real1;
    p_term *real2;
    p_term *real3;
    p_term *real4;
    p_term *real5;
    p_term *var;

    real1 = p_term_create_real(context, 0.0);
    P_VERIFY(real1 != 0);
    P_COMPARE(p_term_real_value(real1), 0.0);
    P_COMPARE(p_term_type(real1), P_TERM_REAL);

    real2 = p_term_create_real(context, 124.0);
    P_COMPARE(p_term_real_value(real2), 124.0);

    real3 = p_term_create_real(context, -124.5);
    P_COMPARE(p_term_real_value(real3), -124.5);

    real4 = p_term_create_real(context, 1e12);
    P_COMPARE(p_term_real_value(real4), 1e12);

    real5 = p_term_create_real(context, 1e-12);
    P_COMPARE(p_term_real_value(real5), 1e-12);

    P_COMPARE(p_term_real_value(real1), 0);
    P_COMPARE(p_term_real_value(real2), 124.0);
    P_COMPARE(p_term_real_value(real3), -124.5);
    P_COMPARE(p_term_real_value(real4), 1e12);
    P_COMPARE(p_term_real_value(real5), 1e-12)

    P_COMPARE(p_term_real_value(0), 0.0);

    var = p_term_create_variable(context);
    P_COMPARE(p_term_real_value(var), 0.0);

    P_VERIFY(p_term_bind_variable(context, var, real2, P_BIND_DEFAULT));
    P_COMPARE(p_term_real_value(var), 124.0);
}

static void test_list()
{
    p_term *member1;
    p_term *member2;
    p_term *member3;
    p_term *nil;
    p_term *list1;
    p_term *list2;
    p_term *var;

    member1 = p_term_create_atom(context, "foo");
    member2 = p_term_create_string(context, "bar");
    member3 = p_term_create_integer(context, 42);
    nil = p_term_nil_atom(context);

    list1 = p_term_create_list(context, member1, nil);
    P_VERIFY(p_term_head(list1) == member1);
    P_VERIFY(p_term_tail(list1) == nil);
    P_COMPARE(p_term_type(list1), P_TERM_LIST);

    list2 = p_term_create_list(context, member1,
                p_term_create_list(context, member2,
                    p_term_create_list(context, member3, nil)));
    P_VERIFY(p_term_head(list2) == member1);
    P_VERIFY(p_term_head(p_term_tail(list2)) == member2);
    P_VERIFY(p_term_head(p_term_tail(p_term_tail(list2))) == member3);
    P_VERIFY(p_term_tail(p_term_tail(p_term_tail(list2))) == nil);

    P_VERIFY(p_term_head(0) == 0);
    P_VERIFY(p_term_tail(0) == 0);

    var = p_term_create_variable(context);
    P_VERIFY(p_term_head(var) == 0);
    P_VERIFY(p_term_tail(var) == 0);

    P_VERIFY(p_term_bind_variable(context, var, list1, P_BIND_DEFAULT));
    P_VERIFY(p_term_head(var) == member1);
    P_VERIFY(p_term_tail(var) == nil);
}

static void test_variable()
{
    p_term *var1;
    p_term *var2;
    p_term *var3;
    p_term *var4;

    var1 = p_term_create_variable(context);
    P_VERIFY(p_term_name(var1) == 0);
    P_COMPARE(p_term_type(var1), P_TERM_VARIABLE);

    var2 = p_term_create_named_variable(context, "foo");
    P_VERIFY(strcmp(p_term_name(var2), "foo") == 0);
    P_COMPARE(p_term_type(var2), P_TERM_VARIABLE);

    var3 = p_term_create_named_variable(context, "");
    P_VERIFY(p_term_name(var3) == 0);
    P_COMPARE(p_term_type(var3), P_TERM_VARIABLE);

    var4 = p_term_create_named_variable(context, 0);
    P_VERIFY(p_term_name(var4) == 0);
    P_COMPARE(p_term_type(var4), P_TERM_VARIABLE);

    P_VERIFY(p_term_deref(var1) == var1);

    P_VERIFY(p_term_bind_variable(context, var1, var2, P_BIND_DEFAULT));
    P_VERIFY(strcmp(p_term_name(var1), "foo") == 0);
    P_COMPARE(p_term_type(var1), P_TERM_VARIABLE);

    /* Occurs check fail */
    P_VERIFY(!p_term_bind_variable(context, var2, var1, P_BIND_DEFAULT));
    P_VERIFY(strcmp(p_term_name(var1), "foo") == 0);
    P_COMPARE(p_term_type(var1), P_TERM_VARIABLE);

    P_VERIFY(p_term_bind_variable(context, var1, var3, P_BIND_DEFAULT));
    P_VERIFY(p_term_name(var1) == 0);
    P_COMPARE(p_term_type(var1), P_TERM_VARIABLE);
    P_VERIFY(p_term_name(var2) == 0);
    P_COMPARE(p_term_type(var2), P_TERM_VARIABLE);

    P_VERIFY(p_term_deref(var1) == var3);
    P_VERIFY(p_term_deref(0) == 0);
}

static void test_member_variable()
{
    p_term *object;
    p_term *name;
    p_term *var1;

    object = p_term_create_variable(context);
    name = p_term_create_atom(context, "foo");

    P_VERIFY(!p_term_create_member_variable(context, object, 0, 0));
    P_VERIFY(!p_term_create_member_variable(context, 0, name, 0));
    P_VERIFY(!p_term_create_member_variable(context, object, object, 0));

    var1 = p_term_create_member_variable(context, object, name, 0);
    P_COMPARE(p_term_type(var1), P_TERM_MEMBER_VARIABLE);
    P_VERIFY(strcmp(p_term_name(var1), p_term_name(name)) == 0);
    P_VERIFY(p_term_object(var1) == object);
}

static void test_functor()
{
    p_term *functor1;
    p_term *functor2;
    p_term *name;
    p_term *vars[5];

    name = p_term_create_atom(context, "foo");
    vars[0] = p_term_create_variable(context);
    vars[1] = p_term_create_variable(context);
    vars[2] = p_term_create_variable(context);
    vars[3] = p_term_create_variable(context);
    vars[4] = p_term_create_variable(context);

    P_VERIFY(p_term_create_functor(context, 0, 0) == 0);
    P_VERIFY(p_term_create_functor(context, name, -1) == 0);
    P_VERIFY(p_term_create_functor(context, vars[0], 0) == 0);

    P_VERIFY(p_term_create_functor(context, name, 0) == name);

    functor1 = p_term_create_functor(context, name, 5);
    P_COMPARE(p_term_type(functor1), P_TERM_FUNCTOR);
    P_VERIFY(p_term_functor(functor1) == name);
    P_COMPARE(p_term_arg_count(functor1), 5);
    P_VERIFY(strcmp(p_term_name(functor1), "foo") == 0);

    P_VERIFY(p_term_arg(functor1, -1) == 0);
    P_VERIFY(p_term_arg(functor1, 0) == 0);
    P_VERIFY(p_term_arg(functor1, 1) == 0);
    P_VERIFY(p_term_arg(functor1, 2) == 0);
    P_VERIFY(p_term_arg(functor1, 3) == 0);
    P_VERIFY(p_term_arg(functor1, 4) == 0);
    P_VERIFY(p_term_arg(functor1, 5) == 0);

    P_VERIFY(!p_term_bind_functor_arg(0, 0, vars[0]));
    P_VERIFY(!p_term_bind_functor_arg(vars[0], 0, vars[1]));
    P_VERIFY(!p_term_bind_functor_arg(functor1, 0, 0));

    P_VERIFY(!p_term_bind_functor_arg(functor1, -1, vars[0]));
    P_VERIFY(p_term_bind_functor_arg(functor1, 0, vars[0]));
    P_VERIFY(p_term_bind_functor_arg(functor1, 1, vars[1]));
    P_VERIFY(p_term_bind_functor_arg(functor1, 2, vars[2]));
    P_VERIFY(p_term_bind_functor_arg(functor1, 3, vars[3]));
    P_VERIFY(p_term_bind_functor_arg(functor1, 4, vars[4]));
    P_VERIFY(!p_term_bind_functor_arg(functor1, 5, vars[4]));

    P_VERIFY(!p_term_bind_functor_arg(functor1, 3, vars[3]));

    P_VERIFY(p_term_arg(functor1, -1) == 0);
    P_VERIFY(p_term_arg(functor1, 0) == vars[0]);
    P_VERIFY(p_term_arg(functor1, 1) == vars[1]);
    P_VERIFY(p_term_arg(functor1, 2) == vars[2]);
    P_VERIFY(p_term_arg(functor1, 3) == vars[3]);
    P_VERIFY(p_term_arg(functor1, 4) == vars[4]);
    P_VERIFY(p_term_arg(functor1, 5) == 0);

    functor2 = p_term_create_functor_with_args(context, name, vars, 5);
    P_COMPARE(p_term_type(functor2), P_TERM_FUNCTOR);
    P_VERIFY(p_term_functor(functor2) == name);
    P_COMPARE(p_term_arg_count(functor2), 5);
    P_VERIFY(strcmp(p_term_name(functor2), "foo") == 0);

    P_VERIFY(p_term_arg(functor2, -1) == 0);
    P_VERIFY(p_term_arg(functor2, 0) == vars[0]);
    P_VERIFY(p_term_arg(functor2, 1) == vars[1]);
    P_VERIFY(p_term_arg(functor2, 2) == vars[2]);
    P_VERIFY(p_term_arg(functor2, 3) == vars[3]);
    P_VERIFY(p_term_arg(functor2, 4) == vars[4]);
    P_VERIFY(p_term_arg(functor2, 5) == 0);

    P_VERIFY(p_term_create_functor_with_args(context, name, vars, 0) == name);
    P_VERIFY(p_term_create_functor_with_args(context, vars[0], vars, 0) == 0);
}

static void test_object()
{
    p_term *base_atom;
    p_term *base_class;
    p_term *sub_atom;
    p_term *sub_class;
    p_term *obj1;
    p_term *obj2;
    p_term *prop_atom;
    p_term *prop_value;
    int index;
    char namebuf[64];

    base_atom = p_term_create_atom(context, "Base");
    sub_atom = p_term_create_atom(context, "Sub");

    base_class = p_term_create_class_object(context, base_atom, 0);
    P_COMPARE(p_term_type(base_class), P_TERM_OBJECT);
    P_VERIFY(p_term_is_class_object(context, base_class));
    P_VERIFY(!p_term_is_instance_object(context, base_class));
    P_VERIFY(p_term_inherits(context, base_class, base_class));

    P_VERIFY(p_term_property(context, base_class, p_term_prototype_atom(context)) == 0);
    P_VERIFY(p_term_property(context, base_class, p_term_class_name_atom(context)) == base_atom);

    P_VERIFY(p_term_own_property(context, base_class, p_term_prototype_atom(context)) == 0);
    P_VERIFY(p_term_own_property(context, base_class, p_term_class_name_atom(context)) == base_atom);

    sub_class = p_term_create_class_object(context, sub_atom, base_class);
    P_COMPARE(p_term_type(sub_class), P_TERM_OBJECT);
    P_VERIFY(p_term_is_class_object(context, sub_class));
    P_VERIFY(!p_term_is_instance_object(context, sub_class));
    P_VERIFY(p_term_inherits(context, sub_class, base_class));
    P_VERIFY(p_term_inherits(context, sub_class, sub_class));
    P_VERIFY(!p_term_inherits(context, base_class, sub_class));
    P_VERIFY(!p_term_is_instance_of(context, sub_class, base_class));

    P_VERIFY(p_term_property(context, sub_class, p_term_prototype_atom(context)) == base_class);
    P_VERIFY(p_term_property(context, sub_class, p_term_class_name_atom(context)) == sub_atom);

    P_VERIFY(p_term_own_property(context, sub_class, p_term_prototype_atom(context)) == base_class);
    P_VERIFY(p_term_own_property(context, sub_class, p_term_class_name_atom(context)) == sub_atom);

    obj1 = p_term_create_object(context, base_class);
    P_COMPARE(p_term_type(obj1), P_TERM_OBJECT);
    P_VERIFY(!p_term_is_class_object(context, obj1));
    P_VERIFY(p_term_is_instance_object(context, obj1));
    P_VERIFY(p_term_inherits(context, obj1, base_class));
    P_VERIFY(p_term_is_instance_of(context, obj1, base_class));

    P_VERIFY(p_term_property(context, obj1, p_term_prototype_atom(context)) == base_class);
    P_VERIFY(p_term_property(context, obj1, p_term_class_name_atom(context)) == base_atom);

    P_VERIFY(p_term_own_property(context, obj1, p_term_prototype_atom(context)) == base_class);
    P_VERIFY(p_term_own_property(context, obj1, p_term_class_name_atom(context)) == 0);

    obj2 = p_term_create_object(context, sub_class);
    P_COMPARE(p_term_type(obj2), P_TERM_OBJECT);
    P_VERIFY(!p_term_is_class_object(context, obj2));
    P_VERIFY(p_term_is_instance_object(context, obj2));
    P_VERIFY(p_term_inherits(context, obj2, base_class));
    P_VERIFY(p_term_inherits(context, obj2, sub_class));
    P_VERIFY(p_term_is_instance_of(context, obj2, base_class));
    P_VERIFY(p_term_is_instance_of(context, obj2, sub_class));
    P_VERIFY(!p_term_is_instance_of(context, obj2, obj2));

    P_VERIFY(p_term_property(context, obj2, p_term_prototype_atom(context)) == sub_class);
    P_VERIFY(p_term_property(context, obj2, p_term_class_name_atom(context)) == sub_atom);

    P_VERIFY(p_term_own_property(context, obj2, p_term_prototype_atom(context)) == sub_class);
    P_VERIFY(p_term_own_property(context, obj2, p_term_class_name_atom(context)) == 0);

    for (index = 1; index < 100; ++index) {
        sprintf(namebuf, "name%d", index);
        prop_atom = p_term_create_atom(context, namebuf);
        prop_value = p_term_create_integer(context, index);
        P_VERIFY(p_term_add_property(context, obj2, prop_atom, prop_value));
    }
    for (index = 99; index >= 1; --index) {
        sprintf(namebuf, "name%d", index);
        prop_atom = p_term_create_atom(context, namebuf);
        prop_value = p_term_property(context, obj2, prop_atom);
        P_COMPARE(p_term_integer_value(prop_value), index);
        prop_value = p_term_own_property(context, obj2, prop_atom);
        P_COMPARE(p_term_integer_value(prop_value), index);
        prop_value = p_term_own_property(context, sub_class, prop_atom);
        P_VERIFY(prop_value == 0);
    }

    P_VERIFY(p_term_property(context, obj2, p_term_prototype_atom(context)) == sub_class);
    P_VERIFY(p_term_property(context, obj2, p_term_class_name_atom(context)) == sub_atom);

    P_VERIFY(p_term_own_property(context, obj2, p_term_prototype_atom(context)) == sub_class);
    P_VERIFY(p_term_own_property(context, obj2, p_term_class_name_atom(context)) == 0);

    P_VERIFY(!p_term_add_property(context, 0, 0, 0));
    P_VERIFY(!p_term_add_property(context, obj2, 0, 0));
    P_VERIFY(!p_term_add_property(context, sub_atom, sub_atom, 0));
    P_VERIFY(!p_term_add_property(context, obj2, obj1, 0));
    P_VERIFY(!p_term_add_property(context, obj2, p_term_prototype_atom(context), sub_atom));
    P_VERIFY(!p_term_add_property(context, obj2, p_term_class_name_atom(context), sub_atom));

    P_VERIFY(!p_term_property(context, 0, 0));
    P_VERIFY(!p_term_property(context, sub_atom, sub_atom));
    P_VERIFY(!p_term_property(context, obj2, 0));
    P_VERIFY(!p_term_property(context, obj2, obj1));

    P_VERIFY(!p_term_own_property(context, 0, 0));
    P_VERIFY(!p_term_own_property(context, sub_atom, sub_atom));
    P_VERIFY(!p_term_own_property(context, obj2, 0));
    P_VERIFY(!p_term_own_property(context, obj2, obj1));
}

static void test_predicate()
{
    p_term *pred1;
    p_term *name;

    name = p_term_create_atom(context, "foo");

    P_VERIFY(p_term_create_predicate(context, 0, 0) == 0);
    P_VERIFY(p_term_create_predicate(context, name, -1) == 0);
    P_VERIFY(p_term_create_predicate(context, p_term_create_variable(context), 0) == 0);

    pred1 = p_term_create_predicate(context, name, 3);
    P_COMPARE(p_term_type(pred1), P_TERM_PREDICATE);
    P_VERIFY(p_term_functor(pred1) == name);
    P_COMPARE(p_term_arg_count(pred1), 3);
    P_VERIFY(strcmp(p_term_name(pred1), "foo") == 0);
}

#define P_BIND_FAIL         0x1000
#define P_BIND_NO_REVERSE   0x2000

static void test_unify()
{
    struct unify_type
    {
        const char *row;
        const char *term1;
        const char *term2;
        int flags;
        const char *result;
    };
    static struct unify_type const unify_data[] = {
        {"null_var", 0, "X", P_BIND_DEFAULT | P_BIND_FAIL | P_BIND_NO_REVERSE, 0},
        {"var_null", "X", 0, P_BIND_DEFAULT | P_BIND_FAIL | P_BIND_NO_REVERSE, 0},

        {"var_atom", "X", "atom", P_BIND_DEFAULT, "atom"},
        {"atom_var", "atom", "X", P_BIND_DEFAULT, "atom"},

        {"var_var_1", "X", "X", P_BIND_DEFAULT, "X"},
        {"var_var_2", "X", "Y", P_BIND_DEFAULT, "Y"},
        {"var_var_3", "X", "Y", P_BIND_EQUALITY | P_BIND_FAIL, 0},
        {"var_var_4", "X", "X", P_BIND_EQUALITY, "X"},

        {"atom_atom_1", "atom", "mota", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"atom_atom_2", "atom", "atom", P_BIND_DEFAULT, "atom"},

        {"atom_functor_2", "atom", "foo(a)", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"atom_functor_3", "foo(a)", "atom", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"atom_functor_4", "atom", "atom()", P_BIND_DEFAULT | P_BIND_NO_REVERSE, "atom"},

        {"functor_functor_1", "foo(a)", "foo(a)", P_BIND_DEFAULT, "foo(a)"},
        {"functor_functor_2", "foo(a, b)", "foo(a)", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"functor_functor_3", "foo(a)", "foo(X)", P_BIND_DEFAULT, "foo(a)"},
        {"functor_functor_4", "foo(X)", "foo(a)", P_BIND_DEFAULT, "foo(a)"},
        {"functor_functor_5", "foo(X, Y)", "foo(Y, Z)", P_BIND_DEFAULT, "foo(Z, Z)"},
        {"functor_functor_6", "foo(a)", "foo(b)", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"list_atom_1", "[a]", "a", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"list_atom_2", "[a]", "[]", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"list_list_1", "[]", "[]", P_BIND_DEFAULT, "[]"},
        {"list_list_2", "[a]", "[a]", P_BIND_DEFAULT, "[a]"},
        {"list_list_3", "[a]", "[b]", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"list_list_4", "[a|T]", "[a|U]", P_BIND_DEFAULT, "[a|U]"},
        {"list_list_5", "[a|T]", "[a, b, c]", P_BIND_DEFAULT, "[a, b, c]"},
        {"list_list_6", "[a, b|T]", "[a, b, c]", P_BIND_DEFAULT, "[a, b, c]"},
        {"list_list_7", "[a, b|[]]", "[a, b|T]", P_BIND_DEFAULT | P_BIND_NO_REVERSE, "[a, b]"},

        {"string_atom_1", "\"foo\"", "foo", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"string_var_1", "\"foo\"", "Foo", P_BIND_DEFAULT, "\"foo\""},
        {"string_var_2", "Foo", "\"foo\"", P_BIND_DEFAULT, "\"foo\""},

        {"string_string_1", "\"foo\"", "\"foo\"", P_BIND_DEFAULT, "\"foo\""},
        {"string_string_2", "\"foo\"", "\"bar\"", P_BIND_DEFAULT | P_BIND_FAIL, 0},
        {"string_string_3", "\"foo\"", "\"foobar\"", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"int_atom_1", "42", "foo", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"int_var_1", "42", "X", P_BIND_DEFAULT, "42"},
        {"int_var_2", "X", "42", P_BIND_DEFAULT, "42"},

        {"int_int_1", "42", "42", P_BIND_DEFAULT, "42"},
        {"int_int_2", "42", "41", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"real_atom_1", "42", "foo", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"real_var_1", "42.5", "X", P_BIND_DEFAULT, "42.5"},
        {"real_var_2", "X", "42.5", P_BIND_DEFAULT, "42.5"},

        {"real_real_1", "42.5", "42.5", P_BIND_DEFAULT, "42.5"},
        {"real_real_2", "42.5", "41.5", P_BIND_DEFAULT | P_BIND_FAIL, 0},

        {"one_way_1", "X", "foo(Y, Z)", P_BIND_DEFAULT | P_BIND_ONE_WAY, "foo(Y, Z)"},
        {"one_way_2", "foo(Y, Z)", "X", P_BIND_DEFAULT | P_BIND_ONE_WAY | P_BIND_FAIL, 0},
        {"one_way_3", "Y", "X", P_BIND_DEFAULT | P_BIND_ONE_WAY, "X"},
    };
    #define unify_data_len (sizeof(unify_data) / sizeof(struct unify_type))

    size_t index;
    p_term *term1;
    p_term *term2;
    char *result1;
    char *result2;
    int flags, unify_result;
    for (index = 0; index < unify_data_len; ++index) {
        void *marker = p_context_mark_trail(context);
        clear_parse_state();
        P_TEST_SET_ROW(unify_data[index].row);
        term1 = unify_data[index].term1
                    ? parse_term(unify_data[index].term1) : 0;
        term2 = unify_data[index].term2
                    ? parse_term(unify_data[index].term2) : 0;
        flags = unify_data[index].flags;
        unify_result = p_term_unify(context, term1, term2, flags);
        if (flags & P_BIND_FAIL) {
            P_VERIFY(!unify_result);
        } else {
            P_VERIFY(unify_result);
            result1 = term_to_string(term1);
            result2 = term_to_string(term2);
            P_VERIFY(!strcmp(result1, unify_data[index].result));
            P_VERIFY(!strcmp(result2, unify_data[index].result));
            p_context_backtrack_trail(context, marker);
        }
        /* Did the backtrack return to the original state? */
        if (flags & P_BIND_NO_REVERSE)
            continue;
        result1 = term_to_string(term1);
        result2 = term_to_string(term2);
        P_VERIFY(!strcmp(result1, unify_data[index].term1));
        P_VERIFY(!strcmp(result2, unify_data[index].term2));
    }
    clear_parse_state();
}

static void test_precedes()
{
    struct precedes_type
    {
        const char *row;
        const char *term1;
        const char *term2;
        int result;
    };
    static struct precedes_type const precedes_data[] = {
        {"null_var", 0, "X", -1},
        {"var_null", "X", 0, 1},

        {"var_1", "X", "X", 0},
        {"var_2", "X", "Y", 2},
        {"var_real_1", "X", "42.5", -1},
        {"var_real_2", "42.5", "X", 1},
        {"var_int_1", "X", "42", -1},
        {"var_int_2", "42", "X", 1},
        {"var_string_1", "X", "\"foo\"", -1},
        {"var_string_2", "\"foo\"", "X", 1},
        {"var_atom_1", "X", "foo", -1},
        {"var_atom_2", "foo", "X", 1},
        {"var_functor_1", "X", "f(a)", -1},
        {"var_functor_2", "f(a)", "X", 1},

        {"real_1", "42.5", "42.0", 1},
        {"real_2", "42.0", "42.5", -1},
        {"real_3", "42.5", "42.5", 0},
        {"real_int_1", "42.5", "42", -1},
        {"real_int_2", "42", "42.5", 1},
        {"real_string_1", "42.5", "\"foo\"", -1},
        {"real_string_2", "\"foo\"", "42.5", 1},
        {"real_atom_1", "42.5", "foo", -1},
        {"real_atom_2", "foo", "42.5", 1},
        {"real_functor_1", "42.5", "f(a)", -1},
        {"real_functor_1", "f(a)", "42.5", 1},

        {"int_1", "42", "40", 1},
        {"int_2", "40", "42", -1},
        {"int_3", "42", "42", 0},
        {"int_4", "-42", "42", -1},
        {"int_5", "42", "-42", 1},
        {"int_string_1", "42", "\"foo\"", -1},
        {"int_string_2", "\"foo\"", "42", 1},
        {"int_atom_1", "42", "foo", -1},
        {"int_atom_2", "foo", "42", 1},
        {"int_functor_1", "42", "f(a)", -1},
        {"int_functor_1", "f(a)", "42", 1},

        {"string_1", "\"foo\"", "\"bar\"", 1},
        {"string_2", "\"bar\"", "\"foo\"", -1},
        {"string_3", "\"foo\"", "\"foo\"", 0},
        {"string_4", "\"foo\"", "\"foox\"", -1},
        {"string_atom_1", "\"foo\"", "foo", -1},
        {"string_atom_2", "foo", "\"foo\"", 1},
        {"string_functor_1", "\"foo\"", "f(a)", -1},
        {"string_functor_1", "f(a)", "\"foo\"", 1},

        {"atom_1", "foo", "bar", 1},
        {"atom_2", "bar", "foo", -1},
        {"atom_3", "foo", "foo", 0},
        {"atom_4", "foo", "foox", -1},
        {"atom_functor_1", "foo", "f(a)", -1},
        {"atom_functor_1", "f(a)", "foo", 1},

        {"functor_1", "f(a)", "f(a,b)", -1},
        {"functor_2", "f(a,b)", "f(a)", 1},
        {"functor_3", "f(a,b)", "[a|b]", 1},    // "f" > "."
        {"functor_4", "[a|b]", "f(a,b)", -1},
        {"functor_5", "f(a)", "[a|b]", -1},
        {"functor_6", "[a|b]", "f(a)", 1},
        {"functor_7", "f(a,b,X)", "f(a,b,X)", 0},
        {"functor_8", "f(a,b,X)", "f(a,b,Y)", 2},
        {"functor_9", "[a,b,X]", "[a,b,X]", 0},
        {"functor_10", "[a,b,X]", "[a,b,Y]", 2},
        {"functor_11", "[a,b]", "[a,b,c]", -1},
        {"functor_12", "[a,b|X]", "[a,b|Y]", 2},
    };
    #define precedes_data_len (sizeof(precedes_data) / sizeof(struct precedes_type))

    size_t index;
    p_term *term1;
    p_term *term2;
    int expected;
    int precedes_result;
    for (index = 0; index < precedes_data_len; ++index) {
        clear_parse_state();
        P_TEST_SET_ROW(precedes_data[index].row);
        term1 = precedes_data[index].term1
                    ? parse_term(precedes_data[index].term1) : 0;
        term2 = precedes_data[index].term2
                    ? parse_term(precedes_data[index].term2) : 0;
        expected = precedes_data[index].result;
        precedes_result = p_term_precedes(context, term1, term2);
        if (expected != 2) {
            P_COMPARE(precedes_result, expected);
        } else {
            /* Ordering is dependent upon ordering of X and Y vars */
            term1 = parse_term("X");
            term2 = parse_term("Y");
            expected = (term1 < term2) ? -1 : 1;
            P_COMPARE(precedes_result, expected);
        }
    }
    clear_parse_state();
}

static void test_witness()
{
    struct witness_type
    {
        const char *row;
        const char *term;
        const char *result;
    };
    static struct witness_type const witness_data[] = {
        {"null", 0, "[]"},
        {"atom_1", "a", "[]"},
        {"atom_2", "[]", "[]"},
        {"functor_1", "f(X)", "[X]"},
        {"functor_2", "f(X, X)", "[X]"},
        {"functor_3", "f(X, Y)", "[Y, X]"},
        {"list_1", "[X, Y, a, Z]", "[Z, Y, X]"},
        {"list_2", "[X, Y, a, Z|W]", "[W, Z, Y, X]"},
        {"string_1", "\"a\"", "[]"},
        {"integer_1", "1", "[]"},
        {"real_1", "1.5", "[]"},
    };
    #define witness_data_len (sizeof(witness_data) / sizeof(struct witness_type))

    size_t index;
    p_term *term;
    p_term *witness;
    p_term *subgoal;
    char *result;
    for (index = 0; index < witness_data_len; ++index) {
        clear_parse_state();
        P_TEST_SET_ROW(witness_data[index].row);
        term = witness_data[index].term
                    ? parse_term(witness_data[index].term) : 0;
        witness = p_term_witness(context, term, &subgoal);
        result = term_to_string(witness);
        P_VERIFY(!strcmp(result, witness_data[index].result));
    }
    clear_parse_state();
}

int _p_term_next_utf8(const char *str, size_t len, size_t *size);

static void test_utf8()
{
    struct utf8_type
    {
        const char *row;
        const char *str1;
        int index;
        int ch;
        size_t size;
        size_t name_length;
    };
    static struct utf8_type const utf8_data[] = {
        {"null", 0, 0, -1, 0, 0},
        {"empty", "", 3, -1, 0, 0},

        {"xyz_1", "xyz", 0, 'x', 1, 3},
        {"xyz_2", "xyz", 1, 'y', 1, 3},
        {"xyz_3", "xyz", 2, 'z', 1, 3},
        {"xyz_4", "xyz", 3, -1, 0, 3},

        {"unicode_1", "\xC1y1", 0, -1, 1, 3},
        {"unicode_2", "\xC1\x81", 0, 0x41, 2, 1},
        {"unicode_3", "\xE1\x81", 0, -1, 2, 1},
        {"unicode_4", "\xE1\x81y", 0, -1, 2, 2},
        {"unicode_5", "y\xE1\x81\x81", 1, 0x1041, 3, 2},
        {"unicode_6", "y\xF1\x81\x81z", 1, -1, 3, 3},
        {"unicode_7", "\xF1\xC1\x81\x81", 0, -1, 1, 3},
        {"unicode_8", "\xF1\x81\x81\x81", 0, 0x41041, 4, 1},
        {"unicode_9", "\xF9\x81\x81\x81\x81", 0, -1, 5, 1}
    };
    #define utf8_data_len    (sizeof(utf8_data) / sizeof(struct utf8_type))

    size_t index, size;
    int ch, len;
    const char *s1;
    p_term *term;
    for (index = 0; index < utf8_data_len; ++index) {
        P_TEST_SET_ROW(utf8_data[index].row);
        s1 = utf8_data[index].str1;
        if (s1) {
            len = ((int)(strlen(s1))) - utf8_data[index].index;
            if (len < 0)
                len = 0;
            s1 += utf8_data[index].index;
        } else {
            len = 0;
        }

        size = ~(size_t)0;
        ch = _p_term_next_utf8(s1, len, &size);

        P_COMPARE(ch, utf8_data[index].ch);
        P_COMPARE(size, utf8_data[index].size);

        term = p_term_create_string(context, utf8_data[index].str1);
        P_COMPARE(p_term_name_length_utf8(term),
                  utf8_data[index].name_length);
    }
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-term");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(atom);
    P_TEST_RUN(standard_atoms);
    P_TEST_RUN(string);
    P_TEST_RUN(integer);
    P_TEST_RUN(real);
    P_TEST_RUN(list);
    P_TEST_RUN(variable);
    P_TEST_RUN(member_variable);
    P_TEST_RUN(functor);
    P_TEST_RUN(object);
    P_TEST_RUN(predicate);
    P_TEST_RUN(unify);
    P_TEST_RUN(precedes);
    P_TEST_RUN(witness);
    P_TEST_RUN(utf8);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
