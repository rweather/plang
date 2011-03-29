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

:- import(testcase).

list_cons()
{
    verify(([H | T] = [a, b], H == a, T == [b]));
    verify([a, b] = '.'(a, '.'(b, [])));
}

arg()
{
    verify((arg(1, foo(a, b), X), X == a));
    verify((arg(2, foo(a, b), Y), Y == b));
    verify(!arg(3, foo(a, b), Z));
    verify(!arg(0, foo(a, b), W));
    verify((arg(1, [a, b], V), V == a));
    verify((arg(2, [a, b], U), U == [b]));
    verify(!arg(0, [a, b], T));
    verify(!arg(3, [a, b], S));
    verify(arg(1, foo(a, b), a));
    verify(!arg(1, foo(a, b), b));
    verify(!arg(1, foo(R, b), f(R)));
    verify_error(arg(N, foo(a, b), Q), instantiation_error);
    verify_error(arg(1, Term, P), instantiation_error);
    verify_error(arg(a, [a, b], N), type_error(integer, a));
    verify_error(arg(-3, [a, b], M), domain_error(not_less_than_zero, -3));
    verify_error(arg(1, a, L), type_error(compound, a));
}

copy_term()
{
    verify((copy_term(f(X, Y), Z), compound(Z)));
    verify((copy_term(X1, a), var(X1)));
    verify((copy_term(f(a, X2), f(X2, b)), X2 == a));
    verify((copy_term(f(X3, X3), f(Y1, Z1)), Y1 == Z1));
    verify(!copy_term(foo, bar));
}

functor()
{
    verify((functor(a, N1, A1), N1 == a, A1 == 0));
    verify((functor(1.5, N2, A2), N2 == 1.5, A2 == 0));
    verify((functor("foo", N3, A3), N3 == "foo", A3 == 0));
    verify((functor(-30, N4, A4), N4 == -30, A4 == 0));
    verify((functor(f(a, b), N5, A5), N5 == f, A5 == 2));
    verify((functor([H|T], N6, A6), N6 == '.', A6 == 2));
    verify((functor([], N7, A7), N7 == '[]', A7 == 0));

    verify((functor(T1, a, 0), T1 == a));
    verify((functor(T2, 1.5, 0), T2 == 1.5));
    verify((functor(T3, "foo", 0), T3 == "foo"));
    verify((functor(T4, -30, 0), T4 == -30));
    verify((functor(T5, f, 2), T5 = f(X, Y)));
    verify((functor(T6, '.', 2), T6 = [W, V]));

    verify(!functor(f(a, b), g, A8));
    verify(!functor(f(a, b), f, 3));

    verify_error(functor(T7, N8, 2), instantiation_error);
    verify_error(functor(T8, f, A9), instantiation_error);
    verify_error(functor(T9, f(a), 2), type_error(atomic, f(a)));
    verify_error(functor(T10, f, 1.5), type_error(integer, 1.5));
    verify_error(functor(T11, f, -1), domain_error(not_less_than_zero, -1));
    verify_error(functor(T12, 1.5, 1), type_error(atom, 1.5));
}

univ()
{
    verify(foo =.. [foo]);
    verify(1.5 =.. [1.5]);
    verify([a, b, c] =.. ['.', a, [b, c]]);
    verify(f(a, b, c) =.. [f, a, b, c]);
    verify((f(a, b, c) =.. [W, X, Y, Z], W == f, X == a, Y == b, Z == c));
    verify((f(a, b) =.. List, List == [f, a, b]));
    verify((T1 =.. [foo], T1 == foo));
    verify((T2 =.. [1.5], T2 == 1.5));
    verify((T3 =.. [f, a, b], T3 == f(a, b)));
    verify((T4 =.. ['.', a, []], T4 == [a]));

    verify_error(T5 =.. L1, instantiation_error);
    verify_error(T6 =.. f(a, b), instantiation_error);
    verify_error(T7 =.. [f|L2], instantiation_error);
    verify_error(T8 =.. [], domain_error(non_empty_list, []));
    verify_error(T9 =.. [f(a, b)], type_error(atomic, f(a, b)));
    verify_error(T10 =.. [1.5, a, b], type_error(atom, 1.5));
    verify_error(f(a, b) =.. g(a), type_error(list, g(a)));
}

main(_)
{
    init("test-compose");
    run(list_cons);
    run(arg);
    run(copy_term);
    run(functor);
    run(univ);
    report();
}
