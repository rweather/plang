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

#include "testcase.h"
#include <plang/term.h>
#include "inst-priv.h"
#include "context-priv.h"

P_TEST_DECLARE();

static p_code *code = 0;
static p_code_clause code_clause;
static int code_clause_valid = 0;

static void init_code()
{
    if (code)
        _p_code_finish(code, &code_clause);
    code = _p_code_new();
    code_clause_valid = 0;
}

/*#define P_CODE_DEBUG*/

static p_term *run_code()
{
    p_goal_result result;
    p_term *error = 0;
    _p_code_finish(code, &code_clause);
    code = 0;
    if (!code_clause.code)
        return 0;
    code_clause_valid = 1;
#ifdef P_CODE_DEBUG
    _p_code_disassemble(stdout, context, code_clause);
#endif
    result = _p_code_run(context, &code_clause, &error);
    P_VERIFY(result == P_RESULT_RETURN_BODY);
    return error;
}

static void finish_code()
{
    _p_code_finish(code, &code_clause);
    code = 0;
    code_clause_valid = 1;
#ifdef P_CODE_DEBUG
    _p_code_disassemble(stdout, context, code_clause);
#endif
}

static p_goal_result run_match(p_term *value)
{
    p_term *error = 0;
    _p_code_finish(code, &code_clause);
    code = 0;
    if (!code_clause.code)
        return P_RESULT_ERROR;
    code_clause_valid = 1;
#ifdef P_CODE_DEBUG
    _p_code_disassemble(stdout, context, code_clause);
#endif
    _p_code_set_xreg(context, 0, value);
    return _p_code_run(context, &code_clause, &error);
}

static void cleanup_code()
{
    if (code)
        _p_code_finish(code, &code_clause);
    code = 0;
    code_clause_valid = 0;
}

p_term *_p_context_test_goal(p_context *context);

static p_term *parse_term(const char *source)
{
    _p_context_test_goal(context);          /* Allow goal saving */
    if (p_context_consult_string(context, source) != 0)
        return 0;
    return _p_context_test_goal(context);   /* Fetch test goal */
}

#define TERM(x) ("\?\?-- " x ".\n")

