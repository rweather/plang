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

test(term_eq)
{
    verify(X == X);
    verify(!(X == Y));
    verify(f(X, Y) == f(X, Y));
    verify(!(f(X, Y) == f(Y, X)));
}

test(term_ne)
{
    verify(!(X !== X));
    verify(X !== Y);
    verify(!(f(X, Y) !== f(X, Y)));
    verify(f(X, Y) !== f(Y, X));

    verify(!(X \== X));
    verify(X \== Y);
    verify(!(f(X, Y) \== f(X, Y)));
    verify(f(X, Y) \== f(Y, X));

    verify(!'\\=='(X, X));
    verify('\\=='(X, Y));
    verify(!'\\=='(f(X, Y), f(X, Y)));
    verify('\\=='(f(X, Y), f(Y, X)));
}

test(term_lt)
{
    verify(f(j) @< f(k));
    verify(!(f(k) @< f(j)));
    verify(!(f(j) @< f(j)));
    verify(2.0 @< 1);
}

test(term_le)
{
    verify(f(j) @<= f(k));
    verify(!(f(k) @<= f(j)));
    verify(f(j) @<= f(j));
    verify(2.0 @<= 1);

    verify(f(j) @=< f(k));
    verify(!(f(k) @=< f(j)));
    verify(f(j) @=< f(j));
    verify(2.0 @=< 1);

    verify('@=<'(f(j), f(k)));
    verify(!'@=<'(f(k), f(j)));
    verify('@=<'(f(j), f(j)));
    verify('@=<'(2.0, 1));
}

test(term_gt)
{
    verify(!(f(j) @> f(k)));
    verify(f(k) @> f(j));
    verify(!(f(j) @> f(j)));
    verify(!(2.0 @> 1));
}

test(term_ge)
{
    verify(!(f(j) @>= f(k)));
    verify(f(k) @>= f(j));
    verify(f(j) @>= f(j));
    verify(!(2.0 @>= 1));
}
