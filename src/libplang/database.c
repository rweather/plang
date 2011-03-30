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
#include "database-priv.h"
#include "context-priv.h"

/**
 * \defgroup database Database
 *
 * This module manages the predicate an operator database.
 */
/*\@{*/

/**
 * \enum p_op_specifier
 * \ingroup database
 * This enum defines the specifier for a prefix, infix, or postfix
 * operator.
 * \sa p_db_operator_info()
 */

/**
 * \var P_OP_NONE
 * \ingroup database
 * No operator information is available.
 */

/**
 * \var P_OP_XF
 * \ingroup database
 * Non-associative postfix operator.
 */

/**
 * \var P_OP_YF
 * \ingroup database
 * Left-associative postfix operator.
 */

/**
 * \var P_OP_XFX
 * \ingroup database
 * Non-associative infix operator.
 */

/**
 * \var P_OP_XFY
 * \ingroup database
 * Right-associative infix operator.
 */

/**
 * \var P_OP_YFX
 * \ingroup database
 * Left-associative infix operator.
 */

/**
 * \var P_OP_FX
 * \ingroup database
 * Non-associative prefix operator.
 */

/**
 * \var P_OP_FY
 * \ingroup database
 * Right-associative prefix operator.
 */

/**
 * \enum p_predicate_flags
 * \ingroup database
 * This enum defines flags that are associated with predicates
 * in the database.
 * \sa p_db_predicate_flags()
 */

/**
 * \var P_PREDICATE_NONE
 * \ingroup database
 * No flags are specified for the predicate.
 */

/**
 * \var P_PREDICATE_COMPILED
 * \ingroup database
 * The predicate has been compiled into a read-only form that
 * cannot be modified by \ref asserta_1 "asserta/1" and friends.
 */

/**
 * \var P_PREDICATE_DYNAMIC
 * \ingroup database
 * The predicate is marked as dynamic.  Its clauses will not
 * be compiled.  This is intended for predicates that are
 * created dynamically in the database at runtime.
 */

/**
 * \var P_PREDICATE_BUILTIN
 * \ingroup database
 * The predicate was set by p_db_set_builtin_predicate(),
 * is read-only, and cannot be modified by
 * \ref asserta_1 "asserta/1" and friends.
 */

void _p_db_init(p_context *context)
{
    struct p_db_op_info
    {
        const char *name;
        p_op_specifier specifier;
        int priority;
    };
    static struct p_db_op_info const ops[] = {
        /* Traditional operators from ISO Prolog */
        {":-",      P_OP_XFX, 1200},
        {"-->",     P_OP_XFX, 1200},
        {":-",      P_OP_FX,  1200},
        {"?-",      P_OP_FX,  1200},
        {";",       P_OP_XFY, 1100},
        {"->",      P_OP_XFY, 1050},
        {",",       P_OP_XFY, 1000},
        {"\\+",     P_OP_FY,   900},
        {"=",       P_OP_XFX,  700},
        {"\\=",     P_OP_XFX,  700},
        {"==",      P_OP_XFX,  700},
        {"\\==",    P_OP_XFX,  700},
        {"@<",      P_OP_XFX,  700},
        {"@=<",     P_OP_XFX,  700},
        {"@>",      P_OP_XFX,  700},
        {"@>=",     P_OP_XFX,  700},
        {"=..",     P_OP_XFX,  700},
        {"is",      P_OP_XFX,  700},
        {"=:=",     P_OP_XFX,  700},
        {"=\\=",    P_OP_XFX,  700},
        {"<",       P_OP_XFX,  700},
        {"=<",      P_OP_XFX,  700},
        {">",       P_OP_XFX,  700},
        {">=",      P_OP_XFX,  700},
        {"+",       P_OP_YFX,  500},
        {"-",       P_OP_YFX,  500},
        {"/\\",     P_OP_YFX,  500},
        {"\\/",     P_OP_YFX,  500},
        {"*",       P_OP_YFX,  400},
        {"/",       P_OP_YFX,  400},
        {"//",      P_OP_YFX,  400},
        {"rem",     P_OP_YFX,  400},
        {"mod",     P_OP_YFX,  400},
        {"<<",      P_OP_YFX,  400},
        {">>",      P_OP_YFX,  400},
        {"**",      P_OP_XFX,  200},
        {"^",       P_OP_XFY,  200},
        {"-",       P_OP_FY,   200},
        {"\\",      P_OP_FY,   200},

        /* Operators specific to this implementation that give
         * better C-style names to some of the above */
        {"||",      P_OP_XFY, 1100},    /* ; */
        {"&&",      P_OP_XFY, 1000},    /* , */
        {"!",       P_OP_FY,   900},    /* \+ */
        {"!=",      P_OP_XFX,  700},    /* \= */
        {"!==",     P_OP_XFX,  700},    /* \== */
        {"=!=",     P_OP_XFX,  700},    /* =\= */
        {"@<=",     P_OP_XFX,  700},    /* @=< */
        {"<=",      P_OP_XFX,  700},    /* =< */
        {"~",       P_OP_FY,   200},    /* \ */

        /* New operators specific to this implementation */
        {":=",      P_OP_XFX,  700},    /* Variable assignment */ 
        {"::=",     P_OP_XFX,  700},    /* Numeric assignment */ 
        {"in",      P_OP_XFX,  700},    /* List membership test */
        {">>>",     P_OP_YFX,  400},    /* Unsigned shift right */
        {":",       P_OP_XFX,  100},    /* Type constraint */

        {0,         0,           0}
    };
    int index;
    for (index = 0; ops[index].name; ++index) {
        p_db_set_operator_info
            (p_term_create_atom(context, ops[index].name),
             ops[index].specifier, ops[index].priority);
    }
}

