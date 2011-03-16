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

p_term *_p_context_test_goal(p_context *context);

static p_goal_result execute_goal(const char *source)
{
    p_term *goal;
    _p_context_test_goal(context);          /* Allow goal saving */
    if (p_context_consult_string(context, source) != 0)
        return P_RESULT_ERROR;
    goal = _p_context_test_goal(context);   /* Fetch test goal */
    return p_context_execute_goal(context, goal);
}

#define run_goal(x) execute_goal("\?\?-- " x ".\n")

static void test_logic_and_control()
{
    P_COMPARE(run_goal("true"), P_RESULT_TRUE);
    P_COMPARE(run_goal("fail"), P_RESULT_FAIL);
    P_COMPARE(run_goal("false"), P_RESULT_FAIL);
}

static void test_term_comparison()
{
    P_COMPARE(run_goal("X == X"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X == Y"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(X,Y) == f(X,Y)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(Y,X) == f(X,Y)"), P_RESULT_FAIL);

    P_COMPARE(run_goal("X !== X"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X !== Y"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(X,Y) !== f(X,Y)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(Y,X) !== f(X,Y)"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(j) @< f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(j) @< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @< 1"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(j) @<= f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @<= f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @<= f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @<= 1"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(j) @=< f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @=< f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @=< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @=< 1"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(j) @> f(k)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(k) @> f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @> f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @> 1"), P_RESULT_FAIL);

    P_COMPARE(run_goal("f(j) @>= f(k)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(k) @>= f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @>= f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("2.0 @>= 1"), P_RESULT_FAIL);
}

static void test_term_unification()
{
    P_COMPARE(run_goal("f(X,b) = f(a,Y)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(X,b) = g(X,b)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = f(X)"), P_RESULT_FAIL);

    P_COMPARE(run_goal("unify_with_occurs_check(f(X,b), f(a,Y))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("unify_with_occurs_check(f(X,b), g(X,b))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("unify_with_occurs_check(X, f(X))"), P_RESULT_FAIL);

    P_COMPARE(run_goal("f(X,b) != f(a,Y)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(X,b) != g(X,b)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X != f(X)"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(X,b) \\= f(a,Y)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(X,b) \\= g(X,b)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X \\= f(X)"), P_RESULT_TRUE);

    P_COMPARE(run_goal("unifiable(f(X,b), f(a,Y))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("unifiable(f(X,b), g(X,b))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("unifiable(X, f(X))"), P_RESULT_FAIL);

    /* Check that the variables are bound as expected */
    P_COMPARE(run_goal("f(X,b) = f(a,Y), nonvar(X), nonvar(Y)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(X,b) = f(a,Y), X == a, Y == b"), P_RESULT_TRUE);
    P_COMPARE(run_goal("unifiable(f(X,b), f(a,Y)), var(X), var(Y)"), P_RESULT_TRUE);
}

static void test_type_testing()
{
    P_COMPARE(run_goal("atom(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(f(a))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom([a])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(\"foo\")"), P_RESULT_FAIL);

    P_COMPARE(run_goal("atomic(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, atomic(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(f(a))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atomic([a])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atomic(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(\"foo\")"), P_RESULT_TRUE);

    P_COMPARE(run_goal("compound(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound([])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(f(X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = f(Y), compound(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("compound([a])"), P_RESULT_TRUE);
    P_COMPARE(run_goal("compound(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(\"foo\")"), P_RESULT_FAIL);

    P_COMPARE(run_goal("float(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1.5, float(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("float(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(\"foo\")"), P_RESULT_FAIL);

    P_COMPARE(run_goal("integer(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1, integer(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("integer(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(\"foo\")"), P_RESULT_FAIL);

    P_COMPARE(run_goal("nonvar(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("nonvar(f(X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = a, nonvar(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(\"foo\")"), P_RESULT_TRUE);

    P_COMPARE(run_goal("number(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1, number(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("number(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("number(\"foo\")"), P_RESULT_FAIL);

    P_COMPARE(run_goal("string(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(\"foo\")"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = \"foo\", string(X)"), P_RESULT_TRUE);

    P_COMPARE(run_goal("var(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("var(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("var(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("var(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, var(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = Y, var(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("var(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("var(\"foo\")"), P_RESULT_FAIL);
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-builtins");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(logic_and_control);
    P_TEST_RUN(term_comparison);
    P_TEST_RUN(term_unification);
    P_TEST_RUN(type_testing);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