static void test_put_common(int preferred_reg, int force_large_regs)
{
    struct put_type
    {
        const char *row;
        const char *term;
        int compare_type;
    };
    static struct put_type const put_data[] = {
        {"atom_1", TERM("a"), P_BIND_EQUALITY},
        {"atom_2", TERM("[]"), P_BIND_EQUALITY},

        {"integer_1", TERM("42"), P_BIND_EQUALITY},
        {"integer_2", TERM("-42"), P_BIND_EQUALITY},

        {"float_1", TERM("4.5"), P_BIND_EQUALITY},
        {"float_2", TERM("-4.5"), P_BIND_EQUALITY},

        {"string_1", TERM("\"\""), P_BIND_EQUALITY},
        {"string_2", TERM("\"foo\""), P_BIND_EQUALITY},

        {"variable_1", TERM("X"), P_BIND_ONE_WAY},

        {"functor_1", TERM("f(X)"), P_BIND_ONE_WAY},
        {"functor_2", TERM("f(X, a, 4.5)"), P_BIND_ONE_WAY},
        {"functor_3", TERM("f(g(X, h(i), h(X), u), \"a\", 5)"), P_BIND_ONE_WAY},

        {"list_1", TERM("[a, b, c]"), P_BIND_EQUALITY},
        {"list_2", TERM("[a, f(b), c]"), P_BIND_EQUALITY},
        {"list_3", TERM("[a|c]"), P_BIND_EQUALITY},
        {"list_4", TERM("[a|T]"), P_BIND_ONE_WAY},
        {"list_5", TERM("[f(a)|T]"), P_BIND_ONE_WAY},
        {"list_6", TERM("[a]"), P_BIND_EQUALITY},

        {"functor_list_1", TERM("h([a, b, c])"), P_BIND_EQUALITY},
        {"functor_list_2", TERM("h([a, f(b), c], d)"), P_BIND_EQUALITY},
        {"functor_list_3", TERM("h([a|c])"), P_BIND_EQUALITY},
        {"functor_list_4", TERM("h([a|T], d)"), P_BIND_ONE_WAY},
        {"functor_list_5", TERM("h([f(a)|T])"), P_BIND_ONE_WAY},
        {"functor_list_6", TERM("h([a], d)"), P_BIND_EQUALITY},
    };
    #define put_data_len (sizeof(put_data) / sizeof(struct put_type))

    size_t index;
    p_term *expected;
    p_term *actual;
    int reg, compare_type, result;
    for (index = 0; index < put_data_len; ++index) {
        P_TEST_SET_ROW(put_data[index].row);
        expected = parse_term(put_data[index].term);
#ifdef P_CODE_DEBUG
        fputs("put(", stdout);
        p_term_print(context, expected, p_term_stdio_print_func, stdout);
        fputs(")\n", stdout);
#endif
        init_code();
        code->force_large_regs = force_large_regs;
        if (preferred_reg != -1)
            _p_code_allocate_args(code, preferred_reg + 3);
        reg = _p_code_generate_builder
            (context, expected, code, preferred_reg);
        _p_code_generate_return(code, reg);
        actual = run_code();
        compare_type = put_data[index].compare_type;
        if (compare_type == P_BIND_ONE_WAY) {
            /* Test involves variables, so test if the terms are
             * identical up to unification of the variables only */
            void *marker = p_context_mark_trail(context);
            result = p_term_unify
                (context, actual, expected, compare_type);
            if (result) {
                p_context_backtrack_trail(context, marker);
                result = p_term_unify
                    (context, expected, actual, compare_type);
            }
            p_context_backtrack_trail(context, marker);
        } else {
            result = p_term_unify
                (context, actual, expected, compare_type);
        }
        if (!result) {
            fputs("actual: ", stdout);
            p_term_print(context, actual, p_term_stdio_print_func, stdout);
            fputs("\nexpected: ", stdout);
            p_term_print(context, expected, p_term_stdio_print_func, stdout);
            putc('\n', stdout);
            P_FAIL("compiled form does not generate correct term");
        }
        cleanup_code();
    }
}

static void test_put()
{
    test_put_common(-1, 0);
}

static void test_put_preferred()
{
    test_put_common(3, 0);
}

static void test_put_large()
{
    test_put_common(-1, 1);
}

static void test_put_large_preferred()
{
    test_put_common(3, 1);
}

