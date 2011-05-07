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

:- import(iostream).

class stderr : iostream
{
    canWrite() { true; }

    flush()
    {
        '$$print_flush'(2);
    }

    static flush()
    {
        '$$print_flush'(2);
    }

    writeByte(Byte)
    {
        '$$print_byte'(2, Byte);
    }

    writeString(String)
    {
        '$$print_string'(2, String);
    }

    static write(Term)
    {
        var(Term);
        commit;
        '$$print'(2, Term);
    }
    static write(A + B)
    {
        commit;
        stderr::write(A);
        stderr::write(B);
    }
    static write(Term)
    {
        string(Term);
        commit;
        '$$print_string'(2, Term);
    }
    static write(Term)
    {
        '$$print'(2, Term);
    }

    static writeln(Term)
    {
        stderr::write(Term);
        '$$print_string'(2, "\n");
    }

    static writeln()
    {
        '$$print_string'(2, "\n");
    }

    static writeTerm(Term)
    {
        '$$print'(2, Term);
    }

    static writeTerm(Term, Vars)
    {
        '$$print'(2, Term, Vars);
    }
}
