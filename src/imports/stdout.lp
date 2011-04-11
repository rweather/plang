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

/** @cond */
class stdout : iostream
{
    canWrite() { true; }

    writeByte(Byte)
    {
        '$$print_byte'(1, Byte);
    }

    writeBytes(Bytes)
    {
        '$$print_string'(1, Bytes);
    }

    // TODO: remove writeTerm() from here once iostream is finished.
    writeTerm(Term)
    {
        '$$print'(1, Term);
    }
    writeTerm(Term, Vars)
    {
        '$$print'(1, Term);
    }

    static write(Term)
    {
        var(Term);
        !;
        '$$print'(1, Term);
    }
    static write(A + B)
    {
        !;
        stdout::write(A);
        stdout::write(B);
    }
    static write(Term)
    {
        string(Term);
        !;
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
}
/** @endcond */