P_INLINE p_database_info *p_db_find_arity(const p_term *atom, unsigned int arity)
{
    p_database_info *info = atom->atom.db_info;
    while (info && info->arity != arity)
        info = info->next;
    return info;
}

p_database_info *_p_db_find_arity(const p_term *atom, unsigned int arity)
{
    return p_db_find_arity(atom, arity);
}

P_INLINE p_database_info *p_db_create_arity(p_term *atom, unsigned int arity)
{
    p_database_info *info = p_db_find_arity(atom, arity);
    if (!info) {
        info = GC_NEW(p_database_info);
        if (!info)
            return 0;
        info->next = atom->atom.db_info;
        info->arity = arity;
        atom->atom.db_info = info;
    }
    return info;
}

p_database_info *_p_db_create_arity(p_term *atom, unsigned int arity)
{
    return _p_db_create_arity(atom, arity);
}

/**
 * \brief Retrieves the operator details for the atom \a name and
 * the specified \a arity (1 or 2).
 *
 * Returns the operator prefix/infix/postfix specifier from the
 * function, and return the operator priority in \a priority.
 *
 * \ingroup database
 * \sa p_db_set_operator_info()
 */
p_op_specifier p_db_operator_info(const p_term *name, int arity, int *priority)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM) {
        *priority = 0;
        return P_OP_NONE;
    }

    /* Search for the arity's information block */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (!info) {
        *priority = 0;
        return P_OP_NONE;
    }

    /* Return the operator details */
    *priority = (int)(info->op_priority);
    return (p_op_specifier)(info->op_specifier);
}

/**
 * \brief Sets the operator details for the atom \a name according
 * to \a specifier and \a priority.
 *
 * If \a priority is zero, then the operator details for \a specifier
 * will be removed.
 *
 * \ingroup database
 * \sa p_db_operator_info()
 */
void p_db_set_operator_info(p_term *name, p_op_specifier specifier, int priority)
{
    unsigned int arity;
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return;

    /* Determine the operator's arity from the specifier */
    switch (specifier) {
    case P_OP_XF:
    case P_OP_YF:
    case P_OP_FX:
    case P_OP_FY:   arity = 1; break;
    case P_OP_XFX:
    case P_OP_XFY:
    case P_OP_YFX:  arity = 2; break;
    default:        return;
    }

    /* Clear the operator details if setting the priority to zero */
    if (!priority)
        specifier = P_OP_NONE;

    /* Find or create an information block for the arity */
    info = p_db_create_arity(name, arity);
    if (!info)
        return;

    /* Set the operator details */
    info->op_specifier = (unsigned int)specifier;
    info->op_priority = (unsigned int)priority;
}

