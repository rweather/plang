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
#include <stdio.h>
#include "term-priv.h"
#include "database-priv.h"
#include "context-priv.h"

/**
 * \defgroup predicates Builtin predicates - Overview
 *
 * \par Clause handling
 * \ref abolish_1 "abolish/1",
 * \ref asserta_1 "asserta/1",
 * \ref assertz_1 "assertz/1",
 * \ref retract_1 "retract/1"
 *
 * \par Directives
 * \ref directive_1 "(:-)/1",
 * \ref import_1 "import/1",
 * \ref initialization_1 "(?-)/1",
 * \ref initialization_1 "initialization/1"
 *
 * \par Logic and control
 * \ref logical_and_2 "(&amp;&amp;)/2",
 * \ref logical_or_2 "(||)/2",
 * \ref call_1 "call/1",
 * \ref catch_3 "catch/3",
 * \ref logical_and_2 "(,)/2",
 * \ref cut_0 "(!)/0",
 * \ref do_stmt "do",
 * \ref not_provable_1 "(!)/1",
 * \ref not_provable_1 "(\\+)/1",
 * \ref fail_0 "fail/0",
 * \ref false_0 "false/0",
 * \ref for_stmt "for",
 * \ref if_stmt "(->)/2",
 * \ref if_stmt "if",
 * \ref once_1 "once/1",
 * \ref repeat_0 "repeat/0",
 * \ref switch_stmt "switch",
 * \ref throw_1 "throw/1",
 * \ref true_0 "true/0",
 * \ref catch_3 "try",
 * \ref while_stmt "while"
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
 * additional term types for member variables, strings, and objects.
 * It also has a completely new clause syntax based on C-style
 * procedural programming languages.  However, Plang does try to
 * be compatible with the standard when there is no good reason
 * to diverge.  The documentation for the builtin predicates indicate
 * where they attempt compatibility with Standard Prolog.
 */
/*\@{*/

/*\@}*/

/* Create a "type_error" term */
static p_term *p_builtin_type_error
    (p_context *context, const char *name, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "type_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, term));
    return error;
}

/* Create a "domain_error" term */
static p_term *p_builtin_domain_error
    (p_context *context, const char *name, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "domain_error"), 2);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name));
    p_term_bind_functor_arg(error, 1, p_term_clone(context, term));
    return error;
}

/* Create a "permission_error" term */
static p_term *p_builtin_permission_error
    (p_context *context, const char *name1, const char *name2, p_term *term)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "permission_error"), 3);
    p_term_bind_functor_arg
        (error, 0, p_term_create_atom(context, name1));
    p_term_bind_functor_arg
        (error, 1, p_term_create_atom(context, name2));
    p_term_bind_functor_arg(error, 2, p_term_clone(context, term));
    return error;
}

/* Destructively sets a variable to a value */
P_INLINE void p_builtin_set_variable(p_term *var, p_term *value)
{
    if (!var || (var->header.type & P_TERM_VARIABLE) == 0)
        return;
    var->var.value = value;
}

/* Unbinds a list of local loop variables */
static void p_builtin_unbind_variables(p_term *list)
{
    list = p_term_deref(list);
    while (list && list->header.type == P_TERM_LIST) {
        p_builtin_set_variable(list->list.head, 0);
        list = p_term_deref(list->list.tail);
    }
}

/**
 * \defgroup clause_handling Builtin predicates - Clause handling
 *
 * Predicates in this group are used to add and remove clauses
 * from the predicate database.
 *
 * \ref abolish_1 "abolish/1",
 * \ref asserta_1 "asserta/1",
 * \ref assertz_1 "assertz/1",
 * \ref retract_1 "retract/1"
 */
/*\@{*/

