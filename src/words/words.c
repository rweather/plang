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

#include <plang/database.h>
#include <plang/errors.h>
#include <stdlib.h>
#include <string.h>
#include <wn.h>

/**
 * \addtogroup module_words
 *
 * \section words_intro Introduction
 *
 * The <tt>words</tt> module provides access to the
 * <a href="http://wordnet.princeton.edu/">WordNet</a> lexical
 * database from Princeton University.  The database contains
 * huge amounts of information about English words, particularly
 * their part of speech (noun, verb, adverb, or adjective).
 * This information can be very useful when building Natural
 * Language Processing systems in Plang.
 *
 * Note: the <tt>words</tt> module is optional in Plang and will
 * only be available if WordNet was present on the system when
 * Plang was built.  Many GNU/Linux distributions have WordNet
 * packages already.  You can try installing <tt>wordnet</tt> and
 * <tt>wordnet-devel</tt> (or <tt>wordnet-dev</tt>) to see if
 * your distribution already has WordNet.  If not, download and
 * build it from the sources at the link above.  You will need
 * the "devel" package installed to build Plang with WordNet.
 *
 * The WordNet library and database is distributed under a
 * <a href="http://wordnet.princeton.edu/wordnet/license/">BSD-style
 * license</a>.  The Plang <tt>words</tt> module that wraps the
 * WordNet library is distributed under the GNU Lesser General
 * Public License, Version 3 (LGPLv3).
 *
 * The only language supported by WordNet is English.  Primarily
 * American English, but there are also British English spellings
 * in the database (e.g. "colour", "organise", etc).  Other languages
 * have different parts of speech and sentence structure, so will
 * require separate add-on modules to handle them in Plang.
 *
 * \section words_testing Word testing
 *
 * To use the Plang module, first import the <tt>words</tt> module
 * into your application:
 *
 * \code
 * :- import(words).
 * \endcode
 *
 * The names of the predicates in the module are prefixed with
 * "words::" such as \ref words_adjective_1 "words::adjective/1"
 * and \ref words_noun_1 "words::noun/1".  The part of speech
 * testing predicates take a single argument and succeed or fail
 * depending upon whether the word has a specific part of speech:
 *
 * \code
 * :- import(words).
 * :- import(stdout).
 *
 * dump_word(Word)
 * {
 *     stdout::write(Word);
 *     stdout::write(":");
 *     if (words::adjective(Word))
 *         stdout::write(" adjective");
 *     if (words::adverb(Word))
 *         stdout::write(" adverb");
 *     if (words::noun(Word))
 *         stdout::write(" noun");
 *     if (words::verb(Word))
 *         stdout::write(" verb");
 *     stdout::writeln();
 * }
 * \endcode
 *
 * The predicates in the <tt>words</tt> module can also be used
 * in definite clause grammar (DCG) rules to help parse sentences
 * in English:
 *
 * \code
 * sentence --> noun_phrase, verb_phrase.
 * noun_phrase --> det, words::noun.
 * verb_phrase --> words::verb, noun_phrase.
 * det --> ["the"].
 * det --> ["a"].
 * \endcode
 *
 * \section words_multi Multi-word entries
 *
 * The WordNet database contains a large number of multi-word
 * entries such as "bobby_fischer", "hand_out", "short_and_sweet", etc.
 * These are also recognized by the part of speech testing predicates.
 * Either a space or an underscore can be used as the word separator.
 * It is possible to modify a set of DCG rules to recognize
 * multi-word forms in a regular sentence as shown in the
 * following example:
 *
 * \code
 * noun_phrase --> det, noun.
 *
 * noun([Word|Out], Out)
 * {
 *     words::noun(Word);
 * }
 * noun([Word1,Word2|Out], Out)
 * {
 *     Word is Word1 + "_" + Word2;
 *     words::noun(Word);
 * }
 * noun([Word1,Word2,Word3|Out], Out)
 * {
 *     Word is Word1 + "_" + Word2 + "_" + Word3;
 *     words::noun(Word);
 * }
 * \endcode
 *
 * \section words_queries Advanced queries
 *
 * WordNet has a large number of queries that can be performed
 * on words.  Most produce a list of related words that are in some
 * relationship with the word being queried.  In Plang, advanced
 * queries on the WordNet database can be performed with
 * \ref words_search_5 "words::search/5",
 * \ref words_overview_2 "words::overview/2", and
 * \ref words_description_5 "words::description/5".
 * For example, the following code queries for the parts of
 * the noun "hand" according to all senses of the word:
 *
 * \code
 * words::search("hand", noun, haspartptr, allsenses, Result);
 * \endcode
 *
 * The <tt>Result</tt> will be a list that includes words like
 * "finger", "palm", etc.  An overview of the word, similar to a
 * dictionary entry, can be obtained with
 * \ref words_description_5 "words::description/5":
 *
 * \code
 * words::description("hand", noun, overview, allsenses, Description);
 * \endcode
 *
 * A dictionary-like entry that lists all parts of speech
 * and senses of a word can be retrieved with
 * \ref words_overview_2 "words::overview/2":
 *
 * \code
 * words::word("hand", Description);
 * \endcode
 *
 * The permitted query types are based on the names given to them by
 * <a href="http://wordnet.princeton.edu/wordnet/man/wnsearch.3WN.html">WordNet</a>:
 *
 * \code
 * antptr, hyperptr, hypoptr, entailptr, simptr, ismemberptr,
 * isstuffptr, ispartptr, hasmemberptr, hasstuffptr, haspartptr,
 * meronym, holonym, causeto, pplptr, seealsoptr, pertptr,
 * attribute, verbgroup, derivation, classification, class, syns,
 * freq, frames, coords, relatives, hmeronym, hholonym, wngrep,
 * overview, classif_category, classif_usage, classif_regional,
 * class_category, class_usage, class_regional, instance, instances
 * \endcode
 *
 * Some familiarity with WordNet's terminology will be required to
 * correctly format an advanced query on the database.
 *
 * \ref words_adjective_1 "words::adjective/1",
 * \ref words_adverb_1 "words::adverb/1",
 * \ref words_description_5 "words::description/5",
 * \ref words_noun_1 "words::noun/1",
 * \ref words_overview_2 "words::overview/2",
 * \ref words_search_5 "words::search/5",
 * \ref words_verb_1 "words::verb/1"
 */

