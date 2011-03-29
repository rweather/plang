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

foo(X, Y) {}
bar(X, Y) {}

predicate()
{
    verify(predicate(T1, foo/2));
    verify(predicate(T2, bar/2));
    verify(predicate(T3, foo/2));
    verify(T1 !== T2);
    verify(T1 == T3);
    verify(!predicate(T4, baz/2));
    verify(predicate(T5, abolish/1));
    verify(predicate(T6, stdout::writeln/0));
    verify(predicate(T7, stdout::writeln/1));
    verify(T6 !== T7);

    verify((predicate(T1, N1), N1 == foo/2));
    verify((predicate(T2, N2), N2 == bar/2));
    verify((predicate(T5, N5), N5 == abolish/1));
    verify((predicate(T6, N6), N6 == stdout::writeln/0));
    verify((predicate(T7, N7), N7 == stdout::writeln/1));

    verify_error(predicate(_, _), instantiation_error);
    verify_error(predicate(_, _/2), instantiation_error);
    verify_error(predicate(_, foo/_), instantiation_error);
    verify_error(predicate(_, foo), type_error(predicate_indicator, foo));
    verify_error(predicate(_, foo/bar), type_error(integer, bar));
    verify_error(predicate(_, foo/-1), domain_error(not_less_than_zero, -1));
    verify_error(predicate(_, "foo"/0), type_error(atom, "foo"));
}

main(_)
{
    init("test-type");
    run(predicate);
    report();
}
