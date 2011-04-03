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
:- import(findall).

ca(X, Y) { cb(X); cc(Y); }
cb(X) { X = b; }
cb(X) { X = a; }
cb(X) { X = b; }
cc(X) { X = 1; }
cc(X) { X = 2; }

findall()
{
    verify(findall((X, Y), ca(X, Y), L1));
    verify(L1 == [(b, 1), (b, 2), (a, 1), (a, 2), (b, 1), (b, 2)]);
    verify(findall((X, Y), ca(X, Y), L1));
    verify((findall((X, Y), ca(X, Y), [A, B|C]), A == (b, 1), B == (b, 2), C == [(a, 1), (a, 2), (b, 1), (b, 2)]));
    verify((findall(X, fail, L2), L2 == []));
    verify((findall(foo, true, L3), L3 == [foo]));

    verify_error(findall(X, Goal, L9), instantiation_error);
    verify_error(findall(X, true, a), type_error(list, a));
    verify_error(findall(X, true, [a, b|h]), type_error(list, [a, b|h]));
}

main(_)
{
    init("test-findall");
    run(findall);
    report();
}