static p_term *p_builtin_parse_indicator
    (p_context *context, p_term *pred, int *arity, p_term **error)
{
    p_term *name_term;
    p_term *arity_term;
    pred = p_term_deref(pred);
    if (!pred || (pred->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return 0;
    } else if (pred->header.type != P_TERM_FUNCTOR ||
               pred->header.size != 2 ||
               pred->functor.functor_name != context->slash_atom) {
        *error = p_builtin_type_error
            (context, "predicate_indicator", pred);
        return 0;
    }
    name_term = p_term_deref(pred->functor.arg[0]);
    arity_term = p_term_deref(pred->functor.arg[1]);
    if (!name_term || (name_term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return 0;
    }
    if (!arity_term || (arity_term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return 0;
    }
    if (arity_term->header.type != P_TERM_INTEGER) {
        *error = p_builtin_type_error(context, "integer", arity_term);
        return 0;
    }
    if (name_term->header.type != P_TERM_ATOM) {
        *error = p_builtin_type_error(context, "atom", name_term);
        return 0;
    }
    *arity = p_term_integer_value(arity_term);
    if (*arity < 0) {
        *error = p_builtin_domain_error
            (context, "not_less_than_zero", arity_term);
        return 0;
    }
    return name_term;
}

/**
 * \addtogroup clause_handling
 * <hr>
 * \anchor abolish_1
 * <b>abolish/1</b> - removes a user-defined predicate from
 * the predicate database.
 *
 * \par Usage
 * \b abolish(\em Pred)
 *
 * \par Description
 * Removes all clauses from the predicate database that are
 * associated with the predicate indicator \em Pred and succeeds.
 * The indicator should have the form \em Name / \em Arity.
 * If the predicate does not exist, then succeeds.
 * \par
 * Removing a predicate that is in the process of being executed
 * leads to undefined behavior.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - one of \em Pred, \em Name,
 *     or \em Arity, is a variable.
 * \li <tt>type_error(predicate_indicator, \em Pred)</tt> - \em Pred
 *     does not have the form \em Name / \em Arity.
 * \li <tt>type_error(integer, \em Arity)</tt> - \em Arity is not
 *     an integer.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Name is not an atom.
 * \li <tt>domain_error(not_less_than_zero, \em Arity)</tt> - \em Arity
 *     is less than zero.
 * \li <tt>permission_error(modify, static_procedure, \em Pred)</tt> -
 *     \em Pred is a builtin or read-only predicate.
 *
 * \par Examples
 * \code
 * abolish(userdef/3)       succeeds
 * abolish(Pred)            instantiation_error
 * abolish(Name/3)          instantiation_error
 * abolish(userdef/Arity)   instantiation_error
 * abolish(1.5)             type_error(predicate_indicator, 1.5)
 * abolish(userdef/a)       type_error(integer, a)
 * abolish(1/a)             type_error(integer, a)
 * abolish(1/3)             type_error(atom, 1)
 * abolish(userdef/-3)      domain_error(not_less_than_zero, -3)
 * abolish(abolish/1)       permission_error(modify, static_procedure, abolish/1)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref asserta_1 "asserta/1",
 * \ref retract_1 "retract/1"
 */
static p_goal_result p_builtin_abolish
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name;
    int arity;
    name = p_builtin_parse_indicator(context, args[0], &arity, error);
    if (!name)
        return P_RESULT_ERROR;
    if (!p_db_clause_abolish(context, name, arity)) {
        *error = p_builtin_permission_error
            (context, "modify", "static_procedure", args[0]);
        return P_RESULT_ERROR;
    }
    return P_RESULT_TRUE;
}

/**
 * \addtogroup clause_handling
 * <hr>
 * \anchor asserta_1
 * \anchor assertz_1
 * <b>asserta/1</b>, <b>assertz/1</b> - adds a clause to the
 * start/end of a user-defined predicate in the predicate database.
 *
 * \par Usage
 * \b asserta(\em Clause)
 * \par
 * \b assertz(\em Clause)
 *
 * \par Description
 * If the \em Clause has <b>(:-)/2</b> as its top-level functor,
 * then break it down into \em Head :- \em Body.  Otherwise, let
 * \em Head be \em Clause and \em Body be \b true.
 * \par
 * A freshly renamed version of the clause \em Head :- \em Body is
 * added to the predicate database at the start (<b>asserta/1</b>)
 * or end (<b>assertz/1</b>) of the predicate defined by \em Head.
 * \par
 * Adding a clause to a predicate that is in the process of being
 * executed leads to undefined behavior.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Clause or \em Head
 *     is a variable.
 * \li <tt>type_error(callable, \em Head)</tt> - \em Head is not a
 *     callable term (atom or functor).
 * \li <tt>type_error(callable, \em Body)</tt> - \em Body is not a
 *     callable term.  In a departure from \ref standard "Standard Prolog",
 *     this error is thrown when the \em Body is executed, not when
 *     the \em Clause is asserted into the database.
 * \li <tt>permission_error(modify, static_procedure, \em Pred)</tt> -
 *     the predicate indicator \em Pred of \em Head refers to a
 *     builtin or read-only predicate.
 *
 * \par Examples
 * \code
 * asserta(Clause)              instantiation_error
 * assertz((Head :- true))      instantiation_error
 * asserta((1.5 :- true))       type_error(callable, 1.5)
 * assertz((a :- true))         succeeds
 * asserta((a(X) :- b(X,Y)))    succeeds
 * assertz(a(X))                succeeds
 * asserta((a :- X))            type_error(callable, X) when executed
 * assertz(asserta(X))          permission_error(modify, static_procedure, asserta/1)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref abolish_1 "abolish/1",
 * \ref retract_1 "retract/1"
 */
static p_goal_result p_builtin_assert
    (p_context *context, p_term **args, p_term **error, int at_start)
{
    p_term *clause = p_term_deref(args[0]);
    p_term *head;
    p_term *pred;
    if (!clause || (clause->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return P_RESULT_ERROR;
    }
    if (clause->header.type == P_TERM_FUNCTOR &&
            clause->header.size == 2 &&
            clause->functor.functor_name == context->clause_atom) {
        head = p_term_deref(clause->functor.arg[0]);
    } else {
        head = clause;
        clause = p_term_create_functor
            (context, context->clause_atom, 2);
        p_term_bind_functor_arg(clause, 0, head);
        p_term_bind_functor_arg
            (clause, 1, p_term_create_atom(context, "true"));
    }
    if (!head || (head->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return P_RESULT_ERROR;
    }
    if (head->header.type != P_TERM_ATOM &&
            head->header.type != P_TERM_FUNCTOR) {
        *error = p_builtin_type_error(context, "callable", head);
        return P_RESULT_ERROR;
    }
    clause = p_term_clone(context, clause);
    if (at_start) {
        if (p_db_clause_assert_first(context, clause))
            return P_RESULT_TRUE;
    } else {
        if (p_db_clause_assert_last(context, clause))
            return P_RESULT_TRUE;
    }
    pred = p_term_create_functor(context, context->slash_atom, 2);
    if (head->header.type == P_TERM_ATOM) {
        p_term_bind_functor_arg(pred, 0, head);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer(context, 0));
    } else {
        p_term_bind_functor_arg(pred, 0, head->functor.functor_name);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer
                            (context, (int)(head->header.size)));
    }
    *error = p_builtin_permission_error
        (context, "modify", "static_procedure", pred);
    return P_RESULT_ERROR;
}
static p_goal_result p_builtin_asserta
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_assert(context, args, error, 1);
}
static p_goal_result p_builtin_assertz
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_assert(context, args, error, 0);
}