/** @cond */
#define P_WORD_HASH_SIZE 511
typedef struct p_word p_word;
struct p_word
{
    p_word *next;
    unsigned int word_flags;
    char word[1];
};
static p_word *p_word_hash[P_WORD_HASH_SIZE];
static int p_initialized = 0;
/** @endcond */

/* Match two words, where "word1" is assumed to already
 * be in lower case and normalized */
static int p_word_match(const char *word1, const char *word2)
{
    int ch1, ch2;
    while (*word1 != '\0' && *word2 != '\0') {
        ch1 = ((int)(*word1++)) & 0xFF;
        ch2 = ((int)(*word2++)) & 0xFF;
        if (ch2 >= 'A' && ch2 <= 'Z')
            ch2 += 'a' - 'A';
        else if (ch2 == ' ')
            ch2 = '_';
        if (ch1 != ch2)
            return 0;
    }
    return *word1 == '\0' && *word2 == '\0';
}

/* Copy a string, converting to lower case and normalizing */
static void p_word_strcpy(char *dest, const char *src)
{
    int ch;
    while (*src != '\0') {
        ch = ((int)(*src++)) & 0xFF;
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        else if (ch == ' ')
            ch = '_';
        *dest++ = (char)ch;
    }
    *dest = '\0';
}

/* Check the kind on a word.  We cache the results from WordNet
 * because it doesn't implement its own cache */
