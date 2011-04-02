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

:- import(testcase).

// Examples from: http://en.wikipedia.org/wiki/Definite_clause_grammar

sentence --> noun_phrase, verb_phrase.
sentence --> pronoun(subject), verb_phrase.
noun_phrase --> det, noun.
verb_phrase --> verb, noun_phrase.
verb_phrase --> verb, pronoun(object).
det --> [the].
det --> [a].
noun --> [cat].
noun --> [bat].
verb --> [eats].
verb --> [likes].
pronoun(subject) --> [he].
pronoun(subject) --> [she].
pronoun(object) --> [him].
pronoun(object) --> [her].

s --> symbols(Sem, a), symbols(Sem, b), symbols(Sem, c).
symbols(end, S) --> [].
symbols(s(Sem), S) --> [S], symbols(Sem, S).

check_sentences()
{
    verify(sentence([the, cat, eats, the, bat], []));
    verify(sentence([a, bat, eats, the, cat], []));

    verify(sentence([he, likes, her], []));
    verify(!sentence([her, likes, he], []));

    verify(s([a, b, c], []));
    verify(!s([c, b, a], []));
    verify(s([a, a, a, b, b, b, c, c, c], []));
    verify(!s([a, a, b, b, b, c, c, c], []));
}

main(_)
{
    init("test-dcg");
    run(check_sentences);
    report();
}
