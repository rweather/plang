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
#include "term-priv.h"
#include "database-priv.h"

/**
 * \defgroup predicates Builtin predicates - Overview
 *
 * \par Logic and control
 * \ref fail_0 "fail/0",
 * \ref false_0 "false/0",
 * \ref true_0 "true/0"
 *
 * \par Term comparison
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2"
 *
 * \par Term unification
 * \ref unify_2 "(=)/2",
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unifiable_2 "unifiable/2",
 * \ref unify_2 "unify_with_occurs_check/2"
 *
 * \par Type testing
 * \ref atom_1 "atom/1",
 * \ref atomic_1 "atomic/1",
 * \ref class_object_1 "class_object/1",
 * \ref class_object_2 "class_object/2",
 * \ref compound_1 "compound/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref nonvar_1 "nonvar/1",
 * \ref number_1 "number/1",
 * \ref object_1 "object/1",
 * \ref object_2 "object/2",
 * \ref string_1 "string/1",
 * \ref var_1 "var/1"
 *
 * \par Compatibility
 * \anchor standard
 * Many of the builtin predicates share names and behavior with
 * <a href="http://pauillac.inria.fr/~deransar/prolog/docs.html">ISO
 * Standard Prolog</a> predicates.  Plang is not a strict subset or
 * superset of Standard Prolog and does not pretend to be.  It contains
 * additional term types for typed variables, member variables,
 * strings, and objects.  It also has a completely new clause syntax
 * based on C-style procedural programming languages.  However,
 * Plang does try to be compatible with the standard when there is
 * no good reason to diverge.  The documentation for the builtin
 * predicates indicate where they attempt compatibility with
 * Standard Prolog.
 */
/*\@{*/

/*\@}*/

/**
 * \defgroup logic_and_control Builtin predicates - Logic and control
 *
 * Predicates in this group are used to structure the flow of
 * execution through a Plang program.
 *
 * \ref fail_0 "fail/0",
 * \ref false_0 "false/0",
 * \ref true_0 "true/0"
 */
/*\@{*/

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor fail_0
 * \anchor false_0
 * <b>fail/0</b>, <b>false/0</b> - always fail.
 *
 * \par Usage
 * \b fail
 * \par
 * \b false
 *
 * \par Description
 * The \b fail predicate always fails execution of the current goal.
 *
 * \par Examples
 * \code
 * fail                 fails
 * false                fails
 * repeat; f(a); fail   executes f(a) an infinite number of times
 * \endcode
 *
 * \par Compatibility
 * The <b>fail/0</b> predicate is part of
 * \ref standard "Standard Prolog".  The <b>false/0</b> predicate
 * exists as an alias in Plang because it is a more natural
 * opposite to \ref true_0 "true/0".
 *
 * \par See Also
 * \ref true_0 "true/0"
 */
static p_goal_result p_builtin_fail
    (p_context *context, p_term **args, p_term **error)
{
    return P_RESULT_FAIL;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor true_0
 * <b>true/0</b> - always succeed.
 *
 * \par Usage
 * \b true
 *
 * \par Description
 * The \b true predicate always succeeds execution of the current goal.
 *
 * \par Examples
 * \code
 * true                 succeeds
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref fail_0 "fail/0"
 */
static p_goal_result p_builtin_true
    (p_context *context, p_term **args, p_term **error)
{
    return P_RESULT_TRUE;
}

/*\@}*/

/**
 * \defgroup term_comparison Builtin predicates - Term comparison
 * \anchor term_precedes
 *
 * Predicates in this group take two terms as arguments and succeed
 * without modification if the terms have a specified relationship.
 * Most of the predeciates use the "term-precedes" relationship
 * to order the two terms, which is defined as follows:
 *
 * \li Variables precede all floating-point numbers, which precede
 * all integers, which precede all strings, which precede all
 * atoms, which precede all compound terms, which precede all objects.
 * \li Variables and objects are ordered on pointer.
 * \li Integers and floating-point numbers are ordered according
 * to their numeric value.
 * \li Atoms and strings are ordered on their name using strcmp().
 * \li Compound terms (functors and lists) are ordered according to
 * their arity.  If the arity is the same, then the compound terms
 * are ordered on their functor names.  If the arity and functor
 * names are the same, then the compound terms are ordered on the
 * arguments from left to right.
 * \li List terms have arity 2 and "." as their functor name.
 *
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2"
 */
/*\@{*/

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_eq_2
 * <b>(==)/2</b> - tests if two terms are identical.
 *
 * \par Usage
 * \em Term1 \b == \em Term2
 *
 * \par Description
 * If \em Term1 and \em Term2 are identical, then
 * \em Term1 \b == \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * X == X               succeeds
 * X == Y               fails
 * f(X,Y) == f(X,Y)     succeeds
 * f(Y,X) == f(X,Y)     fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2"
 */
static p_goal_result p_builtin_term_eq
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_unify(context, args[0], args[1], P_BIND_EQUALITY))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_ne_2
 * <b>(!==)/2</b> - tests if two terms are not identical.
 *
 * \par Usage
 * \em Term1 \b !== \em Term2
 * \par
 * \em Term1 \b \\== \em Term2
 *
 * \par Description
 * If \em Term1 and \em Term2 are not identical, then
 * \em Term1 \b !== \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * X !== X              fails
 * X !== Y              succeeds
 * f(X,Y) !== f(X,Y)    fails
 * f(Y,X) !== f(X,Y)    succeeds
 * \endcode
 *
 * \par Compatibility
 * The <b>(\\==)/2</b> predicate is from \ref standard "Standard Prolog".
 * The new name <b>(!==)/2</b> is the recommended spelling.
 *
 * \par See Also
 * \ref term_eq_2 "(==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2"
 */