/**
 * \addtogroup clause_handling
 * <hr>
 * \anchor retract_1
 * <b>retract/1</b> - removes a clause from a user-defined
 * predicate in the predicate database.
 *
 * \par Usage
 * \b retract(\em Clause)
 *
 * \par Description
 * If the \em Clause has <b>(:-)/2</b> as its top-level functor,
 * then break it down into \em Head :- \em Body.  Otherwise, let
 * \em Head be \em Clause and \em Body be \b true.
 * \par
 * The <b>retract/1</b> predicate finds the first clause in
 * the predicate database that unifies with \em Head :- \em Body,
 * removes it, and then succeeds.  If \em Head :- \em Body does
 * not unify with any clause in the database, then
 * <b>retract/1</b> fails.
 * \par
 * Upon success, \em Clause is unified with the clause that was
 * removed.
 * \par
 * Removing a clause from a predicate that is in the process of
 * being executed leads to undefined behavior.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Clause or \em Head
 *     is a variable.
 * \li <tt>type_error(callable, \em Head)</tt> - \em Head is not a
 *     callable term (atom or functor).
 * \li <tt>permission_error(modify, static_procedure, \em Pred)</tt> -
 *     the predicate indicator \em Pred of \em Head refers to a
 *     builtin or read-only predicate.
 *
 * \par Examples
 * \code
 * retract(Clause)              instantiation_error
 * retract((Head :- true))      instantiation_error
 * retract((1.5 :- true))       type_error(callable, 1.5)
 * retract((a(X) :- b(X, Y))    fails
 * assertz((a(X) :- b(X, Y))); retract((a(X) :- b(X, Y)))   succeeds
 * retract(retract(X))          permission_error(modify, static_procedure, retract/1)
 * \endcode
 *
 * \par Compatibility
 * The <b>retract/1</b> predicate in \ref standard "Standard Prolog"
 * removes multiple clauses that match \em Head :- \em Body upon
 * back-tracking.  In Plang, only the first clause is removed and
 * back-tracking will cause a failure.  It is possible to remove
 * all clauses that match a specific \em Head :- \em Body as follows:
 * \code
 * while [X, Y] (retract((a(X) :- b(X, Y)))) {}
 * \endcode
 *
 * \par See Also
 * \ref abolish_1 "abolish/1",
 * \ref asserta_1 "asserta/1"
 */
static p_goal_result p_builtin_retract
    (p_context *context, p_term **args, p_term **error)
{
    p_term *clause = p_term_deref(args[0]);
    p_term *head;
    p_term *pred;
    int result;
    if (!clause || (clause->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return P_RESULT_ERROR;
    }
    if (clause->header.type == P_TERM_FUNCTOR &&
            clause->header.size == 2 &&
            clause->functor.functor_name == context->clause_atom) {
        head = p_term_deref(clause->functor.arg[0]);
    } else {
        head = clause;
        clause = p_term_create_functor
            (context, context->clause_atom, 2);
        p_term_bind_functor_arg(clause, 0, head);
        p_term_bind_functor_arg
            (clause, 1, p_term_create_atom(context, "true"));
    }
    if (!head || (head->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return P_RESULT_ERROR;
    }
    if (head->header.type != P_TERM_ATOM &&
            head->header.type != P_TERM_FUNCTOR) {
        *error = p_builtin_type_error(context, "callable", head);
        return P_RESULT_ERROR;
    }
    result = p_db_clause_retract(context, clause);
    if (result > 0)
        return P_RESULT_TRUE;
    else if (result < 0)
        return P_RESULT_FAIL;
    pred = p_term_create_functor(context, context->slash_atom, 2);
    if (head->header.type == P_TERM_ATOM) {
        p_term_bind_functor_arg(pred, 0, head);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer(context, 0));
    } else {
        p_term_bind_functor_arg(pred, 0, head->functor.functor_name);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer
                            (context, (int)(head->header.size)));
    }
    *error = p_builtin_permission_error
        (context, "modify", "static_procedure", pred);
    return P_RESULT_ERROR;
}

/*\@}*/

/**
 * \defgroup directives Builtin predicates - Directives
 *
 * Directives are executed while a source file is being loaded to
 * modify environmental parameters, adjust language flags, etc.
 * Two kinds of directives are provided: immediate and deferred.
 * Immediate directives are immediately executed when the source
 * file is parsed, but before predicates in the source file are
 * defined into the database.  The following are examples:
 *
 * \code
 * :- import(stdout).
 * :- dynamic(person/1).
 * \endcode
 *
 * Deferred directives are executed after the source file has
 * been parsed and the predicates have been defined into the
 * database.  The following is an example:
 *
 * \code
 * class fridge {
 *     ...
 *     open()
 *     {
 *         ...
 *     }
 * }
 *
 * ?- {
 *     new fridge(F);
 *     set_global_object(my_fridge, F);
 *     my_fridge.open();
 * }
 * \endcode
 *
 * After the directive is executed, the Plang engine will execute a
 * cut, \ref cut_0 "(!)/0", and \ref fail_0 "fail/0" to backtrack
 * to the original system state.  The only permanent modifications
 * to the system state will be in the form of side-effects
 * (e.g. the call to <b>set_global_object/2</b> above).
 *
 * Directives may also be called as regular builtin predicates
 * during normal program execution.  The exception is
 * \ref import_1 "import/1", which must be used within an
 * immediate directive.
 *
 * \ref directive_1 "(:-)/1",
 * \ref import_1 "import/1",
 * \ref initialization_1 "(?-)/1",
 * \ref initialization_1 "initialization/1"
 */
/*\@{*/

/**
 * \addtogroup directives
 * <hr>
 * \anchor directive_1
 * <b>(:-)/1</b> - execute a directive immediately while a
 * source file is being loaded.
 *
 * \par Usage
 * <b>:-</b> \em Directive.
 *
 * \par Description
 * Executes \em Directive immediately when it is encountered
 * in the source file during loading.  After execution of the
 * \em Directive, an implicit cut, \ref cut_0 "(!)/0", and
 * \ref fail_0 "fail/0" are performed to return the system
 * to its original state before the call.  The only permanent
 * modifications to the system state will be in the form
 * of side-effects in \em Directive.
 * \par
 * The \em Directive is limited to an atom or a functor call
 * by the Plang parser.  More complex terms are not permitted.
 * \par
 * If <b>(:-)/1</b> is called during normal program execution
 * instead of within a directive, it will have the same
 * effect as \ref call_1 "call/1".
 *
 * \par Examples
 * \code
 * :- import(stdout).
 * :- dynamic(person/1).
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref initialization_1 "(?-)/1",
 * \ref call_1 "call/1",
 * \ref import_1 "import/1"
 */

