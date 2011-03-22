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

stderr::write(Term)
{
    '$$print_error'(Term);
}

stderr::writeln(Term)
{
    '$$print_error'(Term);
    '$$println_error'();
}

stderr::writeln()
{
    '$$println_error'();
}

stderr::writeq(Term)
{
    '$$printq_error'(Term);
}

stderr::writeqln(Term)
{
    '$$printq_error'(Term);
    '$$println_error'();
}