static void test_get_common(int input_only, int force_large_regs)
{
    struct get_type
    {
        const char *row;
        const char *term;
        const char *arg;
        p_goal_result result;
        int input_only_fail;
    };
    static struct get_type const get_data[] = {
        {"atom_1", TERM("a"), TERM("a"), P_RESULT_TRUE, 0},
        {"atom_2", TERM("a"), TERM("b"), P_RESULT_FAIL, 0},
        {"atom_3", TERM("a"), TERM("X"), P_RESULT_TRUE, 1},
        {"atom_4", TERM("a"), TERM("1"), P_RESULT_FAIL, 0},

        {"integer_1", TERM("42"), TERM("42"), P_RESULT_TRUE, 0},
        {"integer_2", TERM("-42"), TERM("-42"), P_RESULT_TRUE, 0},
        {"integer_3", TERM("-42"), TERM("42"), P_RESULT_FAIL, 0},
        {"integer_4", TERM("-42"), TERM("X"), P_RESULT_TRUE, 1},
        {"integer_5", TERM("-42"), TERM("a"), P_RESULT_FAIL, 0},

        {"float_1", TERM("4.5"), TERM("4.5"), P_RESULT_TRUE, 0},
        {"float_2", TERM("-4.5"), TERM("-4.5"), P_RESULT_TRUE, 0},
        {"float_3", TERM("-4.5"), TERM("4.5"), P_RESULT_FAIL, 0},
        {"float_4", TERM("-4.5"), TERM("X"), P_RESULT_TRUE, 1},
        {"float_5", TERM("-4.5"), TERM("6"), P_RESULT_FAIL, 0},

        {"string_1", TERM("\"\""), TERM("\"\""), P_RESULT_TRUE, 0},
        {"string_2", TERM("\"foo\""), TERM("\"foo\""), P_RESULT_TRUE, 0},
        {"string_3", TERM("\"foo\""), TERM("\"bar\""), P_RESULT_FAIL, 0},
        {"string_4", TERM("\"foo\""), TERM("X"), P_RESULT_TRUE, 1},
        {"string_5", TERM("\"foo\""), TERM("a"), P_RESULT_FAIL, 0},

        {"variable_1", TERM("X"), TERM("Y"), P_RESULT_TRUE, 0},
        {"variable_2", TERM("X"), TERM("a"), P_RESULT_TRUE, 0},
        {"variable_3", TERM("X"), TERM("f(a)"), P_RESULT_TRUE, 0},

        {"functor_1", TERM("f(X, X)"), TERM("f(a, a)"), P_RESULT_TRUE, 0},
        {"functor_2", TERM("f(X, X)"), TERM("f(a, b)"), P_RESULT_FAIL, 0},
        {"functor_3", TERM("f(a, a)"), TERM("f(X, X)"), P_RESULT_TRUE, 1},
        {"functor_4", TERM("f(g(b), 4.5)"), TERM("f(X, Y)"), P_RESULT_TRUE, 1},
        {"functor_5", TERM("f(g(b), 1)"), TERM("f(g(b), 1)"), P_RESULT_TRUE, 0},
        {"functor_6", TERM("f(g(b, h(c), \"foo\"), a)"), TERM("f(X, Y)"), P_RESULT_TRUE, 1},
        {"functor_7", TERM("f(g(b, h(c), \"foo\"), a)"), TERM("f(g(b, h(c), \"foo\"), a)"), P_RESULT_TRUE, 0},
        {"functor_8", TERM("f(g(b, h(c), \"foo\"), a)"), TERM("f(g(b, h(c)), a)"), P_RESULT_FAIL, 0},
        {"functor_9", TERM("(A + B) * C"), TERM("X * Y"), P_RESULT_TRUE, 1},
        {"functor_10", TERM("(A + B) * C"), TERM("(X + Z) * Y"), P_RESULT_TRUE, 0},
        {"functor_11", TERM("A * (B + C)"), TERM("X * Y"), P_RESULT_TRUE, 1},
        {"functor_12", TERM("A * (B + C)"), TERM("X * (Y + Z)"), P_RESULT_TRUE, 0},

        {"list_1", TERM("[a, b, c]"), TERM("[a, b, c]"), P_RESULT_TRUE, 0},
        {"list_2", TERM("[a, b|c]"), TERM("[a, b|c]"), P_RESULT_TRUE, 0},
        {"list_3", TERM("[a]"), TERM("[a]"), P_RESULT_TRUE, 0},
        {"list_4", TERM("[a, b, c]"), TERM("[X, b, c]"), P_RESULT_TRUE, 1},
        {"list_5", TERM("[X, b, c]"), TERM("[a, b, c]"), P_RESULT_TRUE, 0},
        {"list_6", TERM("[a, b, c]"), TERM("[a, X, c]"), P_RESULT_TRUE, 1},
        {"list_7", TERM("[a, X, c]"), TERM("[a, b, c]"), P_RESULT_TRUE, 0},
        {"list_8", TERM("[a, f(b), c]"), TERM("[a, f(b), c]"), P_RESULT_TRUE, 0},
        {"list_9", TERM("[a, f(b)]"), TERM("[a, f(b)]"), P_RESULT_TRUE, 0},
        {"list_10", TERM("[a, f(b)|c]"), TERM("[a, f(b)|c]"), P_RESULT_TRUE, 0},

        {"functor_list_1", TERM("f([a], 3)"), TERM("f([a], 3)"), P_RESULT_TRUE, 0},
        {"functor_list_2", TERM("f([a], 3)"), TERM("f(X, 3)"), P_RESULT_TRUE, 1},
    };
    #define get_data_len (sizeof(get_data) / sizeof(struct get_type))

    size_t index;
    p_term *term;
    p_term *arg;
    p_goal_result result;
    int ok;
    for (index = 0; index < get_data_len; ++index) {
        P_TEST_SET_ROW(get_data[index].row);
        term = parse_term(get_data[index].term);
        arg = parse_term(get_data[index].arg);
#ifdef P_CODE_DEBUG
        fputs("get(", stdout);
        p_term_print(context, term, p_term_stdio_print_func, stdout);
        fputs(" = ", stdout);
        p_term_print(context, arg, p_term_stdio_print_func, stdout);
        fputs(")\n", stdout);
#endif
        init_code();
        code->force_large_regs = force_large_regs;
        _p_code_allocate_args(code, 1);
        _p_code_generate_matcher
            (context, term, code, 0, input_only);
        _p_code_generate_return(code, -1);
        result = run_match(arg);
        if (input_only && get_data[index].input_only_fail)
            ok = (result == P_RESULT_FAIL);
        else
            ok = (result == get_data[index].result);
        if (!ok) {
            fputs("compiled: ", stdout);
            p_term_print(context, term, p_term_stdio_print_func, stdout);
            fputs("\nargument: ", stdout);
            p_term_print(context, arg, p_term_stdio_print_func, stdout);
            putc('\n', stdout);
            P_FAIL("match did not operate as expected");
        }
        cleanup_code();
    }
}