/**
 * \addtogroup directives
 * <hr>
 * \anchor import_1
 * <b>import/1</b> - imports another source file's definitions.
 *
 * \par Usage
 * <b>:-</b> \b import(\em Name).
 *
 * \par Description
 * The \em Name must be an atom or string, whose name refers to a
 * Plang source file along the import search path.  If the referred
 * to source file has not been loaded yet, it will be parsed and
 * loaded into the current execution context.  If the referred
 * to source file has already been loaded, then <b>import/1</b>
 * does nothing and succeeds.
 * \par
 * If \em Name does not have a file extension, then <tt>.lp</tt>
 * is added to \em Name.  Plang then searches in the same directory
 * as the including source file for \em Name.  If not found,
 * Plang will search the system-specific import search path
 * looking for \em Name.
 * \par
 * If \em Name includes system-specific directory separator
 * characters (e.g. /), then the specified file will be loaded
 * directly without searching the import search path.
 *
 * \par Errors
 *
 * \li <tt>system_error</tt> - <b>import/1</b> was not used
 *     within a \ref directive_1 "(:-)/1" directive.
 *
 * \par Examples
 * \code
 * :- import(stdout).
 * :- import("stdout.lp").
 * :- import(1.5).              error message: not an atom or string
 * :- import("not_found.lp").   error message: non-existent file
 * :- import("../dir/file.lp").
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog" has directives called
 * <b>ensure_loaded/1</b> and <b>include/1</b> that perform a
 * similar function to <b>import/1</b>.  Those Standard Prolog
 * directives are not supported by Plang.
 *
 * \par See Also
 * \ref directive_1 "(:-)/1"
 */
static p_goal_result p_builtin_import
    (p_context *context, p_term **args, p_term **error)
{
    /* Importing is only allowed when parsing a source file
     * because it isn't possible to know the filename of the
     * parent when executed from within a normal predicate */
    *error = p_term_create_atom(context, "system_error");
    return P_RESULT_ERROR;
}

/**
 * \addtogroup directives
 * <hr>
 * \anchor initialization_1
 * <b>(?-)/1</b>, <b>initialization/1</b> - execute a goal after a
 * source file has been loaded.
 *
 * \par Usage
 * <b>?-</b> \em Goal.
 * \par
 * <b>?-</b> { \em Goal }
 * \par
 * <b>:-</b> \b initialization(\em Goal).
 *
 * \par Description
 * Executes \em Goal after the current source file has been
 * completely loaded.  After execution of the \em Goal, an implicit
 * cut, \ref cut_0 "(!)/0", and \ref fail_0 "fail/0" are performed
 * to return the system to its original state before the call.
 * The only permanent modifications to the system state will be
 * in the form of side-effects in \em Goal.
 * \par
 * If <b>(?-)/1</b> or <b>initialization/1</b> is called during
 * normal program execution instead of within a directive,
 * it will have the same effect as \ref call_1 "call/1".
 *
 * \par Examples
 * \code
 * ?- stdout::writeln("Hello World!").
 * \endcode
 *
 * \par Compatibility
 * The <b>initialization/1</b> directive is compatible with
 * \ref standard "Standard Prolog".  The <b>(?-)/1</b> form
 * is the recommended syntax.
 *
 * \par See Also
 * \ref directive_1 "(:-)/1",
 * \ref call_1 "call/1"
 */

/*\@}*/

/**
 * \defgroup logic_and_control Builtin predicates - Logic and control
 *
 * Predicates in this group are used to structure the flow of
 * execution through a Plang program.
 *
 * \ref logical_and_2 "(&amp;&amp;)/2",
 * \ref logical_or_2 "(||)/2",
 * \ref call_1 "call/1",
 * \ref catch_3 "catch/3",
 * \ref logical_and_2 "(,)/2",
 * \ref cut_0 "(!)/0",
 * \ref do_stmt "do",
 * \ref not_provable_1 "(!)/1",
 * \ref not_provable_1 "(\\+)/1",
 * \ref fail_0 "fail/0",
 * \ref false_0 "false/0",
 * \ref for_stmt "for",
 * \ref if_stmt "(->)/2",
 * \ref if_stmt "if",
 * \ref once_1 "once/1",
 * \ref repeat_0 "repeat/0",
 * \ref switch_stmt "switch",
 * \ref throw_1 "throw/1",
 * \ref true_0 "true/0",
 * \ref catch_3 "try",
 * \ref while_stmt "while"
 */
/*\@{*/

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor logical_and_2
 * <b>(&amp;&amp;)/2</b>, <b>(,)/2</b> - sequential execution of two goals.
 *
 * \par Usage
 * \em Goal1 <b>&amp;&amp;</b> \em Goal2
 * \par
 * \em (Goal1, \em Goal2)
 *
 * \par Description
 * Executes \em Goal1 and then executes \em Goal2 if \em Goal1
 * was successful.
 * \par
 * The <b>(&amp;&amp;)/2</b> form is recommended for use within
 * boolean conditions for <b>if</b>, <b>while</b>, and similar
 * statements.  The <b>(,)/2</b> form usually occurs as a result
 * of parsing the statement sequence { \em Goal1 ; \em Goal2 }.
 *
 * \par Examples
 * \code
 * nonvar(X); integer(X)
 * (nonvar(X), integer(X))
 * if (nonvar(X) && integer(X)) { ... }
 * \endcode
 *
 * \par Compatibility
 * The <b>(,)/2</b> predicate is compatible with
 * \ref standard "Standard Prolog".  The <b>(&amp;&amp;)/2</b>
 * predicate is an alias for <b>(,)/2</b>.
 *
 * \par See Also
 * \ref logical_or_2 "(||)/2",
 * \ref not_provable_1 "(!)/1"
 */

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor logical_or_2
 * <b>(||)/2</b> - alternative execution of two goals.
 *
 * \par Usage
 * \em Goal1 <b>||</b> \em Goal2
 *
 * \par Description
 * Executes \em Goal1 and succeeds if \em Goal1 succeeds.
 * Otherwise executes \em Goal2.
 *
 * \par Examples
 * \code
 * nonvar(X) || integer(X)
 * if (atom(X) || integer(X)) { ... }
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog" has a <b>(;)/2</b> predicate
 * that is used for disjunction.  That predicate has been omitted
 * from Plang because it conflicts with ";" used as a conjunction
 * operator in statement lists.
 *
 * \par See Also
 * \ref logical_and_2 "(&amp;&amp;)/2",
 * \ref not_provable_1 "(!)/1",
 * \ref if_stmt "if"
 */
