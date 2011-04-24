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
:- import(fuzzy).

cold1(3) <<1.0>>.
cold1(10) <<0.7>> { true; }
cold1(27) <<0.0>>.

warm1(3) <<0.0>>.
warm1(10) <<0.3>> { true; }
warm1(27) <<1.0>>.

cold2(Temperature)
{
    fuzzy(1.0 - (Temperature - 3.0) / 24.0);
}

cold3(Temperature)
{
    fuzzy(Temperature, [(3, 1), (27, 0)]);
}

verify_fuzzy(Expected)
{
    fuzzy(Actual);
    Delta is abs(Actual - Expected);
    verify(Delta <= 0.0001);
}

fuzzy()
{
    verify_fuzzy(1.0);

    fuzzy(0.6);
    verify_fuzzy(0.6);

    fuzzy(0.5);
    verify_fuzzy(0.5);

    fuzzy(0.8);
    verify_fuzzy(0.5);

    verify(!fuzzy(0));
    verify_fuzzy(0.5);

    verify(!fuzzy(-42));
    verify_fuzzy(0.5);

    fuzzy(0.5 / 2);
    verify_fuzzy(0.25);

    verify_error(fuzzy("0.1"), type_error(number, "0.1"));
    verify_fuzzy(0.25);
}

fuzzy_and()
{
    verify(fuzzy_and(fuzzy(0.5), fuzzy(0.4)));
    verify_fuzzy(0.4);

    verify(fuzzy_and(fuzzy(0.8), fuzzy(0.6)));
    verify_fuzzy(0.4);

    verify(fuzzy_and(fuzzy(0.1), fuzzy(0.6)));
    verify_fuzzy(0.1);

    verify(!fuzzy_and(fuzzy(0.0), fuzzy(0.6)));
    verify_fuzzy(0.1);

    verify(!fuzzy_and(fuzzy(0.6), fuzzy(0.0)));
    verify_fuzzy(0.1);
}

fuzzy_findall()
{
    verify(fuzzy_findall(T1, cold1(T1), List1));
    verify(List1 == [(3, 1.0), (10, 0.7)]);

    verify(fuzzy_findall(T2, warm1(T2), List2));
    verify(List2 == [(10, 0.3), (27, 1.0)]);

    verify(fuzzy_findall_sorted(T3, cold1(T3), List3));
    verify(List3 == [(3, 1.0), (10, 0.7)]);

    verify(fuzzy_findall_sorted(T4, warm1(T4), List4));
    verify(List4 == [(27, 1.0), (10, 0.3)]);
}

fuzzy_not()
{
    verify(fuzzy_not(fuzzy(0.4)));
    verify_fuzzy(0.6);

    verify(fuzzy_not(fuzzy(0.3)));
    verify_fuzzy(0.6);

    verify(fuzzy_not(fuzzy(0.7)));
    verify_fuzzy(0.3);

    verify(fuzzy_not(fuzzy(0.0)));
    verify_fuzzy(0.3);
}

fuzzy_or()
{
    verify(fuzzy_or(fuzzy(0.5), fuzzy(0.7)));
    verify_fuzzy(0.7);

    verify(fuzzy_or(fuzzy(0.5), fuzzy(0.25)));
    verify_fuzzy(0.5);

    set_fuzzy(1);
    verify(fuzzy_or(fuzzy(0.5), fuzzy(0.0)));
    verify_fuzzy(0.5);

    set_fuzzy(1);
    verify(fuzzy_or(fuzzy(0.0), fuzzy(0.25)));
    verify_fuzzy(0.25);

    verify(!fuzzy_or(fuzzy(0.0), fuzzy(0.0)));
    verify_fuzzy(0.25);
}

fuzzy_prod()
{
    verify(fuzzy_prod(fuzzy(0.5), fuzzy(0.4)));
    verify_fuzzy(0.5 * 0.4);

    verify(fuzzy_prod(fuzzy(0.7), fuzzy(0.4)));
    verify_fuzzy(0.5 * 0.4);

    verify(fuzzy_prod(fuzzy(0.5), fuzzy(0.3)));
    verify_fuzzy(0.5 * 0.3);

    verify(!fuzzy_prod(fuzzy(0.0), fuzzy(0.3)));
    verify_fuzzy(0.5 * 0.3);

    verify(!fuzzy_prod(fuzzy(0.5), fuzzy(0.0)));
    verify_fuzzy(0.5 * 0.3);
}

