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
#include "database-priv.h"

/**
 * \defgroup context Execution Contexts
 */
/*\@{*/

/**
 * \brief Creates and returns a new execution context.
 *
 * \ingroup context
 * \sa p_context_free()
 */
p_context *p_context_create(void)
{
    GC_INIT();
    p_context *context = GC_NEW_UNCOLLECTABLE(struct p_context);
    if (!context)
        return 0;
    context->nil_atom = p_term_create_atom(context, "[]");
    context->prototype_atom = p_term_create_atom(context, "prototype");
    context->class_name_atom = p_term_create_atom(context, "className");
    context->clause_atom = p_term_create_atom(context, ":-");
    context->trace_top = P_TRACE_SIZE;
    _p_db_init(context);
    return context;
}

/**
 * \brief Frees an execution \a context.
 *
 * \ingroup context
 * \sa p_context_create()
 */
void p_context_free(p_context *context)
{
    if (!context)
        return;
    GC_FREE(context);
}

/* Push a word onto the trace */
P_INLINE int p_context_push_trace(p_context *context, void **word)
{
    if (context->trace_top >= P_TRACE_SIZE) {
        struct p_trace *trace = GC_NEW(struct p_trace);
        if (!trace)
            return 0;
        trace->next = context->trace;
        context->trace = trace;
        context->trace_top = 0;
    }
    context->trace->bindings[(context->trace_top)++] = word;
    return 1;
}

/* Pop a word from the trace */
P_INLINE void **p_context_pop_trace(p_context *context, void *marker)
{
    void **word;
    void ***wordp;

    /* Have we reached the marker? */
    if (!context->trace)
        return 0;
    wordp = &(context->trace->bindings[context->trace_top]);
    if (wordp == (void ***)marker)
        return 0;

    /* Pop the word and zero it out so that the garbage collector
     * will forget the reference to the popped word */
    --(context->trace_top);
    word = *(--wordp);
    *wordp = 0;

    /* Free the trace block if it is now empty */
    if (context->trace_top <= 0) {
        struct p_trace *trace = context->trace;
        context->trace = trace->next;
        context->trace_top = P_TRACE_SIZE;
        GC_FREE(trace);
    }
    return word;
}

/**
 * \brief Marks the current position in the backtrack trace
 * in \a context and returns a marker pointer.
 *
 * \ingroup context
 * \sa p_context_backtrace_trace()
 */
void *p_context_mark_trace(p_context *context)
{
    if (context->trace)
        return (void *)&(context->trace[context->trace_top]);
    else
        return 0;
}

/**
 * \brief Backtracks the trace in \a context, undoing variable
 * bindings until \a marker is reached.
 *
 * \ingroup context
 * \sa p_context_mark_trace()
 */
void p_context_backtrack_trace(p_context *context, void *marker)
{
    void **word;
    while ((word = p_context_pop_trace(context, marker)) != 0) {
        if (!(((long)word) & 1L)) {
            /* Reset a regular variable to unbound */
            *word = 0;
        } else {
            /* Restore a previous value from before an assignment */
            void *value = (void *)p_context_pop_trace(context, marker);
            word = (void **)(((long)word) & ~1L);
            *word = value;
        }
    }
}

int _p_context_record_in_trace(p_context *context, p_term *var)
{
    return p_context_push_trace(context, (void **)&(var->var.value));
}

int _p_context_record_contents_in_trace(p_context *context, void **location)
{
    long loc = ((long)location) | 1L;
    if (!p_context_push_trace(context, (void **)(*location)))
        return 0;
    if (p_context_push_trace(context, (void **)loc))
        return 1;
    p_context_pop_trace(context, 0);
    return 0;
}

/*\@}*/
