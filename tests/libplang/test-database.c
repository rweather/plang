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
#include <plang/database.h>

P_TEST_DECLARE();

static void test_operators()
{
    struct op_info
    {
        int priority;
        p_op_specifier specifier;
        int arity;
        const char *name;
    };
    static struct op_info const ops[] = {
        {1200, P_OP_XFX, 2, ":-"},
        {1200, P_OP_XFX, 2, "-->"},
        {1200, P_OP_FX,  1, ":-"},
        {1200, P_OP_FX,  1, "?-"},
        {1100, P_OP_XFY, 2, ";"},
        {1100, P_OP_XFY, 2, "||"},
        {1050, P_OP_XFY, 2, "->"},
        {1000, P_OP_XFY, 2, ","},
        {1000, P_OP_XFY, 2, "&&"},
        { 900, P_OP_FY,  1, "\\+"},
        { 900, P_OP_FY,  1, "!"},
        { 700, P_OP_XFX, 2, "="},
        { 700, P_OP_XFX, 2, "\\="},
        { 700, P_OP_XFX, 2, "!="},
        { 700, P_OP_XFX, 2, "=="},
        { 700, P_OP_XFX, 2, "\\=="},
        { 700, P_OP_XFX, 2, "!=="},
        { 700, P_OP_XFX, 2, "@<"},
        { 700, P_OP_XFX, 2, "@=<"},
        { 700, P_OP_XFX, 2, "@<="},
        { 700, P_OP_XFX, 2, "@>"},
        { 700, P_OP_XFX, 2, "@>="},
        { 700, P_OP_XFX, 2, "=.."},
        { 700, P_OP_XFX, 2, "is"},
        { 700, P_OP_XFX, 2, "in"},
        { 700, P_OP_XFX, 2, "=:="},
        { 700, P_OP_XFX, 2, "=\\="},
        { 700, P_OP_XFX, 2, "=!="},
        { 700, P_OP_XFX, 2, "<"},
        { 700, P_OP_XFX, 2, "=<"},
        { 700, P_OP_XFX, 2, "<="},
        { 700, P_OP_XFX, 2, ">"},
        { 700, P_OP_XFX, 2, ">="},
        { 700, P_OP_XFX, 2, ":="},
        { 700, P_OP_XFX, 2, "::="},
        { 500, P_OP_YFX, 2, "+"},
        { 500, P_OP_YFX, 2, "-"},
        { 500, P_OP_YFX, 2, "/\\"},
        { 500, P_OP_YFX, 2, "\\/"},
        { 400, P_OP_YFX, 2, "*"},
        { 400, P_OP_YFX, 2, "/"},
        { 400, P_OP_YFX, 2, "//"},
        { 400, P_OP_YFX, 2, "rem"},
        { 400, P_OP_YFX, 2, "mod"},
        { 400, P_OP_YFX, 2, "<<"},
        { 400, P_OP_YFX, 2, ">>"},
        { 400, P_OP_YFX, 2, ">>>"},
        { 200, P_OP_XFX, 2, "**"},
        { 200, P_OP_XFY, 2, "^"},
        { 200, P_OP_FY,  1, "-"},
        { 200, P_OP_FY,  1, "\\"},
        { 200, P_OP_FY,  1, "~"},
        { 100, P_OP_XFX, 2, ":"},
        {   0, P_OP_NONE,0, 0},
    };
    int index, priority;
    p_op_specifier specifier;
    for (index = 0; ops[index].priority; ++index) {
        P_TEST_SET_ROW(ops[index].name);
        specifier = p_db_operator_info
            (p_term_create_atom(context, ops[index].name),
             ops[index].arity, &priority);
        P_COMPARE(specifier, ops[index].specifier);
        P_COMPARE(priority, ops[index].priority);
    }
}

p_term *_p_context_test_goal(p_context *context);

static p_goal_result execute_goal(const char *source, const char *expected_error)
{
    p_term *goal;
    p_term *error;
    p_goal_result result;
    _p_context_test_goal(context);          /* Allow goal saving */
    if (p_context_consult_string(context, source) != 0)
        return (p_goal_result)(P_RESULT_HALT + 1);
    goal = _p_context_test_goal(context);   /* Fetch test goal */
    error = 0;
    result = p_context_execute_goal(context, goal, &error);
    if ((result == P_RESULT_ERROR || result == P_RESULT_HALT)
            && expected_error) {
        p_term *expected;
        p_term *wrapped;
        p_context_consult_string(context, expected_error);
        expected = _p_context_test_goal(context);
        if (p_term_unify(context, error, expected, P_BIND_DEFAULT))
            return result;
        wrapped = p_term_create_functor
            (context, p_term_create_atom(context, "error"), 2);
        p_term_bind_functor_arg(wrapped, 0, expected);
        p_term_bind_functor_arg
            (wrapped, 1, p_term_create_variable(context));
        expected = wrapped;
        if (!p_term_unify(context, error, expected, P_BIND_DEFAULT)) {
            fputs("actual error: ", stdout);
            p_term_print(context, error, p_term_stdio_print_func, stdout);
            fputs("\nexpected error: ", stdout);
            p_term_print(context, expected, p_term_stdio_print_func, stdout);
            putc('\n', stdout);
            P_FAIL("did not receive the expected error");
        }
    }
    return result;
}

#define run_goal(x) execute_goal("\?\?-- " x ".\n", 0)
#define run_goal_error(x,error)     \
    execute_goal("\?\?-- " x ".\n", "\?\?-- " error ".\n")
#define run_stmt(x) execute_goal("\?\?-- { " x " }\n", 0)
#define run_stmt_error(x,error)     \
    execute_goal("\?\?-- { " x " }\n", "\?\?-- " error ".\n")

static void test_user_predicate()
{
    static char const user_source[] =
        "a(b).\n"
        "a(c) :- true.\n"
        "a(X) :- b(X).\n"
        "b(e).\n"
        "b(f) :- c(f).\n"
        "b(g) { throw(foo); }\n"
        ;
    P_VERIFY(p_context_consult_string(context, user_source) == 0);
    P_COMPARE(run_goal("a(b)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("a(c)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("a(d)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("a(e)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("a(f)", "existence_error(procedure, c/1)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("a(g)", "foo"), P_RESULT_ERROR);
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-database");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(operators);
    P_TEST_RUN(user_predicate);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