/**
 * \typedef p_db_builtin
 * \ingroup database
 * This type defines the function prototype of a builtin predicate.
 *
 * The arguments are the execution context, a pointer to the terms
 * that define the predicates arguments, and a return pointer for
 * error terms.
 *
 * The return value should be one of P_RESULT_FAIL, P_RESULT_TRUE,
 * or P_RESULT_ERROR.
 *
 * Builtin predicates must be deterministic; they cannot backtrack.
 *
 * \sa p_db_set_builtin_predicate()
 */

/**
 * \brief Returns the builtin predicate function for \a name and
 * \a arity, or null if there is no builtin predicate function.
 *
 * \ingroup database
 * \sa p_db_builtin, p_db_set_builtin_predicate()
 * \sa p_db_builtin_arith()
 */
p_db_builtin p_db_builtin_predicate(const p_term *name, int arity)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return 0;

    /* Search for the arity's information block */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (info)
        return info->builtin_func;
    else
        return 0;
}

/**
 * \brief Sets the \a builtin predicate function for \a name and
 * \a arity.
 *
 * If \a builtin is null, then the previous builtin function
 * association is removed.
 *
 * \ingroup database
 * \sa p_db_builtin, p_db_builtin_predicate()
 * \sa p_db_set_builtin_arith()
 */
void p_db_set_builtin_predicate(p_term *name, int arity, p_db_builtin builtin)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return;

    /* Find or create an information block for the arity */
    info = p_db_create_arity(name, (unsigned int)arity);
    if (!info)
        return;

    /* Set the builtin */
    info->builtin_func = builtin;
    if (builtin)
        info->flags |= P_PREDICATE_BUILTIN;
    else
        info->flags &= ~P_PREDICATE_BUILTIN;
}

/* Register a table of builtin predicates */
void _p_db_register_builtins(p_context *context, const struct p_builtin *builtins)
{
    while (builtins->name != 0) {
        p_db_set_builtin_predicate
            (p_term_create_atom(context, builtins->name),
             builtins->arity, builtins->func);
        ++builtins;
    }
}

/**
 * \typedef p_db_arith
 * \ingroup database
 * This type defines the function prototype of a builtin
 * arithmetic function.
 *
 * The arguments are the execution context, a pointer to the
 * result value, a pointer to an array of argument values,
 * a pointer to the raw terms that resulted in the argument
 * values, and a return pointer for error terms.
 *
 * The return value should be one of P_RESULT_TRUE or P_RESULT_ERROR.
 *
 * Builtin arithmetic functions must be deterministic;
 * they cannot backtrack.
 *
 * \sa p_db_set_builtin_arith()
 */

/**
 * \brief Returns the builtin arithmetic function for \a name and
 * \a arity, or null if there is no builtin arithmetic function.
 *
 * \ingroup database
 * \sa p_db_arith, p_db_set_builtin_arith(), p_db_builtin_predicate()
 */
p_db_arith p_db_builtin_arith(const p_term *name, int arity)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return 0;

    /* Search for the arity's information block */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (info)
        return info->arith_func;
    else
        return 0;
}

/**
 * \brief Sets the \a builtin arithmetic function for \a name and
 * \a arity.
 *
 * If \a builtin is null, then the previous builtin function
 * association is removed.
 *
 * \ingroup database
 * \sa p_db_arith, p_db_builtin_arith(), p_db_set_builtin_predicate()
 */
void p_db_set_builtin_arith
    (p_term *name, int arity, p_db_arith builtin)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return;

    /* Find or create an information block for the arity */
    info = p_db_create_arity(name, (unsigned int)arity);
    if (!info)
        return;

    /* Set the arithmetic builtin */
    info->arith_func = builtin;
}

