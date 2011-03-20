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
#include "parser-priv.h"
#include <errno.h>
#include <string.h>

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
    context->comma_atom = p_term_create_atom(context, ",");
    context->line_atom = p_term_create_atom(context, "$$line");
    context->if_atom = p_term_create_atom(context, "->");
    context->slash_atom = p_term_create_atom(context, "/");
    context->trace_top = P_TRACE_SIZE;
    _p_db_init(context);
    _p_db_init_builtins(context);
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
        return (void *)&(context->trace->bindings[context->trace_top]);
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

/* Imports from the flex-generated lexer and bison-generated parser */
int p_term_lex_init_extra(p_input_stream *extra, yyscan_t *scanner);
int p_term_lex_destroy(yyscan_t scanner);
int p_term_parse(p_context *context, yyscan_t scanner);

/* Dereference a term and strip out "$$line()" declarations */
P_INLINE p_term *p_term_deref_line(p_context *context, p_term *term)
{
    for (;;) {
        term = p_term_deref(term);
        if (!term || term->header.type != P_TERM_FUNCTOR)
            break;
        if (term->functor.functor_name != context->line_atom)
            break;
        if (term->header.size != 3)
            break;
        term = p_term_arg(term, 2);
    }
    return term;
}

static int p_context_consult(p_context *context, p_input_stream *stream)
{
    yyscan_t scanner;
    int error, ok;

    /* Initialize the lexer */
    if (p_term_lex_init_extra(stream, &scanner) != 0) {
        error = errno;
        if (stream->close_stream)
            fclose(stream->stream);
        return error;
    }

    /* Parse and evaluate the contents of the input stream */
    ok = (p_term_parse(context, scanner) == 0);
    if (stream->error_count != 0)
        ok = 0;

    /* Close the input stream */
    if (stream->variables)
        GC_FREE(stream->variables);
    if (stream->close_stream)
        fclose(stream->stream);
    p_term_lex_destroy(scanner);

    /* Process the declarations from the file */
    if (ok && stream->declarations) {
        p_term *list = stream->declarations;
        p_term *clause_atom = context->clause_atom;
        p_term *goal_atom = p_term_create_atom(context, "?-");
        p_term *test_goal_atom = p_term_create_atom(context, "\?\?--");
        p_term *decl;
        while (list->header.type == P_TERM_LIST) {
            decl = p_term_deref_line(context, list->list.head);
            if (decl && decl->header.type == P_TERM_FUNCTOR) {
                if (decl->functor.functor_name == clause_atom) {
                    /* TODO */
                } else if (decl->functor.functor_name == goal_atom) {
                    /* TODO */
                } else if (decl->functor.functor_name == test_goal_atom) {
                    /* Special goal that is used by unit tests.
                     * Ignored if unit testing is not active */
                    if (context->allow_test_goals)
                        context->test_goal = p_term_arg(decl, 0);
                }
            }
            list = list->list.tail;
        }
    }
    return ok ? 0 : EINVAL;
}

/**
 * \brief Loads and consults the contents of \a filename as
 * predicates and directives to be executed within \a context.
 *
 * Returns zero if the file was successfully consulted, or an
 * errno code otherwise.  EINVAL indicates that the contents of
 * \a filename could not be completely parsed.  Other errno codes
 * indicate errors in opening or reading from \a filename.
 *
 * The special \a filename \c - can be used to read from
 * standard input.
 *
 * \ingroup context
 * \sa p_context_consult_string()
 */
int p_context_consult_file(p_context *context, const char *filename)
{
    p_input_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (!strcmp(filename, "-")) {
        stream.stream = stdin;
        stream.filename = "(standard-input)";
        stream.close_stream = 0;
    } else {
        stream.stream = fopen(filename, "r");
        if (!stream.stream)
            return errno;
        stream.filename = filename;
        stream.close_stream = 1;
    }
    return p_context_consult(context, &stream);
}

/**
 * \brief Loads and consults the contents of \a str as
 * predicates and directives to be executed within \a context.
 *
 * Returns zero if the contents of \a str were successfully
 * consulted, or an errno code otherwise.  EINVAL indicates that
 * the contents of \a str could not be completely parsed.
 *
 * This function is intended for parsing small snippets of source
 * code that have been embedded in a larger native application.
 * Use p_context_consult_file() for parsing external files.
 *
 * \ingroup context
 * \sa p_context_consult_file()
 */