static p_goal_result p_builtin_term_ne
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_unify(context, args[0], args[1], P_BIND_EQUALITY))
        return P_RESULT_FAIL;
    else
        return P_RESULT_TRUE;
}

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_lt_2
 * <b>(\@<)/2</b> - tests if the first argument term-precedes
 * the second.
 *
 * \par Usage
 * \em Term1 <b>\@&lt;</b> \em Term2
 *
 * \par Description
 * If \em Term1 term-precedes \em Term2, then
 * \em Term1 <b>\@&lt;</b> \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * f(j) @< f(k)         succeeds
 * f(k) @< f(j)         fails
 * f(j) @< f(j)         fails
 * 2.0 @< 1             succeeds
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2",
 * \ref term_precedes "term-precedes"
 */
static p_goal_result p_builtin_term_lt
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_precedes(args[0], args[1]) < 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_le_2
 * <b>(\@<=)/2</b> - tests if the first argument is identical to
 * or term-precedes the second.
 *
 * \par Usage
 * \em Term1 <b>\@&lt;=</b> \em Term2
 * \par
 * \em Term1 <b>\@=&lt;</b> \em Term2
 *
 * \par Description
 * If \em Term1 is identical to or term-precedes \em Term2, then
 * \em Term1 <b>\@&lt;=</b> \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * f(j) @<= f(k)        succeeds
 * f(j) @<= f(j)        succeeds
 * f(k) @<= f(j)        fails
 * 2.0 @<= 1            succeeds
 * \endcode
 *
 * \par Compatibility
 * The <b>(\@=<)/2</b> predicate is from \ref standard "Standard Prolog".
 * The new name <b>(\@<=)/2</b> is the recommended spelling.
 *
 * \par See Also
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2",
 * \ref term_precedes "term-precedes"
 */
static p_goal_result p_builtin_term_le
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_precedes(args[0], args[1]) <= 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_gt_2
 * <b>(\@>)/2</b> - tests if the second argument term-precedes
 * the first.
 *
 * \par Usage
 * \em Term1 <b>\@&gt;</b> \em Term2
 *
 * \par Description
 * If \em Term2 term-precedes \em Term1, then
 * \em Term1 <b>\@&gt;</b> \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * f(j) @> f(k)         fails
 * f(k) @> f(j)         succeeds
 * f(j) @> f(j)         fails
 * 2.0 @> 1             fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_ge_2 "(\@>=)/2",
 * \ref term_precedes "term-precedes"
 */
static p_goal_result p_builtin_term_gt
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_precedes(args[0], args[1]) > 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup term_comparison
 * <hr>
 * \anchor term_ge_2
 * <b>(\@>=)/2</b> - tests if the second argument is identical to
 * or term-precedes the first.
 *
 * \par Usage
 * \em Term1 <b>\@&gt;=</b> \em Term2
 *
 * \par Description
 * If \em Term2 is identical to or term-precedes \em Term1, then
 * \em Term1 <b>\@&gt;=</b> \em Term2 succeeds with no modification to
 * \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * f(j) @>= f(k)        fails
 * f(j) @>= f(j)        succeeds
 * f(k) @>= f(j)        succeeds
 * 2.0 @>= 1            fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_precedes "term-precedes"
 */
