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

#ifndef PLANG_ERRORS_PRIV_H
#define PLANG_ERRORS_PRIV_H

#include <plang/term.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

p_term *p_create_instantiation_error(p_context *context);
p_term *p_create_type_error(p_context *context, const char *name, p_term *term);
p_term *p_create_domain_error(p_context *context, const char *name, p_term *term);
p_term *p_create_existence_error(p_context *context, const char *name, p_term *term);
p_term *p_create_permission_error(p_context *context, const char *name1, const char *name2, p_term *term);
p_term *p_create_evaluation_error(p_context *context, const char *name);
p_term *p_create_system_error(p_context *context);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
