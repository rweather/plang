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

:- import(stdout).
:- dynamic(test::_current_suite/1).
:- dynamic(test::_current_test/1).
:- dynamic(test::_current_results/2).
:- dynamic(test/1).

fail(Message)
{
    test::_current_suite(Suite);
    test::_current_test(Name);
    stdout::writeln(Suite + ": " + Name + ": " + Message);
    throw(test_case_failed);
}

fail(Message, Message2)
{
    test::_current_suite(Suite);
    test::_current_test(Name);
    stdout::writeln(Suite + ": " + Name + ": " + Message + " " + Message2);
    throw(test_case_failed);
}

verify(Goal)
{
    if (call(Goal)) {
        true;
    } else {
        test::_current_suite(Suite);
        test::_current_test(Name);
        stdout::write(Suite + ": " + Name + ": ");
        stdout::writeTerm(Goal);
        stdout::writeln(" failed");
        throw(test_case_failed);
    }
}

verify_error(Goal, ExpectedError)
{
    try {
        call(Goal);
    } catch (error(Error, _)) {
        if (Error !== ExpectedError)
            fail(Goal, Error);
        SawError = true;
    }
    if (var(SawError))
        fail(Goal, "expecting an error");
}

verify_var_error(Goal, ExpectedError)
{
    try {
        call(Goal);
    } catch (error(Error, _)) {
        if (Error != ExpectedError)
            fail(Goal, Error);
        SawError = true;
    }
    if (var(SawError))
        fail(Goal, "expecting an error");
}

verify_fuzzy(Expected)
{
    '$$fuzzy'(Actual);
    Delta is abs(Actual - Expected);
    verify(Delta <= 0.0001);
}

compare(Actual, Expected)
{
    if (Actual !== Expected) {
        test::_current_suite(Suite);
        test::_current_test(Name);
        stdout::write(Suite + ": " + Name + ": ");
        stdout::writeTerm(Actual);
        stdout::write(" == ");
        stdout::writeTerm(Expected);
        stdout::writeln(" failed");
        throw(test_case_failed);
    }
}

test::_init(Suite)
{
    abolish(test::_current_suite/1);
    assertz(test::_current_suite(Suite));

    abolish(test::_current_results/2);
    assertz(test::_current_results(0, 0));
}

test::_report()
{
    test::_current_suite(Suite);
    test::_current_results(Pass, Fail);
    stdout::writeln(Suite + ": " + Pass + " passed, " + Fail + " failed");
    if (Fail !== 0)
        fail;
}

test::_run_tests()
{
    clause(test(Name), Body);
    abolish(test::_current_test/1);
    assertz(test::_current_test(Name));
    '$$set_fuzzy'(1);
    try {
        if (!once(Body))
            fail("did not succeed");
    } catch (test_case_failed) {
        TestFailed = true;
    } catch (Error) {
        test::_current_suite(Suite);
        stdout::write(Suite + ": " + Name + ": ");
        stdout::writeTerm(Error);
        stdout::writeln();
        TestFailed = true;
    }
    if (nonvar(TestFailed)) {
        retract(test::_current_results(Pass, Fail));
        Fail2 is Fail + 1;
        assertz(test::_current_results(Pass, Fail2));
    } else {
        test::_current_suite(Suite);
        stdout::writeln(Suite + ": " + Name + ": ok");
        retract(test::_current_results(Pass, Fail));
        Pass2 is Pass + 1;
        assertz(test::_current_results(Pass2, Fail));
    }
    fail;
}
test::_run_tests().

test::_suite_name(File, Suite)
{
    Length is length(File);
    Found = false;
    while [Char] (Length > 0 && !Found) {
        Char is char(File, Length - 1);
        if (Char == 0'/' || Char == 0'\\')
            Found := true;
        else
            Length ::= Length - 1;
    }
    Base is mid(File, Length);
    if (right(Base, 3) =:= ".lp")
        Suite is left(Base, length(Base) - 3);
    else
        Suite = Base;
}

test::main([File|_])
{
    test::_suite_name(File, Suite);
    test::_init(Suite);
    test::_run_tests();
    test::_report();
}
