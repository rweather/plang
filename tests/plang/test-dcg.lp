/*
 * plang logic programming language
 * Copyright (C) 2011,2012  Southern Storm Software, Pty Ltd.
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

:- import(test).

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

test(check_sentences)
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

empty1 --> [].
empty2 --> empty3, [].
empty3 --> [a].

test(empty_rule)
{
    verify(empty1([], []));
    verify(empty2([a], []));
}

or1 --> [a] || [b].
or2 --> ([a] || [b]), [c].

test(or_rule)
{
    verify(or1([a], []));
    verify(or1([b], []));
    verify(or2([a, c], []));
    verify(or2([b, c], []));
}

not1 --> [a], ![b], [c].

test(not_rule)
{
    verify(not1([a, c], []));
    verify(!not1([a, b, c], []));
}

commit1 --> [a], [b], !, [c].
commit1 --> [a], [b], [d].

commit2 --> [a], [b], commit, [c].
commit2 --> [a], [b], [d].

test(commit_rule)
{
    verify(commit1([a, b, c], []));
    verify(!commit1([a, b, d], []));

    verify(commit2([a, b, c], []));
    verify(!commit2([a, b, d], []));
}

statement1 --> [X], {integer(X);}, [a].
statement1 --> [b], [c].
statement1 --> [c], {}, [d].

test(statement_rule)
{
    verify(statement1([42, a], []));
    verify(statement1([b, c], []));
    verify(!statement1([b, a], []));
    verify(statement1([c, d], []));
}

list1 --> [a], [b].
list1 --> [a, c, D], [b].

test(list_rule)
{
    verify(list1([a, b], []));
    verify(list1([a, c, d, b], []));
    verify(list1([a, c, e, b], []));
}

string1 --> ["a"], ["b"].
string1 --> ["a", "c", D], ["b"].

string2 --> "a", "b".
string2 --> "a", "c", [D], "b".

string3 --> string2, "f".

test(string_rule)
{
    verify(string1(["a", "b"], []));
    verify(string1(["a", "c", "d", "b"], []));
    verify(string1(["a", "c", "e", "b"], []));

    verify(string2(["a", "b"], []));
    verify(string2(["a", "c", "d", "b"], []));
    verify(string2(["a", "c", "e", "b"], []));

    verify(string3(["a", "b", "f"], []));
    verify(string3(["a", "c", "d", "b", "f"], []));
    verify(string3(["a", "c", "e", "b", "f"], []));
}

test(assert)
{
    assertz((dt --> "the"));
    verify(dt(["the"], []));
    retract((dt --> "the"));
    verify_error(dt(["the"], []), existence_error(procedure, dt/2));
}
