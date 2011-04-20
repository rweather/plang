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
:- import(stdout).
:- import(findall).

unify_one_way()
{
    verify((unify_one_way(X1, Y1 + Z1), X1 == Y1 + Z1));
    verify(!unify_one_way(Y2 + Z2, X2));
    verify((unify_one_way(X, Y), X == Y));
}

foo(X + Y).
bar(in X + Y).

expand(A * (B + C), A * B + A * C).
expand((A + B) * C, A * C + B * C).

expand_input_only(in A * (B + C), A * B + A * C).
expand_input_only(in (A + B) * C, A * C + B * C).

one_way_predicate()
{
    verify(foo(A));
    verify(!bar(B));

    verify(foo(C + D));
    verify(bar(E + F));

    findall(Solution1, expand((X1 + Y1) * Z1, Solution1), Solutions1);
    verify(Solutions1 = [(X1 + Y1) * B + (X1 + Y1) * C, X1 * Z1 + Y1 * Z1]);

    findall(Solution2, expand_input_only((X2 + Y2) * Z2, Solution2), Solutions2);
    verify(Solutions2 = [X2 * Z2 + Y2 * Z2]);
}

main(_)
{
    init("test-one-way");
    run(unify_one_way);
    run(one_way_predicate);
    report();
}