static p_goal_result p_builtin_logical_or
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref(args[0]);
    p_goal_result result;
    if (term->header.type == P_TERM_FUNCTOR &&
            term->header.size == 2 &&
            term->functor.functor_name == context->if_atom) {
        /* The term has the form (A -> B || C) */
        result = p_goal_call(context, p_term_arg(term, 0), error);
        if (result == P_RESULT_TRUE || result == P_RESULT_CUT_TRUE)
            return p_goal_call(context, p_term_arg(term, 1), error);
        else if (result == P_RESULT_FAIL)
            return p_goal_call(context, args[1], error);
        else if (result == P_RESULT_CUT_FAIL)
            result = P_RESULT_FAIL;
        return result;
    } else {
        /* Regular disjunction */
        result = p_goal_call(context, term, error);
        if (result == P_RESULT_FAIL)
            result = p_goal_call(context, args[1], error);
        else if (result == P_RESULT_CUT_FAIL)
            result = P_RESULT_FAIL;
        else if (result == P_RESULT_CUT_TRUE)
            result = P_RESULT_TRUE;
        return result;
    }
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor call_1
 * <b>call/1</b> - meta-execution of a goal.
 *
 * \par Usage
 * \b call(\em Goal)
 *
 * \par Description
 * If \em Goal is a callable term, then execute it as though it
 * had been compiled normally.  If \em Goal is not a callable term,
 * then throw an error as described below.
 *
 * \par
 * The effect of a \ref cut_0 "(!)/0" inside \em Goal is limited
 * to the goal itself and does not affect control flow outside
 * the <b>call/1</b> term.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - if \em Goal is a variable.
 * \li <tt>type_error(callable, </tt>\em Goal<tt>)</tt> - if \em Goal
 *     is not a variable and not callable.
 *
 * \par Examples
 * \code
 * call(fail)           fails
 * X = atom(a); call(X) calls atom(a) and then succeeds
 * call(X)              instantiation_error
 * call(1.5)            type_error(callable, 1.5)
 * call((atom(a), 1.5)) atom(a) succeeds and then type_error(callable, 1.5)
 * \endcode
 *
 * \par Compatibility
 * The <b>call/1</b> predicate is mostly compatible with
 * \ref standard "Standard Prolog".  The main departure is that
 * Standard Prolog will scan the entire structure of \em Goal
 * and throw a <tt>type_error</tt> if some part of it is not callable.
 * The last example above would throw
 * <tt>type_error(callable, (atom(a), 1.5))</tt> and not execute
 * <tt>atom(a)</tt> in Standard Prolog.  Plang implements lazy
 * evaluation of <b>call/1</b> subgoals, so only the outermost
 * layer of \em Goal is checked before execution begins.
 *
 * \par See Also
 * \ref cut_0 "(!)/0",
 * \ref once_1 "once/1"
 */
static p_goal_result p_builtin_call
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result = p_goal_call(context, args[0], error);
    if (result == P_RESULT_CUT_FAIL)
        result = P_RESULT_FAIL;
    else if (result == P_RESULT_CUT_TRUE)
        result = P_RESULT_TRUE;
    return result;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor catch_3
 * <b>catch/3</b>, <b>try</b> - catches an error that was thrown during
 * execution of a goal.
 *
 * \par Usage
 * \b catch(\em Goal, \em Pattern, \em Recovery)
 * \par
 * \b try { \em Goal } \b catch (\em Pattern1) { \em Recovery1 } \b catch (\em Pattern2) { \em Recovery2 } ...
 *
 * \par Description
 * Executes <b>call</b>(\em Goal) and succeeds or fails accordingly.
 * If \em Goal throws an error with \ref throw_1 "throw/1", and the
 * error can be unified with \em Pattern, then <b>call</b>(\em Recovery)
 * will be executed.  If the error does not unify with \em Pattern,
 * then the error will continue to be thrown further up the call chain.
 * \par
 * In the case of the <b>try</b> statement, each \em PatternN is
 * tried in turn until a match is found.  The \em RecoveryN goal
 * for that pattern is then executed.  Note that this is not the
 * same as \b catch(catch(\em Goal, \em Pattern1, \em Recovery1),
 * \em Pattern2, \em Recovery2).  The <b>catch/3</b> form may
 * execute \em Recovery2 if an error is thrown during \em Recovery1.
 * The <b>try</b> statement form will not.
 *
 * \par Compatibility
 * The <b>catch/3</b> predicate is compatible with
 * \ref standard "Standard Prolog".  The <b>try ... catch ...</b>
 * statement is the recommended equivalent in Plang because of its
 * better support for multiple catch blocks.
 *
 * \par See Also
 * \ref throw_1 "throw/1",
 * \ref call_1 "call/1"
 */
