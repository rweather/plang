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

class stdout : iostream
{
    canWrite() { true; }

    flush()
    {
        '$$print_flush'(1);
    }

    static flush()
    {
        '$$print_flush'(1);
    }

    writeByte(Byte)
    {
        '$$print_byte'(1, Byte);
    }

    writeString(String)
    {
        '$$print_string'(1, String);
    }

    static write(Term)
    {
        var(Term);
        commit;
        '$$print'(1, Term);
    }
    static write(A + B)
    {
        commit;
        stdout::write(A);
        stdout::write(B);
    }
    static write(Term)
    {
        string(Term);
        commit;
        '$$print_string'(1, Term);
    }
    static write(Term)
    {
        '$$print'(1, Term);
    }

    static writeln(Term)
    {
        stdout::write(Term);
        '$$print_string'(1, "\n");
    }

    static writeln()
    {
        '$$print_string'(1, "\n");
    }

    static writeTerm(Term)
    {
        '$$print'(1, Term);
    }

    static writeTerm(Term, Vars)
    {
        '$$print'(1, Term, Vars);
    }
}
