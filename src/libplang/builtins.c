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
#include <errno.h>
#include "term-priv.h"
#include "database-priv.h"
#include "context-priv.h"

/**
 * \defgroup predicates Builtin predicates - Overview
 *
 * \par Arithmetic and string operations
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2",
 * \ref atom_name_2 "atom_name/2",
 * \ref fperror_1 "fperror/1",
 * \ref isnan_1 "isnan/1",
 * \ref isinf_1 "isinf/1",
 * \ref randomize_0 "randomize/0",
 * \ref randomize_1 "randomize/1"
 *
 * \par Classes and objects
 * \ref decl_class "class",
 * \ref new_object_3 "new",
 * \ref new_class_4 "new_class/4",
 * \ref new_object_3 "new_object/3"
 *
 * \par Clause handling
 * \ref abolish_1 "abolish/1",
 * \ref asserta_1 "asserta/1",
 * \ref assertz_1 "assertz/1",
 * \ref clause_2 "clause/2",
 * \ref retract_1 "retract/1"
 *
 * \par Directives
 * \ref directive_1 "(:-)/1",
 * \ref initialization_1 "(?-)/1",
 * \ref consult_1 "consult/1",
 * \ref dynamic_1 "dynamic/1",
 * \ref import_1 "import/1",
 * \ref initialization_1 "initialization/1",
 * \ref load_library_1 "load_library/1"
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
 * \ref halt_0 "halt/0",
 * \ref halt_1 "halt/1",
 * \ref if_stmt "(->)/2",
 * \ref if_stmt "if",
 * \ref in_2 "in/2",
 * \ref once_1 "once/1",
 * \ref repeat_0 "repeat/0",
 * \ref switch_stmt "switch",
 * \ref throw_1 "throw/1",
 * \ref true_0 "true/0",
 * \ref catch_3 "try",
 * \ref while_stmt "while"
 *
 * \par Sorting
 * \ref keysort_2 "keysort/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref msort_2 "msort/2",
 * \ref msortd_2 "msortd/2",
 * \ref rkeysort_2 "rkeysort/2",
 * \ref rkeysortd_2 "rkeysortd/2",
 * \ref sort_2 "sort/2",
 * \ref sortd_2 "sortd/2"
 *
 * \par Term comparison
 * \ref term_eq_2 "(==)/2",
 * \ref term_ne_2 "(!==)/2",
 * \ref term_lt_2 "(\@<)/2",
 * \ref term_le_2 "(\@<=)/2",
 * \ref term_gt_2 "(\@>)/2",
 * \ref term_ge_2 "(\@>=)/2"
 *
 * \par Term creation and decomposition
 * \ref list_cons_2 "(.)/2",
 * \ref univ_2 "(=..)/2",
 * \ref arg_3 "arg/3",
 * \ref copy_term_2 "copy_term/2",
 * \ref functor_3 "functor/3"
 *
 * \par Term unification
 * \ref unify_2 "(=)/2",
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unifiable_2 "unifiable/2",
 * \ref unify_one_way_2 "unify_one_way/2"
 * \ref unify_2 "unify_with_occurs_check/2"
 *
 * \par Type testing
 * \ref atom_1 "atom/1",
 * \ref atomic_1 "atomic/1",
 * \ref class_1 "class/1",
 * \ref class_2 "class/2",
 * \ref compound_1 "compound/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref nonvar_1 "nonvar/1",
 * \ref number_1 "number/1",
 * \ref object_1 "object/1",
 * \ref object_2 "object/2",
 * \ref predicate_1 "predicate/1",
 * \ref predicate_2 "predicate/2",
 * \ref string_1 "string/1",
 * \ref var_1 "var/1"
 *
 * \par Variable assignment
 * \ref assign_2 "(:=)/2",
 * \ref num_assign_2 "(::=)/2",
 * \ref bt_assign_2 "(:==)/2",
 * \ref bt_num_assign_2 "(::==)/2"
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
 * \par
 * The following book is an excellent reference work on Standard
 * Prolog, and the inspiration for Plang's documentation style:
 * P. Debransart, A. Ed-Dbali, L. Cervoni, "Prolog: The Standard;
 * Reference Manual", Springer-Verlag, 1996, ISBN 3-540-59304-7.
 * \par
 * \anchor swi_prolog
 * Where Standard Prolog lacks a particular feature, but
 * <a href="http://www.swi-prolog.org/">SWI-Prolog</a>
 * has that feature, we sometimes attempt compatibility with that
 * version of Prolog instead.
 */
/*\@{*/

/*\@}*/

/* Destructively sets a variable to a value */
P_INLINE void p_builtin_set_variable(p_term *var, p_term *value)
{
    if (!var || (var->header.type & P_TERM_VARIABLE) == 0)
        return;
    var->var.value = value;
}

/* Unbinds a list of local loop variables: $$unbind() predicate */
static p_goal_result p_builtin_unbind
    (p_context *context, p_term **args, p_term **error)
{
    p_term *list = p_term_deref(args[0]);
    while (list && list->header.type == P_TERM_LIST) {
        p_builtin_set_variable(list->list.head, 0);
        list = p_term_deref(list->list.tail);
    }
    return P_RESULT_TRUE;
}

/**
 * \defgroup arithmetic Builtin predicates - Arithmetic and string operations
 */
/*\@{*/
/* Predicates and functions for this group are defined in arith.c */
/*\@}*/

/**
 * \defgroup classes_and_objects Builtin predicates - Classes and objects
 *
 * Predicates in this group are used to create classes and objects,
 * and to manipulate them after creation.
 *
 * \ref decl_class "class",
 * \ref new_object_3 "new",
 * \ref new_class_4 "new_class/4",
 * \ref new_object_3 "new_object/3"
 */
/*\@{*/

static void p_builtin_add_member_predicate
    (p_context *context, p_term *class_object,
     p_term *member_name, p_term *predicate)
{
    p_term *list;
    p_term *property = p_term_own_property
        (context, class_object, member_name);
    if (property && property != predicate) {
        /* Search the existing predicate list and add this
         * predicate if it does not currently exist.  We need
         * a list of predicates to support arity overloading */
        if (property->header.type == P_TERM_LIST) {
            list = property;
            while (list && list->header.type == P_TERM_LIST) {
                if (list->list.head == predicate)
                    return;     /* Predicate is already in the list */
                list = list->list.tail;
            }
        } else {
            /* Convert a single predicate reference into a list */
            property = p_term_create_list
                (context, property, context->nil_atom);
        }
        property = p_term_create_list(context, predicate, property);
        p_term_set_own_property
            (context, class_object, member_name, property);
    } else if (!property) {
        /* First time predicate was added to this property */
        p_term_add_property
            (context, class_object, member_name, predicate);
    }
}

/**
 * \addtogroup classes_and_objects
 * <hr>
 * \anchor decl_class
 * \anchor new_class_4
 * <b>class</b>, <b>new_class/4</b> - declares a new class.
 *
 * \par Usage
 * \b class \em Name { \em Members }
 * \par
 * \b class \em Name : \em Parent { \em Members }
 * \par
 * \b new_class(\em Name, \em Parent, \em Vars, \em Clauses)
 *
 * \par Description
 * The \em Name must be an atom to identify the new class that
 * is different than all previous atoms used as class names.
 * If \em Parent is present, then it must be an atom that identifies
 * an existing class, or <tt>[]</tt> for no parent.
 * \par
 * Usually classes are created in the source file with the \b class
 * keyword.  The <b>new_class/4</b> predicate can be used to create a
 * class dynamically.
 * \par
 * \em Vars must be a list of atoms for the member field names.
 * These members will be added as unbound variable properties
 * to new objects of the class \em Name.  Duplicates in \em Vars
 * will be ignored.
 * \par
 * \em Clauses must be a list of \b clause(\em MemberName, \em Kind,
 * \em Clause) terms:
 * \li \em MemberName is the name of the predicate member, without
 *     the class \em Name as a qualifier.
 * \li \em Kind is one of the atoms \c member, \c static, or
 *     \c constructor, indicating an ordinary member predicate,
 *     a static predicate, or an object constructor.
 * \li \em Clause is a <b>(:-)/2</b> functor term for a single
 *     clause within the predicate being defined.  The clause
 *     should have the predicate name \em Name::\em MemberName.
 *     If the \em Kind is \c member or \c constructor, then the
 *     first argument of \em Clause will be passed the <tt>Self</tt>
 *     object when the predicate is called.
 * \par
 * The atoms \c className and \c prototype are reserved and cannot
 * be used as member names for declared variable or predicate members.
 * \par
 * Class \em Members may have one of the following forms,
 * corresponding to member variables, regular member
 * predicates, static predicates, and constructor predicates:
 * \code
 * var name1, ..., nameN
 * name(Args) { ... }
 * static name(Args) { ... }
 * new(Args) { ... }
 * \endcode
 * \par
 * When an object is constructed with the \ref new_object_3 "new"
 * keyword, it is initially populated with \c name1, ..., \c nameN
 * set to unbound variables, and \c prototype set to the class object.
 * One of the constructor predicates is then called to initialize
 * the object.
 * \par
 * Regular member predicates and constructor predicates are passed
 * the \c Self object as a hidden first argument.  Static predicates
 * do not have any hidden arguments.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Name, \em Parent, \em Vars,
 *     or \em Clauses is a variable.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Name is not an atom.
 * \li <tt>type_error(atom, \em Parent)</tt> - \em Parent is not
 *     an atom.
 * \li <tt>type_error(atom, \em Var)</tt> - \em Var is a member of
 *     \em Vars but it is not an atom.
 * \li <tt>type_error(atom_list, \em Vars)</tt> - \em Vars is not a
 *     valid list of atoms.
 * \li <tt>type_error(clause_list, \em Clauses)</tt> - \em Clauses
 *     is not a valid list of clauses.
 * \li <tt>type_error(member_name, \em MemberName)</tt> -
 *     \em MemberName is \c className or \c prototype.
 * \li <tt>permission_error(create, class, \em Name)</tt> -
 *     \em Name already exists as a class.
 * \li <tt>permission_error(modify, static_procedure, \em Pred)</tt> -
 *     attempting to define a predicate \em Pred that is
 *     already defined.
 * \li <tt>existence_error(class, \em Parent)</tt> - \em Parent
 *     does not exist as a class, and \em Parent is not <tt>[]</tt>.
 *
 * \par Examples
 * \code
 * class vehicle
 * {
 *     var owner, wheels
 *     transferOwnership(NewOwner)
 *     {
 *         Self.owner := NewOwner;
 *     }
 * }
 *
 * class passenger_car : vehicle
 * {
 *     var make, model
 *     new(Make, Model)
 *     {
 *         Self.wheels = 4;
 *         Self.make = Make;
 *         Self.model = Model;
 *     }
 * }
 *
 * class company
 * {
 *     var name
 *     var fleet
 *     new(Name)
 *     {
 *         Self.name = Name;
 *         Self.fleet = [];
 *     }
 *     add_vehicle(Vehicle)
 *     {
 *         Self.fleet := [Vehicle|Self.fleet];
 *     }
 *     remove_vehicle(Vehicle)
 *     {
 *         remove(Vehicle, Self.fleet, List);
 *         Self.fleet := List;
 *     }
 * }
 *
 * class truck : vehicle
 * {
 *     var company
 *     new(Company, Wheels)
 *     {
 *         Self.company = Company;
 *         Self.owner = Company.name;
 *         Self.wheels = Wheels;
 *         Company.add_vehicle(Self);
 *     }
 *     transferOwnership(Company)
 *     {
 *         Self.company.remove_vehicle(Self);
 *         Self.company := Company;
 *         vehicle::transferOwnership(Self, Company.name);
 *         Company.add_vehicle(Self);
 *     }
 * }
 *
 * class semi_trailer : truck
 * {
 *     new(Company)
 *     {
 *         truck::new(Self, Company, 16);
 *     }
 * }
 *
 * new passenger_car(P, "MegaCarz", "FastKar 2000");
 * P.owner = "Fred";
 * P.transferOwnership("Mary");
 *
 * new company(C1, "Package Delivery, Inc.");
 * new company(C2, "Deliver Packages, Inc.");
 * new semi_trailer(S, C1);
 * S.transferOwnership(C2);
 * \endcode
 *
 * \par See Also
 * \ref class_1 "class/1",
 * \ref new_object_3 "new"
 */