static p_goal_result p_builtin_term_ge
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_precedes(args[0], args[1]) >= 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/*\@}*/

/**
 * \defgroup unification Builtin predicates - Term unification
 *
 * Predicates in this group take two terms as arguments and unifies
 * them under various conditions.
 *
 * \ref unify_2 "(=)/2",
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unifiable_2 "unifiable/2",
 * \ref unify_2 "unify_with_occurs_check/2"
 */
/*\@{*/

/**
 * \addtogroup unification
 * <hr>
 * \anchor unify_2
 * <b>(=)/2</b> - unifies two terms.
 *
 * \par Usage
 * \em Term1 \b = \em Term2
 * \par
 * \b unify_with_occurs_check(\em Term1, \em Term2)
 *
 * \par Description
 * If \em Term1 and \em Term2 can be unified, then perform
 * variable substitutions to unify them and succeed.  Fails otherwise.
 * \par
 * An occurs check is performed to ensure that circular terms will
 * not be created by the unification.  Therefore, in this
 * implementation the \ref standard "Standard Prolog" predicate
 * <b>unify_with_occurs_check/2</b> is identical to <b>(=)/2</b>.
 *
 * \par Examples
 * \code
 * f(X,b) = f(a,Y)      succeeds with X = a, Y = b
 * f(X,b) = g(X,b)      fails
 * X = f(X)             fails due to occurs check
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unifiable_2 "unifiable/2"
 */
