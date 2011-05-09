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

:- import(test).
:- dynamic(index_pred/2).
:- dynamic(index_pred_second/2).

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

test(retract)
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
