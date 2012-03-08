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

:- import(test).
:- dynamic(userdef/3).
:- dynamic(index_pred/2).
:- dynamic(index_pred_second/2).

test(abolish)
{
    verify(abolish(userdef/3));
    verify(abolish(userdef/3));

    verify(assertz(userdef(a, b, c)));
    verify(userdef(a, b, c));
    verify(abolish(userdef/3));
    verify_error(userdef(a, b, c), existence_error(procedure, userdef/3));

    verify_error(abolish(Pred), instantiation_error);
    verify_error(abolish(Name/3), instantiation_error);
    verify_error(abolish(userdef/Arity), instantiation_error);
    verify_error(abolish(1.5), type_error(predicate_indicator, 1.5));
    verify_error(abolish(userdef/a), type_error(integer, a));
    verify_error(abolish(1/a), type_error(integer, a));
    verify_error(abolish(1/3), type_error(atom, 1));
    verify_error(abolish(userdef/-1), domain_error(not_less_than_zero, -1));
    verify_error(abolish(abolish/1), permission_error(modify, static_procedure, abolish/1));
}

test(assert)
{
    verify(asserta((a :- true)));
    verify(asserta((a(X) :- b(X, Y))));
    verify(assertz(a(Z)));
    verify(assertz((a :- W)));

    verify_error(asserta(Clause), instantiation_error);
    verify_error(assertz((Head :- true)), instantiation_error);
    verify_error(asserta((1.5 :- true)), type_error(callable, 1.5));
    verify_error(assertz(asserta(V)), permission_error(modify, static_procedure, asserta/1));
    verify_error(asserta(true), permission_error(modify, static_procedure, true/0));
}

test(retract)
{
    verify(!retract((b(X1) :- c(X1, Y1))));
    assertz((b(X2) :- c(X2, Y2)));
    verify(retract((b(X3) :- c(X3, Y3))));
    verify(!retract((b(X4) :- c(X4, Y4))));

    assertz((b(X5) :- c(X5, Y5)));
    verify(retract((b(X6) :- c(X6, Y6))));
    verify(X5 !== X6 && Y5 !== Y6);

    assertz((b(a) :- c(a, d)));
    verify(retract((b(X7) :- c(X7, Y7))));
    verify(X7 == a && Y7 == d);

    verify_error(retract(Clause), instantiation_error);
    verify_error(retract((Head :- true)), instantiation_error);
    verify_error(retract((1.5 :- true)), type_error(callable, 1.5));
}

test(dynamic)
{
    verify(dynamic(udef/3));
    verify(dynamic(udef/3));

    verify_error(dynamic(Pred), instantiation_error);
    verify_error(dynamic(Name/3), instantiation_error);
    verify_error(dynamic(udef/Arity), instantiation_error);
    verify_error(dynamic(1.5), type_error(predicate_indicator, 1.5));
    verify_error(dynamic(udef/a), type_error(integer, a));
    verify_error(dynamic(1/a), type_error(integer, a));
    verify_error(dynamic(1/3), type_error(atom, 1));
    verify_error(dynamic(udef/-1), domain_error(not_less_than_zero, -1));
    verify_error(dynamic(dynamic/1), permission_error(modify, static_procedure, dynamic/1));
}

test(local_abolish)
{
    verify(!database(DB));
    new_database(DB);
    verify(database(DB));

    verify(abolish(userdef/3, DB));
    verify(abolish(userdef/3, DB));

    verify(assertz(userdef(a, b, c), DB));
    verify(call(userdef(a, b, c), DB));
    verify(abolish(userdef/3, DB));
    verify(abolish(abolish/1, DB));
    verify_error(call(userdef(a, b, c), DB), existence_error(procedure, userdef/3));

    verify_error(abolish(userdef/3, DB2), instantiation_error);
    verify_error(abolish(userdef/3, a), type_error(database, a));
    verify_error(abolish(Pred, DB), instantiation_error);
    verify_error(abolish(Name/3, DB), instantiation_error);
    verify_error(abolish(userdef/Arity, DB), instantiation_error);
    verify_error(abolish(1.5, DB), type_error(predicate_indicator, 1.5));
    verify_error(abolish(userdef/a, DB), type_error(integer, a));
    verify_error(abolish(1/a, DB), type_error(integer, a));
    verify_error(abolish(1/3, DB), type_error(atom, 1));
    verify_error(abolish(userdef/-1, DB), domain_error(not_less_than_zero, -1));
}

