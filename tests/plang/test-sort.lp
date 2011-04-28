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

keysort()
{
    verify(keysort([], []));
    verify(keysort([a - 1], [a - 1]));
    verify(keysort([a - 1, y - 2, b - 8, m - 6],
                   [a - 1, b - 8, m - 6, y - 2]));
    verify(keysort([a - 1, y - 2, b - 8, m - 6 | T],
                   [a - 1, b - 8, m - 6, y - 2]));
    verify(keysort([a, y - 2, b, m - 6],
                   [a, b, m - 6, y - 2]));
    verify(keysort([a - 23, a - 1, b - 1, a - 6],
                   [a - 23, a - 1, a - 6, b - 1]));
    verify_error(keysort(L, _), instantiation_error);
    verify_error(keysort(a, _), type_error(list, a));
    verify_error(keysort([a - 1 | b], _), type_error(list, [a - 1 | b]));
}

keysortd()
{
    verify(keysortd([], []));
    verify(keysortd([a - 1], [a - 1]));
    verify(keysortd([a - 1, y - 2, b - 8, m - 6],
                    [y - 2, m - 6, b - 8, a - 1]));
    verify(keysortd([a - 1, y - 2, b - 8, m - 6 | T],
                    [y - 2, m - 6, b - 8, a - 1]));
    verify(keysortd([a, y - 2, b, m - 6],
                    [y - 2, m - 6, b, a]));
    verify(keysortd([a - 23, a - 1, b - 1, a - 6],
                    [b - 1, a - 23, a - 1, a - 6]));
    verify_error(keysortd(L, _), instantiation_error);
    verify_error(keysortd(a, _), type_error(list, a));
    verify_error(keysortd([a - 1 | b], _), type_error(list, [a - 1 | b]));
}

msort()
{
    verify(msort([], []));
    verify(msort([a - 1], [a - 1]));
    verify(msort([a, y, b, m], [a, b, m, y]));
    verify(msort([a, y, b, m | T], [a, b, m, y]));
    verify(msort([a, y, a, m], [a, a, m, y]));
    verify_error(msort(L, _), instantiation_error);
    verify_error(msort(a, _), type_error(list, a));
    verify_error(msort([a | b], _), type_error(list, [a | b]));
}

msortd()
{
    verify(msortd([], []));
    verify(msortd([a - 1], [a - 1]));
    verify(msortd([a, y, b, m], [y, m, b, a]));
    verify(msortd([a, y, b, m | T], [y, m, b, a]));
    verify(msortd([a, y, a, m], [y, m, a, a]));
    verify_error(msortd(L, _), instantiation_error);
    verify_error(msortd(a, _), type_error(list, a));
    verify_error(msortd([a | b], _), type_error(list, [a | b]));
}

rkeysort()
{
    verify(rkeysort([], []));
    verify(rkeysort([a - 1], [a - 1]));
    verify(rkeysort([a - 1, y - 2, b - 8, m - 6],
                    [a - 1, y - 2, m - 6, b - 8]));
    verify(rkeysort([a - 1, y - 2, b - 8, m - 6 | T],
                    [a - 1, y - 2, m - 6, b - 8]));
    verify(rkeysort([1, y - 2, 8, m - 6],
                    [1, y - 2, m - 6, 8]));
    verify(rkeysort([a - 23, b - 1, a - 1, a - 6],
                    [b - 1, a - 1, a - 6, a - 23]));
    verify_error(rkeysort(L, _), instantiation_error);
    verify_error(rkeysort(a, _), type_error(list, a));
    verify_error(rkeysort([a - 1 | b], _), type_error(list, [a - 1 | b]));
}

rkeysortd()
{
    verify(rkeysortd([], []));
    verify(rkeysortd([a - 1], [a - 1]));
    verify(rkeysortd([a - 1, y - 2, b - 8, m - 6],
                     [b - 8, m - 6, y - 2, a - 1]));
    verify(rkeysortd([a - 1, y - 2, b - 8, m - 6 | T],
                     [b - 8, m - 6, y - 2, a - 1]));
    verify(rkeysortd([1, y - 2, 8, m - 6],
                     [8, m - 6, y - 2, 1]));
    verify(rkeysortd([a - 23, b - 1, a - 1, a - 6],
                     [a - 23, a - 6, b - 1, a - 1]));
    verify_error(rkeysortd(L, _), instantiation_error);
    verify_error(rkeysortd(a, _), type_error(list, a));
    verify_error(rkeysortd([a - 1 | b], _), type_error(list, [a - 1 | b]));
}

sort()
{
    verify(sort([], []));
    verify(sort([a - 1], [a - 1]));
    verify(sort([a, y, b, m], [a, b, m, y]));
    verify(sort([a, y, b, m | T], [a, b, m, y]));
    verify(sort([a, y, a, m], [a, m, y]));
    verify_error(sort(L, _), instantiation_error);
    verify_error(sort(a, _), type_error(list, a));
    verify_error(sort([a | b], _), type_error(list, [a | b]));
}

sortd()
{
    verify(sortd([], []));
    verify(sortd([a - 1], [a - 1]));
    verify(sortd([a, y, b, m], [y, m, b, a]));
    verify(sortd([a, y, b, m | T], [y, m, b, a]));
    verify(sortd([a, y, a, m], [y, m, a]));
    verify_error(sortd(L, _), instantiation_error);
    verify_error(sortd(a, _), type_error(list, a));
    verify_error(sortd([a | b], _), type_error(list, [a | b]));
}

generate_list(L, 0)
{
    L = [];
    !;
}
generate_list([N|L], N)
{
    M is N - 1;
    generate_list(L, M);
}

generate_random_list(L, 0, _)
{
    L = [];
    !;
}
generate_random_list([H|L], N, R)
{
    M is N - 1;
    H is integer(random * R);
    generate_random_list(L, M, R);
}

validate_list([], 1501).
validate_list([N|L], N)
{
    M is N + 1;
    validate_list(L, M);
}

validate_ascending_list([], _).
validate_ascending_list([H|L], N)
{
    H >= N;
    validate_ascending_list(L, H);
}

large_sort()
{
    generate_list(L1, 1500);
    sort(L1, OutL1);
    verify(validate_list(OutL1, 1));

    generate_random_list(L2, 1500, 1500);
    msort(L2, OutL2);
    verify(validate_ascending_list(OutL2, 0));
}

main(_)
{
    init("test-sort");
    run(keysort);
    run(keysortd);
    run(msort);
    run(msortd);
    run(rkeysort);
    run(rkeysortd);
    run(sort);
    run(sortd);
    run(large_sort);
    report();
}