/* Register a table of builtin arithmetic functions */
void _p_db_register_ariths(p_context *context, const struct p_arith *ariths)
{
    while (ariths->name != 0) {
        p_db_set_builtin_arith
            (p_term_create_atom(context, ariths->name),
             ariths->arity, ariths->arith_func);
        ++ariths;
    }
}

/* Register a table of Plang source strings for builtin predicates */
void _p_db_register_sources(p_context *context, const char * const *sources)
{
    while (*sources != 0) {
        p_context_consult_string(context, *sources);
        ++sources;
    }
}

/* Extract the predicate name and arity from a clause */
static p_term *p_db_predicate_name
    (p_context *context, p_term *clause, int *arity)
{
    p_term *head;
    clause = p_term_deref(clause);
    if (!clause || clause->header.type != P_TERM_FUNCTOR ||
            clause->header.size != 2 ||
            clause->functor.functor_name != context->clause_atom)
        return 0;
    head = p_term_deref(clause->functor.arg[0]);
    if (!head)
        return 0;
    if (head->header.type == P_TERM_ATOM) {
        *arity = 0;
        return head;
    } else if (head->header.type == P_TERM_FUNCTOR) {
        *arity = (int)(head->header.size);
        return head->functor.functor_name;
    }
    return 0;
}

/**
 * \brief Asserts \a clause as the first clause in a database
 * predicate on \a context.
 *
 * Returns non-zero if the clause was added, or zero if the
 * predicate is builtin or compiled.  It is assumed that \a clause
 * is a freshly renamed term, is well-formed, and the top-level
 * functor is "(:-)/2".
 *
 * \ingroup database
 * \sa p_db_clause_assert_last(), p_db_clause_retract()
 * \sa p_db_clause_abolish()
 */
int p_db_clause_assert_first(p_context *context, p_term *clause)
{
    p_database_info *info;
    p_term *name;
    int arity;
    p_term *predicate;

    /* Fetch the clause name and arity */
    name = p_db_predicate_name(context, clause, &arity);
    if (!name)
        return 0;

    /* Find or create the information block for the arity */
    info = p_db_create_arity(name, (unsigned int)arity);
    if (!info)
        return 0;

    /* Bail out if the predicate is builtin or compiled */
    if (info->flags & (P_PREDICATE_BUILTIN | P_PREDICATE_COMPILED))
        return 0;

    /* Add the clause to the head of the list */
    predicate = info->predicate;
    if (!predicate) {
        predicate = p_term_create_predicate(context, name, arity);
        if (!predicate)
            return 0;
        info->predicate = predicate;
    }
    p_term_add_clause_first(context, predicate, clause);
    return 1;
}

/**
 * \brief Asserts \a clause as the last clause in a database
 * predicate on \a context.
 *
 * Returns non-zero if the clause was added, or zero if the
 * predicate is compiled or builtin.  It is assumed that \a clause
 * is a freshly renamed term, is well-formed, and the top-level
 * functor is "(:-)/2".
 *
 * \ingroup database
 * \sa p_db_clause_assert_first(), p_db_clause_retract()
 * \sa p_db_clause_abolish()
 */
int p_db_clause_assert_last(p_context *context, p_term *clause)
{
    p_database_info *info;
    p_term *name;
    int arity;
    p_term *predicate;

    /* Fetch the clause name and arity */
    name = p_db_predicate_name(context, clause, &arity);
    if (!name)
        return 0;

    /* Find or create the information block for the arity */
    info = p_db_create_arity(name, (unsigned int)arity);
    if (!info)
        return 0;

    /* Bail out if the predicate is builtin or compiled */
    if (info->flags & (P_PREDICATE_BUILTIN | P_PREDICATE_COMPILED))
        return 0;

    /* Add the clause to the tail of the list */
    predicate = info->predicate;
    if (!predicate) {
        predicate = p_term_create_predicate(context, name, arity);
        if (!predicate)
            return 0;
        info->predicate = predicate;
    }
    p_term_add_clause_last(context, predicate, clause);
    return 1;
}