int p_context_consult_string(p_context *context, const char *str)
{
    p_input_stream stream;
    if (!str)
        return ENOENT;
    memset(&stream, 0, sizeof(stream));
    stream.buffer = str;
    stream.buffer_len = strlen(str);
    return p_context_consult(context, &stream);
}

/**
 * \enum p_goal_result
 * \ingroup context
 * This enum defines the result from a goal or builtin predicate.
 * \sa p_context_execute_goal()
 */

/**
 * \var P_RESULT_FAIL
 * \ingroup context
 * The goal failed and further back-tracking is not possible.
 */

/**
 * \var P_RESULT_TRUE
 * \ingroup context
 * The goal succeeded and further back-tracking may be possible.
 */

/**
 * \var P_RESULT_CUT_FAIL
 * \ingroup context
 * The goal failed but it contained a \ref cut_0 "!/0" declaration
 * to reduce further back-tracking at the caller's level.
 */

/**
 * \var P_RESULT_CUT_TRUE
 * \ingroup context
 * The goal succeeded but it contained a \ref cut_0 "!/0" declaration
 * to reduce further back-tracking at the caller's level.
 */

/**
 * \var P_RESULT_ERROR
 * \ingroup context
 * The goal resulted in a thrown error that has not been caught.
 * Further back-tracking may be possible.
 */

