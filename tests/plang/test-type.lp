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
:- import(stdout).

foo(X, Y) {}
bar(X, Y) {}

class baz
{
    var member
    new() {}
    new(Value) { Self.member = Value; }
}

test(atom)
{
    verify(atom(a));
    verify(!atom(X));
    verify((X = a, atom(X)));
    verify(!atom(f(a)));
    verify(!atom([a]));
    verify(!atom(1));
    verify(!atom(1.5));
    verify(!atom("foo"));

    predicate(Foo, foo/2);
    verify(!atom(Foo));

    new baz(Baz1);
    new baz(Baz2, a);
    new baz(Baz3, 1);
    verify(!atom(Baz1.member));
    verify(atom(Baz2.member));
    verify(!atom(Baz3.member));
}

test(atomic)
{
    verify(atomic(a));
    verify(!atomic(X));
    verify((X = a, atomic(X)));
    verify(!atomic(f(a)));
    verify(!atomic([a]));
    verify(atomic(1));
    verify(atomic(1.5));
    verify(atomic("foo"));

    predicate(Foo, foo/2);
    verify(!atomic(Foo));

    new baz(Baz1);
    new baz(Baz2, a);
    new baz(Baz3, 1);
    verify(!atomic(Baz1.member));
    verify(atomic(Baz2.member));
    verify(atomic(Baz3.member));
}

test(compound)
{
    verify(!compound(a));
    verify(!compound([]));
    verify(!compound(X));
    verify(compound(f(X)));
    verify((X = f(Y), compound(X)));
    verify(compound([a]));
    verify(!compound(1));
    verify(!compound(1.5));
    verify(!compound("foo"));

    predicate(Foo, foo/2);
    verify(!compound(Foo));

    new baz(Baz1);
    new baz(Baz2, f(Z));
    new baz(Baz3, 1);
    verify(!compound(Baz1.member));
    verify(compound(Baz2.member));
    verify(!compound(Baz3.member));
}

test(float)
{
    verify(!float(a));
    verify(!float(X));
    verify(!float(f(X)));
    verify(float(1.5));
    verify((X = 1.5, float(X)));
    verify(!float(1));
    verify(!float("foo"));

    predicate(Foo, foo/2);
    verify(!float(Foo));

    new baz(Baz1);
    new baz(Baz2, 1.5);
    new baz(Baz3, 1);
    verify(!float(Baz1.member));
    verify(float(Baz2.member));
    verify(!float(Baz3.member));
}

test(integer)
{
    verify(!integer(a));
    verify(!integer(X));
    verify(!integer(f(X)));
    verify(integer(1));
    verify((X = 1, integer(X)));
    verify(!integer(1.5));
    verify(!integer("foo"));

    predicate(Foo, foo/2);
    verify(!integer(Foo));

    new baz(Baz1);
    new baz(Baz2, 1);
    new baz(Baz3, 1.5);
    verify(!integer(Baz1.member));
    verify(integer(Baz2.member));
    verify(!integer(Baz3.member));
}

test(nonvar)
{
    verify(nonvar(a));
    verify(!nonvar(X));
    verify(nonvar(f(X)));
    verify(nonvar(1));
    verify((X = a, nonvar(X)));
    verify(nonvar(1.5));
    verify(nonvar("foo"));

    predicate(Foo, foo/2);
    verify(nonvar(Foo));

    new baz(Baz1);
    new baz(Baz2, 1);
    new baz(Baz3, Z);
    verify(!nonvar(Baz1.member));
    verify(nonvar(Baz2.member));
    verify(!nonvar(Baz3.member));
}

test(number)
{
    verify(!number(a));
    verify(!number(X));
    verify(!number(f(X)));
    verify(number(1));
    verify((X = 1, number(X)));
    verify(number(1.5));
    verify(!number("foo"));

    predicate(Foo, foo/2);
    verify(!number(Foo));

    new baz(Baz1);
    new baz(Baz2, 1);
    new baz(Baz3, a);
    verify(!number(Baz1.member));
    verify(number(Baz2.member));
    verify(!number(Baz3.member));
}

test(string)
{
    verify(!string(a));
    verify(!string(X));
    verify(!string(f(X)));
    verify(!string(1));
    verify(!string(1.5));
    verify(string("foo"));
    verify((X = "foo", string(X)));

    predicate(Foo, foo/2);
    verify(!string(Foo));

    new baz(Baz1);
    new baz(Baz2, "a");
    new baz(Baz3, a);
    verify(!string(Baz1.member));
    verify(string(Baz2.member));
    verify(!string(Baz3.member));
}

test(var)
{
    verify(!var(a));
    verify(var(X));
    verify(!var(f(X)));
    verify(!var(1));
    verify(!(X = a, var(X)));
    verify((X = Y, var(X)));
    verify(!var(1.5));
    verify(!var("foo"));

    predicate(Foo, foo/2);
    verify(!var(Foo));

    new baz(Baz1);
    new baz(Baz2, Z);
    new baz(Baz3, a);
    verify(var(Baz1.member));
    verify(var(Baz2.member));
    verify(!var(Baz3.member));
}

test(predicate)
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

    verify(predicate(T1));
    verify(!predicate(a));
    verify(!predicate(X));
    verify(!predicate(foo(X, Y)));
    verify(!predicate(foo/2));
    verify(!predicate(1.5));
    verify(!predicate([a]));
    verify(!predicate("foo"));

    new baz(Baz1);
    new baz(Baz2, T1);
    new baz(Baz3, a);
    verify(!predicate(Baz1.member));
    verify(predicate(Baz2.member));
    verify(!predicate(Baz3.member));
}
