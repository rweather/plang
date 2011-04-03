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

:- load_library(plang_wordnet).

/** @cond */

// DCG-compatible word checking predicates.

words::adjective([Word|Out], Out)
{
    words::adjective(Word);
}
words::adjective(Word, [Word|Out], Out)
{
    words::adjective(Word);
}

words::adverb([Word|Out], Out)
{
    words::adverb(Word);
}
words::adverb(Word, [Word|Out], Out)
{
    words::adverb(Word);
}

words::noun([Word|Out], Out)
{
    words::noun(Word);
}
words::noun(Word, [Word|Out], Out)
{
    words::noun(Word);
}

words::verb([Word|Out], Out)
{
    words::verb(Word);
}
words::verb(Word, [Word|Out], Out)
{
    words::verb(Word);
}

// Convenience search predicates.

words::overview(Word, Result)
{
    if (nonvar(Result))
        throw(error(type_error(variable, Result), words::overview/2));
    if (words::description(Word, adjective, overview, allsenses, AResult))
        ADesc is AResult;
    else
        ADesc is "";
    if (words::description(Word, adverb, overview, allsenses, DResult))
        DDesc is DResult;
    else
        DDesc is "";
    if (words::description(Word, noun, overview, allsenses, NResult))
        NDesc is NResult;
    else
        NDesc is "";
    if (words::description(Word, verb, overview, allsenses, VResult))
        VDesc is VResult;
    else
        VDesc is "";
    Result is ADesc + DDesc + NDesc + VDesc;
    if (Result == "") fail;
}

/** @endcond */