static p_goal_result word_check
    (p_context *context, p_term *word, int kind)
{
    int type;
    const char *name;
    const char *temp;
    unsigned int hash;
    unsigned int ch;
    size_t len;
    p_word *current;

    /* Check that the word term is an atom or string */
    word = p_term_deref_member(context, word);
    type = p_term_type(word);
    if (type != P_TERM_ATOM && type != P_TERM_STRING)
        return 0;

    /* Compute the hash value for the word */
    name = p_term_name(word);
    temp = name;
    hash = 0;
    len = 0;
    while (*temp != '\0') {
        ch = ((unsigned int)(*temp++)) & 0xFF;
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        else if (ch == ' ')
            ch = '_';
        hash = hash * 5 + ch;
        ++len;
    }
    hash %= P_WORD_HASH_SIZE;

    /* Search for the word in the hash */
    current = p_word_hash[hash];
    while (current) {
        if (p_word_match(current->word, name)) {
            if (current->word_flags & bit(kind))
                return P_RESULT_TRUE;
            else
                return P_RESULT_FAIL;
        }
        current = current->next;
    }

    /* Create a new hash node for the word */
    current = (p_word *)malloc(sizeof(p_word) + len);
    if (!current)
        return P_RESULT_FAIL;
    current->next = p_word_hash[hash];
    p_word_strcpy(current->word, name);
    p_word_hash[hash] = current;

    /* Fetch all flags for the word and cache them in the hash */
    current->word_flags = in_wn(current->word, ALL_POS);
    if (current->word_flags & bit(kind))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

static int is_instantiated(const p_term *term)
{
    if (!term)
        return 0;
    else if ((p_term_type(term) & P_TERM_VARIABLE) != 0)
        return 0;
    else
        return 1;
}

/** @cond */
struct p_wn_code
{
    const char *name;
    int value;
};
/** @endcond */
static struct p_wn_code const parts_of_speech[] = {
    {"noun", NOUN},
    {"verb", VERB},
    {"adjective", ADJ},
    {"adverb", ADV},
    {0, -1}
};
#define FETCH_SYNSET    1024    /* Special value for words::search/5 */
static struct p_wn_code const queries[] = {
    {"antptr", ANTPTR},
    {"hyperptr", HYPERPTR},
    {"hypoptr", HYPOPTR},
    {"entailptr", ENTAILPTR},
    {"simptr", SIMPTR},
    {"ismemberptr", ISMEMBERPTR},
    {"isstuffptr", ISSTUFFPTR},
    {"ispartptr", ISPARTPTR},
    {"hasmemberptr", HASMEMBERPTR},
    {"hasstuffptr", HASSTUFFPTR},
    {"haspartptr", HASPARTPTR},
    {"meronym", MERONYM},
    {"holonym", HOLONYM},
    {"causeto", CAUSETO},
    {"pplptr", PPLPTR},
    {"seealsoptr", SEEALSOPTR},
    {"pertptr", PERTPTR},
    {"attribute", ATTRIBUTE},
    {"verbgroup", VERBGROUP},
    {"derivation", DERIVATION},
    {"classification", CLASSIFICATION},
    {"class", CLASS},
    {"syns", SYNS},
    {"freq", FREQ},
    {"frames", FRAMES},
    {"coords", COORDS},
    {"relatives", RELATIVES},
    {"hmeronym", HMERONYM},
    {"hholonym", HHOLONYM},
    {"wngrep", WNGREP},
    {"overview", OVERVIEW},
    {"classif_category", CLASSIF_CATEGORY},
    {"classif_usage", CLASSIF_USAGE},
    {"classif_regional", CLASSIF_REGIONAL},
    {"class_category", CLASS_CATEGORY},
    {"class_usage", CLASS_USAGE},
    {"class_regional", CLASS_REGIONAL},
    {"instance", INSTANCE},
    {"instances", INSTANCES},
    {"synset", FETCH_SYNSET},
    {0, -1}
};
static int lookup_code(const struct p_wn_code *codes, const p_term *code)
{
    const char *name;
    if (p_term_type(code) != P_TERM_ATOM)
        return -1;
    name = p_term_name(code);
    while (codes->name && strcmp(codes->name, name) != 0)
        ++codes;
    return codes->value;
}

/* Collect up search results from findtheinfo_ds() */
static void collect_db_search
    (p_context *context, p_term **head, p_term **tail,
     SynsetPtr synset, int recurse)
{
    int index;
    p_term *list;
    while (synset) {
        for (index = 0; index < synset->wcount; ++index) {
            list = *head;
            while (list) {
                if (!strcmp(p_term_name(p_term_head(list)),
                            synset->words[index]))
                    break;
                list = p_term_tail(list);
            }
            if (!list) {
                p_term *str = p_term_create_string
                    (context, synset->words[index]);
                if (*tail) {
                    list = p_term_create_list(context, str, 0);
                    p_term_set_tail(*tail, list);
                    *tail = list;
                } else {
                    *tail = p_term_create_list(context, str, 0);
                    *head = *tail;
                }
            }
        }
        if (recurse) {
            collect_db_search
                (context, head, tail, synset->ptrlist, recurse);
        }
        synset = synset->nextss;
    }
}

/* Search the database for a description or a matching word list */
#define DB_SEARCH           0
#define DB_DESCRIPTION      1
static p_goal_result words_db_search
    (p_context *context, p_term **args, p_term **error, int search_type)
{
    p_term *word = p_term_deref_member(context, args[0]);
    p_term *part_of_speech = p_term_deref_member(context, args[1]);
    p_term *query = p_term_deref_member(context, args[2]);
    p_term *sense = p_term_deref_member(context, args[3]);
    p_term *result = p_term_deref_member(context, args[4]);
    int type, wn_part_of_speech, wn_query, wn_sense, ch;
    char *norm_word, *temp;
    p_term *str;
    p_goal_result goal_result;

    /* Validate the parameters to the predicate */
    if (!is_instantiated(word) || !is_instantiated(part_of_speech) ||
            !is_instantiated(query) || !is_instantiated(sense)) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (is_instantiated(result)) {
        *error = p_create_type_error(context, "variable", result);
        return P_RESULT_ERROR;
    }
    type = p_term_type(word);
    if (type != P_TERM_ATOM && type != P_TERM_STRING) {
        *error = p_create_type_error(context, "atom_or_string", word);
        return P_RESULT_ERROR;
    }
    wn_part_of_speech = lookup_code(parts_of_speech, part_of_speech);
    if (wn_part_of_speech < 0) {
        *error = p_create_type_error(context, "part_of_speech", part_of_speech);
        return P_RESULT_ERROR;
    }
    wn_query = lookup_code(queries, query);
    if (wn_query < 0 || (wn_query == FETCH_SYNSET &&
                         search_type != DB_SEARCH)) {
        *error = p_create_type_error(context, "word_query", query);
        return P_RESULT_ERROR;
    }
    type = p_term_type(sense);
    if (type == P_TERM_INTEGER) {
        wn_sense = p_term_integer_value(sense);
        if (wn_sense < 1)
            wn_sense = -1;
    } else if (type == P_TERM_ATOM &&
               !strcmp(p_term_name(sense), "allsenses")) {
        wn_sense = ALLSENSES;
    } else {
        wn_sense = -1;
    }
    if (wn_sense < 0) {
        *error = p_create_type_error(context, "word_sense", sense);
        return P_RESULT_ERROR;
    }

    /* Normalize the word to lower case with "_" as word separator */
    norm_word = (char *)malloc(p_term_name_length(word) + 1);
    if (!norm_word)
        return P_RESULT_FAIL;
    strcpy(norm_word, p_term_name(word));
    temp = norm_word;
    while ((ch = *temp) != '\0') {
        if (ch >= 'A' && ch <= 'Z')
            *temp = (char)(ch + 'a' - 'A');
        else if (ch == ' ')
            *temp = '_';
        ++temp;
    }

    /* Perform the query against the database */
    goal_result = P_RESULT_FAIL;
    if (search_type == DB_DESCRIPTION) {
        /* Fetch the human-readable description of the results */
        temp = findtheinfo
            (norm_word, wn_part_of_speech, wn_query, wn_sense);
        if (temp && *temp != '\0') {
            str = p_term_create_string(context, temp);
            if (p_term_unify(context, result, str, P_BIND_DEFAULT))
                goal_result = P_RESULT_TRUE;
        }
    } else if (wn_query != FETCH_SYNSET) {
        /* Search for words in some relation with this word */
        p_term *head = 0;
        p_term *tail = 0;
        SynsetPtr synset = findtheinfo_ds
            (norm_word, wn_part_of_speech, wn_query, wn_sense);
        while (synset) {
            collect_db_search
                (context, &head, &tail, synset->ptrlist, 1);
            synset = synset->nextss;
        }
        if (head) {
            p_term_set_tail(tail, p_term_nil_atom(context));
            if (p_term_unify(context, result, head, P_BIND_DEFAULT))
                goal_result = P_RESULT_TRUE;
        }
    } else {
        /* Search for members of the synset itself.  Choose a
         * "synonym query" that will work for the part of speech */
        p_term *head = 0;
        p_term *tail = 0;
        if (wn_part_of_speech == ADJ)
            wn_query = SIMPTR;
        else if (wn_part_of_speech == ADV)
            wn_query = SYNS;
        else
            wn_query = HYPERPTR;
        SynsetPtr synset = findtheinfo_ds
            (norm_word, wn_part_of_speech, wn_query, wn_sense);
        while (synset) {
            collect_db_search(context, &head, &tail, synset, 0);
            synset = synset->nextss;
        }
        if (head) {
            p_term_set_tail(tail, p_term_nil_atom(context));
            if (p_term_unify(context, result, head, P_BIND_DEFAULT))
                goal_result = P_RESULT_TRUE;
        }
    }
    free(norm_word);
    return goal_result;
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_adjective_1
 * <b>words::adjective/1</b> - tests a word to determine if it is
 * an adjective.
 *
 * \par Usage
 * <b>words::adjective</b>(\em Word)
 *
 * \par Description
 * If \em Word is an atom or string whose name is registered in
 * the WordNet database as an adjective, then succeed.  Fail otherwise.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before testing.
 * \par
 * There are also arity-2 and arity-3 versions of this predicate in
 * the module that can be used in definite clause grammar rules to
 * recognize adjectives:
 * \code
 * noun_phrase --> det, words::adjective, words::noun.
 * noun_phrase(np(D,adj(A),n(N))) --> det(D), words::adjective(A), words::noun(N).
 * \endcode
 * In the second example above, \c A will be unified with the
 * adjective to assist with building a parse tree for the sentence.
 *
 * \par Examples
 * \code
 * words::adjective(animated)       succeeds
 * words::adjective("Animated")     succeeds
 * words::adjective(harpsichord)    fails
 * words::adjective(X)              fails
 * words::adjective(15)             fails
 * \endcode
 *
 * \par See Also
 * \ref words_adverb_1 "words::adverb/1",
 * \ref words_noun_1 "words::noun/1",
 * \ref words_verb_1 "words::verb/1"
 */
static p_goal_result words_adjective
    (p_context *context, p_term **args, p_term **error)
{
    return word_check(context, args[0], ADJ);
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_adverb_1
 * <b>words::adverb/1</b> - tests a word to determine if it is
 * an adverb.
 *
 * \par Usage
 * <b>words::adverb</b>(\em Word)
 *
 * \par Description
 * If \em Word is an atom or string whose name is registered in
 * the WordNet database as an adverb, then succeed.  Fail otherwise.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before testing.
 * \par
 * There are also arity-2 and arity-3 versions of this predicate in
 * the module that can be used in definite clause grammar rules to
 * recognize adverbs:
 * adverbs:
 * \code
 * verb_phrase --> words::adverb, words::verb, noun_phrase.
 * verb_phrase(vp(adv(A),v(V),NP)) --> words::adverb(A), words::verb(V), noun_phrase(NP).
 * \endcode
 * In the second example above, \c A will be unified with the
 * adverb to assist with building a parse tree for the sentence.
 *
 * \par Examples
 * \code
 * words::adverb(fitfully)          succeeds
 * words::adverb("Fitfully")        succeeds
 * words::adverb(harpsichord)       fails
 * words::adverb(X)                 fails
 * words::adverb(15)                fails
 * \endcode
 *
 * \par See Also
 * \ref words_adjective_1 "words::adjective/1",
 * \ref words_noun_1 "words::noun/1",
 * \ref words_verb_1 "words::verb/1"
 */
static p_goal_result words_adverb
    (p_context *context, p_term **args, p_term **error)
{
    return word_check(context, args[0], ADV);
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_description_5
 * <b>words::description/5</b> - fetches the description of a word.
 *
 * \par Usage
 * <b>words::description</b>(\em Word, \em PartOfSpeech, \em Query,
 * \em Sense, \em Result)
 *
 * \par Description
 * \em Word is an atom or string that is used to query the
 * WordNet database for a descriptive text about the word.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before searching.
 * \par
 * \em PartOfSpeech should be one of the atoms \c adjective, \c adverb,
 * \c noun, or \c verb, indicating the part of speech to search for.
 * \par
 * \em Query should be an atom representing a valid WordNet
 * \ref words_queries "query type".
 * \par
 * \em Sense should be an integer greater than or equal to 1 that
 * indicates which sense of the word should be queried for.
 * If \em Sense is the atom \c allsenses, then all senses will
 * be queried.
 * \par
 * \em Result is unified with the description, represented as a string.
 * If a description that matches \em Word, \em PartOfSpeech,
 * \em Query, and \em Sense cannot be found in the WordNet database,
 * then <b>words::description/5</b> fails.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Word, \em PartOfSpeech,
 *     \em Query, or \em Sense is a variable.
 * \li <tt>type_error(variable, \em Result)</tt> - \em Result is
 *     not a variable.
 * \li <tt>type_error(atom_or_string, \em Word)</tt> - \em Word is
 *     not an atom or string.
 * \li <tt>type_error(part_of_speech, \em PartOfSpeech)</tt> -
 *     \em PartOfSpeech is not one of the atoms \c adjective,
 *     \c adverb, \c noun, or \c verb.
 * \li <tt>type_error(word_query, \em Query)</tt> - \em Query is not
 *     an atom corresponding to one of the valid WordNet
 *     \ref words_queries "query types".
 * \li <tt>type_error(word_sense, \em Sense)</tt> - \em Sense is not
 *     an integer greater than or equal to 1 or the atom \c allsenses.
 *
 * \par Examples
 * \code
 * words::description("hand", noun, overview, allsenses, Description);
 * stdout::writeln(Description);
 *
 * The noun hand has 14 senses (first 8 from tagged texts)
 *
 * 1. (215) hand, manus, mitt, paw -- (the (prehensile) extremity of
 * the superior limb; "he had the hands of a surgeon"; "he extended
 * his mitt")
 * 2. (5) hired hand, hand, hired man -- (a hired laborer on a farm
 * or ranch; "the hired hand fixed the railing"; "a ranch hand")
 * ...
 * \endcode
 *
 * \par See Also
 * \ref words_overview_2 "words::overview/2",
 * \ref words_search_5 "words::search/5"
 */
static p_goal_result words_description
    (p_context *context, p_term **args, p_term **error)
{
    return words_db_search(context, args, error, DB_DESCRIPTION);
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_noun_1
 * <b>words::noun/1</b> - tests a word to determine if it is
 * a noun.
 *
 * \par Usage
 * <b>words::noun</b>(\em Word)
 *
 * \par Description
 * If \em Word is an atom or string whose name is registered in
 * the WordNet database as a noun, then succeed.  Fail otherwise.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before testing.
 * \par
 * There are also arity-2 and arity-3 versions of this predicate in
 * the module that can be used in definite clause grammar rules to
 * recognize nouns:
 * \code
 * noun_phrase --> det, words::adjective, words::noun.
 * noun_phrase(np(D,adj(A),n(N))) --> det(D), words::adjective(A), words::noun(N).
 * \endcode
 * In the second example above, \c N will be unified with the
 * noun to assist with building a parse tree for the sentence.
 *
 * \par Examples
 * \code
 * words::noun(harpsichord)         succeeds
 * words::noun("Harpsichord")       succeeds
 * words::noun("Bobby Fischer")     succeeds
 * words::noun(fitfully)            fails
 * words::noun(X)                   fails
 * words::noun(15)                  fails
 * \endcode
 *
 * \par See Also
 * \ref words_adjective_1 "words::adjective/1",
 * \ref words_adverb_1 "words::adverb/1",
 * \ref words_verb_1 "words::verb/1"
 */
static p_goal_result words_noun
    (p_context *context, p_term **args, p_term **error)
{
    return word_check(context, args[0], NOUN);
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_overview_2
 * <b>words::overview/2</b> - fetches an overview description of
 * the word.
 *
 * \par Usage
 * <b>words::overview</b>(\em Word, \em Result)
 *
 * \par Description
 * \em Word is an atom or string that is used to query the
 * WordNet database for a descriptive text about the word.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before searching.
 * \em Result is unified with the description, represented as a string.
 * \par
 * This predicate is a wrapper around
 * \ref words_description_5 "description/5" that produces a
 * human-readable dictionary-like entry in \em Result for all
 * parts of speech and senses that \em Word is a member of.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Word is a variable.
 * \li <tt>type_error(variable, \em Result)</tt> - \em Result is
 *     not a variable.
 *
 * \par Examples
 * \code
 * words::overview("hand", Description);
 * stdout::writeln(Description);
 *
 * The noun hand has 14 senses (first 8 from tagged texts)
 *
 * 1. (215) hand, manus, mitt, paw -- (the (prehensile) extremity of
 * the superior limb; "he had the hands of a surgeon"; "he extended
 * his mitt")
 * 2. (5) hired hand, hand, hired man -- (a hired laborer on a farm
 * or ranch; "the hired hand fixed the railing"; "a ranch hand")
 * ...
 * The verb hand has 2 senses (first 1 from tagged texts)
 *
 * 1. (25) pass, hand, reach, pass on, turn over, give -- (place into
 * the hands or custody of; "hand me the spoon, please"; "Turn the
 * files over to me, please"; "He turned over the prisoner to
 * his lawyers")
 * 2. hand -- (guide or conduct or usher somewhere; "hand the
 * elderly lady into the taxi")
 * \endcode
 *
 * \par See Also
 * \ref words_description_5 "words::description/5",
 * \ref words_search_5 "words::search/5"
 */

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_search_5
 * <b>words::search/5</b> - searches the database for other words
 * related to a search word.
 *
 * \par Usage
 * <b>words::search</b>(\em Word, \em PartOfSpeech, \em Query,
 * \em Sense, \em Result)
 *
 * \par Description
 * \em Word is an atom or string that is used to query the
 * WordNet database for a descriptive text about the word.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before searching.
 * \par
 * \em PartOfSpeech should be one of the atoms \c adjective, \c adverb,
 * \c noun, or \c verb, indicating the part of speech to search for.
 * \par
 * \em Query should be an atom representing a valid WordNet
 * \ref words_queries "query type".  \em Query can also be the
 * special atom \c synset which fetches the members of the
 * WordNet synonym set for \em Sense that contains \em Word.
 * \par
 * \em Sense should be an integer greater than or equal to 1 that
 * indicates which sense of the word should be queried for.
 * If \em Sense is the atom \c allsenses, then all senses will
 * be queried.
 * \par
 * \em Result is unified with a list of strings, corresponding
 * to the other words that are related to \em Word due to \em Query.
 * If there are no words that match the query, then
 * <b>words::search/5</b> fails.
 * \par
 * The \em Word itself may appear in the \em Result list, and each
 * member of \em Result will be unique (no duplicates).
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Word, \em PartOfSpeech,
 *     \em Query, or \em Sense is a variable.
 * \li <tt>type_error(variable, \em Result)</tt> - \em Result is
 *     not a variable.
 * \li <tt>type_error(atom_or_string, \em Word)</tt> - \em Word is
 *     not an atom or string.
 * \li <tt>type_error(part_of_speech, \em PartOfSpeech)</tt> -
 *     \em PartOfSpeech is not one of the atoms \c adjective,
 *     \c adverb, \c noun, or \c verb.
 * \li <tt>type_error(word_query, \em Query)</tt> - \em Query is not
 *     \c synset or an atom corresponding to one of the valid WordNet
 *     \ref words_queries "query types".
 * \li <tt>type_error(word_sense, \em Sense)</tt> - \em Sense is not
 *     an integer greater than or equal to 1 or the atom \c allsenses.
 *
 * \par Examples
 * \code
 * words::search("walk", verb, antptr, allsenses, List);
 * stdout::writeln(List);
 *
 * words::search("hand", noun, synset, 1, List2);
 * stdout::writeln(List2);
 * \endcode
 *
 * \par See Also
 * \ref words_description_5 "words::description/5",
 * \ref words_overview_2 "words::overview/2"
 */
static p_goal_result words_search
    (p_context *context, p_term **args, p_term **error)
{
    return words_db_search(context, args, error, DB_SEARCH);
}

/**
 * \addtogroup module_words
 * <hr>
 * \anchor words_verb_1
 * <b>words::verb/1</b> - tests a word to determine if it is
 * a verb.
 *
 * \par Usage
 * <b>words::verb</b>(\em Word)
 *
 * \par Description
 * If \em Word is an atom or string whose name is registered in
 * the WordNet database as a verb, then succeed.  Fail otherwise.
 * The \em Word will be converted to lower case, with spaces
 * replaced with underscores, before testing.
 * \par
 * There are also arity-2 and arity-3 versions of this predicate in
 * the module that can be used in definite clause grammar rules to
 * recognize verbs:
 * \code
 * verb_phrase --> words::adverb, words::verb, noun_phrase.
 * verb_phrase(vp(adv(A),v(V),NP)) --> words::adverb(A), words::verb(V), noun_phrase(NP).
 * \endcode
 * In the second example above, \c V will be unified with the
 * verb to assist with building a parse tree for the sentence.
 *
 * \par Examples
 * \code
 * words::verb(annoy)               succeeds
 * words::verb("Annoy")             succeeds
 * words::verb("hand_out")          succeeds
 * words::verb(harpsichord)         fails
 * words::verb(X)                   fails
 * words::verb(15)                  fails
 * \endcode
 *
 * \par See Also
 * \ref words_adjective_1 "words::adjective/1",
 * \ref words_adverb_1 "words::adverb/1",
 * \ref words_noun_1 "words::noun/1"
 */
static p_goal_result words_verb
    (p_context *context, p_term **args, p_term **error)
{
    return word_check(context, args[0], VERB);
}

void plang_module_setup(p_context *context)
{
    if (p_initialized) {
        re_wninit();
    } else {
        wninit();
        p_initialized = 1;
    }
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::adjective"), 1, words_adjective);
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::adverb"), 1, words_adverb);
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::description"), 5, words_description);
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::noun"), 1, words_noun);
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::search"), 5, words_search);
    p_db_set_builtin_predicate
        (p_term_create_atom(context, "words::verb"), 1, words_verb);
}

void plang_module_shutdown(p_context *context)
{
    int index;
    p_word *current;
    p_word *next;
    for (index = 0; index < P_WORD_HASH_SIZE; ++index) {
        current = p_word_hash[index];
        while (current) {
            next = current->next;
            free(current);
            current = next;
        }
        p_word_hash[index] = 0;
    }
}