test(local_assert)
{
    verify(!database(DB));
    new_database(DB);
    verify(database(DB));

    verify(asserta((a :- true), DB));
    verify(asserta((a(X) :- b(X, Y)), DB));
    verify(assertz(a(Z), DB));
    verify(assertz((a :- W), DB));

    verify_error(asserta((b :- c), DB2), instantiation_error);
    verify_error(asserta((b :- c), a), type_error(database, a));
    verify_error(asserta(Clause, DB), instantiation_error);
    verify_error(assertz((Head :- true), DB), instantiation_error);
    verify_error(asserta((1.5 :- true), DB), type_error(callable, 1.5));
    verify_error(assertz(asserta(V), DB), permission_error(modify, static_procedure, asserta/1));
    verify_error(asserta(true, DB), permission_error(modify, static_procedure, true/0));
}

test(local_retract)
{
    verify(!database(DB));
    new_database(DB);
    verify(database(DB));

    verify(!retract((b(X1) :- c(X1, Y1)), DB));
    assertz((b(X2) :- c(X2, Y2)), DB);
    verify(retract((b(X3) :- c(X3, Y3)), DB));
    verify(!retract((b(X4) :- c(X4, Y4)), DB));

    assertz((b(X5) :- c(X5, Y5)), DB);
    verify(retract((b(X6) :- c(X6, Y6)), DB));
    verify(X5 !== X6 && Y5 !== Y6);

    assertz((b(a) :- c(a, d)), DB);
    verify(retract((b(X7) :- c(X7, Y7)), DB));
    verify(X7 == a && Y7 == d);

    verify_error(retract((b :- c), DB2), instantiation_error);
    verify_error(retract((b :- c), a), type_error(database, a));
    verify_error(retract(Clause, DB), instantiation_error);
    verify_error(retract((Head :- true), DB), instantiation_error);
    verify_error(retract((1.5 :- true), DB), type_error(callable, 1.5));
}

test(local_call)
{
    verify(!database(DB));
    new_database(DB);
    verify(database(DB));

    assertz(m(b), DB);
    verify(call((m(X), X == b), DB));
    verify_error(call((m(Y), Y == b)), existence_error(procedure, m/1));

    assertz((m(X1) :- n(X1)), DB);
    assertz(n(c), DB);
    verify(call(m(c), DB));
    verify(!call(m(d), DB));
    verify_error(call(m(c)), existence_error(procedure, m/1));
    verify_error(call(n(c)), existence_error(procedure, n/1));

    assertz((m(X2) :- integer(X2)), DB);
    verify(call(m(3), DB));

    verify_error(call((m(Z), Z == b), DB2), instantiation_error);
    verify_error(call((m(Z), Z == b), a), type_error(database, a));
}

test(large_index)
{
    N = 0;
    while (N < 5000) {
        assertz(index_pred(N, -N));
        assertz(index_pred_second(-N, N)); // Indexed on 2nd argument.
        N ::= N + 1;
    }

    while [M, L] (N > 0) {
        N ::= N - 1;
        index_pred(N, M);
        M == -N;
        index_pred_second(L, N);
        L == -N;
    }

    abolish(index_pred/2);
    abolish(index_pred_second/2);
}

test(retract_from_index)
{
    N = 0;
    while (N < 50) {
        assertz(index_pred(N, -N));
        N ::= N + 1;
    }

    verify(retract(index_pred(25, M)));

    while [L] (N > 0) {
        N ::= N - 1;
        if (N == 25)
            verify(!index_pred(N, L));
        else
            verify(index_pred(N, L));
    }

    abolish(index_pred/2);
}