static p_goal_result p_builtin_unify
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_unify(context, args[0], args[1], P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup unification
 * <hr>
 * \anchor not_unifiable_2
 * <b>(!=)/2</b> - tests if two terms are not unifiable.
 *
 * \par Usage
 * \em Term1 <b>!=</b> \em Term2
 * \par
 * \em Term1 <b>\\=</b> \em Term2
 *
 * \par Description
 * If \em Term1 and \em Term2 can not be unified, then succeed
 * without modifying \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * f(X,b) != f(a,Y)     fails
 * f(X,b) != g(X,b)     succeeds
 * X != f(X)            succeeds
 * \endcode
 *
 * \par Compatibility
 * The <b>(\\=)/2</b> predicate is from \ref standard "Standard Prolog".
 * The new name <b>(!=)/2</b> is the recommended spelling.
 *
 * \par See Also
 * \ref unify_2 "(=)/2",
 * \ref unifiable_2 "unifiable/2"
 */
static p_goal_result p_builtin_not_unifiable
    (p_context *context, p_term **args, p_term **error)
{
    void *marker = p_context_mark_trace(context);
    if (p_term_unify(context, args[0], args[1], P_BIND_DEFAULT)) {
        p_context_backtrack_trace(context, marker);
        return P_RESULT_FAIL;
    } else {
        return P_RESULT_TRUE;
    }
}

/**
 * \addtogroup unification
 * <hr>
 * \anchor unifiable_2
 * <b>unifiable/2</b> - tests if two terms are unifiable.
 *
 * \par Usage
 * \b unifiable(\em Term1, \em Term2)
 *
 * \par Description
 * If \em Term1 and \em Term2 can be unified, then succeed
 * without modifying \em Term1 or \em Term2.  Fails otherwise.
 *
 * \par Examples
 * \code
 * unifiable(f(X,b), f(a,Y))    succeeds without modifying X or Y
 * unifiable(f(X,b), g(X,b))    fails
 * unifiable(X, f(X))           fails due to occurs check
 * \endcode
 *
 * \par See Also
 * \ref unify_2 "(=)/2",
 * \ref not_unifiable_2 "(!=)/2"
 */
static p_goal_result p_builtin_unifiable
    (p_context *context, p_term **args, p_term **error)
{
    void *marker = p_context_mark_trace(context);
    if (p_term_unify(context, args[0], args[1], P_BIND_DEFAULT)) {
        p_context_backtrack_trace(context, marker);
        return P_RESULT_TRUE;
    } else {
        return P_RESULT_FAIL;
    }
}

/*\@}*/

/**
 * \defgroup type_testing Builtin predicates - Type testing
 *
 * Predicates in this group take a term as an argument and succeed
 * if the term has a certain type (atom, variable, integer,
 * object, etc).
 *
 * \ref atom_1 "atom/1",
 * \ref atomic_1 "atomic/1",
 * \ref class_object_1 "class_object/1",
 * \ref class_object_2 "class_object/2",
 * \ref compound_1 "compound/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref nonvar_1 "nonvar/1",
 * \ref number_1 "number/1",
 * \ref object_1 "object/1",
 * \ref object_2 "object/2",
 * \ref string_1 "string/1",
 * \ref var_1 "var/1"
 */
/*\@{*/

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor atom_1
 * \b atom/1 - tests if a term is an atom.
 *
 * \par Usage
 * \b atom(\em Term)
 *
 * \par Description
 * If \em Term is an atom, then \b atom(\em Term) succeeds.
 * Fails otherwise.
 *
 * \par Examples
 * \code
 * atom(fred)           succeeds
 * atom([])             succeeds
 * atom(f(X))           fails
 * atom(1.5)            fails
 * atom("mary")         fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atomic_1 "atomic/1",
 * \ref compound_1 "compound/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref number_1 "number/1",
 * \ref string_1 "string/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_atom
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) == P_TERM_ATOM)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor atomic_1
 * \b atomic/1 - tests if a term is atomic.
 *
 * \par Usage
 * \b atomic(\em Term)
 *
 * \par Description
 * If \em Term is an atom, integer, floating-point number, or string,
 * then \b atomic(\em Term) succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * atomic(fred)         succeeds
 * atomic([])           succeeds
 * atomic(f(X))         fails
 * atomic(1.5)          succeeds
 * atomic("mary")       succeeds
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog", with the addition that strings
 * are also considered atomic.
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref number_1 "number/1",
 * \ref string_1 "string/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_atomic
    (p_context *context, p_term **args, p_term **error)
{
    int type = p_term_type(args[0]);
    if (type == P_TERM_ATOM || type == P_TERM_INTEGER ||
            type == P_TERM_REAL || type == P_TERM_STRING)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor class_object_1
 * \b class_object/1 - tests if a term is a class object or name.
 *
 * \par Usage
 * \b class_object(\em Term)
 *
 * \par Description
 * If \em Term is a class object or an atom that names a class,
 * then \b class_object(\em Term) succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * class person { ... }
 * new person (P)
 *
 * class_object(person)         succeeds
 * class_object(people)         fails (assuming 'people' is not a class)
 * class_object(1.5)            fails
 * class_object(f(X))           fails
 * class_object(P)              fails
 * class_object(P.prototype)    succeeds (P's prototype is the person class)
 * class_object("person")       fails
 * \endcode
 *
 * \par See Also
 * \ref class_object_2 "class_object/2",
 * \ref object_1 "object/1"
 */
static p_goal_result p_builtin_class_object_1
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref(args[0]);
    if (p_term_is_class_object(context, term)) {
        return P_RESULT_TRUE;
    } else if (p_term_type(term) == P_TERM_ATOM) {
        p_database_info *db_info = term->atom.db_info;
        if (!db_info || !(db_info->class_info))
            return P_RESULT_FAIL;
        return P_RESULT_TRUE;
    } else {
        return P_RESULT_FAIL;
    }
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor class_object_2
 * \b class_object/2 - tests if a class name is associated with a
 * specific class object.
 *
 * \par Usage
 * \b class_object(\em Name, \em Class)
 *
 * \par Description
 * If \em Name is an atom that names a class, then \em Class is
 * unified with the class object corresponding to \em Name.
 * Otherwise, if \em Class is a class object, then \em Name is
 * unified with the name of \em Class.  Fails in all other cases.
 *
 * \par
 * This predicate is typically used to retrieve the class object
 * for a specific class name.  The name of a class object can
 * also be retrieved with <tt>Class.className</tt>.
 *
 * \par Examples
 * \code
 * class person { ... }
 * new person (P)
 *
 * class_object(person, C)          succeeds
 * class_object(people, C)          fails
 * class_object(1.5, C)             fails
 * class_object(P, C)               fails
 * class_object(P.className, C)     succeeds
 * class_object(Name, P.prototype)  succeeds
 * \endcode
 *
 * \par See Also
 * \ref class_object_1 "class_object/1",
 * \ref object_1 "object/1"
 */
static p_goal_result p_builtin_class_object_2
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref(args[0]);
    int type = p_term_type(name);
    if (type == P_TERM_ATOM) {
        p_database_info *db_info = name->atom.db_info;
        if (!db_info || !(db_info->class_info))
            return P_RESULT_FAIL;
        if (p_term_unify(context, args[1],
                         db_info->class_info->class_object,
                         P_BIND_DEFAULT))
            return P_RESULT_TRUE;
    } else if ((type & P_TERM_VARIABLE) &&
               p_term_is_class_object(context, args[1])) {
        p_term *class_name = p_term_property
            (context, args[1], p_term_class_name_atom(context));
        if (p_term_unify(context, name, class_name, P_BIND_DEFAULT))
            return P_RESULT_TRUE;
    }
    return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor compound_1
 * \b compound/1 - tests if a term is a compound functor or list.
 *
 * \par Usage
 * \b compound(\em Term)
 *
 * \par Description
 * If \em Term is a compound functor or list term, then
 * \b compound(\em Term) succeeds.  Fails otherwise.
 * Lists are considered functors with the name "./2".
 *
 * \par Examples
 * \code
 * compound(fred)       fails
 * compound([])         fails
 * compound(f(X))       succeeds
 * compound([a])        succeeds
 * compound(1.5)        fails
 * compound("mary")     fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1", \ref var_1 "var/1"
 */
static p_goal_result p_builtin_compound
    (p_context *context, p_term **args, p_term **error)
{
    int type = p_term_type(args[0]);
    if (type == P_TERM_FUNCTOR || type == P_TERM_LIST)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor float_1
 * \b float/1 - tests if a term is a floating-point number.
 *
 * \par Usage
 * \b float(\em Term)
 *
 * \par Description
 * If \em Term is a floating-point number, then \b float(\em Term)
 * succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * float(fred)          fails
 * float(f(X))          fails
 * float(1.5)           succeeds
 * float(2)             fails
 * float("mary")        fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref integer_1 "integer/1",
 * \ref number_1 "number/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_float
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) == P_TERM_REAL)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor integer_1
 * \b integer/1 - tests if a term is an integer number.
 *
 * \par Usage
 * \b integer(\em Term)
 *
 * \par Description
 * If \em Term is an integer number, then \b integer(\em Term)
 * succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * integer(fred)        fails
 * integer(f(X))        fails
 * integer(1.5)         fails
 * integer(2)           succeeds
 * integer("mary")      fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref float_1 "float/1",
 * \ref number_1 "number/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_integer
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) == P_TERM_INTEGER)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor nonvar_1
 * \b nonvar/1 - tests if a term is not an unbound variable.
 *
 * \par Usage
 * \b nonvar(\em Term)
 *
 * \par Description
 * If \em Term is an unbound variable, then \b nonvar(\em Term)
 * fails.  Otherwise \b nonvar(\em Term) succeeds with no
 * modification to \em Term.
 *
 * \par Examples
 * \code
 * nonvar(X)            fails if X is unbound
 * nonvar(fred)         succeeds
 * nonvar(f(X))         succeeds
 * nonvar(1.5)          succeeds
 * nonvar("mary")       succeeds
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref compound_1 "compound/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_nonvar
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) & P_TERM_VARIABLE)
        return P_RESULT_FAIL;
    else
        return P_RESULT_TRUE;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor number_1
 * \b number/1 - tests if a term is an integer or floating-point
 * number.
 *
 * \par Usage
 * \b number(\em Term)
 *
 * \par Description
 * If \em Term is an integer or floating-point number, then
 * \b number(\em Term) succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * number(fred)         fails
 * number(f(X))         fails
 * number(1.5)          succeeds
 * number(2)            succeeds
 * number("mary")       fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_number
    (p_context *context, p_term **args, p_term **error)
{
    int type = p_term_type(args[0]);
    if (type == P_TERM_INTEGER || type == P_TERM_REAL)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor object_1
 * \b object/1 - tests if a term is an instance object.
 *
 * \par Usage
 * \b object(\em Term)
 *
 * \par Description
 * If \em Term is an instance object, then \b object(\em Term)
 * succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * class person { ... }
 * new person (P)
 *
 * object(P)            succeeds
 * object(person)       fails
 * object(1.5)          fails
 * object(f(X))         fails
 * object(P.prototype)  fails (P's prototype is the person class)
 * object("person")     fails
 * \endcode
 *
 * \par See Also
 * \ref class_object_1 "class_object/1",
 * \ref object_2 "object/2"
 */
static p_goal_result p_builtin_object_1
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_is_instance_object(context, args[0]))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor object_2
 * \b object/2 - tests if a term is an object that is an
 * instance of a specific class.
 *
 * \par Usage
 * \b object(\em Term, \em Class)
 *
 * \par Description
 * If \em Term is an object that is an instance of \em Class or
 * one of its subclasses, then \b object(\em Term, \em Class)
 * succeeds.  Fails otherwise.  The \em Class may be a class
 * object or an atom that names a class.
 *
 * \par Examples
 * \code
 * class person { ... }
 * new person (P)
 *
 * object(P, person)            succeeds
 * object(P, people)            fails
 * object(P, P)                 fails
 * object(1.5, person)          fails
 * object(f(X), person)         fails
 * object(P, 1.5)               fails
 * object(P.prototype, person)  fails
 * object(P, "person")          fails
 * \endcode
 *
 * \par See Also
 * \ref class_object_1 "class_object/1",
 * \ref object_1 "object/1"
 */
static p_goal_result p_builtin_object_2
    (p_context *context, p_term **args, p_term **error)
{
    p_term *class_object = p_term_deref(args[1]);
    if (p_term_type(class_object) == P_TERM_ATOM) {
        p_database_info *db_info = class_object->atom.db_info;
        if (!db_info || !(db_info->class_info))
            return P_RESULT_FAIL;
        class_object = db_info->class_info->class_object;
    }
    if (p_term_is_instance_of(context, args[0], class_object))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor string_1
 * \b string/1 - tests if a term is a string.
 *
 * \par Usage
 * \b string(\em Term)
 *
 * \par Description
 * If \em Term is a string, \b string(\em Term) succeeds.
 * Fails otherwise.
 *
 * \par Examples
 * \code
 * string(fred)         fails
 * string(f(X))         fails
 * string(1.5)          fails
 * string(2)            fails
 * string("mary")       succeeds
 * \endcode
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref compound_1 "compound/1",
 * \ref var_1 "var/1"
 */
static p_goal_result p_builtin_string
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) == P_TERM_STRING)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor var_1
 * \b var/1 - tests if a term is an unbound variable.
 *
 * \par Usage
 * \b var(\em Term)
 *
 * \par Description
 * If \em Term is an unbound variable, then \b var(\em Term)
 * succeeds with no modification to \em Term.  Fails otherwise.
 *
 * \par Examples
 * \code
 * var(X)               succeeds if X is unbound
 * var(fred)            fails
 * var(f(X))            fails
 * var(1.5)             fails
 * var("mary")          fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref atom_1 "atom/1",
 * \ref compound_1 "compound/1",
 * \ref nonvar_1 "nonvar/1"
 */