static void test_get()
{
    test_get_common(0, 0);
}

static void test_get_in()
{
    test_get_common(1, 0);
}

static void test_get_large()
{
    test_get_common(0, 1);
}

static void test_get_large_in()
{
    test_get_common(1, 1);
}

/* Test generation of a large "put" that will need overflow blocks */
static void test_overflow()
{
    p_term *term;
    p_term *term2;
    int index, reg;
    term = p_term_create_functor
        (context, p_term_create_atom(context, "bar"), 200);
    for (index = 0; index < 200; ++index) {
        p_term_bind_functor_arg
            (term, index, p_term_create_integer(context, index));
    }
    init_code();
    reg = _p_code_generate_builder(context, term, code, -1);
    _p_code_generate_return(code, reg);
    term2 = run_code();
    P_VERIFY(p_term_unify(context, term, term2, P_BIND_EQUALITY));
}

static void rbkey_init(p_rbkey *key, p_term *term)
{
    if (!_p_rbkey_init(key, term)) {
        key->type = P_TERM_VARIABLE;
        key->size = 0;
        key->name = 0;
    }
}

static void test_argument_key_common
    (int input_only, int force_large_regs)
{
    struct key_type
    {
        const char *row;
        const char *arg0;
        const char *arg1;
    };
    static struct key_type const key_data[] = {
        {"atom_atom", TERM("a"), TERM("b")},
        {"atom_var", TERM("a"), TERM("X")},
        {"atom_member_var", TERM("a"), TERM("Y.foo")},
        {"atom_int", TERM("a"), TERM("42")},
        {"atom_float", TERM("a"), TERM("4.5")},
        {"atom_string", TERM("a"), TERM("\"a\"")},
        {"atom_functor_1", TERM("a"), TERM("f(Y, 3)")},
        {"atom_functor_2", TERM("a"), TERM("f(g([Y]), 3)")},
        {"atom_list", TERM("a"), TERM("[a, b, c]")},

        {"var_var", TERM("X"), TERM("Y")},
        {"var_member_var", TERM("X"), TERM("Y.foo")},
        {"var_atom", TERM("X"), TERM("a")},
        {"var_int", TERM("X"), TERM("42")},
        {"var_float", TERM("X"), TERM("4.5")},
        {"var_string", TERM("X"), TERM("\"a\"")},
        {"var_functor_1", TERM("X"), TERM("f(Y, 3)")},
        {"var_functor_2", TERM("X"), TERM("f(g([Y]), 3)")},
        {"var_list", TERM("X"), TERM("[a, b, c]")},

        {"member_var_member_var", TERM("Y.foo"), TERM("Z.bar")},
        {"member_var_var", TERM("Y.foo"), TERM("X")},
        {"member_var_atom", TERM("Y.foo"), TERM("a")},
        {"member_var_int", TERM("Y.foo"), TERM("42")},
        {"member_var_float", TERM("Y.foo"), TERM("4.5")},
        {"member_var_string", TERM("Y.foo"), TERM("\"a\"")},
        {"member_var_functor_1", TERM("Y.foo"), TERM("f(Y, 3)")},
        {"member_var_functor_2", TERM("Y.foo"), TERM("f(g([Y]), 3)")},
        {"member_var_list", TERM("Y.foo"), TERM("[a, b, c]")},

        {"int_int", TERM("42"), TERM("24")},
        {"int_atom", TERM("42"), TERM("a")},
        {"int_var", TERM("42"), TERM("X")},
        {"int_member_var", TERM("42"), TERM("Y.foo")},
        {"int_float", TERM("42"), TERM("4.5")},
        {"int_string", TERM("42"), TERM("\"a\"")},
        {"int_functor_1", TERM("42"), TERM("f(Y, 3)")},
        {"int_functor_2", TERM("42"), TERM("f(g([Y]), 3)")},
        {"int_list", TERM("42"), TERM("[a, b, c]")},

        {"float_float", TERM("4.5"), TERM("0.5")},
        {"float_atom", TERM("4.5"), TERM("a")},
        {"float_var", TERM("4.5"), TERM("X")},
        {"float_member_var", TERM("4.5"), TERM("Y.foo")},
        {"float_int", TERM("4.5"), TERM("42")},
        {"float_string", TERM("4.5"), TERM("\"a\"")},
        {"float_functor_1", TERM("4.5"), TERM("f(Y, 3)")},
        {"float_functor_2", TERM("4.5"), TERM("f(g([Y]), 3)")},
        {"float_list", TERM("4.5"), TERM("[a, b, c]")},

        {"string_string", TERM("\"a\""), TERM("\"b\"")},
        {"string_atom", TERM("\"a\""), TERM("a")},
        {"string_var", TERM("\"a\""), TERM("X")},
        {"string_member_var", TERM("\"a\""), TERM("Y.foo")},
        {"string_int", TERM("\"a\""), TERM("42")},
        {"string_float", TERM("\"a\""), TERM("4.5")},
        {"string_functor_1", TERM("\"a\""), TERM("f(Y, 3)")},
        {"string_functor_2", TERM("\"a\""), TERM("f(g([Y]), 3)")},
        {"string_list", TERM("\"a\""), TERM("[a, b, c]")},

        {"functor_1_functor", TERM("f(Y, 3)"), TERM("f(g([Y]), 3)")},
        {"functor_1_atom", TERM("f(Y, 3)"), TERM("a")},
        {"functor_1_var", TERM("f(Y, 3)"), TERM("X")},
        {"functor_1_member_var", TERM("f(Y, 3)"), TERM("Y.foo")},
        {"functor_1_int", TERM("f(Y, 3)"), TERM("42")},
        {"functor_1_float", TERM("f(Y, 3)"), TERM("4.5")},
        {"functor_1_string", TERM("f(Y, 3)"), TERM("\"b\"")},
        {"functor_1_list", TERM("f(Y, 3)"), TERM("[a, b, c]")},

        {"functor_2_functor", TERM("f(g([Y]), 3)"), TERM("f(Y, 3)")},
        {"functor_2_atom", TERM("f(g([Y]), 3)"), TERM("a")},
        {"functor_2_var", TERM("f(g([Y]), 3)"), TERM("X")},
        {"functor_2_member_var", TERM("f(g([Y]), 3)"), TERM("Y.foo")},
        {"functor_2_int", TERM("f(g([Y]), 3)"), TERM("42")},
        {"functor_2_float", TERM("f(g([Y]), 3)"), TERM("4.5")},
        {"functor_2_string", TERM("f(g([Y]), 3)"), TERM("\"b\"")},
        {"functor_2_list", TERM("f(g([Y]), 3)"), TERM("[a, b, c]")},

        {"list_list", TERM("[a, b, c]"), TERM("[d, e, f]")},
        {"list_atom", TERM("[a, b, c]"), TERM("a")},
        {"list_var", TERM("[a, b, c]"), TERM("X")},
        {"list_member_var", TERM("[a, b, c]"), TERM("Y.foo")},
        {"list_int", TERM("[a, b, c]"), TERM("42")},
        {"list_float", TERM("[a, b, c]"), TERM("0.5")},
        {"list_string", TERM("[a, b, c]"), TERM("\"a\"")},
        {"list_functor_1", TERM("[a, b, c]"), TERM("f(Y, 3)")},
        {"list_functor_2", TERM("[a, b, c]"), TERM("f(g([Y]), 3)")},
    };
    #define key_data_len (sizeof(key_data) / sizeof(struct key_type))

    size_t index;
    p_term *arg0;
    p_term *arg1;
    p_rbkey expected_key;
    p_rbkey actual_key;
    for (index = 0; index < key_data_len; ++index) {
        P_TEST_SET_ROW(key_data[index].row);
        arg0 = parse_term(key_data[index].arg0);
        arg1 = parse_term(key_data[index].arg1);
#ifdef P_CODE_DEBUG
        fputs("argument_key(", stdout);
        p_term_print(context, arg0, p_term_stdio_print_func, stdout);
        fputs(", ", stdout);
        p_term_print(context, arg1, p_term_stdio_print_func, stdout);
        fputs(")\n", stdout);
#endif
        init_code();
        code->force_large_regs = force_large_regs;
        _p_code_allocate_args(code, 2);
        _p_code_generate_matcher(context, arg0, code, 0, input_only);
        _p_code_generate_matcher(context, arg1, code, 1, input_only);
        _p_code_generate_return(code, -1);
        finish_code();

        rbkey_init(&expected_key, arg0);
        if (!_p_code_argument_key(&actual_key, &code_clause, 0)) {
            actual_key.type = P_TERM_VARIABLE;
            actual_key.size = 0;
            actual_key.name = 0;
        }
        P_COMPARE(expected_key.type, actual_key.type);
        P_COMPARE(expected_key.size, actual_key.size);
        P_COMPARE(expected_key.name, actual_key.name);

        rbkey_init(&expected_key, arg1);
        if (!_p_code_argument_key(&actual_key, &code_clause, 1)) {
            actual_key.type = P_TERM_VARIABLE;
            actual_key.size = 0;
            actual_key.name = 0;
        }
        P_COMPARE(expected_key.type, actual_key.type);
        P_COMPARE(expected_key.size, actual_key.size);
        P_COMPARE(expected_key.name, actual_key.name);

        cleanup_code();
    }
}

static void test_argument_key()
{
    test_argument_key_common(0, 0);
}

static void test_argument_key_in()
{
    test_argument_key_common(1, 0);
}

static void test_argument_key_large()
{
    test_argument_key_common(0, 1);
}

static void test_argument_key_in_large()
{
    test_argument_key_common(1, 1);
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-compiler");
    P_TEST_CREATE_CONTEXT();

    GC_add_roots(&code, code + 1);
    GC_add_roots(&code_clause, &code_clause + 1);

    P_TEST_RUN(put);
    P_TEST_RUN(put_preferred);
    P_TEST_RUN(put_large);
    P_TEST_RUN(put_large_preferred);

    P_TEST_RUN(get);
    P_TEST_RUN(get_in);
    P_TEST_RUN(get_large);
    P_TEST_RUN(get_large_in);

    P_TEST_RUN(overflow);

    P_TEST_RUN(argument_key);
    P_TEST_RUN(argument_key_in);
    P_TEST_RUN(argument_key_large);
    P_TEST_RUN(argument_key_in_large);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
