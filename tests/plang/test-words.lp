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
:- import(words).

test(adjective)
{
    verify(words::adjective(animated));
    verify(words::adjective("Animated"));
    verify(!words::adjective(harpsichord));
    verify(!words::adjective(X));
    verify(!words::adjective(15));
}

test(adverb)
{
    verify(words::adverb(fitfully));
    verify(words::adverb("Fitfully"));
    verify(!words::adverb(harpsichord));
    verify(!words::adverb(X));
    verify(!words::adverb(15));
}

test(noun)
{
    verify(words::noun(harpsichord));
    verify(words::noun("Harpsichord"));
    verify(words::noun("Bobby Fischer"));
    verify(!words::noun(fitfully));
    verify(!words::noun(X));
    verify(!words::noun(15));
}

test(verb)
{
    verify(words::verb(annoy));
    verify(words::verb("Annoy"));
    verify(words::verb("hand out"));
    verify(!words::verb(harpsichord));
    verify(!words::verb(X));
    verify(!words::verb(15));
}

test(description)
{
    // walk as a verb has one sense with an antonym.
    verify(words::description(walk, verb, antptr, allsenses, Desc1));
    verify(words::description(walk, verb, antptr, 1, Desc2));
    verify(!words::description(walk, verb, antptr, 2, Desc3));

    verify_error(words::description(Word, verb, antptr, 1, Desc4), instantiation_error);
    verify_error(words::description(walk, Verb, antptr, 1, Desc5), instantiation_error);
    verify_error(words::description(walk, verb, Antptr, 1, Desc6), instantiation_error);
    verify_error(words::description(walk, verb, antptr, One, Desc7), instantiation_error);
    verify_error(words::description(walk, verb, antptr, 1, nonvar), type_error(variable, nonvar));
    verify_error(words::description(1.5, verb, antptr, 1, Desc8), type_error(atom_or_string, 1.5));
    verify_error(words::description(walk, unverb, antptr, 1, Desc9), type_error(part_of_speech, unverb));
    verify_error(words::description(walk, verb, query, 1, Desc10), type_error(word_query, query));
    verify_error(words::description(walk, verb, synset, 1, Desc11), type_error(word_query, synset));
    verify_error(words::description(walk, verb, antptr, 0, Desc12), type_error(word_sense, 0));
    verify_error(words::description(walk, verb, antptr, foo, Desc13), type_error(word_sense, foo));
}

test(overview)
{
    verify(words::overview(hand, Desc1) && Desc1 =!= "");
    verify(words::overview("Bobby Fischer", Desc2) && Desc2 =!= "");
    verify(!words::overview(xyzzy, Desc3));

    verify_error(words::overview(Word, Desc4), instantiation_error);
    verify_error(words::overview(hand, res), type_error(variable, res));
}

test(search)
{
    verify(words::search("walk", verb, antptr, allsenses, List) && List == ["ride"]);
    verify(words::search("hand", noun, haspartptr, allsenses, List2));
    verify(words::search("hand", noun, synset, 1, List3) && List3 == ["hand", "manus", "mitt", "paw"]);
    verify(!words::search("xyzzy", noun, synset, allsenses, List4));

    verify_error(words::search(Word, verb, antptr, 1, List5), instantiation_error);
    verify_error(words::search(walk, Verb, antptr, 1, List6), instantiation_error);
    verify_error(words::search(walk, verb, Antptr, 1, List7), instantiation_error);
    verify_error(words::search(walk, verb, antptr, One, List8), instantiation_error);
    verify_error(words::search(walk, verb, antptr, 1, nonvar), type_error(variable, nonvar));
    verify_error(words::search(1.5, verb, antptr, 1, List9), type_error(atom_or_string, 1.5));
    verify_error(words::search(walk, unverb, antptr, 1, List10), type_error(part_of_speech, unverb));
    verify_error(words::search(walk, verb, query, 1, List11), type_error(word_query, query));
    verify_error(words::search(walk, verb, antptr, 0, List12), type_error(word_sense, 0));
    verify_error(words::search(walk, verb, antptr, foo, List13), type_error(word_sense, foo));
}

test(base_forms)
{
    verify(words::base_forms("eating", verb, Base1) && Base1 == ["eat"]);
    verify(words::base_forms(eating, verb, ["eat"]));
    verify(!words::base_forms("eat", verb, Base2));

    verify(words::base_form("eating", verb, Base3) && Base3 == "eat");
    verify(words::base_form("Eating", verb, Base4) && Base4 == "eat");
    verify(words::base_form(eating, verb, Base5) && Base5 == "eat");
    verify(words::base_form("Eat", verb, Base6) && Base6 == "Eat");
    verify(words::base_form(eat, verb, Base7) && Base7 == eat);
    verify(!words::base_form(eatb, verb, _));
    verify(!words::base_form("bobby", verb, _));
    verify(words::base_form("bobby", noun, "bobby"));

    verify_error(words::base_forms(Word, verb, Base8), instantiation_error);
    verify_error(words::base_forms("eating", Verb, Base9), instantiation_error);
    verify_error(words::base_forms(1.5, verb, Base10), type_error(atom_or_string, 1.5));
    verify_error(words::base_forms("eating", unverb, Base11), type_error(part_of_speech, unverb));
}

sentence --> noun_phrase, verb_phrase.
noun_phrase --> det, words::noun.
verb_phrase --> words::verb, noun_phrase.
det --> "the".
det --> "a".

test(parse)
{
    verify(sentence(["the", "cat", "eats", "a", "bat"], []));
    verify(sentence(["the", "car", "killed", "a", "pedestrian"], []));
}