static p_goal_result p_builtin_var
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) & P_TERM_VARIABLE)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/*\@}*/

void _p_db_init_builtins(p_context *context)
{
    struct p_builtin
    {
        const char *name;
        int arity;
        p_db_builtin func;
    };
    static struct p_builtin const builtins[] = {
        {"=", 2, p_builtin_unify},
        {"!=", 2, p_builtin_not_unifiable},
        {"\\=", 2, p_builtin_not_unifiable},
        {"==", 2, p_builtin_term_eq},
        {"!==", 2, p_builtin_term_ne},
        {"\\==", 2, p_builtin_term_ne},
        {"@<", 2, p_builtin_term_lt},
        {"@<=", 2, p_builtin_term_le},
        {"@=<", 2, p_builtin_term_le},
        {"@>", 2, p_builtin_term_gt},
        {"@>=", 2, p_builtin_term_ge},
        {"atom", 1, p_builtin_atom},
        {"atomic", 1, p_builtin_atomic},
        {"class_object", 1, p_builtin_class_object_1},
        {"class_object", 2, p_builtin_class_object_2},
        {"compound", 1, p_builtin_compound},
        {"fail", 0, p_builtin_fail},
        {"false", 0, p_builtin_fail},
        {"float", 1, p_builtin_float},
        {"integer", 1, p_builtin_integer},
        {"nonvar", 1, p_builtin_nonvar},
        {"number", 1, p_builtin_number},
        {"object", 1, p_builtin_object_1},
        {"object", 2, p_builtin_object_2},
        {"string", 1, p_builtin_string},
        {"true", 0, p_builtin_true},
        {"unifiable", 2, p_builtin_unifiable},
        {"unify_with_occurs_check", 2, p_builtin_unify},
        {"var", 1, p_builtin_var},
        {0, 0, 0}
    };
    int index;
    for (index = 0; builtins[index].name != 0; ++index) {
        p_db_set_builtin_predicate
            (p_term_create_atom(context, builtins[index].name),
             builtins[index].arity, builtins[index].func);
    }
}
