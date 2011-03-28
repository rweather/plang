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
:- dynamic(current_suite/1).
:- dynamic(current_test/1).
:- dynamic(current_results/2).

fail(Message)
{
    current_suite(Suite);
    current_test(Name);
    stdout::write(Suite);
    stdout::write(": ");
    stdout::write(Name);
    stdout::write(": ");
    stdout::writeln(Message);
    throw(test_case_failed);
}

fail(Message, Message2)
{
    current_suite(Suite);
    current_test(Name);
    stdout::write(Suite);
    stdout::write(": ");
    stdout::write(Name);
    stdout::write(": ");
    stdout::write(Message);
    stdout::write(" ");
    stdout::writeln(Message2);
    throw(test_case_failed);
}

warning(Message)
{
    current_suite(Suite);
    current_test(Name);
    stdout::write(Suite);
    stdout::write(": ");
    stdout::write(Name);
    stdout::write(": warning: ");
    stdout::writeln(Message);
}

verify(Goal)
{
    if (!call(Goal)) {
        current_suite(Suite);
        current_test(Name);
        stdout::write(Suite);
        stdout::write(": ");
        stdout::write(Name);
        stdout::write(": ");
        stdout::write(Goal);
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

compare(Actual, Expected)
{
    if (Actual !== Expected) {
        current_suite(Suite);
        current_test(Name);
        stdout::write(Suite);
        stdout::write(": ");
        stdout::write(Name);
        stdout::write(": ");
        stdout::writeq(Actual);
        stdout::write(" == ");
        stdout::writeq(Expected);
        stdout::writeln(" failed");
        throw(test_case_failed);
    }
}

init(Suite)
{
    abolish(current_suite/1);
    assertz(current_suite(Suite));

    abolish(current_results/2);
    assertz(current_results(0, 0));
}

run(Name)
{
    abolish(current_test/1);
    assertz(current_test(Name));
    try {
        if (!Name)
            fail("did not succeed");
    } catch (test_case_failed) {
        TestFailed = true;
    } catch (Error) {
        current_suite(Suite);
        stdout::write(Suite);
        stdout::write(": ");
        stdout::write(Name);
        stdout::write(": ");
        stdout::writeqln(Error);
        TestFailed = true;
    }
    if (nonvar(TestFailed)) {
        retract(current_results(Pass, Fail));
        Fail2 is Fail + 1;
        assertz(current_results(Pass, Fail2));
    } else {
        current_suite(Suite);
        stdout::write(Suite);
        stdout::write(": ");
        stdout::write(Name);
        stdout::writeln(": ok");
        retract(current_results(Pass, Fail));
        Pass2 is Pass + 1;
        assertz(current_results(Pass2, Fail));
    }
}

report()
{
    current_suite(Suite);
    current_results(Pass, Fail);
    stdout::write(Suite);
    stdout::write(": ");
    stdout::write(Pass);
    stdout::write(" passed, ");
    stdout::write(Fail);
    stdout::writeln(" failed");
    if (Fail !== 0)
        fail;
}