fuzzy_prod_or()
{
    verify(fuzzy_prod_or(fuzzy(0.5), fuzzy(0.7)));
    verify_fuzzy(0.5 + 0.7 - 0.5 * 0.7);

    verify(fuzzy_prod_or(fuzzy(0.5), fuzzy(0.25)));
    verify_fuzzy(0.5 + 0.25 - 0.5 * 0.25);

    set_fuzzy(1);
    verify(fuzzy_prod_or(fuzzy(0.5), fuzzy(0.0)));
    verify_fuzzy(0.5);

    set_fuzzy(1);
    verify(fuzzy_prod_or(fuzzy(0.0), fuzzy(0.25)));
    verify_fuzzy(0.25);

    verify(!fuzzy_prod_or(fuzzy(0.0), fuzzy(0.0)));
    verify_fuzzy(0.25);
}

interval()
{
    verify(fuzzy(3, [(56, 0.5)]));
    verify_fuzzy(0.5);

    set_fuzzy(1);
    verify(fuzzy(2, [(3, 1), (27, 0)]));
    verify_fuzzy(1);

    verify(fuzzy(3, [(3, 1), (27, 0)]));
    verify_fuzzy(1);

    verify(fuzzy(15, [(3, 1), (27, 0)]));
    verify_fuzzy(0.5);

    verify(!fuzzy(27, [(3, 1), (27, 0)]));

    verify(!fuzzy(42, [(3, 1), (27, 0)]));

    verify_error(fuzzy("1.0", _), type_error(number, "1.0"));
    verify_error(fuzzy(1.0, _), instantiation_error);
    verify_error(fuzzy(1.0, a), type_error(interval_list, a));
    verify_error(fuzzy(1.0, []), type_error(interval_list, []));
    verify_error(fuzzy(1.0, [a]), type_error(interval_list, [a]));
    verify_var_error(fuzzy(1.0, [H1]), type_error(interval_list, [H1]));
    verify_var_error(fuzzy(1.0, [(3,1)|T2]), type_error(interval_list, [T2]));
    verify_var_error(fuzzy(1.0, [(3,1), H3]), type_error(interval_list, [(3,1), H3]));
    verify_error(fuzzy(1.0, [(3,1), (2,1)]), type_error(interval_list, [(3,1), (2,1)]));
    verify_error(fuzzy(1.0, [(2,a), (3,1)]), type_error(interval_list, [(2,a), (3,1)]));
    verify_error(fuzzy(1.0, [(2,1), (a,1)]), type_error(interval_list, [(2,1), (a,1)]));
    verify_error(fuzzy(1.0, [(2,a), (3,1)]), type_error(interval_list, [(2,a), (3,1)]));
    verify_error(fuzzy(1.0, [(2,1), (3,a)]), type_error(interval_list, [(2,1), (3,a)]));
}

predicate()
{
    verify(cold1(3));
    verify_fuzzy(1.0);

    verify(cold1(10));
    verify_fuzzy(0.7);

    verify(!cold1(15));
    verify(!cold1(27));
    verify_fuzzy(0.7);

    set_fuzzy(1);
    verify(cold2(0));
    verify_fuzzy(1.0);

    verify(cold2(3));
    verify_fuzzy(1.0);

    verify(cold2(15));
    verify_fuzzy(0.5);

    verify(!cold2(27));
    verify_fuzzy(0.5);

    set_fuzzy(1);
    verify(cold3(0));
    verify_fuzzy(1.0);

    verify(cold3(3));
    verify_fuzzy(1.0);

    verify(cold3(15));
    verify_fuzzy(0.5);

    verify(!cold3(27));
    verify_fuzzy(0.5);
}

set_fuzzy()
{
    verify_fuzzy(1.0);

    set_fuzzy(0.8);
    verify_fuzzy(0.8);

    verify(!set_fuzzy(0.0));
    verify_fuzzy(0.8);

    verify(!set_fuzzy(-42));
    verify_fuzzy(0.8);

    set_fuzzy(1.5);
    verify_fuzzy(1.0);

    set_fuzzy(0.5 / 2);
    verify_fuzzy(0.25);

    verify_error(set_fuzzy("0.1"), type_error(number, "0.1"));
    verify_fuzzy(0.25);
}

main(_)
{
    init("test-fuzzy");
    run(fuzzy);
    run(fuzzy_and);
    run(fuzzy_findall);
    run(fuzzy_not);
    run(fuzzy_or);
    run(fuzzy_prod);
    run(fuzzy_prod_or);
    run(interval);
    run(predicate);
    run(set_fuzzy);
    report();
}
