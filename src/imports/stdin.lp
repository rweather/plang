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

class stdin : iostream
{
    canRead() { true; }

    readByte(Byte)
    {
        '$$stdin_read_byte'(Byte);
    }

    readBytes(Bytes, MaxLength)
    {
        if (var(MaxLength))
            throw(error(instantiation_error, stdin::readBytes/3));
        else if (!integer(MaxLength))
            throw(error(type_error(integer, MaxLength), stdin::readBytes/3));
        else if (MaxLength < 0)
            throw(error(domain_error(not_less_than_zero, MaxLength), stdin::readBytes/3));
        else
            '$$stdin_read_bytes'(Bytes, MaxLength);
    }

    readLine(Line)
    {
        '$$stdin_read_line'(Line);
    }
}