/**
 * \brief Retracts \a clause from the predicate database
 * on \a context.
 *
 * Returns a positive value if the clause was retracted, zero if the
 * predicate is compiled or builtin, or a negative value if there
 * are no more matching clauses.  It is assumed that the top-level
 * functor of \a clause is "(:-)/2".
 *
 * \ingroup database
 * \sa p_db_clause_assert_first(), p_db_clause_assert_last()
 * \sa p_db_clause_abolish()
 */
int p_db_clause_retract(p_context *context, p_term *clause)
{
    p_database_info *info;
    p_term *name;
    int arity;
    p_term *list;
    p_term *prev;
    p_term *predicate;

    /* Fetch the clause name and arity */
    name = p_db_predicate_name(context, clause, &arity);
    if (!name)
        return 0;

    /* Find the information block for the arity */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (!info)
        return -1;

    /* Bail out if the predicate is builtin or compiled */
    if (info->flags & (P_PREDICATE_BUILTIN | P_PREDICATE_COMPILED))
        return 0;

    /* Retract the first clause that unifies */
    predicate = info->predicate;
    if (!predicate)
        return -1;
    list = predicate->predicate.clauses_head;
    prev = 0;
    while (list) {
        if (p_term_unify(context, clause, list->list.head,
                         P_BIND_DEFAULT)) {
            if (prev)
                p_term_set_tail(prev, list->list.tail);
            else
                predicate->predicate.clauses_head = list->list.tail;
            if (!list->list.tail)
                predicate->predicate.clauses_tail = prev;
            if (!predicate->predicate.clauses_head)
                info->predicate = 0;    /* Completely removed */
            return 1;
        }
        prev = list;
        list = list->list.tail;
    }
    return -1;
}

/**
 * \brief Abolishes all clauses from the predicate database
 * on \a context that match \a name and \a arity.
 *
 * Returns non-zero if the clauses were abolished, or zero if the
 * predicate is compiled or builtin.
 *
 * \ingroup database
 * \sa p_db_clause_assert_first(), p_db_clause_assert_last()
 * \sa p_db_clause_retract()
 */
int p_db_clause_abolish(p_context *context, const p_term *name, int arity)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return 1;   /* Absolishing a non-existent clause succeeds */

    /* Find the information block for the arity */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (!info)
        return 1;   /* Absolishing a non-existent clause succeeds */

    /* Bail out if the predicate is builtin or compiled */
    if (info->flags & (P_PREDICATE_BUILTIN | P_PREDICATE_COMPILED))
        return 0;

    /* Retract all of the clauses */
    info->predicate = 0;
    return 1;
}

/**
 * \brief Returns the flags associated with the predicate
 * \a name / \a arity in \a context.
 *
 * \ingroup database
 * \sa p_db_set_predicate_flag()
 */
p_predicate_flags p_db_predicate_flags(p_context *context, const p_term *name, int arity)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return P_PREDICATE_NONE;

    /* Search for the arity's information block */
    info = p_db_find_arity(name, (unsigned int)arity);
    if (!info)
        return P_PREDICATE_NONE;

    /* Return the predicate flag details */
    return (p_predicate_flags)(info->flags);
}

/**
 * \brief Sets the \a flag associated with the predicate
 * \a name / \a arity in \a context to \a value (0 or 1).
 *
 * \ingroup database
 * \sa p_db_predicate_flags()
 */
void p_db_set_predicate_flag(p_context *context, p_term *name, int arity, p_predicate_flags flag, int value)
{
    p_database_info *info;

    /* Check that the name is actually an atom */
    name = p_term_deref(name);
    if (!name || name->header.type != P_TERM_ATOM)
        return;

    /* Find or create an information block for the arity */
    info = p_db_create_arity(name, arity);
    if (!info)
        return;

    /* Alter the specified flag */
    if (value)
        info->flags |= flag;
    else
        info->flags &= ~flag;
}

/*\@}*/
