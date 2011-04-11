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

/** @cond */
class iostream
{
    canRead() { fail; }
    canSeek() { fail; }
    canWrite() { fail; }

    close() { true; }

    flush()
    {
        throw(error(permission_error(output, stream, Self), iostream::flush/1));
    }

    length(Length)
    {
        throw(error(permission_error(reposition, stream, Self), iostream::length/2));
    }

    readByte(Byte)
    {
        throw(error(permission_error(input, stream, Self), iostream::readByte/2));
    }

    readBytes(Bytes, MaxLength)
    {
        throw(error(permission_error(input, stream, Self), iostream::readBytes/3));
    }

    readLine(Line)
    {
        throw(error(permission_error(input, stream, Self), iostream::readLine/2));
    }

    readTerm(Term)
    {
        '$$iostream_readTerm'(Self, Term);
    }

    readTerm(Term, Vars)
    {
        '$$iostream_readTerm'(Self, Term, Vars);
    }

    seek(Position, Origin)
    {
        throw(error(permission_error(reposition, stream, Self), iostream::seek/3));
    }

    seek(Position)
    {
        Self.seek(Position, start);
    }

    tell(Position)
    {
        throw(error(permission_error(reposition, stream, Self), iostream::tell/2));
    }

    writeByte(Byte)
    {
        throw(error(permission_error(output, stream, Self), iostream::writeByte/2));
    }

    writeBytes(Bytes)
    {
        throw(error(permission_error(output, stream, Self), iostream::writeBytes/2));
    }

    writeLine(Line)
    {
        Self.writeBytes(Line);
        Self.writeBytes("\n");
    }

    writeTerm(Term)
    {
        '$$iostream_writeTerm'(Self, Term);
    }

    writeTerm(Term, Vars)
    {
        '$$iostream_writeTerm'(Self, Term, Vars);
    }
}
/** @endcond */