static p_goal_result p_builtin_catch
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result = p_goal_call(context, args[0], error);
    if (result != P_RESULT_ERROR)
        return result;
    if (!p_term_unify(context, args[1], *error, P_BIND_DEFAULT))
        return result;
    *error = 0;
    return p_goal_call(context, args[2], error);
}
static p_goal_result p_builtin_try
    (p_context *context, p_term **args, p_term **error)
{
    p_term *list, *head, *catch;
    p_goal_result result = p_goal_call(context, args[0], error);
    if (result != P_RESULT_ERROR)
        return result;
    catch = p_term_create_atom(context, "$$catch");
    list = p_term_deref(args[1]);
    while (list && list->header.type == P_TERM_LIST) {
        head = p_term_deref(p_term_head(list));
        if (head->header.type == P_TERM_FUNCTOR &&
                head->header.size == 2 &&
                head->functor.functor_name == catch) {
            if (p_term_unify(context, p_term_arg(head, 0), *error,
                             P_BIND_DEFAULT)) {
                *error = 0;
                return p_goal_call(context, p_term_arg(head, 1), error);
            }
        }
        list = p_term_deref(p_term_tail(list));
    }
    return result;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor cut_0
 * <b>(!)/0</b> - prunes alternative solutions by cutting unexplored
 * branches in the solution search tree.
 *
 * \par Usage
 * \b !
 *
 * \par Description
 * The \b ! predicate always succeeds but also prunes alternative
 * solutions at the next higher goal level.
 *
 * \par Examples
 * \code
 * !                succeeds
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref call_1 "call/1"
 */
static p_goal_result p_builtin_cut
    (p_context *context, p_term **args, p_term **error)
{
    return P_RESULT_CUT_TRUE;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor do_stmt
 * <b>do</b> - repeatedly execute a statement until a
 * condition is false.
 *
 * \par Usage
 * \b do { \em Statements } (\em Condition);
 * \par
 * \b do [\em UnbindVars] { \em Statements } (\em Condition);
 *
 * \par Description
 * The \b do loop evaluates \em Statements, and then evaluates
 * \em Condition.  If \em Condition is true, then the loop repeats.
 * If \em Statements fails, then the loop will fail.
 * \par
 * If \em UnbindVars is specified, then it contains a list of
 * local variables that will be unbound at the beginning of
 * each loop iteration before \em Statements is evaluated.
 *
 * \par Examples
 * \code
 * X = 1; do { stdout::writeln(X); X ::= X + 1; } while (X <= 10);
 * X = 1; do [Y] { Y is X * 2; stdout::writeln(Y); X ::= X + 1; } while (X <= 10);
 * \endcode
 *
 * \par See Also
 * \ref for_stmt "for",
 * \ref while_stmt "while"
 */
static p_goal_result p_builtin_do
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result;
    for (;;) {
        if (args[0] != context->nil_atom)
            p_builtin_unbind_variables(args[0]);
        result = p_goal_call(context, args[1], error);
        if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
            return P_RESULT_FAIL;
        else if (result == P_RESULT_ERROR)
            return P_RESULT_ERROR;
        result = p_goal_call(context, args[2], error);
        if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
            break;
        else if (result == P_RESULT_ERROR)
            return P_RESULT_ERROR;
    }
    return P_RESULT_TRUE;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor not_provable_1
 * <b>(!)/1</b>, <b>(\\+)/1</b> - negation by failure.
 *
 * \par Usage
 * \b ! \em Term
 * \par
 * <b>\\+</b> \em Term
 *
 * \par Description
 * If \b call(\em Term) succeeds, then fail; otherwise succeed.
 *
 * \par Examples
 * \code
 * X = a; !(X = b)      succeeds with X = a
 * X = a; !(X = a)      fails
 * \+ fail              succeeds
 * ! true               fails
 * \endcode
 *
 * \par Compatibility
 * The <b>(\\+)/1</b> predicate is compatible with
 * \ref standard "Standard Prolog".  The new name <b>(!)/1</b>
 * is the recommended spelling.
 *
 * \par See Also
 * \ref logical_and_2 "(&amp;&amp;)/2",
 * \ref logical_or_2 "(||)/2"
 */
static p_goal_result p_builtin_not_provable
    (p_context *context, p_term **args, p_term **error)
{
    void *marker = p_context_mark_trace(context);
    p_goal_result result = p_goal_call(context, args[0], error);
    p_context_backtrack_trace(context, marker);
    if (result == P_RESULT_TRUE || result == P_RESULT_CUT_TRUE)
        return P_RESULT_FAIL;
    else if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
        return P_RESULT_TRUE;
    return result;
}

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
 * \anchor for_stmt
 * <b>for</b> - loop over the members of a list.
 *
 * \par Usage
 * \b for (\em Variable \b in \em List) \em Statement
 * \par
 * \b for [\em UnbindVars] (\em Variable \b in \em List) \em Statement
 *
 * \par Description
 * The \b for loop iterates over the members of \em List, binding
 * \em Variable to each member in turn and performing \em Statement.
 * The \em Variable must have a new name that does not occur
 * previously in the clause.
 * \par
 * If \em Statement fails for a member of \em List, then the loop
 * will fail.  If \em Statement succeeds for all members of
 * \em List, then the loop will succeed.
 * \par
 * If \em UnbindVars is specified, then it contains a list of
 * local variables that will be unbound at the beginning of
 * each loop iteration.  The \em Variable must not appear in
 * \em UnbindVars.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - if \em List is a variable,
 *     or the tail of \em List is a variable.
 * \li <tt>type_error(list, </tt>\em List<tt>)</tt> - if \em List
 *     is not a variable or a list, or the tail of \em List
 *     is not a variable or <b>[]</b>.
 *
 * \par Examples
 * \code
 * for (X in List) { stdout::writeln(X); }
 * for [Y] (X in List) { Y is X * 2; stdout::writeln(Y); }
 * \endcode
 *
 * \par See Also
 * \ref do_stmt "do",
 * \ref while_stmt "while"
 */
static p_goal_result p_builtin_for
    (p_context *context, p_term **args, p_term **error)
{
    p_term *list = p_term_deref(args[2]);
    p_goal_result result;
    for (;;) {
        if (!list || (list->header.type & P_TERM_VARIABLE) != 0) {
            *error = p_term_create_atom(context, "instantiation_error");
            return P_RESULT_ERROR;
        } else if (list == context->nil_atom) {
            break;
        } else if (list->header.type != P_TERM_LIST) {
            *error = p_builtin_type_error(context, "list", list);
            return P_RESULT_ERROR;
        }
        if (args[0] != context->nil_atom)
            p_builtin_unbind_variables(args[0]);
        p_builtin_set_variable(args[1], list->list.head);
        result = p_goal_call(context, args[3], error);
        if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
            return P_RESULT_FAIL;
        else if (result == P_RESULT_ERROR)
            return P_RESULT_ERROR;
        list = p_term_deref(list->list.tail);
    }
    return P_RESULT_TRUE;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor if_stmt
 * <b>(->)/2</b>, <b>if</b> - if-then statement.
 *
 * \par Usage
 * \b if (\em Goal1) \em Goal2
 * \par
 * \b if (\em Goal1) \em Goal2 \b else \em Goal3
 * \par
 * (\em Goal1 -> \em Goal2 || \b true)
 * \par
 * (\em Goal1 -> \em Goal2 || \em Goal3)
 *
 * \par Description
 * The \em Goal1 is executed, and if it succeeds then \em Goal2 is
 * executed.  If \em Goal1 fails, then \em Goal3 is executed.
 * If \em Goal3 is omitted, then the statement succeeds if
 * \em Goal1 fails.
 * \par
 * If the goal has the form (\em Goal1 -> \em Goal2) without a
 * \em Goal3 else goal, then the goal will fail if \em Goal1 fails.
 * By contrast, \b if (\em Goal1) \em Goal2 will succeed if
 * \em Goal1 fails.
 *
 * \par Examples
 * \code
 * if (A) B; else C;
 * (A -> B || C)
 * if (A) B;
 * (A -> B || true)         succeeds if A fails
 * (A -> B)                 fails if A fails
 * \endcode
 *
 * \par Compatibility
 * The <b>(->)/2</b> predicate is compatible with
 * \ref standard "Standard Prolog".  Standard prolog expresses
 * if-then-else as <tt>(A -> B ; C)</tt> which is not supported
 * in Plang.  The <b>if</b> statement form is the recommended
 * method to express conditionals in Plang.
 *
 * \par See Also
 * \ref logical_or_2 "(||)/2",
 * \ref switch_stmt "switch"
 */
static p_goal_result p_builtin_if
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result = p_goal_call(context, args[0], error);
    if (result == P_RESULT_TRUE || result == P_RESULT_CUT_TRUE)
        return p_goal_call(context, args[1], error);
    else if (result == P_RESULT_CUT_FAIL)
        return P_RESULT_FAIL;
    else
        return result;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor once_1
 * <b>once/1</b> - executes a goal only once, ignoring subsequent
 * solutions.
 *
 * \par Usage
 * \b once(\em Goal)
 *
 * \par Description
 * Executes \b call(\em Goal) and then executes a cut,
 * \ref cut_0 "(!)/0", to prune searches for further solutions.
 * In essence, \b once(\em Goal) behaves like \b call(\em Goal, !).
 *
 * \par Examples
 * \code
 * once((X = a || Y = b))   succeeds with X = a, never performs Y = b
 * once(fail)               fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref call_1 "call/1"
 */
static char const p_builtin_once[] =
    "once(Goal) { call((Goal, !)); }";

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor repeat_0
 * <b>repeat/0</b> - succeeds repeatedly and indefinitely.
 *
 * \par Usage
 * \b repeat; ...; \b fail
 *
 * \par Description
 * Repeats the sequence of statements between \b repeat and
 * \b fail indefinitely until a cut, \ref cut_0 "(!)/0",
 * is encountered.
 *
 * \par Examples
 * \code
 * repeat; stdout::writeln("hello"); fail
 *                      outputs "hello" indefinitely
 * repeat; !            succeeds
 * repeat; !; fail      fails
 * repeat; fail         loops indefinitely
 * repeat; a = b        loops indefinitely due to unification failure
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref fail_0 "fail/0"
 */
static char const p_builtin_repeat[] =
    "repeat() {}"
    "repeat() { repeat(); }";

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor switch_stmt
 * <b>switch</b> - switch on a term and choose a matching statement
 * to handle the term.
 *
 * \par Usage
 * \b switch (\em Term) { \b case \em Label1: \em Statement1; ...; \b case \em LabelN: \em StatementN; \b default: \em DefaultStatement; }
 *
 * \par Description
 * Finds the first \em LabelM term in the \b case list that unifies
 * with \em Term, and executes the associated \em StatementM.
 * If none of the \b case labels match, then executes the
 * \em DefaultStatement associated with the \b default label.
 * If there is no \b default label, then the \b switch statement fails.
 * \par
 * Multiple case labels can be specified for the same statement with
 * \b case \em Label1: \b case \em Label2: ... \b case \em LabelN: \em Statement.
 * The \b default label can be mixed with regular \b case labels.
 * \par
 * Unlike C/C++, execution does not fall through from
 * \em StatementM to the following \em StatementM+1.
 * \par
 * Once a \em LabelM is found that unifies with \em Term, the \b switch
 * statement does an implicit cut, \ref cut_0 "(!)/0", to commit the
 * clause to that choice.  Backtracking does not select later
 * \b case labels even if they may have otherwise unified
 * with \em Term.
 *
 * \par Examples
 * \code
 * eval(Term, Answer) {
 *     switch (Term) {
 *         case X + Y: {
 *             eval(X, XAnswer);
 *             eval(Y, YAnswer);
 *             Answer is XAnswer + YAnswer;
 *         }
 *         case X - Y: {
 *             eval(X, XAnswer);
 *             eval(Y, YAnswer);
 *             Answer is XAnswer - YAnswer;
 *         }
 *         case X * Y: {
 *             eval(X, XAnswer);
 *             eval(Y, YAnswer);
 *             Answer is XAnswer * YAnswer;
 *         }
 *         case X / Y: {
 *             eval(X, XAnswer);
 *             eval(Y, YAnswer);
 *             Answer is XAnswer / YAnswer;
 *         }
 *         case -X: {
 *             eval(X, XAnswer);
 *             Answer is -XAnswer;
 *         }
 *         default: {
 *             if (number(Term))
 *                 Answer = Term;
 *             else
 *                 lookup_variable(Term, Answer);
 *         }
 *     }
 * }
 *
 * eval(2 * x + y, Answer)
 * \endcode
 *
 * \par See Also
 * \ref if_stmt "if"
 */
static p_goal_result p_builtin_switch
    (p_context *context, p_term **args, p_term **error)
{
    p_term *list = p_term_deref(args[1]);
    p_term *case_atom = p_term_create_atom(context, "$$case");
    p_term *case_term;
    p_term *label_list;
    while (list && list->header.type == P_TERM_LIST) {
        case_term = p_term_deref(list->list.head);
        if (case_term && case_term->header.type == P_TERM_FUNCTOR &&
                case_term->header.size == 2 &&
                case_term->functor.functor_name == case_atom) {
            label_list = p_term_deref(p_term_arg(case_term, 0));
            while (label_list && label_list->header.type == P_TERM_LIST) {
                if (p_term_unify(context, args[0],
                                 label_list->list.head,
                                 P_BIND_DEFAULT)) {
                    return p_goal_call
                        (context, p_term_arg(case_term, 1), error);
                }
                label_list = p_term_deref(label_list->list.tail);
            }
        }
        list = p_term_deref(list->list.tail);
    }
    return p_goal_call(context, args[2], error);
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor throw_1
 * <b>throw/1</b> - throws an error to an enclosing catch goal.
 *
 * \par Usage
 * \b throw(\em Term)
 *
 * \par Description
 * Throws a freshly renamed version of \em Term as an error to
 * an enclosing \ref catch_3 "catch/3" goal that matches \em Term.
 * Plang will backtrack to the matching \ref catch_3 "catch/3"
 * goal and then execute the associated recovery goal.
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref catch_3 "catch/3"
 */
static p_goal_result p_builtin_throw
    (p_context *context, p_term **args, p_term **error)
{
    *error = p_term_clone(context, args[0]);
    return P_RESULT_ERROR;
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

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor while_stmt
 * <b>while</b> - repeatedly execute a statement while a
 * condition is true.
 *
 * \par Usage
 * \b while (\em Condition) \em Statement
 * \par
 * \b while [\em UnbindVars] (\em Condition) \em Statement
 *
 * \par Description
 * The \b while loop evaluates \em Condition at the beginning
 * of each iteration.  If \em Condition is true then \em Statement
 * will be executed.  Otherwise, the loop terminates.
 * If \em Statement fails, then the loop will fail.
 * \par
 * If \em UnbindVars is specified, then it contains a list of
 * local variables that will be unbound at the beginning of
 * each loop iteration before \em Condition is evaluated.
 *
 * \par Examples
 * \code
 * X = 1; while (X <= 10) { stdout::writeln(X); X ::= X + 1; }
 * X = 1; while [Y] (X <= 10) { Y is X * 2; stdout::writeln(Y); X ::= X + 1; }
 * \endcode
 *
 * \par See Also
 * \ref do_stmt "do",
 * \ref for_stmt "for"
 */
static p_goal_result p_builtin_while
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result;
    for (;;) {
        if (args[0] != context->nil_atom)
            p_builtin_unbind_variables(args[0]);
        result = p_goal_call(context, args[1], error);
        if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
            break;
        else if (result == P_RESULT_ERROR)
            return result;
        result = p_goal_call(context, args[2], error);
        if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL)
            return P_RESULT_FAIL;
        else if (result == P_RESULT_ERROR)
            return P_RESULT_ERROR;
    }
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

/* Dummy implementations for stdout/stderr printing until
 * we get a better I/O system up and running */
static p_goal_result p_builtin_print
    (p_context *context, p_term **args, p_term **error)
{
    p_term_print_unquoted
        (context, args[0], p_term_stdio_print_func, stdout);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_println
    (p_context *context, p_term **args, p_term **error)
{
    putc('\n', stdout);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_printq
    (p_context *context, p_term **args, p_term **error)
{
    p_term_print(context, args[0], p_term_stdio_print_func, stdout);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_print_error
    (p_context *context, p_term **args, p_term **error)
{
    p_term_print_unquoted
        (context, args[0], p_term_stdio_print_func, stderr);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_println_error
    (p_context *context, p_term **args, p_term **error)
{
    putc('\n', stderr);
    return P_RESULT_TRUE;
}
static p_goal_result p_builtin_printq_error
    (p_context *context, p_term **args, p_term **error)
{
    p_term_print(context, args[0], p_term_stdio_print_func, stderr);
    return P_RESULT_TRUE;
}

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
        {"!", 0, p_builtin_cut},
        {"!", 1, p_builtin_not_provable},
        {"\\+", 1, p_builtin_not_provable},
        {"||", 2, p_builtin_logical_or},
        {"->", 2, p_builtin_if},
        {"?-", 1, p_builtin_call},
        {":-", 1, p_builtin_call},
        {"abolish", 1, p_builtin_abolish},
        {"asserta", 1, p_builtin_asserta},
        {"assertz", 1, p_builtin_assertz},
        {"atom", 1, p_builtin_atom},
        {"atomic", 1, p_builtin_atomic},
        {"call", 1, p_builtin_call},
        {"catch", 3, p_builtin_catch},
        {"class_object", 1, p_builtin_class_object_1},
        {"class_object", 2, p_builtin_class_object_2},
        {"compound", 1, p_builtin_compound},
        {"$$do", 3, p_builtin_do},
        {"fail", 0, p_builtin_fail},
        {"false", 0, p_builtin_fail},
        {"float", 1, p_builtin_float},
        {"$$for", 4, p_builtin_for},
        {"import", 1, p_builtin_import},
        {"initialization", 1, p_builtin_call},
        {"integer", 1, p_builtin_integer},
        {"nonvar", 1, p_builtin_nonvar},
        {"number", 1, p_builtin_number},
        {"object", 1, p_builtin_object_1},
        {"object", 2, p_builtin_object_2},
        {"$$print", 1, p_builtin_print},
        {"$$println", 0, p_builtin_println},
        {"$$printq", 1, p_builtin_printq},
        {"$$print_error", 1, p_builtin_print_error},
        {"$$println_error", 0, p_builtin_println_error},
        {"$$printq_error", 1, p_builtin_printq_error},
        {"retract", 1, p_builtin_retract},
        {"string", 1, p_builtin_string},
        {"$$switch", 3, p_builtin_switch},
        {"throw", 1, p_builtin_throw},
        {"true", 0, p_builtin_true},
        {"$$try", 2, p_builtin_try},
        {"unifiable", 2, p_builtin_unifiable},
        {"unify_with_occurs_check", 2, p_builtin_unify},
        {"var", 1, p_builtin_var},
        {"$$while", 3, p_builtin_while},
        {0, 0, 0}
    };
    static const char * const builtin_sources[] = {
        p_builtin_once,
        p_builtin_repeat,
        0
    };
    int index;

    /* Register predicates that are implemented in C */
    for (index = 0; builtins[index].name != 0; ++index) {
        p_db_set_builtin_predicate
            (p_term_create_atom(context, builtins[index].name),
             builtins[index].arity, builtins[index].func);
    }

    /* Register predicates that are implemented in Plang */
    for (index = 0; builtin_sources[index] != 0; ++index)
        p_context_consult_string(context, builtin_sources[index]);
}
