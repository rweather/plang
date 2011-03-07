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

#ifndef PLANG_DATABASE_PRIV_H
#define PLANG_DATABASE_PRIV_H

#include "term-priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

/* Information that is attached to an atom to provide information
 * about the operators and predicates with that name */
struct p_database_info
{
    p_database_info *next;
    unsigned int arity;
    unsigned int op_specifier : 16;
    unsigned int op_priority : 16;
};

void _p_db_init(p_context *context);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
