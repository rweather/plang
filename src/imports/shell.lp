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
:- import(stdin).

shell::write_vars([], _) {}
shell::write_vars([Name = Var], Vars)
{
    !;
    stdout::write(Name + " = ");
    stdout::writeTerm(Var, Vars);
}
shell::write_vars([Name = Var|Tail], Vars)
{
    !;
    stdout::write(Name + " = ");
    stdout::writeTerm(Var, Vars);
    stdout::writeln();
    shell::write_vars(Tail, Vars);
}

shell::call_until_fail(Term, Vars, Stdin)
{
    call(Term);
    stdout::writeln();
    shell::write_vars(Vars, Vars);
    stdout::write(" ? ");
    stdout::flush();
    Stdin.readLine(Line);
    if (Line == ";")
        fail;
    else
        throw(shell(stop));
}

shell::call(Term, _, _)
{
    var(Term);
    !;
    throw(error(instantiation_error, call/1));
}
shell::call(quit, _, _)
{
    throw(shell(quit));
}
shell::call(exit, _, _)
{
    throw(shell(quit));
}
shell::call(help, _, _)
{
    !;
    stdout::writeln();
    stdout::writeln("At the '| ?-' prompt, type expressions to evaluate, followed by '.':");
    stdout::writeln();
    stdout::writeln("    | ?- X = 1.");
    stdout::writeln("    | ?- stdout::writeln(\"Hello World!\").");
    stdout::writeln();
    stdout::writeln("At the '?' prompt, type ';' to get the next solution, or Enter");
    stdout::writeln("to stop searching for solutions:");
    stdout::writeln();
    stdout::writeln("    | ?- X = 1 || X = 2.");
    stdout::writeln("    X = 1 ? ;");
    stdout::writeln("    X = 2 ? ;");
    stdout::writeln("    no");
    stdout::writeln();
    stdout::writeln("consult(\"filename.lp\") - Load a source file.");
    stdout::writeln("exit or quit           - Exit the shell.");
    stdout::writeln();
}
shell::call(Term, [], _)
{
    !;
    if (call(Term))
        stdout::writeln("\nyes\n");
    else
        stdout::writeln("\nno\n");
}
shell::call(Term, Vars, Stdin)
{
    try {
        (shell::call_until_fail(Term, Vars, Stdin) || true);
        stdout::writeln("\nno\n");
    } catch (shell(stop)) {
        stdout::writeln();
    }
}

shell::main(Prompt)
{
    new stdin(Stdin);
    do [Term, Vars, Error] {
        stdout::write(Prompt);
        stdout::flush();
        try {
            (Stdin.readTerm(Term, Vars)
                || Term = exit, stdout::writeln("exit."));
            shell::call(Term, Vars, Stdin);
        } catch (error(syntax_error(_), _)) {
            true;
        } catch (shell(quit)) {
            Quit = yes;
        } catch (Error) {
            stdout::writeln(Error);
        }
    } while (Quit !== yes);
}
