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

class foo
{
    var bar

    baz() {}
};

class foobar : foo
{
};

test(assign)
{
    verify(X := 42);
    verify(X == 42);
    verify(X := 20 + 4);
    verify(X == (20 + 4));
    verify(!(X := f(a, b), fail));
    verify(X == f(a, b));
    verify(Y := f(Y));
    verify(Y = f(_));
    verify(!(X := f(b, Z), fail));
    verify((X = f(b, W), W !== Z));

    new foo(F);
    verify(F.bar := [a, b]);
    verify(F.bar == [a, b]);
    verify(F.bar := "foo");
    verify(F.bar == "foo");
    verify(!(F.bar := "bar", fail));
    verify(F.bar == "bar");

    new foobar(FB);
    verify_error(FB.baz := 42, type_error(variable, FB.baz));
    verify(FB..baz := 42);
    verify(FB.baz == 42);
    class(foobar, C);
    verify(C.baz !== 42);

    verify_error(a := H, type_error(variable, a));
}

test(bt_assign)
{
    verify(X :== 42);
    verify(X == 42);
    verify(X :== 20 + 4);
    verify(X == (20 + 4));
    verify(!(X :== f(a, b), fail));
    verify(X == (20 + 4));
    verify(!(X :== f(X)));
    verify(X == (20 + 4));
    verify(X :== f(b, Z));
    verify((X = f(b, W), W == Z));

    new foo(F);
    verify(F.bar :== [a, b]);
    verify(F.bar == [a, b]);
    verify(F.bar :== "foo");
    verify(F.bar == "foo");
    verify(!(F.bar :== "bar", fail));
    verify(F.bar == "foo");

    verify_error(a :== H, type_error(variable, a));
}

test(num_assign)
{
    verify(X ::= 42);
    verify(X == 42);
    verify(X ::= 20 + 4);
    verify(X == 24);
    verify(X ::= 25.0 + 4);
    verify(X == 29.0);
    verify(!(X ::= pi, fail));
    verify(X =:= pi);

    new foo(F);
    verify(F.bar ::= 3.0 / 2);
    verify(F.bar == 1.5);
    verify(F.bar ::= "foo" + "bar");
    verify(F.bar == "foobar");
    verify(!(F.bar ::= "bar", fail));
    verify(F.bar == "bar");

    verify_error(a ::= H, type_error(variable, a));
}

test(bt_num_assign)
{
    verify(X ::== 42);
    verify(X == 42);
    verify(X ::== 20 + 4);
    verify(X == 24);
    verify(X ::== 25.0 + 4);
    verify(X == 29.0);
    verify(!(X ::== pi, fail));
    verify(X =:= 29.0);

    new foo(F);
    verify(F.bar ::== 3.0 / 2);
    verify(F.bar == 1.5);
    verify(F.bar ::== "foo" + "bar");
    verify(F.bar == "foobar");
    verify(!(F.bar ::== "bar", fail));
    verify(F.bar == "foobar");

    verify_error(a ::== H, type_error(variable, a));
}