/* Deterministic execution of goals - FIXME: non-determinism */
static p_goal_result p_goal_call_inner(p_context *context, p_term *goal, p_term **error)
{
    p_goal_result result;
    p_term *pred;
    p_term *name;
    unsigned int arity;
    p_db_builtin builtin;
    p_database_info *info;
    void *marker;

    /* Bail out if the goal is a variable */
    goal = p_term_deref_line(context, goal);
    if (!goal || (goal->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_term_create_atom(context, "instantiation_error");
        return P_RESULT_ERROR;
    }

    /* Get the name and arity of the predicate to be called */
    if (goal->header.type == P_TERM_ATOM) {
        name = goal;
        arity = 0;
    } else if (goal->header.type == P_TERM_FUNCTOR) {
        /* Handle comma terms, assumed to be right-recursive */
        if (goal->header.size == 2 &&
                goal->functor.functor_name == context->comma_atom) {
            int have_cut = 0;
            do {
                result = p_goal_call(context, goal->functor.arg[0], error);
                if (result == P_RESULT_CUT_TRUE)
                    have_cut = 1;
                else if (result == P_RESULT_FAIL && have_cut)
                    return P_RESULT_CUT_FAIL;
                else if (result != P_RESULT_TRUE)
                    return result;
                goal = p_term_deref(goal->functor.arg[1]);
                if (!goal || goal->header.type != P_TERM_FUNCTOR)
                    break;
                if (goal->header.size != 2)
                    break;
            } while (goal->functor.functor_name == context->comma_atom);
            result = p_goal_call(context, goal, error);
            if (have_cut) {
                if (result == P_RESULT_TRUE)
                    result = P_RESULT_CUT_TRUE;
                else if (result == P_RESULT_FAIL)
                    result = P_RESULT_CUT_FAIL;
            }
            return result;
        }
        name = goal->functor.functor_name;
        arity = goal->header.size;
    } else {
        /* Not an atom or functor, so not a callable term */
        *error = p_term_create_functor
            (context, p_term_create_atom(context, "type_error"), 2);
        p_term_bind_functor_arg
            (*error, 0, p_term_create_atom(context, "callable"));
        p_term_bind_functor_arg(*error, 1, p_term_clone(context, goal));
        return P_RESULT_ERROR;
    }

    /* Find a builtin to handle the functor */
    info = name->atom.db_info;
    while (info && info->arity != arity)
        info = info->next;
    if (info && (builtin = info->builtin_func) != 0) {
        if (arity != 0)
            return (*builtin)(context, goal->functor.arg, error);
        else
            return (*builtin)(context, 0, error);
    }

    /* Look for a user-defined predicate to handle the functor */
    if (info && info->clauses_head) {
        p_term *clause_list = info->clauses_head;
        p_term *body;
        while (clause_list != 0) {
            /* Try each clause in turn and commit to the one that
             * succeeds, throws an error, or fails with cut.
             * TODO: non-determinism and backtracking */
            marker = p_context_mark_trace(context);
            body = p_term_unify_clause
                (context, goal, clause_list->list.head);
            if (body) {
                if (body == P_TERM_TRUE_BODY)
                    return P_RESULT_TRUE;
                result = p_goal_call(context, body, error);
                if (result == P_RESULT_TRUE ||
                        result == P_RESULT_CUT_TRUE)
                    return P_RESULT_TRUE;
                else if (result == P_RESULT_CUT_FAIL)
                    return P_RESULT_FAIL;
                else if (result == P_RESULT_ERROR)
                    return P_RESULT_ERROR;
                p_context_backtrack_trace(context, marker);
            }
            clause_list = clause_list->list.tail;
        }
    }

    /* The predicate does not exist - throw an error or fail */
    if (context->fail_on_unknown)
        return P_RESULT_FAIL;
    *error = p_term_create_functor
        (context, p_term_create_atom(context, "existence_error"), 2);
    p_term_bind_functor_arg
        (*error, 0, p_term_create_atom(context, "procedure"));
    pred = p_term_create_functor
        (context, p_term_create_atom(context, "/"), 2);
    p_term_bind_functor_arg(pred, 0, name);
    p_term_bind_functor_arg
        (pred, 1, p_term_create_integer(context, (int)arity));
    p_term_bind_functor_arg(*error, 1, pred);
    return P_RESULT_ERROR;
}
p_goal_result p_goal_call(p_context *context, p_term *goal, p_term **error)
{
    void *marker = p_context_mark_trace(context);
    p_goal_result result = p_goal_call_inner(context, goal, error);
    if (result != P_RESULT_TRUE && result != P_RESULT_CUT_TRUE)
        p_context_backtrack_trace(context, marker);
    return result;
}

/**
 * \brief Executes \a goal against the current database state
 * of \a context.
 *
 * Returns a goal status of P_RESULT_FAIL, P_RESULT_TRUE, or
 * P_RESULT_ERROR.  The previous goal, if any, will be abandoned
 * before execution of \a goal starts.
 *
 * If \a error is not null, then it will be set to the error
 * term for P_RESULT_ERROR.
 *
 * \ingroup context
 * \sa p_context_reexecute_goal(), p_context_abandon_goal()
 */
p_goal_result p_context_execute_goal
    (p_context *context, p_term *goal, p_term **error)
{
    p_term *error_term = 0;
    p_goal_result result;
    p_context_abandon_goal(context);
    context->goal_active = 1;
    context->goal = goal;
    context->goal_marker = p_context_mark_trace(context);
    result = p_goal_call(context, goal, &error_term);
    if (error)
        *error = error_term;
    if (result == P_RESULT_CUT_TRUE)
        result = P_RESULT_TRUE;
    else if (result == P_RESULT_CUT_FAIL)
        result = P_RESULT_FAIL;
    return result;
}

/**
 * \brief Re-executes the current goal on \a context by forcing a
 * back-track to find a new solution.
 *
 * Returns a goal status of P_RESULT_FAIL, P_RESULT_TRUE, or
 * P_RESULT_ERROR reporting the status of the new solution found.
 * Once P_RESULT_FAIL is reported, no further solutions are possible.
 *
 * If \a error is not null, then it will be set to the error
 * term for P_RESULT_ERROR.
 *
 * \ingroup context
 * \sa p_context_execute_goal(), p_context_abandon_goal()
 */
p_goal_result p_context_reexecute_goal(p_context *context, p_term **error)
{
    /* TODO */
    if (error)
        *error = 0;
    return P_RESULT_FAIL;
}

/**
 * \brief Abandons the current goal on \a context.
 *
 * All variable bindings that were made as part of the current goal
 * are removed.  The \a context returns to its original conditions,
 * except for any side-effects that were performed by the goal.
 *
 * \ingroup context
 * \sa p_context_execute_goal(), p_context_reexecute_goal()
 * \sa p_context_uncaught_error()
 */
void p_context_abandon_goal(p_context *context)
{
    if (context->goal_active) {
        p_context_backtrack_trace(context, context->goal_marker);
        context->goal_active = 0;
        context->goal = 0;
        context->goal_marker = 0;
    }
}

/* Used by the unit test framework, not part of the normal API */
p_term *_p_context_test_goal(p_context *context)
{
    p_term *goal = context->test_goal;
    context->allow_test_goals = 1;
    context->test_goal = 0;
    return goal;
}

/**
 * \brief Returns the current debug state for \a context.
 *
 * \ingroup context
 * \sa p_context_set_debug()
 */
int p_context_is_debug(p_context *context)
{
    return context->debug ? 1 : 0;
}

/**
 * \brief Sets the current \a debug state for \a context.
 *
 * \ingroup context
 * \sa p_context_is_debug()
 */
void p_context_set_debug(p_context *context, int debug)
{
    context->debug = debug;
}

/*\@}*/
