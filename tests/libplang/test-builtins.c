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
        p_context_consult_string(context, expected_error);
        expected = _p_context_test_goal(context);
        if (!p_term_unify(context, error, expected, P_BIND_EQUALITY)) {
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

static void test_clause_abolish()
{
    P_COMPARE(run_goal("abolish(userdef/3)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("abolish(userdef/3)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("abolish(Pred)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(Name/3)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(userdef/Arity)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(1.5)", "type_error(predicate_indicator, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(userdef/a)", "type_error(integer, a)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(1/a)", "type_error(integer, a)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(1/3)", "type_error(atom, 1)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(userdef/-3)", "domain_error(not_less_than_zero, -3)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("abolish(abolish/1)", "permission_error(modify, static_procedure, abolish/1)"), P_RESULT_ERROR);
}

static void test_clause_assert()
{
    P_COMPARE(run_goal_error("asserta(Clause)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("assertz((Head :- true))", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("asserta((1.5 :- true))", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal("asserta((a :- true))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("asserta((a(X) :- b(X,Y)))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("assertz(a(X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("asserta((a :- X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("assertz(asserta(X))", "permission_error(modify, static_procedure, asserta/1)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("assertz(true)", "permission_error(modify, static_procedure, true/0)"), P_RESULT_ERROR);
}

static void test_clause_retract()
{
    P_COMPARE(run_goal_error("retract(Clause)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("retract((Head :- true))", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("retract((1.5 :- true))", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal("retract((b(X) :- c(X, Y)))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("assertz((b(X) :- c(X, Y))), retract((b(Z) :- c(Z, W)))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("retract((b(X) :- c(X, Y)))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("assertz((b(X) :- c(X, Y))), retract((b(Z) :- c(Z, W))), X !== Z, Y !== W"), P_RESULT_TRUE);
    P_COMPARE(run_goal("assertz((b(a) :- c(a, d))), retract((b(Z) :- c(Z, W))), Z == a, W == d"), P_RESULT_TRUE);
}

static void test_directive_dynamic()
{
    P_COMPARE(run_goal("dynamic(userdef/3)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("dynamic(userdef/3)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("dynamic(Pred)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(Name/3)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(userdef/Arity)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(1.5)", "type_error(predicate_indicator, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(userdef/a)", "type_error(integer, a)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(1/a)", "type_error(integer, a)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(1/3)", "type_error(atom, 1)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(userdef/-3)", "domain_error(not_less_than_zero, -3)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("dynamic(dynamic/1)", "permission_error(modify, static_procedure, dynamic/1)"), P_RESULT_ERROR);
}

static void test_logic_values()
{
    P_COMPARE(run_goal("true"), P_RESULT_TRUE);
    P_COMPARE(run_goal("fail"), P_RESULT_FAIL);
    P_COMPARE(run_goal("false"), P_RESULT_FAIL);
}

static void test_logic_and()
{
    P_COMPARE(run_goal("atom(a), atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(X) && atom(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(a) && atom(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X) && atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(X) && atom(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(a) && atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(a) && atom(b)"), P_RESULT_TRUE);
}

static void test_logic_or()
{
    P_COMPARE(run_goal("atom(a) || atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X) || atom(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X) || atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(X) || atom(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(a) || atom(X)"), P_RESULT_TRUE);
}

static void test_logic_not()
{
    P_COMPARE(run_goal("!atom(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = a, !(X = b), X == a"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = a, !(X = a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, \\+(X = a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("\\+ fail"), P_RESULT_TRUE);
    P_COMPARE(run_goal("! true"), P_RESULT_FAIL);
    P_COMPARE(run_goal_error("!X", "instantiation_error"), P_RESULT_ERROR);
}

static void test_logic_call()
{
    P_COMPARE(run_goal("call(fail)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = atom(a), call(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("call(X)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("call(1.5)", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("call((atom(a), 1.5))", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal("call((!, atom(a)))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("call((!, fail))"), P_RESULT_FAIL);
}

static void test_logic_catch()
{
    P_COMPARE(run_goal_error("throw(a)", "a"), P_RESULT_ERROR);
    P_COMPARE(run_goal("catch(throw(a), X, Y = caught), Y == caught"), P_RESULT_TRUE);
    P_COMPARE(run_goal("catch(atom(a), X, Y = caught), Y !== caught"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("catch(throw(a), b, Y = caught)", "a"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("catch(call(1.5), b, Y = caught)", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_goal("catch(throw(a), X, fail)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("catch(atom(a), X, fail)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("catch(throw(a), X, throw(b))", "b"), P_RESULT_ERROR);
    P_COMPARE(run_goal("catch(catch(throw(a), X, throw(b)), Z, Y = caught), Y == caught"), P_RESULT_TRUE);

    P_COMPARE(run_stmt("try { throw(a); } catch(X) { Y = caught; } Y == caught;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("try { atom(a); } catch(X) { Y = caught; } Y !== caught;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt_error("try { throw(a); } catch(b) { Y = caught; }", "a"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("try { call(1.5); } catch(b) { Y = caught; }", "type_error(callable, 1.5)"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("try { throw(a); } catch(X) { fail; }"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("try { atom(a); } catch(X) { fail; }"), P_RESULT_TRUE);
    P_COMPARE(run_stmt_error("try { throw(a); } catch(X) { throw(b); }", "b"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("try { throw(a); } catch(X) { throw(b); } catch(Z) { Y = caught; }", "b"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("try { throw(a); } catch(b) { throw(b); } catch(Z) { Y = caught; }; Y == caught;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("try { try { throw(a); } catch(X) { throw(b); } } catch(Z) { Y = caught; } Y == caught;"), P_RESULT_TRUE);

    P_COMPARE(run_stmt_error("X = f(d); throw(type_error(list, X));", "type_error(list, f(d))"), P_RESULT_ERROR);
}

static void test_logic_do()
{
    P_COMPARE(run_stmt("do {} while (false);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("do { if (X == f(Y)) Y = a; else X = f(Y); } while (X !== f(a));"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("do { fail; } while (true);"), P_RESULT_FAIL);
    P_COMPARE(run_stmt_error("do { throw(a); } while (true);", "a"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("do {} while (throw(b));", "b"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("do { if (Y == c) X = b; else X = a; Y = c; } while (X !== b);"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("do [X] { if (Y == c) X = b; else X = a; Y = c; } while (X !== b);"), P_RESULT_TRUE);
}

static void test_logic_for()
{
    P_COMPARE(run_stmt("for (X in []) {}"), P_RESULT_TRUE);
    P_COMPARE(run_stmt_error("for (X in Y) {}", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("for (X in [a, b, c |Y]) {}", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("for (X in [a, b, c |f(d)]) {}", "type_error(list, f(d))"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("for (X in f(d)) {}", "type_error(list, f(d))"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("for (X in [a, b]) { atom(X); }"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("for (X in [a, b]) { X == a; }"), P_RESULT_FAIL);
    P_COMPARE(run_stmt_error("for (X in [a, b]) { throw(c); }", "c"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("for (X in [a, b]) { Y = X; }"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("for [Y] (X in [a, b]) { Y = X; }"), P_RESULT_TRUE);
}

static void test_logic_halt()
{
    P_COMPARE(run_goal_error("halt", "0"), P_RESULT_HALT);
    P_COMPARE(run_goal_error("halt(3)", "3"), P_RESULT_HALT);
    P_COMPARE(run_goal_error("halt(-321)", "-321"), P_RESULT_HALT);
    P_COMPARE(run_goal_error("halt(X)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("halt(1.0)", "type_error(integer, 1.0)"), P_RESULT_ERROR);

    P_COMPARE(run_goal_error("catch(halt, X, Y)", "0"), P_RESULT_HALT);
    P_COMPARE(run_stmt_error("try { halt(3); } catch(X) {}", "3"), P_RESULT_HALT);
}

static void test_logic_if_expr()
{
    P_COMPARE(run_goal("atom(a) -> atom(b) || atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(a) -> atom(X) || atom(c)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(X) -> atom(X) || atom(c)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("!, atom(X) -> atom(a) || atom(c)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(a) -> atom(a) || atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal_error("call(X) || atom(X)", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_goal_error("call(X) -> atom(a) || atom(X)", "instantiation_error"), P_RESULT_ERROR);

    P_COMPARE(run_goal("atom(a) -> atom(b)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X) -> atom(b)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(X) -> atom(b)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("!, atom(a) -> atom(b)"), P_RESULT_TRUE);
}

static void test_logic_if_stmt()
{
    P_COMPARE(run_stmt("if (atom(a)) atom(b); else atom(X);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("if (atom(a)) atom(X); else atom(c);"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("if (atom(X)) atom(X); else atom(c);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("if (!, atom(X)) atom(a); else atom(c);"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("if (!, atom(a)) atom(a); else atom(X);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt_error("if (call(X)) atom(a); else atom(X);", "instantiation_error"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("if (X) atom(a); else atom(X);", "instantiation_error"), P_RESULT_ERROR);

    P_COMPARE(run_stmt("if (atom(a)) atom(b);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("if (atom(X)) atom(Y);"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("if (!, atom(X)) atom(b);"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("if (!, atom(a)) atom(b);"), P_RESULT_TRUE);
}

static void test_logic_switch()
{
    P_COMPARE(run_stmt("switch (a) {}"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("switch (a) { default: true; }"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("switch (a) { case X: Y = b; } X == a; Y == b;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("switch (f(a)) { case g(X): case f(X): Y = b; } X == a; Y == b;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("switch (f(a)) { case g(X): Y = c; case f(X): Y = b; } X == a; Y == b;"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("switch (f(a)) { case g(X): Y = c; case h(X): Y = b; default: Y = d; } var(X); Y == d;"), P_RESULT_TRUE);
}

static void test_logic_while()
{
    P_COMPARE(run_stmt("while (false) {}"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("while (X !== f(a)) { if (X == f(Y)) Y = a; else X = f(Y); }"), P_RESULT_TRUE);
    P_COMPARE(run_stmt("while (true) { fail; }"), P_RESULT_FAIL);
    P_COMPARE(run_stmt_error("while (true) { throw(a); }", "a"), P_RESULT_ERROR);
    P_COMPARE(run_stmt_error("while (throw(b)) {}", "b"), P_RESULT_ERROR);
    P_COMPARE(run_stmt("while (X !== b) { if (Y == c) X = b; else X = a; Y = c; }"), P_RESULT_FAIL);
    P_COMPARE(run_stmt("while [X] (Z !== d) { if (Y == c) { X = b; Z = d; } else { X = a; } Y = c; }"), P_RESULT_TRUE);
}

static void test_term_eq()
{
    P_COMPARE(run_goal("X == X"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X == Y"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(X,Y) == f(X,Y)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(Y,X) == f(X,Y)"), P_RESULT_FAIL);
}

static void test_term_ne()
{
    P_COMPARE(run_goal("X !== X"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X !== Y"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(X,Y) !== f(X,Y)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(Y,X) !== f(X,Y)"), P_RESULT_TRUE);

    P_COMPARE(run_goal("X \\== X"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X \\== Y"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(X,Y) \\== f(X,Y)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(Y,X) \\== f(X,Y)"), P_RESULT_TRUE);
}

static void test_term_lt()
{
    P_COMPARE(run_goal("f(j) @< f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(j) @< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @< 1"), P_RESULT_TRUE);
}

static void test_term_le()
{
    P_COMPARE(run_goal("f(j) @<= f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @<= f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @<= f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @<= 1"), P_RESULT_TRUE);

    P_COMPARE(run_goal("f(j) @=< f(k)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @=< f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(k) @=< f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @=< 1"), P_RESULT_TRUE);
}

static void test_term_gt()
{
    P_COMPARE(run_goal("f(j) @> f(k)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("f(k) @> f(j)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("f(j) @> f(j)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("2.0 @> 1"), P_RESULT_FAIL);
}

static void test_term_ge()
{
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

static void test_type_atom()
{
    P_COMPARE(run_goal("atom(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, atom(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atom(f(a))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom([a])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atom(\"foo\")"), P_RESULT_FAIL);
}

static void test_type_atomic()
{
    P_COMPARE(run_goal("atomic(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("X = a, atomic(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(f(a))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atomic([a])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("atomic(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("atomic(\"foo\")"), P_RESULT_TRUE);
}

static void test_type_compound()
{
    P_COMPARE(run_goal("compound(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound([])"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(f(X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = f(Y), compound(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("compound([a])"), P_RESULT_TRUE);
    P_COMPARE(run_goal("compound(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("compound(\"foo\")"), P_RESULT_FAIL);
}

static void test_type_float()
{
    P_COMPARE(run_goal("float(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1.5, float(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("float(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("float(\"foo\")"), P_RESULT_FAIL);
}

static void test_type_integer()
{
    P_COMPARE(run_goal("integer(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1, integer(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("integer(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("integer(\"foo\")"), P_RESULT_FAIL);
}

static void test_type_nonvar()
{
    P_COMPARE(run_goal("nonvar(a)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("nonvar(f(X))"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = a, nonvar(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("nonvar(\"foo\")"), P_RESULT_TRUE);
}

static void test_type_number()
{
    P_COMPARE(run_goal("number(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("number(1)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = 1, number(X)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("number(1.5)"), P_RESULT_TRUE);
    P_COMPARE(run_goal("number(\"foo\")"), P_RESULT_FAIL);
}

static void test_type_string()
{
    P_COMPARE(run_goal("string(a)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(X)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(f(X))"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(1)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(1.5)"), P_RESULT_FAIL);
    P_COMPARE(run_goal("string(\"foo\")"), P_RESULT_TRUE);
    P_COMPARE(run_goal("X = \"foo\", string(X)"), P_RESULT_TRUE);
}

static void test_type_var()
{
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

    P_TEST_RUN(clause_abolish);
    P_TEST_RUN(clause_assert);
    P_TEST_RUN(clause_retract);
    P_TEST_RUN(directive_dynamic);
    P_TEST_RUN(logic_values);
    P_TEST_RUN(logic_and);
    P_TEST_RUN(logic_or);
    P_TEST_RUN(logic_not);
    P_TEST_RUN(logic_call);
    P_TEST_RUN(logic_catch);
    P_TEST_RUN(logic_do);
    P_TEST_RUN(logic_for);
    P_TEST_RUN(logic_halt);
    P_TEST_RUN(logic_if_expr);
    P_TEST_RUN(logic_if_stmt);
    P_TEST_RUN(logic_switch);
    P_TEST_RUN(logic_while);
    P_TEST_RUN(term_eq);
    P_TEST_RUN(term_ne);
    P_TEST_RUN(term_lt);
    P_TEST_RUN(term_le);
    P_TEST_RUN(term_gt);
    P_TEST_RUN(term_ge);
    P_TEST_RUN(term_unification);
    P_TEST_RUN(type_atom);
    P_TEST_RUN(type_atomic);
    P_TEST_RUN(type_compound);
    P_TEST_RUN(type_float);
    P_TEST_RUN(type_integer);
    P_TEST_RUN(type_nonvar);
    P_TEST_RUN(type_number);
    P_TEST_RUN(type_string);
    P_TEST_RUN(type_var);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