static p_goal_result p_builtin_new_class
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
    p_term *parent = p_term_deref_member(context, args[1]);
    p_term *vars = p_term_deref_member(context, args[2]);
    p_term *clauses = p_term_deref_member(context, args[3]);
    p_database_info *info;
    p_database_info *info2;
    p_class_info *class_info;
    p_class_info *parent_info;
    p_term *class_object;
    p_term *prototype;
    p_term *list;
    p_term *var_name;
    p_term *member_name;
    p_term *kind;
    p_term *clause_term;
    p_term *clause_body;
    p_term *clause_atom;
    p_term *member_atom;
    p_term *predicate;

    /* Validate the class name - must be an atom and not yet a class */
    if (!name || (name->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (name->header.type != P_TERM_ATOM) {
        *error = p_create_type_error(context, "atom", name);
        return P_RESULT_ERROR;
    }
    info = _p_db_create_arity(name, 0);
    if (!info)
        return P_RESULT_FAIL;
    if (info->class_info) {
        /* There already exists a class with this name */
        *error = p_create_permission_error
            (context, "create", "class", name);
        return P_RESULT_ERROR;
    }

    /* Locate the prototype for the parent class */
    if (!parent || (parent->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (parent && parent != context->nil_atom) {
        if (parent->header.type != P_TERM_ATOM) {
            *error = p_create_type_error(context, "atom", parent);
            return P_RESULT_ERROR;
        }
        info2 = _p_db_find_arity(parent, 0);
        if (!info2 || !info2->class_info) {
            /* Parent class does not exist */
            *error = p_create_existence_error(context, "class", parent);
            return P_RESULT_ERROR;
        }
        parent_info = info2->class_info;
        prototype = parent_info->class_object;
    } else {
        parent_info = 0;
        prototype = 0;
    }

    /* Validate the member variable list */
    list = vars;
    if (!list || (list->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    for (;;) {
        if (list == context->nil_atom)
            break;
        if (!list || list->header.type != P_TERM_LIST) {
            *error = p_create_type_error(context, "atom_list", vars);
            return P_RESULT_ERROR;
        }
        var_name = p_term_deref_member(context, list->list.head);
        if (!var_name ||
                (var_name->header.type & P_TERM_VARIABLE) != 0) {
            *error = p_create_instantiation_error(context);
            return P_RESULT_ERROR;
        }
        if (var_name->header.type != P_TERM_ATOM) {
            *error = p_create_type_error(context, "atom_list", vars);
            return P_RESULT_ERROR;
        }
        if(var_name == context->class_name_atom ||
                var_name == context->prototype_atom) {
            *error = p_create_type_error
                (context, "member_name", var_name);
            return P_RESULT_ERROR;
        }
        list = p_term_deref_member(context, list->list.tail);
    }

    /* Create the class information block and the object */
    class_info = GC_NEW(p_class_info);
    if (!class_info)
        return P_RESULT_FAIL;
    class_object = p_term_create_class_object(context, name, prototype);
    if (!class_object)
        return P_RESULT_FAIL;
    class_info->class_object = class_object;
    class_info->parent = parent_info;
    class_info->var_list = vars;

    /* Define the clauses into the database and attach
     * them to the class object */
    list = clauses;
    clause_atom = p_term_create_atom(context, "clause");
    member_atom = p_term_create_atom(context, "member");
    while (list && list->header.type == P_TERM_LIST) {
        /* Extract clause(MemberName, Kind, ClauseBody) from the list */
        clause_term = p_term_deref_member(context, list->list.head);
        if (!clause_term ||
                (clause_term->header.type & P_TERM_VARIABLE) != 0) {
            *error = p_create_instantiation_error(context);
            return P_RESULT_ERROR;
        }
        if (clause_term->header.type == P_TERM_FUNCTOR &&
                clause_term->header.size == 3 &&
                clause_term->functor.functor_name == context->line_atom)
            clause_term = p_term_deref_member
                (context, clause_term->functor.arg[2]);
        if (clause_term->header.type != P_TERM_FUNCTOR ||
                clause_term->header.size != 3 ||
                clause_term->functor.functor_name != clause_atom) {
            break;
        }
        member_name = p_term_deref_member
            (context, clause_term->functor.arg[0]);
        kind = p_term_deref_member
            (context, clause_term->functor.arg[1]);
        clause_body = p_term_deref_member
            (context, clause_term->functor.arg[2]);
        if (!member_name || member_name->header.type != P_TERM_ATOM)
            break;
        if (!kind || kind->header.type != P_TERM_ATOM)
            break;
        if (!clause_body ||
                clause_body->header.type != P_TERM_FUNCTOR ||
                clause_body->header.size != 2 ||
                clause_body->functor.functor_name
                        != context->clause_atom)
            break;

        /* Validate the member name */
        if(member_name == context->class_name_atom ||
                member_name == context->prototype_atom) {
            *error = p_create_type_error
                (context, "member_name", member_name);
            return P_RESULT_ERROR;
        }

        /* Define the clause into the database */
        predicate = _p_db_clause_assert_last(context, clause_body);
        if (!predicate) {
            /* Predicate is compiled or builtin */
            p_term *pred, *head;
            pred = p_term_create_functor
                (context, context->slash_atom, 2);
            head = p_term_deref_member
                (context, clause_body->functor.arg[0]);
            if (head->header.type == P_TERM_FUNCTOR) {
                p_term_bind_functor_arg
                    (pred, 0, head->functor.functor_name);
                p_term_bind_functor_arg
                    (pred, 1,
                     p_term_create_integer(context, head->header.size));
            } else {
                p_term_bind_functor_arg(pred, 0, head);
                p_term_bind_functor_arg(pred, 1, 0);
            }
            *error = p_create_permission_error
                (context, "modify", "static_procedure", pred);
            return P_RESULT_ERROR;
        }
        if (kind == member_atom) {
            /* Add this predicate to the member_name class property */
            p_builtin_add_member_predicate
                (context, class_object, member_name, predicate);
        }

        /* Move on to the next clause in the list */
        list = p_term_deref_member
            (context, list->list.tail);
    }
    if (!list || (list->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (list != context->nil_atom) {
        *error = p_create_type_error(context, "clause_list", clauses);
        return P_RESULT_ERROR;
    }

    /* Class object is ready to go */
    info->class_info = class_info;
    return P_RESULT_TRUE;
}

static p_goal_result p_builtin_univ
    (p_context *context, p_term **args, p_term **error);

/**
 * \addtogroup classes_and_objects
 * <hr>
 * \anchor new_object_3
 * <b>new</b>, <b>new_object/3</b> - create a new object
 * instance of a class.
 *
 * \par Usage
 * \b new \em Name(\em Var, \em Args)
 * \par
 * \b new_object(\em Name, \em Var, \em ArgList)
 *
 * \par Description
 * The <b>new</b> keyword constructs a new instance of the class
 * \em Name and unifies it with \em Var.  The comma-separated
 * list of \em Args is passed to the constructor for \em Name.
 * \par
 * The <b>new_object/3</b> predicate performs the same operation
 * for constructing objects dynamically.  The \em ArgList is
 * a regular Plang list rather than a comma-separated list.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Name or \em ArgList
 *     is a variable, or the tail of \em ArgList is not <tt>[]</tt>.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Name is not an atom.
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is not a
 *     variable.
 * \li <tt>type_error(list, \em ArgList)</tt> - \em ArgList is not a
 *     list.
 * \li <tt>existence_error(class, \em Name)</tt> - \em Name
 *     does not exist as a class.
 * \li <tt>existence_error(procedure, \em Pred)</tt> - \em Pred
 *     is the name of the constructor predicate that <b>new</b>
 *     attempted to call but the constructor does not exist.
 *
 * \par Examples
 * \code
 * class foo
 * {
 *     new(X, Y) { ... }
 * }
 * class bar
 * {
 * }
 *
 * new foo(F, 1.5, f(X))
 * new bar(B)
 * new_object(foo, F, [1.5, f(X)])
 * new_object(bar, B, [])
 * \endcode
 *
 * \par See Also
 * \ref decl_class "class",
 * \ref object_1 "object/1"
 */
static p_goal_result p_builtin_new
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
    p_term *var = p_term_deref_member(context, args[1]);
    p_term *obj;
    p_database_info *info;
    p_class_info *class_info;
    p_term *vars;
    p_term *var_name;

    /* Validate the parameters */
    if (!name || !var || (name->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (name->header.type != P_TERM_ATOM) {
        *error = p_create_type_error(context, "atom", name);
        return P_RESULT_ERROR;
    }
    if ((var->header.type & P_TERM_VARIABLE) == 0) {
        *error = p_create_type_error(context, "variable", var);
        return P_RESULT_ERROR;
    }

    /* Find the class information for creating the object */
    info = _p_db_find_arity(name, 0);
    if (!info || !(info->class_info)) {
        *error = p_create_existence_error(context, "class", name);
        return P_RESULT_ERROR;
    }

    /* Create the bare object term */
    class_info = info->class_info;
    obj = p_term_create_object(context, class_info->class_object);
    if (!obj)
        return P_RESULT_FAIL;

    /* Create unbound variable property slots for all declared fields */
    do {
        vars = p_term_deref_member(context, class_info->var_list);
        while (vars && vars->header.type == P_TERM_LIST) {
            var_name = p_term_deref_member(context, vars->list.head);
            if (!p_term_own_property(context, obj, var_name)) {
                p_term_add_property
                    (context, obj, var_name,
                     p_term_create_variable(context));
            }
            vars = p_term_deref_member(context, vars->list.tail);
        }
        class_info = class_info->parent;
    } while (class_info);

    /* Unify the object with the variable */
    if (p_term_unify(context, var, obj, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}
static p_goal_result p_builtin_new_object
    (p_context *context, p_term **args, p_term **error)
{
    p_goal_result result;
    p_term *name;
    p_term *list;
    p_term *univ_args[2];

    /* Create the basic object instance */
    result = p_builtin_new(context, args, error);
    if (result != P_RESULT_TRUE)
        return result;

    /* Use the "=.." operator to build a call to the constructor */
    name = p_term_create_member_name
        (context, args[0], p_term_create_atom(context, "new"));
    list = p_term_create_list(context, args[1], args[2]);
    list = p_term_create_list(context, name, list);
    univ_args[0] = p_term_create_variable(context);
    univ_args[1] = list;
    result = p_builtin_univ(context, args, error);
    if (result != P_RESULT_TRUE)
        return result;

    /* Replace the current goal with the constructor call */
    context->current_node->goal = univ_args[0];
    return P_RESULT_TREE_CHANGE;
}

static p_term *p_create_member_existence_error
    (p_context *context, p_term *object, p_term *name, p_term *arg_head)
{
    p_term *class_name = p_term_property
        (context, object, context->class_name_atom);
    p_term *full_name = p_term_create_member_name
        (context, class_name, name);
    p_term *pred = p_term_create_functor
        (context, context->slash_atom, 2);
    p_term_bind_functor_arg(pred, 0, full_name);
    p_term_bind_functor_arg
        (pred, 1, p_term_create_integer
            (context, (int)(arg_head->header.size)));
    return p_create_existence_error(context, "member_predicate", pred);
}

/* Implementation of '$$call_member'(X.name, '$$'(X, args)) */
static p_goal_result p_builtin_call_member
    (p_context *context, p_term **args, p_term **error)
{
    p_term *member = p_term_deref(args[0]);
    p_term *arg_head = p_term_deref(args[1]);
    p_term *object;
    p_term *predicate;
    p_term *list;
    p_term *clause_list;
    p_term *body;
    p_exec_node *current;
    p_exec_node *new_current;
    p_exec_clause_node *next;
    int arity;

    /* Validate the parameters */
    if (!member || member->header.type != P_TERM_MEMBER_VARIABLE)
        return P_RESULT_FAIL;
    if (!arg_head || arg_head->header.type != P_TERM_FUNCTOR)
        return P_RESULT_FAIL;
    object = p_term_deref_member(context, member->member_var.object);
    if (!object || object->header.type != P_TERM_OBJECT) {
        *error = p_create_type_error(context, "object", object);
        return P_RESULT_ERROR;
    }

    /* Find the predicate to be executed via this member */
    predicate = p_term_property
        (context, object, member->member_var.name);
    if (!predicate) {
        *error = p_create_member_existence_error
            (context, object, member->member_var.name, arg_head);
        return P_RESULT_ERROR;
    }
    if (predicate->header.type == P_TERM_LIST) {
        /* Overloaded predicate member: search for the correct arity */
        list = predicate;
        arity = -1;
        do {
            predicate = p_term_deref(list->list.head);
            if (predicate &&
                    predicate->header.type == P_TERM_PREDICATE &&
                    predicate->header.size == arg_head->header.size) {
                arity = (int)(arg_head->header.size);
                break;
            }
            list = p_term_deref(list->list.tail);
        } while (list && list->header.type == P_TERM_LIST);
    } else if (predicate->header.type == P_TERM_PREDICATE) {
        /* Single predicate associated with the member name */
        arity = (int)(predicate->header.size);
    } else {
        *error = p_create_type_error(context, "predicate", predicate);
        return P_RESULT_ERROR;
    }
    if (arity != (int)(arg_head->header.size)) {
        *error = p_create_member_existence_error
            (context, object, member->member_var.name, arg_head);
        return P_RESULT_ERROR;
    }

    /* Search for the first predicate clause that matches */
    clause_list = predicate->predicate.clauses_head;
    while (clause_list != 0) {
        body = p_term_unify_clause
            (context, arg_head, clause_list->list.head);
        if (body) {
            current = context->current_node;
            clause_list = clause_list->list.tail;
            if (clause_list) {
                next = GC_NEW(p_exec_clause_node);
                new_current = GC_NEW(p_exec_node);
                if (!next || !new_current)
                    return P_RESULT_FAIL;
                next->parent.parent.goal = arg_head;
                next->parent.parent.success_node = current->success_node;
                next->parent.parent.cut_node = context->fail_node;
                _p_context_init_fail_node
                    (context, &(next->parent), _p_context_clause_fail_func);
                next->next_clause = clause_list;
                new_current->goal = body;
                new_current->success_node = current->success_node;
                new_current->cut_node = context->fail_node;
                context->current_node = new_current;
                context->fail_node = &(next->parent);
            } else {
                new_current = GC_NEW(p_exec_node);
                if (!new_current)
                    return P_RESULT_FAIL;
                new_current->goal = body;
                new_current->success_node = current->success_node;
                new_current->cut_node = context->fail_node;
                context->current_node = new_current;
            }
            return P_RESULT_TREE_CHANGE;
        }
        clause_list = clause_list->list.tail;
    }
    return P_RESULT_FAIL;
}

/*\@}*/

/**
 * \defgroup clause_handling Builtin predicates - Clause handling
 *
 * Predicates in this group are used to add and remove clauses
 * from the predicate database.
 *
 * \ref abolish_1 "abolish/1",
 * \ref asserta_1 "asserta/1",
 * \ref assertz_1 "assertz/1",
 * \ref clause_2 "clause/2",
 * \ref retract_1 "retract/1"
 */
/*\@{*/

static p_term *p_builtin_parse_indicator
    (p_context *context, p_term *pred, int *arity, p_term **error)
{
    p_term *name_term;
    p_term *arity_term;
    pred = p_term_deref_member(context, pred);
    if (!pred || (pred->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return 0;
    } else if (pred->header.type != P_TERM_FUNCTOR ||
               pred->header.size != 2 ||
               pred->functor.functor_name != context->slash_atom) {
        *error = p_create_type_error
            (context, "predicate_indicator", pred);
        return 0;
    }
    name_term = p_term_deref_member(context, pred->functor.arg[0]);
    arity_term = p_term_deref_member(context, pred->functor.arg[1]);
    if (!name_term || (name_term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return 0;
    }
    if (!arity_term || (arity_term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return 0;
    }
    if (arity_term->header.type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", arity_term);
        return 0;
    }
    if (name_term->header.type != P_TERM_ATOM) {
        *error = p_create_type_error(context, "atom", name_term);
        return 0;
    }
    *arity = p_term_integer_value(arity_term);
    if (*arity < 0) {
        *error = p_create_domain_error
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
 *     \em Pred is a builtin or compiled predicate.
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
        *error = p_create_permission_error
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
 *     builtin or compiled predicate.
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
 * \ref retract_1 "retract/1",
 * \ref dynamic_1 "dynamic/1"
 */
static p_goal_result p_builtin_assert
    (p_context *context, p_term **args, p_term **error, int at_start)
{
    p_term *clause = p_term_deref_member(context, args[0]);
    p_term *head;
    p_term *pred;
    if (!clause || (clause->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (clause->header.type == P_TERM_FUNCTOR &&
            clause->header.size == 2 &&
            clause->functor.functor_name == context->clause_atom) {
        head = p_term_deref_member(context, clause->functor.arg[0]);
    } else {
        head = clause;
        clause = p_term_create_functor
            (context, context->clause_atom, 2);
        p_term_bind_functor_arg(clause, 0, head);
        p_term_bind_functor_arg(clause, 1, context->true_atom);
    }
    if (!head || (head->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (head->header.type != P_TERM_ATOM &&
            head->header.type != P_TERM_FUNCTOR) {
        *error = p_create_type_error(context, "callable", head);
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
    *error = p_create_permission_error
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

/** @cond */
typedef struct p_exec_clause_fetch_node p_exec_clause_fetch_node;
struct p_exec_clause_fetch_node
{
    p_exec_fail_node parent;
    p_term *head;
    p_term *body;
    p_term *next_clause;
};
/** @endcond */

/* Find the next matching clause for clause/2 */
static void _p_context_fetch_clause_fail_func
    (p_context *context, p_exec_fail_node *node)
{
    p_exec_clause_fetch_node *current = (p_exec_clause_fetch_node *)node;
    p_term *clause = current->next_clause;
    p_term *body;
    p_exec_node *next;
    p_exec_clause_fetch_node *retry;
    void *marker;
    _p_context_basic_fail_func(context, node);
    while (clause != 0) {
        marker = p_context_mark_trail(context);
        body = p_term_unify_clause
            (context, current->head, clause->list.head);
        if (body && p_term_unify(context, current->body, body, P_BIND_DEFAULT)) {
            clause = clause->list.tail;
            if (clause) {
                next = GC_NEW(p_exec_node);
                retry = GC_NEW(p_exec_clause_fetch_node);
                if (!next || !retry) {
                    current->parent.parent.goal = context->fail_atom;
                    return;
                }
                next->goal = context->true_atom;
                next->success_node = current->parent.parent.success_node;
                next->cut_node = current->parent.parent.cut_node;
                retry->parent.parent.goal = current->parent.parent.goal;
                retry->parent.parent.success_node =
                        current->parent.parent.success_node;
                retry->parent.parent.cut_node =
                        current->parent.parent.cut_node;
                retry->head = current->head;
                retry->body = current->body;
                retry->next_clause = clause;
                _p_context_init_fail_node
                    (context, &(retry->parent),
                     _p_context_fetch_clause_fail_func);
                retry->parent.fail_marker = marker;
                context->current_node = next;
                context->fail_node = &(retry->parent);
            } else {
                next = GC_NEW(p_exec_node);
                if (next) {
                    next->goal = context->true_atom;
                    next->success_node = current->parent.parent.success_node;
                    next->cut_node = current->parent.parent.cut_node;
                    context->current_node = next;
                } else {
                    current->parent.parent.goal = context->true_atom;
                }
            }
            return;
        }
        p_context_backtrack_trail(context, marker);
        clause = clause->list.tail;
    }
    next = GC_NEW(p_exec_node);
    if (next) {
        next->goal = context->fail_atom;
        next->success_node = current->parent.parent.success_node;
        next->cut_node = current->parent.parent.cut_node;
                context->current_node = next;
        context->current_node = next;
    } else {
        current->parent.parent.goal = context->fail_atom;
    }
}

/**
 * \addtogroup clause_handling
 * <hr>
 * \anchor clause_2
 * <b>clause/2</b> - searches for clauses in the predicate database.
 *
 * \par Usage
 * \b clause(\em Head, \em Body)
 *
 * \par Description
 * Succeeds if \em Head <tt>:-</tt> \em Body unifies with a
 * clause in the predicate database.  If there are multiple
 * clauses that unify, then backtrack through the alternatives.
 * Fails immediately if no clauses unify.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Head is a variable.
 * \li <tt>type_error(callable, \em Head)</tt> - \em Head is not a
 *     callable term (atom or functor).
 * \li <tt>permission_error(access, private_procedure, \em Pred)</tt> -
 *     the predicate indicator \em Pred of \em Head refers to a
 *     predicate that is builtin or compiled.
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref asserta_1 "asserta/1"
 */
static p_goal_result p_builtin_clause
    (p_context *context, p_term **args, p_term **error)
{
    p_term *head = p_term_deref_member(context, args[0]);
    p_term *name;
    unsigned int arity;
    p_database_info *info;
    p_term *pred;
    p_term *clause;
    p_term *body;
    void *marker;
    p_exec_node *current;
    p_exec_node *next;
    p_exec_clause_fetch_node *retry;

    /* Validate the head */
    if (!head || (head->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (head->header.type == P_TERM_ATOM) {
        name = head;
        arity = 0;
    } else if (head->header.type == P_TERM_FUNCTOR) {
        name = head->functor.functor_name;
        arity = head->header.size;
    } else {
        *error = p_create_type_error(context, "callable", head);
        return P_RESULT_ERROR;
    }

    /* Find the predicate associated with the head */
    info = _p_db_find_arity(name, arity);
    if (!info || !info->predicate)
        return P_RESULT_FAIL;

    /* If the predicate is builtin or compiled, then throw an error */
    if (info->flags & (P_PREDICATE_BUILTIN | P_PREDICATE_COMPILED)) {
        pred = p_term_create_functor(context, context->slash_atom, 2);
        p_term_bind_functor_arg(pred, 0, name);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer(context, (int)arity));
        *error = p_create_permission_error
            (context, "access", "private_procedure", pred);
        return P_RESULT_ERROR;
    }

    /* Find the first clause that matches */
    clause = info->predicate->predicate.clauses_head;
    if (!clause)
        return P_RESULT_FAIL;
    while (clause != 0) {
        marker = p_context_mark_trail(context);
        body = p_term_unify_clause(context, head, clause->list.head);
        if (body && p_term_unify(context, args[1], body, P_BIND_DEFAULT)) {
            clause = clause->list.tail;
            if (clause) {
                current = context->current_node;
                next = GC_NEW(p_exec_node);
                retry = GC_NEW(p_exec_clause_fetch_node);
                if (!next || !retry)
                    return P_RESULT_FAIL;
                next->goal = context->true_atom;
                next->success_node = current->success_node;
                next->cut_node = context->fail_node;
                retry->parent.parent.goal = current->goal;
                retry->parent.parent.success_node =
                        current->success_node;
                retry->parent.parent.cut_node = context->fail_node;
                retry->head = head;
                retry->body = args[1];
                retry->next_clause = clause;
                _p_context_init_fail_node
                    (context, &(retry->parent),
                     _p_context_fetch_clause_fail_func);
                context->current_node = next;
                context->fail_node = &(retry->parent);
                return P_RESULT_TREE_CHANGE;
            }
            return P_RESULT_TRUE;
        }
        p_context_backtrack_trail(context, marker);
        clause = clause->list.tail;
    }
    return P_RESULT_FAIL;
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
 *     builtin or compiled predicate.
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
    p_term *clause = p_term_deref_member(context, args[0]);
    p_term *head;
    p_term *pred;
    int result;
    if (!clause || (clause->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (clause->header.type == P_TERM_FUNCTOR &&
            clause->header.size == 2 &&
            clause->functor.functor_name == context->clause_atom) {
        head = p_term_deref_member(context, clause->functor.arg[0]);
    } else {
        head = clause;
        clause = p_term_create_functor
            (context, context->clause_atom, 2);
        p_term_bind_functor_arg(clause, 0, head);
        p_term_bind_functor_arg(clause, 1, context->true_atom);
    }
    if (!head || (head->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (head->header.type != P_TERM_ATOM &&
            head->header.type != P_TERM_FUNCTOR) {
        *error = p_create_type_error(context, "callable", head);
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
    *error = p_create_permission_error
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
 *     F.open();
 * }
 * \endcode
 *
 * After the directive is executed, the Plang engine will execute a
 * cut, \ref cut_0 "(!)/0", and \ref fail_0 "fail/0" to backtrack
 * to the original system state.  The only permanent modifications
 * to the system state will be in the form of side-effects.
 *
 * Directives may also be called as regular builtin predicates
 * during normal program execution.
 *
 * \ref directive_1 "(:-)/1",
 * \ref initialization_1 "(?-)/1",
 * \ref consult_1 "consult/1",
 * \ref dynamic_1 "dynamic/1",
 * \ref import_1 "import/1",
 * \ref initialization_1 "initialization/1",
 * \ref load_library_1 "load_library/1"
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
 * \anchor consult_1
 * <b>consult/1</b> - consults the contents of a source file.
 *
 * \par Usage
 * <b>:-</b> \b consult(\em Filename).
 *
 * \par Description
 * The \em Filename must be an atom or string, whose name refers to a
 * Plang source file.  If the file exists, it will be parsed and
 * loaded into the current execution context.  Fails if \em Filename
 * could be loaded due to an error.
 * \par
 * The <b>consult/1</b> directive differs from \ref import_1 "import/1"
 * in that it will load the file again even if it has been loaded
 * previously.  It also does not search the import search path for
 * \em Filename.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Filename is a variable.
 * \li <tt>type_error(atom_or_string, \em Filename)</tt> - \em Filename
 *     is not an atom or string.
 * \li <tt>existence_error(file, \em Filename)</tt> - \em Filename
 *     could not be opened.
 *
 * \par Examples
 * \code
 * :- consult("myapp.lp").
 * :- consult(X).               instantiation_error
 * :- consult(1.5).             type_error(atom_or_string, 1.5)
 * :- consult("not_found.lp").  existence_error(file, "not_found.lp")
 * :- consult("../dir/file.lp").
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog" has a directive called
 * <b>include/1</b> that performs a similar function
 * to <b>consult/1</b>.
 *
 * \par See Also
 * \ref directive_1 "(:-)/1",
 * \ref import_1 "import/1"
 */
static p_goal_result p_builtin_consult
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
    int result;
    if (!name || (name->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    } else if (name->header.type != P_TERM_ATOM &&
               name->header.type != P_TERM_STRING) {
        *error = p_create_type_error(context, "atom_or_string", name);
        return P_RESULT_ERROR;
    }
    result = p_context_consult_file
        (context, p_term_name(name), P_CONSULT_DEFAULT);
    if (!result) {
        return P_RESULT_TRUE;
    } else if (result != EINVAL) {
        *error = p_create_existence_error(context, "file", name);
        return P_RESULT_ERROR;
    } else {
        return P_RESULT_TRUE;
    }
}

/**
 * \addtogroup directives
 * <hr>
 * \anchor dynamic_1
 * <b>dynamic/1</b> - marks a user-defined predicate as dynamic.
 *
 * \par Usage
 * <b>:-</b> \b dynamic(\em Pred).
 *
 * \par Description
 * Marks the predicate associated with the predicate indicator
 * \em Pred as dynamic.  The indicator should have the form
 * \em Name / \em Arity.
 * \par
 * Clauses of a dynamic predicate will not be compiled.
 * This is intended for predicates that are created dynamically
 * in the database at runtime with \ref asserta_1 "asserta/1"
 * and friends.
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
 *     \em Pred is a builtin or compiled predicate.
 *
 * \par Examples
 * \code
 * dynamic(userdef/3)       succeeds
 * dynamic(Pred)            instantiation_error
 * dynamic(Name/3)          instantiation_error
 * dynamic(userdef/Arity)   instantiation_error
 * dynamic(1.5)             type_error(predicate_indicator, 1.5)
 * dynamic(userdef/a)       type_error(integer, a)
 * dynamic(1/a)             type_error(integer, a)
 * dynamic(1/3)             type_error(atom, 1)
 * dynamic(userdef/-3)      domain_error(not_less_than_zero, -3)
 * dynamic(dynamic/1)       permission_error(modify, static_procedure, dynamic/1)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref asserta_1 "asserta/1"
 */
static p_goal_result p_builtin_dynamic
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name;
    int arity;
    p_predicate_flags flags;
    name = p_builtin_parse_indicator(context, args[0], &arity, error);
    if (!name)
        return P_RESULT_ERROR;
    flags = p_db_predicate_flags(context, name, arity);
    if (flags & (P_PREDICATE_COMPILED | P_PREDICATE_BUILTIN)) {
        *error = p_create_permission_error
            (context, "modify", "static_procedure", args[0]);
        return P_RESULT_ERROR;
    }
    p_db_set_predicate_flag
        (context, name, arity, P_PREDICATE_DYNAMIC, 1);
    return P_RESULT_TRUE;
}

/* Defined in parser.y */
int p_context_builtin_import(p_context *context, const char *name);

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
 * \par
 * The behavior of <b>import/1</b> is slightly different when
 * used in a \ref directive_1 "(:-)/1" directive than when
 * used in other contexts.  Within a directive, the search
 * starts in the same directory as the including source file.
 * Elsewhere, the search starts with the current directory.
 * This is because the name of the including source file is
 * not available outside of a directive context.
 * \par
 * Fails if \em Name could be loaded due to an error.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Name is a variable.
 * \li <tt>type_error(atom_or_string, \em Name)</tt> - \em Name
 *     is not an atom or string.
 * \li <tt>existence_error(import, \em Name)</tt> - \em Name
 *     could not be located on the import search path.
 *
 * \par Examples
 * \code
 * :- import(stdout).
 * :- import("stdout.lp").
 * :- import(X).                instantiation_error
 * :- import(1.5).              type_error(atom_or_string, 1.5)
 * :- import("not_found.lp").   existence_error(import, "not_found.lp")
 * :- import("../dir/file.lp").
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog" has directives called
 * <b>ensure_loaded/1</b> and <b>include/1</b> that perform a
 * similar function to <b>import/1</b>.  Those Standard Prolog
 * directives are not supported by Plang.
 * \par
 * The modules extension to Standard Prolog does have an
 * <b>import/1</b> directive.  Plang's version is not compatible.
 *
 * \par See Also
 * \ref directive_1 "(:-)/1",
 * \ref consult_1 "consult/1",
 * \ref load_library_1 "load_library/1"
 */
static p_goal_result p_builtin_import
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
    int result;
    if (!name || (name->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    } else if (name->header.type != P_TERM_ATOM &&
               name->header.type != P_TERM_STRING) {
        *error = p_create_type_error(context, "atom_or_string", name);
        return P_RESULT_ERROR;
    }
    result = p_context_builtin_import(context, p_term_name(name));
    if (!result) {
        return P_RESULT_FAIL;
    } else if (result < 0) {
        *error = p_create_existence_error(context, "import", name);
        return P_RESULT_ERROR;
    } else {
        return P_RESULT_TRUE;
    }
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

/**
 * \addtogroup directives
 * <hr>
 * \anchor load_library_1
 * <b>load_library/1</b> - loads a native C library that implements
 * Plang predicates.
 *
 * \par Usage
 * <b>:-</b> \b load_library(\em Name).
 *
 * \par Description
 * The \em Name must be an atom or string.  The \em Name is used
 * as a base name to search for a .so or .dll library.  If found,
 * the library is loaded and any native predicates implemented
 * in C are registered with the Plang execution engine.
 * \par
 * If the library associated with \em Name can be loaded,
 * then \b load_library(\em Name) succeeds.  Otherwise an
 * error is thrown describing the reason why the library
 * could not be loaded.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Name is a variable.
 * \li <tt>type_error(atom_or_string, \em Name)</tt> - \em Name
 *     is not an atom or string.
 * \li <tt>type_error(library_name, \em Name)</tt> - \em Name is
 *     not a valid library name.
 * \li <tt>existence_error(library, \em Name)</tt> - \em Name
 *     could not be located on the library search path.
 * \li <tt>load_library_error(\em Name, \em Reason)</tt> - the library
 *     identified by \em Name exists, but it could not be loaded by
 *     the system because of \em Reason (a string).
 *
 * \par Examples
 * \code
 * :- load_library(plang_wordnet).
 * \endcode
 *
 * \par See Also
 * \ref directive_1 "(:-)/1",
 * \ref import_1 "import/1"
 */
static p_goal_result p_builtin_load_library
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
    if (!name || (name->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (name->header.type != P_TERM_ATOM &&
            name->header.type != P_TERM_STRING) {
        *error = p_create_type_error(context, "atom_or_string", name);
        return P_RESULT_ERROR;
    }
    return _p_context_load_library(context, name, error);
}

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
 * \ref halt_0 "halt/0",
 * \ref halt_1 "halt/1",
 * \ref if_stmt "(->)/2",
 * \ref if_stmt "if",
 * \ref in_2 "in/2",
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
 * \par
 * This predicate is equivalent to \ref fuzzy_and_2 "fuzzy_and/2"
 * when reasoning about terms that involve
 * \ref fuzzy_logic "fuzzy logic".
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
 * \ref not_provable_1 "(!)/1",
 * \ref fuzzy_and_2 "fuzzy_and/2"
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
 * \par
 * If \em Goal1 or \em Goal2 involves terms that use
 * \ref fuzzy_logic "fuzzy logic", then the fuzzy confidence
 * value of \em Goal1 <b>||</b> \em Goal2 is the same as
 * \em Goal1 while it succeeds, and then the same as
 * \em Goal2 once \em Goal1 fails.  Use the
 * \ref fuzzy_or_2 "fuzzy_or/2" operator instead for
 * calculating the logical fuzzy OR of \em Goal1 and \em Goal2.
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
 * \ref fuzzy_or_2 "fuzzy_or/2",
 * \ref if_stmt "if"
 */
static p_goal_result p_builtin_logical_or
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
    p_exec_node *current;
    p_exec_fail_node *retry;
    p_exec_node *cut;
    p_exec_node *then;
    p_exec_node *if_node;
    if (term->header.type == P_TERM_FUNCTOR &&
            term->header.size == 2 &&
            term->functor.functor_name == context->if_atom) {
        /* The term has the form (A -> B || C) */
        current = context->current_node;
        if_node = GC_NEW(p_exec_node);
        retry = GC_NEW(p_exec_fail_node);
        cut = GC_NEW(p_exec_node);
        then = GC_NEW(p_exec_node);
        if (!if_node || !retry || !cut || !then)
            return P_RESULT_FAIL;
        retry->parent.goal = args[1];
        retry->parent.success_node = current->success_node;
        retry->parent.cut_node = context->fail_node;
        _p_context_init_fail_node
            (context, retry, _p_context_basic_fail_func);
        cut->goal = context->cut_atom;
        cut->success_node = then;
        cut->cut_node = context->fail_node;
        then->goal = p_term_arg(term, 1);
        then->success_node = current->success_node;
        then->cut_node = context->fail_node;
        if_node->goal = p_term_arg(term, 0);
        if_node->success_node = cut;
        if_node->cut_node = context->fail_node;
        context->current_node = if_node;
        context->fail_node = retry;
        return P_RESULT_TREE_CHANGE;
    } else {
        /* Regular disjunction */
        current = context->current_node;
        if_node = GC_NEW(p_exec_node);
        retry = GC_NEW(p_exec_fail_node);
        if (!if_node || !retry)
            return P_RESULT_FAIL;
        retry->parent.goal = args[1];
        retry->parent.success_node = current->success_node;
        retry->parent.cut_node = context->fail_node;
        _p_context_init_fail_node
            (context, retry, _p_context_basic_fail_func);
        if_node->goal = term;
        if_node->success_node = current->success_node;
        if_node->cut_node = context->fail_node;
        context->current_node = if_node;
        context->fail_node = retry;
        return P_RESULT_TREE_CHANGE;
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
    p_exec_node *current = context->current_node;
    p_exec_node *new_current = GC_NEW(p_exec_node);
    if (!new_current)
        return P_RESULT_FAIL;
    new_current->goal = args[0];
    new_current->success_node = current->success_node;
    new_current->cut_node = context->fail_node;
    context->current_node = new_current;
    return P_RESULT_TREE_CHANGE;
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
 * \ref call_1 "call/1",
 * \ref syntax_try_stmt "Formal syntax of try statements"
 */
int p_builtin_handle_catch(p_context *context, p_term *error)
{
    p_exec_catch_node *catcher = context->catch_node;
    p_term *catch_atom = p_term_create_atom(context, "catch");
    p_term *catch_clause_atom = p_term_create_atom(context, "$$catch");
    p_term *goal;
    while (catcher != 0) {
        _p_context_basic_fail_func(context, &(catcher->parent));
        goal = p_term_deref_member(context, catcher->parent.parent.goal);
        if (goal->functor.functor_name == catch_atom) {
            /* "catch" block */
            if (p_term_unify(context, error,
                             goal->functor.arg[1], P_BIND_DEFAULT)) {
                catcher->parent.parent.goal = goal->functor.arg[2];
                context->current_node = &(catcher->parent.parent);
                context->fail_node = catcher->parent.parent.cut_node;
                context->catch_node = catcher->catch_parent;
                return 1;
            }
        } else {
            /* "try" block */
            p_term *list = p_term_deref_member(context, goal->functor.arg[1]);
            while (list && list->header.type == P_TERM_LIST) {
                p_term *head = p_term_deref_member
                    (context, p_term_head(list));
                if (head->header.type == P_TERM_FUNCTOR &&
                        head->header.size == 2 &&
                        head->functor.functor_name
                                == catch_clause_atom) {
                    if (p_term_unify(context, p_term_arg(head, 0),
                                     error, P_BIND_DEFAULT)) {
                        catcher->parent.parent.goal = p_term_arg(head, 1);
                        context->current_node = &(catcher->parent.parent);
                        context->fail_node = catcher->parent.parent.cut_node;
                        context->catch_node = catcher->catch_parent;
                        return 1;
                    }
                }
                list = p_term_deref_member(context, p_term_tail(list));
            }
        }
        catcher = catcher->catch_parent;
    }
    context->current_node = 0;
    context->fail_node = 0;
    return 0;
}
static p_goal_result p_builtin_catch
    (p_context *context, p_term **args, p_term **error)
{
    p_exec_node *current = context->current_node;
    p_exec_catch_node *catcher = GC_NEW(p_exec_catch_node);
    p_exec_node *new_current = GC_NEW(p_exec_node);
    if (!catcher || !new_current)
        return P_RESULT_FAIL;
    catcher->parent.parent.goal = current->goal;
    catcher->parent.parent.success_node = current->success_node;
    catcher->parent.parent.cut_node = context->fail_node;
    catcher->catch_parent = context->catch_node;
    _p_context_init_fail_node
        (context, &(catcher->parent), _p_context_basic_fail_func);
    new_current->goal = args[0];
    new_current->success_node = current->success_node;
    new_current->cut_node = context->fail_node;
    context->current_node = new_current;
    context->catch_node = catcher;
    return P_RESULT_TREE_CHANGE;
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
    context->fail_node = context->current_node->cut_node;
    return P_RESULT_TRUE;
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
 * \ref while_stmt "while",
 * \ref syntax_do_stmt "Formal syntax of do statements"
 */
static char const p_builtin_do[] =
    "'$$do'(Vars, Body, Cond)\n"
    "{\n"
    "    '$$unbind'(Vars);\n"
    "    call(Body);\n"
    "    !;\n"
    "    if (call(Cond))\n"
    "        '$$do'(Vars, Body, Cond);\n"
    "}\n"
    "'$$do'(Body, Cond)\n"
    "{\n"
    "    call(Body);\n"
    "    !;\n"
    "    if (call(Cond))\n"
    "        '$$do'(Body, Cond);\n"
    "}\n";

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
 * \par
 * Note: <b>(!)/1</b> does not take the current fuzzy confidence
 * value into account, so it is not suitable for use in applications
 * that involve \ref fuzzy_logic "fuzzy logic".  Use the
 * \ref fuzzy_not_1 "fuzzy_not/1" predicate instead.
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
 * \ref logical_or_2 "(||)/2",
 * \ref fuzzy_not_1 "fuzzy_not/1"
 */
static char const p_builtin_not_provable[] =
    "'!'(Goal)\n"
    "{\n"
    "    if (call(Goal))\n"
    "        fail;\n"
    "}\n"
    "'\\\\+'(Goal)\n"
    "{\n"
    "    if (call(Goal))\n"
    "        fail;\n"
    "}\n";

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
 * \ref in_2 "in/2",
 * \ref while_stmt "while",
 * \ref syntax_for_stmt "Formal syntax of for statements"
 */
static p_goal_result p_builtin_set_loop_var
    (p_context *context, p_term **args, p_term **error)
{
    /* We expect the first argument to have the form $$loopvar(X) */
    p_builtin_set_variable(p_term_arg(args[0], 0), args[1]);
    return P_RESULT_TRUE;
}
static char const p_builtin_for[] =
    "'$$for'(Vars, LoopVar, List, Body)\n"
    "{\n"
    "    var(List);\n"
    "    !;\n"
    "    throw(error(instantiation_error, 'for'/2));\n"
    "}\n"
    "'$$for'(Vars, LoopVar, [], Body)\n"
    "{\n"
    "    !;\n"
    "}\n"
    "'$$for'(Vars, LoopVar, [H|T], Body)\n"
    "{\n"
    "    !;\n"
    "    '$$unbind'(Vars);\n"
    "    '$$set_loop_var'(LoopVar, H);\n"
    "    call(Body);\n"
    "    !;\n"
    "    '$$for'(Vars, LoopVar, T, Body);\n"
    "}\n"
    "'$$for'(Vars, LoopVar, List, Body)\n"
    "{\n"
    "    throw(error(type_error(list, List), 'for'/2));\n"
    "}\n";

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor halt_0
 * <b>halt/0</b> - stops execution of the top-level goal.
 *
 * \par Usage
 * \b halt
 *
 * \par Description
 * Exits from execution of the top-level goal, returning control
 * back to the system with an exit value of 0.
 * \par
 * It is not possible to trap the halt state with
 * \ref catch_3 "catch/3".
 *
 * \par Examples
 * \code
 * halt
 * catch(halt, X, stdout::writeln('not reached'))
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref halt_1 "halt/1"
 */
static p_goal_result p_builtin_halt_0
    (p_context *context, p_term **args, p_term **error)
{
    *error = p_term_create_integer(context, 0);
    return P_RESULT_HALT;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor halt_1
 * <b>halt/1</b> - stops execution of the top-level goal with a
 * specific exit value.
 *
 * \par Usage
 * \b halt(\em ExitValue)
 *
 * \par Description
 * Exits from execution of the top-level goal, returning control
 * back to the system with the specified integer \em ExitValue.
 * The \em ExitValue may be clamped to a system-specific range
 * to make it suitable for passing back to the host operating system.
 * \par
 * It is not possible to trap the halt state with
 * \ref catch_3 "catch/3" if \em ExitValue is a valid integer.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em ExitValue is a variable.
 * \li <tt>type_error(integer, \em ExitValue)</tt> - \em ExitValue
 *     is not an integer.
 *
 * \par Examples
 * \code
 * halt(3)
 * catch(halt(3), X, stdout::writeln('not reached'))
 * halt(X)              instantiation_error
 * halt(1.0)            type_error(integer, 1.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref halt_0 "halt/0"
 */
static p_goal_result p_builtin_halt_1
    (p_context *context, p_term **args, p_term **error)
{
    p_term *exitval = p_term_deref_member(context, args[0]);
    if (!exitval || (exitval->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (exitval->header.type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", exitval);
        return P_RESULT_ERROR;
    }
    *error = exitval;
    return P_RESULT_HALT;
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
 * \em Goal1 is executed, and if it succeeds then \em Goal2 is
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
 * \ref switch_stmt "switch",
 * \ref syntax_if_stmt "Formal syntax of if statements"
 */
static p_goal_result p_builtin_if
    (p_context *context, p_term **args, p_term **error)
{
    p_exec_node *current = context->current_node;
    p_exec_node *cut = GC_NEW(p_exec_node);
    p_exec_node *then = GC_NEW(p_exec_node);
    p_exec_node *if_node = GC_NEW(p_exec_node);
    if (!cut || !then || !if_node)
        return P_RESULT_FAIL;
    cut->goal = context->cut_atom;
    cut->success_node = then;
    cut->cut_node = context->fail_node;
    then->goal = args[1];
    then->success_node = current->success_node;
    then->cut_node = context->fail_node;
    if_node->goal = args[0];
    if_node->success_node = cut;
    if_node->cut_node = context->fail_node;
    context->current_node = if_node;
    return P_RESULT_TREE_CHANGE;
}

/**
 * \addtogroup logic_and_control
 * <hr>
 * \anchor in_2
 * <b>in/2</b> - list membership testing.
 *
 * \par Usage
 * \em Term \b in \em List
 *
 * \par Description
 * \em Term \b in \em List succeeds multiple times whenever \em Term
 * unifies with an element of \em List.  Fails at the end of the
 * \em List, if \em Term does not unify with any of the elements,
 * or the tail of \em List is not a list or <tt>[]</tt>.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List or the tail of
 *     \em List is a variable.  This prevents an infinite loop
 *     if \em List is partial.
 *
 * \par Examples
 * \code
 * X in [a, b, c]       succeeds 3 times for X = a/b/c, then fails
 * f(X) in [a]          fails
 * X in Y               instantiation_error
 * X in [a|Y]           succeeds with X = a, then instantiation_error
 * \endcode
 * \par
 * Note: if the <b>in/2</b> predicate is used within a condition
 * for a \ref do_stmt "do", \ref if_stmt "if", or
 * \ref while_stmt "while" statement, then it will succeed only
 * once for the first match.  This is because the statement conditions
 * perform a cut, \ref cut_0 "(!)/0", after success is detected.
 * This is useful for detecting simple list membership only:
 * \code
 * if (f(a) in List) {
 *     ...
 * }
 * \endcode
 * The recommended procedural loop construct for lists is
 * \ref for_stmt "for":
 * \code
 * for (X in [a, b, c])
 *     stdout::writeln(X);
 * \endcode
 *
 * \par See Also
 * \ref for_stmt "for"
 */
static char const p_builtin_in[] =
    "'in'(Term, List)\n"
    "{\n"
    "    var(List);\n"
    "    !;\n"
    "    throw(error(instantiation_error, 'in'/2));\n"
    "}\n"
    "'in'(Term, [Term|Tail]).\n"
    "'in'(Term, [Head|Tail])\n"
    "{\n"
    "    'in'(Term, Tail);\n"
    "}\n";

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
 * \ref if_stmt "if",
 * \ref syntax_switch_stmt "Formal syntax of switch statements"
 */
static char const p_builtin_switch[] =
    "'$$switch'(Value, [], Default)\n"
    "{\n"
    "    !;\n"
    "    call(Default);\n"
    "}\n"
    "'$$switch'(Value, ['$$case'(Cases, Body)|Tail], Default)\n"
    "{\n"
    "    '$$switch_case_match'(Value, Cases);\n"
    "    !;\n"
    "    call(Body);\n"
    "}\n"
    "'$$switch'(Value, [Head|Tail], Default)\n"
    "{\n"
    "    '$$switch'(Value, Tail, Default);\n"
    "}\n"
    "'$$switch_case_match'(Value, [])\n"
    "{\n"
    "    !;\n"
    "    fail;\n"
    "}\n"
    "'$$switch_case_match'(Value, [Value|Tail])\n"
    "{\n"
    "    !;\n"
    "}\n"
    "'$$switch_case_match'(Value, [Head|Tail])\n"
    "{\n"
    "    '$$switch_case_match'(Value, Tail);\n"
    "}\n";

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
 * \ref for_stmt "for",
 * \ref syntax_while_stmt "Formal syntax of while statements"
 */
static char const p_builtin_while[] =
    "'$$while'(Vars, Cond, Body)\n"
    "{\n"
    "    '$$unbind'(Vars);\n"
    "    if (call(Cond)) {\n"
    "        call(Body);\n"
    "        !;\n"
    "        '$$while'(Vars, Cond, Body);\n"
    "    }\n"
    "}\n"
    "'$$while'(Cond, Body)\n"
    "{\n"
    "    if (call(Cond)) {\n"
    "        call(Body);\n"
    "        !;\n"
    "        '$$while'(Cond, Body);\n"
    "    }\n"
    "}\n";

/*\@}*/

/**
 * \defgroup sorting Builtin predicates - Sorting
 */
/*\@{*/
/* Defined in sort.c */
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
 * atoms, which precede all compound terms, which precede all objects,
 * which precede all predicates.
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
    if (p_term_precedes(context, args[0], args[1]) < 0)
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
    if (p_term_precedes(context, args[0], args[1]) <= 0)
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
    if (p_term_precedes(context, args[0], args[1]) > 0)
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
    if (p_term_precedes(context, args[0], args[1]) >= 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/*\@}*/

/**
 * \defgroup create_and_decompose Builtin predicates - Term creation and decomposition
 *
 * Predicates in this group create and decompose terms dynamically.
 *
 * \ref list_cons_2 "(.)/2",
 * \ref univ_2 "(=..)/2",
 * \ref arg_3 "arg/3",
 * \ref copy_term_2 "copy_term/2",
 * \ref functor_3 "functor/3"
 */
/*\@{*/

/**
 * \addtogroup create_and_decompose
 * <hr>
 * \anchor list_cons_2
 * <b>(.)/2</b>, <b>[_|_]/2</b> - constructs a list from head and
 * tail terms.
 *
 * \par Usage
 * \em X = [\em Head | \em Tail]
 * \par
 * \em X = <b>'.'</b>(\em Head, \em Tail)
 *
 * \par Description
 * Unifies \em X with a list constructed from the specified
 * \em Head and \em Tail terms.  The [\em Head | \em Tail] form
 * is the recommended syntax for expressing list construction.
 *
 * \par Examples
 * \code
 * [H | T] = [a, b]             succeeds with H = a, T = [b]
 * [a, b] = '.'(a, '.'(b, []))  succeeds
 * \endcode
 * \par
 * A common use for <b>(.)/2</b> is to decompose a predicate argument
 * into its head and tail components for walking every element
 * in a list:
 * \code
 * is_member(X, [X|T]).
 * is_member(X, [_|T]) { is_member(X, T); }
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref arg_3 "arg/3",
 * \ref functor_3 "functor/3",
 * \ref univ_2 "(=..)/2"
 */

/**
 * \addtogroup create_and_decompose
 * <hr>
 * \anchor univ_2
 * <b>(=..)/2</b> - decomposes a term into a list containing the
 * term's functor name and arguments.
 *
 * \par Usage
 * \em Term <b>=..</b> \em List
 *
 * \par Description
 * If \em Term is an atomic term, then \em List is unified with
 * [\em Term].
 * \par
 * If \em Term is a compound term, then \em List is unified with
 * [\em Name, \em Arg1, ..., \em ArgN], where \em Name is the
 * name of the functor of \em Term, and \em Arg1, ... \em ArgN
 * are its arguments.
 * \par
 * If \em Term is a variable, and \em List contains a single atomic
 * value then \em Term is unified with that atomic value.
 * \par
 * If \em Term is a variable, and \em List contains two or more
 * members, and the first is an atom, then \em Term is unified
 * with a new compound term with the first element as its name,
 * and the remaining list members as its arguments.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Term is a variable
 *     and \em List is not a list, or the tail of \em List is
 *     not the <tt>[]</tt> atom.
 * \li <tt>instantiation_error</tt> - \em Term is a variable and
 *     the first member of \em List is also a variable.
 * \li <tt>domain_error(non_empty_list, \em List)</tt> - \em Term
 *     is a variable and \em List is an empty list.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Term is a variable,
 *     \em List has two or more members, and the first member is
 *     not an atom.
 * \li <tt>type_error(atomic, \em Name)</tt> - \em Term is a variable,
 *     \em List has one member, and it is not atomic.
 * \li <tt>type_error(list, \em List)</tt> - \em Term is not a variable
 *     and \em List is not a list or variable.
 *
 * \par Examples
 * \code
 * foo =.. [foo]                    succeeds
 * 1.5 =.. [1.5]                    succeeds
 * [a, b, c] =.. ['.', a, [b, c]]   succeeds
 * f(a, b, c) =.. [f, X, Y, Z]      succeeds with X = a, Y = b, Z = c
 * f(a, b) =.. List                 succeeds with List = [f, a, b]
 * Term =.. [foo]                   succeeds with Term = foo
 * Term =.. [1.5]                   succeeds with Term = 1.5
 * Term =.. [f, a, b]               succeeds with Term = f(a, b)
 * Term =.. ['.', a, []]            succeeds with Term = [a]
 * Term =.. List                    instantiation_error
 * Term =.. f(a, b)                 instantiation_error
 * Term =.. [f|X]                   instantiation_error
 * Term =.. []                      domain_error(non_empty_list, [])
 * Term =.. [f(a, b)]               type_error(atomic, f(a, b))
 * Term =.. [1.5, a, b]             type_error(atom, 1.5)
 * f(a, b) =.. g(a)                 type_error(list, g(a))
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref list_cons_2 "(.)/2",
 * \ref arg_3 "arg/3",
 * \ref functor_3 "functor/3"
 */
static p_goal_result p_builtin_univ
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
    p_term *list = p_term_deref_member(context, args[1]);
    p_term *new_term;
    p_term *functor;
    p_term *member;
    p_term *list_args;
    int index, length;
    if ((term->header.type & P_TERM_VARIABLE) == 0) {
        if (list->header.type != P_TERM_VARIABLE &&
                list->header.type != P_TERM_LIST) {
            *error = p_create_type_error(context, "list", list);
            return P_RESULT_ERROR;
        }
        switch (term->header.type) {
        case P_TERM_ATOM:
        case P_TERM_INTEGER:
        case P_TERM_REAL:
        case P_TERM_STRING:
        case P_TERM_OBJECT:
        case P_TERM_PREDICATE:
            new_term = p_term_create_list
                (context, term, context->nil_atom);
            break;
        case P_TERM_FUNCTOR:
            new_term = context->nil_atom;
            for (index = (int)(term->header.size - 1); index >= 0; --index) {
                new_term = p_term_create_list
                    (context, term->functor.arg[index], new_term);
            }
            new_term = p_term_create_list
                (context, term->functor.functor_name, new_term);
            break;
        case P_TERM_LIST:
            new_term = p_term_create_list
                (context, context->dot_atom,
                    p_term_create_list
                        (context, term->list.head,
                         p_term_create_list
                            (context, term->list.tail,
                             context->nil_atom)));
            break;
        default: return P_RESULT_FAIL;
        }
        if (p_term_unify(context, list, new_term, P_BIND_DEFAULT))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    } else {
        if (list == context->nil_atom) {
            *error = p_create_domain_error
                (context, "non_empty_list", list);
            return P_RESULT_ERROR;
        }
        if (list->header.type != P_TERM_LIST) {
            *error = p_create_instantiation_error(context);
            return P_RESULT_ERROR;
        }
        length = 1;
        member = p_term_deref_member(context, list->list.tail);
        while (member != context->nil_atom) {
            if (!member || member->header.type != P_TERM_LIST) {
                *error = p_create_instantiation_error(context);
                return P_RESULT_ERROR;
            }
            ++length;
            member = p_term_deref_member(context, member->list.tail);
        }
        functor = p_term_deref_member(context, list->list.head);
        if ((functor->header.type & P_TERM_VARIABLE) != 0) {
            *error = p_create_instantiation_error(context);
            return P_RESULT_ERROR;
        }
        list_args = p_term_deref_member(context, list->list.tail);
        if (length == 1) {
            switch (functor->header.type) {
            case P_TERM_ATOM:
            case P_TERM_INTEGER:
            case P_TERM_REAL:
            case P_TERM_STRING:
            case P_TERM_OBJECT:
            case P_TERM_PREDICATE:
                new_term = functor;
                break;
            default:
                *error = p_create_type_error
                    (context, "atomic", functor);
                return P_RESULT_ERROR;
            }
        } else if (functor == context->dot_atom && length == 3) {
            new_term = p_term_create_list
                (context, list_args->list.head,
                 p_term_deref_member
                    (context, list_args->list.tail)->list.head);
        } else if (functor->header.type != P_TERM_ATOM) {
            *error = p_create_type_error(context, "atom", functor);
            return P_RESULT_ERROR;
        } else {
            new_term = p_term_create_functor
                (context, functor, length - 1);
            for (index = 0; index < (length - 1); ++index) {
                p_term_bind_functor_arg
                    (new_term, index, list_args->list.head);
                list_args = p_term_deref_member
                    (context, list_args->list.tail);
            }
        }
        if (p_term_unify(context, term, new_term, P_BIND_DEFAULT))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    }
}

/**
 * \addtogroup create_and_decompose
 * <hr>
 * \anchor arg_3
 * <b>arg/3</b> - extracts the n'th argument from a term.
 *
 * \par Usage
 * \b arg(\em N, \em Term, \em Arg)
 *
 * \par Description
 * Unifies \em Arg with argument \em N of the compound \em Term.
 * The first argument of \em Term is numbered 1.  Fails if \em N
 * is out of range or \em Arg does not unify with the extracted
 * argument.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em N or \em Term is a variable.
 * \li <tt>type_error(integer, \em N)</tt> - \em N is not an integer.
 * \li <tt>domain_error(not_less_than_zero, \em N)</tt> - \em N is
 *     an integer that is less than zero.
 * \li <tt>type_error(compound, \em Term)</tt> - \em Term is not a
 *     compound term (functor or list).
 *
 * \par Examples
 * \code
 * arg(1, foo(a, b), X)         succeeds with X = a
 * arg(2, foo(a, b), X)         succeeds with X = b
 * arg(3, foo(a, b), X)         fails
 * arg(0, foo(a, b), X)         fails
 * arg(1, [a, b], X)            succeeds with X = a
 * arg(2, [a, b], X)            succeeds with X = [b]
 * arg(1, foo(a, b), a)         succeeds
 * arg(1, foo(a, b), b)         fails
 * arg(1, foo(X, b), f(X))      fails due to occurs check
 * arg(N, foo(a, b), X)         instantiation_error
 * arg(1, Term, X)              instantiation_error
 * arg(a, [a, b], X)            type_error(integer, a)
 * arg(-3, [a, b], X)           domain_error(not_less_than_zero, -3)
 * arg(1, a, X)                 type_error(compound, a)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref list_cons_2 "(.)/2",
 * \ref univ_2 "(=..)/2",
 * \ref functor_3 "functor/3"
 */
static p_goal_result p_builtin_arg
    (p_context *context, p_term **args, p_term **error)
{
    p_term *number = p_term_deref_member(context, args[0]);
    p_term *term = p_term_deref_member(context, args[1]);
    p_term *arg;
    int num;
    if (!number || (number->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (!term || (term->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (number->header.type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", number);
        return P_RESULT_ERROR;
    }
    num = p_term_integer_value(number);
    if (num < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", number);
        return P_RESULT_ERROR;
    }
    if (term->header.type == P_TERM_FUNCTOR) {
        if (num > 0 && num <= (int)(term->header.size))
            arg = term->functor.arg[num - 1];
        else
            return P_RESULT_FAIL;
    } else if (term->header.type == P_TERM_LIST) {
        if (num == 1)
            arg = term->list.head;
        else if (num == 2)
            arg = term->list.tail;
        else
            return P_RESULT_FAIL;
    } else {
        *error = p_create_type_error(context, "compound", term);
        return P_RESULT_ERROR;
    }
    if (p_term_unify(context, args[2], arg, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup create_and_decompose
 * <hr>
 * \anchor copy_term_2
 * <b>copy_term/2</b> - unifies the second argument with a freshly
 * renamed copy of the first.
 *
 * \par Usage
 * \b copy_term(\em Term1, \em Term2)
 *
 * \par Description
 * Creates a copy of \em Term1 where all variables have been
 * replaced with freshly renamed variables, and then unifies
 * the copy with \em Term2.
 *
 * \par Examples
 * In the following examples X and Y are renamed to A and B
 * respectively:
 * \code
 * copy_term(f(X, Y), Z)        succeeds with Z = f(A, B)
 * copy_term(X, a)              succeeds with A = a, X still unbound
 * copy_term(f(a, X), f(X, b))  succeeds with X = a
 * copy_term(f(X, X), f(Y, Z))  succeeds with Y = Z
 * copy_term(foo, bar)          fails
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 */
static p_goal_result p_builtin_copy_term
    (p_context *context, p_term **args, p_term **error)
{
    p_term *renamed = p_term_clone(context, args[0]);
    if (p_term_unify(context, renamed, args[1], P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup create_and_decompose
 * <hr>
 * \anchor functor_3
 * <b>functor/3</b> - extracts the name and arity of a functor term.
 *
 * \par Usage
 * \b functor(\em Term, \em Name, \em Arity)
 *
 * \par Description
 * If \em Term is a compound functor term, then \em Name is unified
 * with the name of the functor, and \em Arity with its arity.
 * \par
 * If \em Term is a list, then \em Name is unified with the atom
 * "." and \em Arity is unified with 2.
 * \par
 * If \em Term is an atomic term, then \em Name is unified with
 * \em Term, and \em Arity is unified with 0.
 * \par
 * If \em Term is a variable, \em Name is an atomic term,
 * and \em Arity is zero, then \em Term is unified with \em Name.
 * \par
 * If \em Term is a variable and \em Name and \em Arity are not
 * variables, then \em Term is bound to a new compound term
 * of type \em Name / \em Arity, with new free variables as
 * the arguments.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Term and \em Name are
 *     both variables.
 * \li <tt>instantiation_error</tt> - \em Term and \em Arity are
 *     both variables.
 * \li <tt>type_error(atomic, \em Name)</tt> - \em Name is not
 *     an atomic term and \em Term is a variable.
 * \li <tt>type_error(integer, \em Arity)</tt> - \em Arity is not
 *     an integer and \em Term is a variable.
 * \li <tt>domain_error(not_less_than_zero, \em Arity)</tt> - \em Arity
 *     is an integer that is less than zero and \em Term is a variable.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Name is not
 *     an atom, \em Arity is not zero, and \em Term is a variable.
 *
 * \par Examples
 * \code
 * functor(a, Name, Arity)          succeeds with Name = a, Arity = 0
 * functor(1.5, Name, Arity)        succeeds with Name = 1.5, Arity = 0
 * functor(f(a, b), Name, Arity)    succeeds with Name = f, Arity = 2
 * functor([H|T], Name, Arity)      succeeds with Name = '.', Arity = 2
 * functor(Term, a, 0)              succeeds with Term = a
 * functor(Term, 1.5, 0)            succeeds with Term = 1.5
 * functor(Term, f, 2)              succeeds with Term = f(X, Y)
 * functor(Term, '.', 2)            succeeds with Term = [X|Y]
 * functor(Term, Name, 2)           instantiation_error
 * functor(Term, f, Arity)          instantiation_error
 * functor(Term, f(a), 1)           type_error(atomic, f(a))
 * functor(Term, f, 1.5)            type_error(integer, 1.5)
 * functor(Term, f, -1)             domain_error(not_less_than_zero, -1)
 * functor(Term, 1.5, 1)            type_error(atom, 1.5)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref list_cons_2 "(.)/2",
 * \ref arg_3 "arg/3",
 * \ref univ_2 "(=..)/2"
 */
static p_goal_result p_builtin_functor
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
    p_term *name = p_term_deref_member(context, args[1]);
    p_term *arity = p_term_deref_member(context, args[2]);
    p_term *new_term;
    int arity_value, index;
    if (!term || !name || !arity) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if ((term->header.type & P_TERM_VARIABLE) == 0) {
        /* Existing term to be split into name and arity */
        switch (term->header.type) {
        case P_TERM_ATOM:
        case P_TERM_INTEGER:
        case P_TERM_REAL:
        case P_TERM_STRING:
        case P_TERM_OBJECT:
        case P_TERM_PREDICATE:
            if (!p_term_unify(context, name, term, P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            if (!p_term_unify(context, arity,
                              p_term_create_integer(context, 0),
                              P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            return P_RESULT_TRUE;
        case P_TERM_FUNCTOR:
            if (!p_term_unify(context, name,
                              term->functor.functor_name,
                              P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            if (!p_term_unify(context, arity,
                              p_term_create_integer
                                (context, (int)(term->header.size)),
                              P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            return P_RESULT_TRUE;
        case P_TERM_LIST:
            if (!p_term_unify(context, name, context->dot_atom,
                              P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            if (!p_term_unify(context, arity,
                              p_term_create_integer(context, 2),
                              P_BIND_DEFAULT))
                return P_RESULT_FAIL;
            return P_RESULT_TRUE;
        default: break;
        }
        return P_RESULT_FAIL;
    } else {
        /* Construct a term from name and arity */
        if ((name->header.type & P_TERM_VARIABLE) != 0 ||
                (arity->header.type & P_TERM_VARIABLE) != 0) {
            *error = p_create_instantiation_error(context);
            return P_RESULT_ERROR;
        }
        switch (name->header.type) {
        case P_TERM_ATOM:
        case P_TERM_INTEGER:
        case P_TERM_REAL:
        case P_TERM_STRING:
        case P_TERM_OBJECT:
        case P_TERM_PREDICATE:
            break;
        default:
            *error = p_create_type_error(context, "atomic", name);
            return P_RESULT_ERROR;
        }
        if (arity->header.type != P_TERM_INTEGER) {
            *error = p_create_type_error(context, "integer", arity);
            return P_RESULT_ERROR;
        }
        arity_value = p_term_integer_value(arity);
        if (arity_value < 0) {
            *error = p_create_domain_error
                (context, "not_less_than_zero", arity);
            return P_RESULT_ERROR;
        }
        if (arity_value == 0) {
            new_term = name;
        } else if (name->header.type != P_TERM_ATOM) {
            *error = p_create_type_error(context, "atom", name);
            return P_RESULT_ERROR;
        } else if (name == context->dot_atom && arity_value == 2) {
            new_term = p_term_create_list
                (context, p_term_create_variable(context),
                 p_term_create_variable(context));
        } else {
            new_term = p_term_create_functor
                (context, name, arity_value);
            for (index = 0; index < arity_value; ++index) {
                p_term_bind_functor_arg
                    (new_term, index, p_term_create_variable(context));
            }
        }
        if (!p_term_unify(context, term, new_term, P_BIND_DEFAULT))
            return P_RESULT_FAIL;
        return P_RESULT_TRUE;
    }
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
 * \ref unify_one_way_2 "unify_one_way/2"
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
 * \ref unifiable_2 "unifiable/2",
 * \ref unify_one_way_2 "unify_one_way/2",
 * \ref assign_2 "(:=)/2"
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
 * \ref unifiable_2 "unifiable/2",
 * \ref unify_one_way_2 "unify_one_way/2"
 */
static p_goal_result p_builtin_not_unifiable
    (p_context *context, p_term **args, p_term **error)
{
    void *marker = p_context_mark_trail(context);
    if (p_term_unify(context, args[0], args[1], P_BIND_DEFAULT)) {
        p_context_backtrack_trail(context, marker);
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
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unify_one_way_2 "unify_one_way/2"
 */
static p_goal_result p_builtin_unifiable
    (p_context *context, p_term **args, p_term **error)
{
    void *marker = p_context_mark_trail(context);
    if (p_term_unify(context, args[0], args[1], P_BIND_DEFAULT)) {
        p_context_backtrack_trail(context, marker);
        return P_RESULT_TRUE;
    } else {
        return P_RESULT_FAIL;
    }
}

/**
 * \addtogroup unification
 * <hr>
 * \anchor unify_one_way_2
 * <b>unify_one_way/2</b> - unifies two terms, but only bind variables
 * in the first term.
 *
 * \par Usage
 * \b unify_one_way(\em Term1, \em Term2)
 *
 * \par Description
 * If \em Term1 and \em Term2 can be unified without modifying
 * \em Term2, then succeed.  Fails otherwise.
 *
 * \par Examples
 * \code
 * unify_one_way(X, Y + Z)      succeeds, binding X to Y + Z
 * unify_one_way(Y + Z, X)      fails, would cause X to be modified
 * unify_one_way(X, Y)          succeeds, binding X to Y
 * \endcode
 *
 * \par See Also
 * \ref unify_2 "(=)/2",
 * \ref not_unifiable_2 "(!=)/2",
 * \ref unifiable_2 "unifiable/2",
 * \ref syntax_input_only "Input-only arguments"
 */
static p_goal_result p_builtin_unify_one_way
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_unify(context, args[0], args[1], P_BIND_ONE_WAY))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
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
 * \ref class_1 "class/1",
 * \ref class_2 "class/2",
 * \ref compound_1 "compound/1",
 * \ref float_1 "float/1",
 * \ref integer_1 "integer/1",
 * \ref nonvar_1 "nonvar/1",
 * \ref number_1 "number/1",
 * \ref object_1 "object/1",
 * \ref object_2 "object/2",
 * \ref predicate_1 "predicate/1",
 * \ref predicate_2 "predicate/2",
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
 * \anchor class_1
 * \b class/1 - tests if a term is a class object or name.
 *
 * \par Usage
 * \b class(\em Term)
 *
 * \par Description
 * If \em Term is a class object or an atom that names a class,
 * then \b class(\em Term) succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * class person { ... }
 * new person (P)
 *
 * class(person)        succeeds
 * class(people)        fails (assuming 'people' is not a class)
 * class(1.5)           fails
 * class(f(X))          fails
 * class(P)             fails
 * class(P.prototype)   succeeds (P's prototype is the person class)
 * class("person")      fails
 * \endcode
 *
 * \par See Also
 * \ref class_2 "class/2",
 * \ref object_1 "object/1",
 * \ref predicate_1 "predicate/1"
 */
static p_goal_result p_builtin_class_1
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
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
 * \anchor class_2
 * \b class/2 - tests if a class name is associated with a
 * specific class object.
 *
 * \par Usage
 * \b class(\em Name, \em Class)
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
 * class(person, C)         succeeds
 * class(people, C)         fails
 * class(1.5, C)            fails
 * class(P, C)              fails
 * class(P.className, C)    succeeds
 * class(Name, P.prototype) succeeds
 * \endcode
 *
 * \par See Also
 * \ref class_1 "class/1",
 * \ref object_1 "object/1",
 * \ref predicate_1 "predicate/1"
 */
static p_goal_result p_builtin_class_2
    (p_context *context, p_term **args, p_term **error)
{
    p_term *name = p_term_deref_member(context, args[0]);
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
 * \ref class_1 "class/1",
 * \ref object_2 "object/2",
 * \ref predicate_1 "predicate/1"
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
 * \ref class_1 "class/1",
 * \ref object_1 "object/1",
 * \ref predicate_1 "predicate/1"
 */
static p_goal_result p_builtin_object_2
    (p_context *context, p_term **args, p_term **error)
{
    p_term *class_object = p_term_deref_member(context, args[1]);
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
 * \anchor predicate_1
 * \b predicate/1 - tests if a term is a predicate.
 *
 * \par Usage
 * \b predicate(\em Term)
 *
 * \par Description
 * If \em Term is a predicate, then succeeds.  Fails otherwise.
 *
 * \par Examples
 * \code
 * foo(X, Y) { ... }
 * predicate(P, foo/2);
 *
 * predicate(P)             succeeds
 * predicate(a)             fails
 * predicate(X)             fails
 * predicate(foo(X, Y))     fails
 * predicate(foo/2)         fails
 * predicate(1.5)           fails
 * \endcode
 *
 * \par See Also
 * \ref class_1 "class/1",
 * \ref object_1 "object/1",
 * \ref predicate_2 "predicate/2"
 */
static p_goal_result p_builtin_predicate_1
    (p_context *context, p_term **args, p_term **error)
{
    if (p_term_type(args[0]) == P_TERM_PREDICATE)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup type_testing
 * <hr>
 * \anchor predicate_2
 * \b predicate/2 - tests if a term is a predicate with a
 * specific name.
 *
 * \par Usage
 * \b predicate(\em Term, \em Pred)
 *
 * \par Description
 * If \em Term is a predicate, then \em Pred is unified with its
 * predicate indicator.  The indicator will have the form
 * \em Name / \em Arity.
 * \par
 * If \em Term is a variable and \em Pred has the form
 * \em Name / \em Arity, then \em Term is unified with the
 * predicate corresponding to \em Pred.  Fails if \em Pred
 * does not exist.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Term is a variable and
 *     one of \em Pred, \em Name, or \em Arity, is also a variable.
 * \li <tt>type_error(predicate_indicator, \em Pred)</tt> - \em Term
 *     is a variable and \em Pred does not have the form
 *     \em Name / \em Arity.
 * \li <tt>type_error(integer, \em Arity)</tt> - \em Term is a variable
 *     and \em Arity is not an integer.
 * \li <tt>type_error(atom, \em Name)</tt> - \em Term is a variable
 *     and \em Name is not an atom.
 * \li <tt>domain_error(not_less_than_zero, \em Arity)</tt> - \em Term
 *     is a variable and \em Arity is less than zero.
 *
 * \par Examples
 * \code
 * foo(X, Y) { ... }
 *
 * predicate(Term, foo/2)
 *      succeeds with Term set to the predicate foo/2
 * predicate(Term, Pred)
 *      succeeds with Pred = foo/2 if Term is the predicate foo/2
 * predicate(Term2, person::age/1)
 *      sets Term2 to the method age/1 within the person class.
 *      Note: the single argument is the "Self" pointer.
 * \endcode
 *
 * \par See Also
 * \ref class_1 "class/1",
 * \ref object_1 "object/1",
 * \ref predicate_1 "predicate/1"
 */
static p_goal_result p_builtin_predicate_2
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
    if (!term) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if ((term->header.type & P_TERM_VARIABLE) != 0) {
        p_term *name;
        int arity;
        p_database_info *info;
        name = p_builtin_parse_indicator
            (context, args[1], &arity, error);
        if (!name)
            return P_RESULT_ERROR;
        info = _p_db_find_arity(name, arity);
        if (info && info->predicate) {
            if (p_term_unify(context, term, info->predicate,
                             P_BIND_DEFAULT))
                return P_RESULT_TRUE;
        } else if (info && info->builtin_func) {
            /* Create a predicate term for the builtin */
            p_term *pred = p_term_create_predicate
                (context, name, arity);
            info->predicate = pred;
            if (p_term_unify(context, term, pred, P_BIND_DEFAULT))
                return P_RESULT_TRUE;
        }
        return P_RESULT_FAIL;
    } else {
        p_term *pred = p_term_create_functor
            (context, context->slash_atom, 2);
        p_term_bind_functor_arg(pred, 0, term->predicate.name);
        p_term_bind_functor_arg
            (pred, 1, p_term_create_integer
                (context, (int)(term->header.size)));
        if (p_term_unify(context, args[1], pred, P_BIND_DEFAULT))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    }
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

/**
 * \defgroup assignment Builtin predicates - Variable assignment
 *
 * Predicates in this group are used to assign values to variables
 * without unification, replacing their previous values.
 * Variable assignment may be destructive [\ref assign_2 "(:=)/2",
 * \ref num_assign_2 "(::=)/2"] or back-trackable
 * [\ref bt_assign_2 "(:==)/2", \ref bt_num_assign_2 "(::==)/2"].
 *
 * Destructive assignment is useful for setting object properties
 * and temporary loop variables.  Back-trackable assignment is
 * recommended for use in code that is searching for a solution
 * amongst alternatives so as to preserve logical consistency.
 *
 * \ref assign_2 "(:=)/2",
 * \ref num_assign_2 "(::=)/2",
 * \ref bt_assign_2 "(:==)/2",
 * \ref bt_num_assign_2 "(::==)/2"
 */
/*\@{*/

int p_term_occurs_in(const p_term *var, const p_term *value);
p_goal_result p_arith_eval
    (p_context *context, p_arith_value *result,
     p_term *expr, p_term **error);

/* Resolve a variable reference down to its basic form.
 * Returns null if not a variable */
P_INLINE p_term *p_term_resolve_variable(p_context *context, p_term *var)
{
    p_term *value;
    if (!var || var->header.type == P_TERM_VARIABLE)
        return var;
    if (var->header.type != P_TERM_MEMBER_VARIABLE)
        return 0;
    p_term_deref_own_member(context, var);
    value = var->var.value;
    if (value && value->header.type == P_TERM_VARIABLE)
        return value;
    return 0;
}

/**
 * \addtogroup assignment
 * <hr>
 * \anchor assign_2
 * <b>(:=)/2</b> - destructive variable assignment.
 *
 * \par Usage
 * \em Var <b>:=</b> \em Term
 *
 * \par Description
 * Assigns a freshly renamed copy of \em Term to \em Var,
 * replacing its previous value, and succeed.  The \em Term is
 * renamed so that the assigned value will survive back-tracking.
 * Use \ref bt_assign_2 "(:==)/2" to cause \em Var to revert to its
 * original value upon back-tracking.
 *
 * \par Errors
 *
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is not
 *     a variable.
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is a member
 *     variable reference but the member does not exist.
 *
 * \par Examples
 * \code
 * X := f(a, b)             succeeds
 * X := f(b, a)             succeeds again, replacing the value
 * X := f(X, a)             succeeds after renaming X to _X
 * X := Y + Z               sets X to (_Y + _Z), does not evaluate
 * X := pi                  sets X to the atom pi, does not evaluate
 * X.name := 42             sets the name property of object X to 42
 * a := X                   type_error(variable, a)
 * \endcode
 *
 * \par See Also
 * \ref num_assign_2 "(::=)/2",
 * \ref bt_assign_2 "(:==)/2",
 * \ref bt_num_assign_2 "(::==)/2",
 * \ref unify_2 "(=)/2",
 * \ref copy_term_2 "copy_term/2"
 */
static p_goal_result p_builtin_assign
    (p_context *context, p_term **args, p_term **error)
{
    p_term *var;
    var = p_term_resolve_variable(context, args[0]);
    if (!var) {
        *error = p_create_type_error(context, "variable", args[0]);
        return P_RESULT_ERROR;
    }
    var->var.value = p_term_clone(context, args[1]);
    return P_RESULT_TRUE;
}

/**
 * \addtogroup assignment
 * <hr>
 * \anchor num_assign_2
 * <b>(::=)/2</b> - destructive variable assignment of an
 * arithmetic term.
 *
 * \par Usage
 * \em Var <b>::=</b> \em Term
 *
 * \par Description
 * Evaluates \em Term according to the rules of \ref is_2 "is/2",
 * and assigns it to \em Var, replacing its previous value.
 * \par
 * The assignment will be permanent; back-tracking will not
 * revert \em Var to its previous value.
 * Use \ref bt_num_assign_2 "(::==)/2" for back-trackable assignment.
 *
 * \par Errors
 *
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is not
 *     a variable.
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is a member
 *     variable reference but the member does not exist.
 *
 * The errors for \ref is_2 "is/2" may also be thrown during
 * the evaluation of \em Term.
 *
 * \par Examples
 * \code
 * X ::= X + 1              increments X
 * X ::= pi                 sets X to 3.14159265358979323846
 * X.name ::= 48 - 6        sets the name property of object X to 42
 * a ::= X                  type_error(variable, a)
 * \endcode
 *
 * \par See Also
 * \ref assign_2 "(:=)/2",
 * \ref bt_assign_2 "(:==)/2",
 * \ref bt_num_assign_2 "(::==)/2",
 * \ref is_2 "is/2"
 */
static p_goal_result p_builtin_num_assign
    (p_context *context, p_term **args, p_term **error)
{
    p_term *var;
    p_arith_value value;
    p_goal_result result;
    var = p_term_resolve_variable(context, args[0]);
    if (!var) {
        *error = p_create_type_error(context, "variable", args[0]);
        return P_RESULT_ERROR;
    }
    result = p_arith_eval(context, &value, args[1], error);
    if (result != P_RESULT_TRUE)
        return result;
    switch (value.type) {
    case P_TERM_INTEGER:
        var->var.value =
            p_term_create_integer(context, value.integer_value);
        break;
    case P_TERM_REAL:
        var->var.value =
            p_term_create_real(context, value.real_value);
        break;
    case P_TERM_STRING:
        var->var.value = value.string_value;
        break;
    default: return P_RESULT_FAIL;
    }
    return P_RESULT_TRUE;
}

/**
 * \addtogroup assignment
 * <hr>
 * \anchor bt_assign_2
 * <b>(:==)/2</b> - back-trackable variable assignment.
 *
 * \par Usage
 * \em Var <b>:==</b> \em Term
 *
 * \par Description
 * Assigns \em Term to \em Var, replacing its previous value,
 * and succeed.  If \em Var occurs in \em Term, then the assignment
 * will fail.
 * \par
 * Upon back-tracking, \em Var will revert to its previous value.
 * Use \ref assign_2 "(:=)/2" for destructive assignment.
 *
 * \par Errors
 *
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is not
 *     a variable.
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is a member
 *     variable reference but the member does not exist.
 *
 * \par Examples
 * \code
 * X :== f(a, b)            succeeds
 * X :== f(b, a)            succeeds again, replacing the value
 * X :== f(X, a)            fails due to occurs check
 * X :== Y + Z              sets X to (Y + Z), does not evaluate
 * X :== pi                 sets X to the atom pi, does not evaluate
 * X.name :== 42            sets the name property of object X to 42
 * a :== X                  type_error(variable, a)
 * \endcode
 *
 * \par See Also
 * \ref assign_2 "(:=)/2",
 * \ref num_assign_2 "(::=)/2",
 * \ref bt_num_assign_2 "(::==)/2",
 * \ref unify_2 "(=)/2"
 */
static p_goal_result p_builtin_bt_assign
    (p_context *context, p_term **args, p_term **error)
{
    p_term *var;
    p_term *prev;
    var = p_term_resolve_variable(context, args[0]);
    if (!var) {
        *error = p_create_type_error(context, "variable", args[0]);
        return P_RESULT_ERROR;
    }
    prev = var->var.value;
    var->var.value = 0;
    if (!p_term_occurs_in(var, args[1])) {
        _p_context_record_contents_in_trail
            (context, (void **)&(var->var.value), prev);
        var->var.value = args[1];
        return P_RESULT_TRUE;
    }
    var->var.value = prev;
    return P_RESULT_FAIL;
}

/**
 * \addtogroup assignment
 * <hr>
 * \anchor bt_num_assign_2
 * <b>(::==)/2</b> - back-trackable variable assignment of an
 * arithmetic term.
 *
 * \par Usage
 * \em Var <b>::==</b> \em Term
 *
 * \par Description
 * Evaluates \em Term according to the rules of \ref is_2 "is/2",
 * and assigns it to \em Var, replacing its previous value.
 * \par
 * Upon back-tracking, \em Var will revert to its previous value.
 * Use \ref num_assign_2 "(::=)/2" for destructive assignment.
 *
 * \par Errors
 *
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is not
 *     a variable.
 * \li <tt>type_error(variable, \em Var)</tt> - \em Var is a member
 *     variable reference but the member does not exist.
 *
 * The errors for \ref is_2 "is/2" may also be thrown during
 * the evaluation of \em Term.
 *
 * \par Examples
 * \code
 * X ::== X + 1
 * X ::== pi
 * X.name ::== 48 - 6       sets the name property of object X to 42
 * a ::== X                 type_error(variable, a)
 * \endcode
 *
 * \par See Also
 * \ref assign_2 "(:=)/2",
 * \ref bt_assign_2 "(:==)/2",
 * \ref num_assign_2 "(::=)/2",
 * \ref is_2 "is/2"
 */
static p_goal_result p_builtin_bt_num_assign
    (p_context *context, p_term **args, p_term **error)
{
    p_term *var;
    p_arith_value value;
    p_goal_result result;
    var = p_term_resolve_variable(context, args[0]);
    if (!var) {
        *error = p_create_type_error(context, "variable", args[0]);
        return P_RESULT_ERROR;
    }
    result = p_arith_eval(context, &value, args[1], error);
    if (result != P_RESULT_TRUE)
        return result;
    _p_context_record_contents_in_trail
        (context, (void **)&(var->var.value), var->var.value);
    switch (value.type) {
    case P_TERM_INTEGER:
        var->var.value =
            p_term_create_integer(context, value.integer_value);
        break;
    case P_TERM_REAL:
        var->var.value =
            p_term_create_real(context, value.real_value);
        break;
    case P_TERM_STRING:
        var->var.value = value.string_value;
        break;
    default: return P_RESULT_FAIL;
    }
    return P_RESULT_TRUE;
}

/*\@}*/

/* $$line(filename, number, goal) encountered in the code */
static p_goal_result p_builtin_line
    (p_context *context, p_term **args, p_term **error)
{
    /* Replace the current goal with the nested goal and retry */
    context->current_node->goal = args[2];
    return P_RESULT_TREE_CHANGE;
}

/* $$unique(Value) to generate a (fairly) unique integer value */
static p_goal_result p_builtin_unique
    (p_context *context, p_term **args, p_term **error)
{
    p_term *value = p_term_create_integer
        (context, (context->unique_num)++);
    if (p_term_unify(context, args[0], value, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/* $$witness(Term, List, Subgoal) helper predicate */
static p_goal_result p_builtin_witness
    (p_context *context, p_term **args, p_term **error)
{
    p_term *term = p_term_deref_member(context, args[0]);
    p_term *subgoal = 0;
    p_term *list = p_term_witness(context, term, &subgoal);
    if (!p_term_unify(context, args[1], list, P_BIND_DEFAULT))
        return P_RESULT_FAIL;
    if (!p_term_unify(context, args[2], subgoal, P_BIND_DEFAULT))
        return P_RESULT_FAIL;
    return P_RESULT_TRUE;
}

void _p_db_init_builtins(p_context *context)
{
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
        {"||", 2, p_builtin_logical_or},
        {"->", 2, p_builtin_if},
        {"?-", 1, p_builtin_call},
        {":-", 1, p_builtin_call},
        {"=..", 2, p_builtin_univ},
        {":=", 2, p_builtin_assign},
        {"::=", 2, p_builtin_num_assign},
        {":==", 2, p_builtin_bt_assign},
        {"::==", 2, p_builtin_bt_num_assign},
        {"abolish", 1, p_builtin_abolish},
        {"arg", 3, p_builtin_arg},
        {"asserta", 1, p_builtin_asserta},
        {"assertz", 1, p_builtin_assertz},
        {"atom", 1, p_builtin_atom},
        {"atomic", 1, p_builtin_atomic},
        {"call", 1, p_builtin_call},
        {"$$call_member", 2, p_builtin_call_member},
        {"catch", 3, p_builtin_catch},
        {"class", 1, p_builtin_class_1},
        {"class", 2, p_builtin_class_2},
        {"clause", 2, p_builtin_clause},
        {"compound", 1, p_builtin_compound},
        {"consult", 1, p_builtin_consult},
        {"copy_term", 2, p_builtin_copy_term},
        {"dynamic", 1, p_builtin_dynamic},
        {"fail", 0, p_builtin_fail},
        {"false", 0, p_builtin_fail},
        {"float", 1, p_builtin_float},
        {"functor", 3, p_builtin_functor},
        {"halt", 0, p_builtin_halt_0},
        {"halt", 1, p_builtin_halt_1},
        {"import", 1, p_builtin_import},
        {"initialization", 1, p_builtin_call},
        {"integer", 1, p_builtin_integer},
        {"$$line", 3, p_builtin_line},
        {"load_library", 1, p_builtin_load_library},
        {"$$new", 2, p_builtin_new},
        {"new_class", 4, p_builtin_new_class},
        {"new_object", 3, p_builtin_new_object},
        {"nonvar", 1, p_builtin_nonvar},
        {"number", 1, p_builtin_number},
        {"object", 1, p_builtin_object_1},
        {"object", 2, p_builtin_object_2},
        {"predicate", 1, p_builtin_predicate_1},
        {"predicate", 2, p_builtin_predicate_2},
        {"retract", 1, p_builtin_retract},
        {"$$set_loop_var", 2, p_builtin_set_loop_var},
        {"string", 1, p_builtin_string},
        {"throw", 1, p_builtin_throw},
        {"true", 0, p_builtin_true},
        {"$$try", 2, p_builtin_catch},
        {"$$unbind", 1, p_builtin_unbind},
        {"unifiable", 2, p_builtin_unifiable},
        {"unify_one_way", 2, p_builtin_unify_one_way},
        {"unify_with_occurs_check", 2, p_builtin_unify},
        {"$$unique", 1, p_builtin_unique},
        {"var", 1, p_builtin_var},
        {"$$witness", 3, p_builtin_witness},
        {0, 0, 0}
    };
    static const char * const builtin_sources[] = {
        p_builtin_do,
        p_builtin_for,
        p_builtin_in,
        p_builtin_not_provable,
        p_builtin_once,
        p_builtin_repeat,
        p_builtin_switch,
        p_builtin_while,
        0
    };
    _p_db_register_builtins(context, builtins);
    _p_db_register_sources(context, builtin_sources);
}
