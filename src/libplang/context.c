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

#include <plang/context.h>
#include "context-priv.h"
#include "term-priv.h"

/**
 * \defgroup context Execution Contexts
 */
/*\@{*/

p_context *p_context_create(void)
{
    GC_INIT();
    p_context *context = GC_NEW_UNCOLLECTABLE(struct p_context);
    if (!context)
        return 0;
    context->nil_atom = p_term_create_atom(context, "[]");
    context->prototype_atom = p_term_create_atom(context, "prototype");
    context->class_name_atom = p_term_create_atom(context, "className");
    return context;
}

void p_context_free(p_context *context)
{
    if (!context)
        return;
    GC_FREE(context);
}

/**
 * \brief Marks the current position in the backtrack trace
 * in \a context and returns a marker pointer.
 *
 * \ingroup context
 * \sa p_context_backtrace_trace(), p_context_record_in_trace()
 */
void *p_context_mark_trace(p_context *context)
{
    if (context->trace)
        return &(context->trace[context->trace_top]);
    else
        return 0;
}

/**
 * \brief Backtracks the trace in \a context, undoing variable
 * bindings until \a marker is reached.
 *
 * \ingroup context
 * \sa p_context_mark_trace(), p_context_record_in_trace()
 */
void p_context_backtrack_trace(p_context *context, void *marker)
{
    /* TODO */
}

/**
 * \brief Records \a var in the trace for \a context, as a variable
 * binding to be undone when p_context_backtrack_trace() is called.
 *
 * \ingroup context
 * \sa p_context_backtrack_trace(), p_context_record_in_trace()
 */
int p_context_record_in_trace(p_context *context, p_term *var)
{
    /* TODO */
    return 1;
}

/*\@}*/
