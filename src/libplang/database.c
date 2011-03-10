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

#if 0
        /* Operators specific to this implementation that give
         * better C-style names to some of the above.  These are
         * implemented as aliases in the parser which are converted
         * into the traditional operators during parsing */
        {"||",      P_OP_XFY, 1100},    /* ; */
        {"&&",      P_OP_XFY, 1000},    /* , */
        {"!",       P_OP_FY,   900},    /* \+ */
        {"!=",      P_OP_XFX,  700},    /* \= */
        {"!==",     P_OP_XFX,  700},    /* \== */
        {"=!=",     P_OP_XFX,  700},    /* =\= */
        {"@<=",     P_OP_XFX,  700},    /* @=< */
        {"<=",      P_OP_XFX,  700},    /* =< */
        {"~",       P_OP_FY,   200},    /* \ */
#endif

        /* New operators specific to this implementation */
        {":=",      P_OP_XFX,  700},    /* Variable assignment */ 
        {"::=",     P_OP_XFX,  700},    /* Numeric assignment */ 
        {"in",      P_OP_XFX,  700},    /* List membership test */
        {":",       P_OP_XFX,  100},    /* Type constraint */
        {".",       P_OP_YFX,   50},    /* Member reference */

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

/**
 * \brief Retrieves the operator details for the atom \a name and
 * the specified \a arity (1 or 2).
 *
 * Returns the operator prefix/infix/postfix specifier from the
 * function, and return the operator priority in \a priority.
 *
 * \ingroup context
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
 * \ingroup context
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

/*\@}*/
