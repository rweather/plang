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
#include <plang/errors.h>
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
    context->dot_atom = p_term_create_atom(context, ".");
    context->clause_atom = p_term_create_atom(context, ":-");
    context->comma_atom = p_term_create_atom(context, ",");
    context->line_atom = p_term_create_atom(context, "$$line");
    context->if_atom = p_term_create_atom(context, "->");
    context->slash_atom = p_term_create_atom(context, "/");
    context->true_atom = p_term_create_atom(context, "true");
    context->fail_atom = p_term_create_atom(context, "fail");
    context->cut_atom = p_term_create_atom(context, "!");
    context->call_member_atom = p_term_create_atom(context, "$$call_member");
    context->trace_top = P_TRACE_SIZE;
    _p_db_init(context);
    _p_db_init_builtins(context);
    _p_db_init_arith(context);
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
            decl = p_term_deref(list->list.head);
            if (decl && decl->header.type == P_TERM_FUNCTOR &&
                    decl->header.size == 3 &&
                    decl->functor.functor_name == context->line_atom)
                decl = p_term_deref(p_term_arg(decl, 2));
            if (decl && decl->header.type == P_TERM_FUNCTOR) {
                if (decl->functor.functor_name == clause_atom) {
                    /* TODO: error reporting */
                    p_db_clause_assert_last(context, decl);
                } else if (decl->functor.functor_name == goal_atom) {
                    /* Execute the initialization goal */
                    p_goal_call_from_parser
                        (context, p_term_arg(decl, 0));
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
 * If \a filename has already been loaded into \a context
 * previously, then this function does nothing and returns zero.
 *
 * \ingroup context
 * \sa p_context_consult_string(), p_context_add_import_path()
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
        size_t index;
        for (index = 0; index < context->loaded_files.num_paths; ++index) {
            if (!strcmp(filename, context->loaded_files.paths[index]))
                return 0;
        }
        stream.stream = fopen(filename, "r");
        if (!stream.stream)
            return errno;
        stream.filename = filename;
        stream.close_stream = 1;
        p_context_add_path(context->loaded_files, filename);
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
 * \var P_RESULT_ERROR
 * \ingroup context
 * The goal resulted in a thrown error that has not been caught.
 * Further back-tracking may be possible.  The error term should
 * be generated with one of the functions in the
 * \ref errors "Error Creation" module.
 */

/**
 * \var P_RESULT_HALT
 * \ingroup context
 * The goal resulted in execution of a \ref halt_0 "halt/0" or
 * \ref halt_1 "halt/1" subgoal.  Execution should wind back
 * to the top-level goal and return control to the caller of
 * p_context_execute_goal().
 */

/* Inner execution of goals - performs an operation deterministically
 * or modifies the search tree according to the control predicate */
static p_goal_result p_goal_execute_inner(p_context *context, p_term *goal, p_term **error)
{
    p_term *pred;
    p_term *name;
    unsigned int arity;
    p_db_builtin builtin;
    p_database_info *info;
    p_exec_node *current;
    p_exec_node *next;

    /* Bail out if the goal is a variable */
    goal = p_term_deref(goal);
    if (!goal || (goal->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }

    /* Get the name and arity of the predicate to be called */
    if (goal->header.type == P_TERM_ATOM) {
        name = goal;
        arity = 0;
    } else if (goal->header.type == P_TERM_FUNCTOR) {
        if (goal->header.size == 2 &&
                goal->functor.functor_name == context->comma_atom) {
            /* Handle comma terms, which are assumed to be
             * right-recursive.  Replace the current goal with
             * the left-hand part of the comma term, and create a
             * new continuation node for the right-hand part */
            current = context->current_node;
            next = GC_NEW(p_exec_node);
            if (!next)
                return P_RESULT_FAIL;
            next->goal = goal->functor.arg[1];
            next->success_node = current->success_node;
            next->cut_node = current->cut_node;
            next->catch_node = current->catch_node;
            current->goal = goal->functor.arg[0];
            current->success_node = next;
            return P_RESULT_TREE_CHANGE;
        }
        name = goal->functor.functor_name;
        arity = goal->header.size;
    } else {
        /* Not an atom or functor, so not a callable term */
        *error = p_create_type_error(context, "callable", goal);
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
    if (info && info->predicate) {
        p_term *clause_list = info->predicate->predicate.clauses_head;
        p_term *body;
        while (clause_list != 0) {
            /* Find the first clause whose head unifies with the goal */
            body = p_term_unify_clause
                (context, goal, clause_list->list.head);
            if (body) {
                current = context->current_node;
                clause_list = clause_list->list.tail;
                if (clause_list) {
                    next = GC_NEW(p_exec_node);
                    if (!next)
                        return P_RESULT_FAIL;
                    next->goal = current->goal;
                    next->next_clause = clause_list;
                    next->success_node = current->success_node;
                    next->cut_node = context->fail_node;
                    next->catch_node = current->catch_node;
                    next->fail_marker = current->fail_marker;
                    current->goal = body;
                    current->cut_node = context->fail_node;
                    context->fail_node = next;
                } else {
                    current->goal = body;
                    current->cut_node = context->fail_node;
                }
                return P_RESULT_TREE_CHANGE;
            }
            clause_list = clause_list->list.tail;
        }
        return P_RESULT_FAIL;
    }

    /* The predicate does not exist - throw an error or fail */
    if (context->fail_on_unknown)
        return P_RESULT_FAIL;
    pred = p_term_create_functor(context, context->slash_atom, 2);
    p_term_bind_functor_arg(pred, 0, name);
    p_term_bind_functor_arg
        (pred, 1, p_term_create_integer(context, (int)arity));
    *error = p_create_existence_error(context, "procedure", pred);
    return P_RESULT_ERROR;
}

/* Defined in builtins.c */
int p_builtin_handle_catch(p_context *context, p_term *error);

/* Execution of top-level goals */
static p_goal_result p_goal_execute(p_context *context, p_term **error)
{
    p_term *goal;
    p_goal_result result = P_RESULT_FAIL;
    p_exec_node *current;

    for (;;) {
        /* Fetch the current goal */
        current = context->current_node;
        if (current->next_clause) {
            /* We have backtracked into a new clause of a predicate.
             * See if the clause, or one of the following clauses
             * matches the current goal.  If no match, then fail */
            p_term *clause_list = current->next_clause;
            p_term *body = 0;
            current->next_clause = 0;
            while (clause_list != 0) {
                body = p_term_unify_clause
                    (context, current->goal, clause_list->list.head);
                if (body)
                    break;
                clause_list = clause_list->list.tail;
            }
            if (body) {
                clause_list = clause_list->list.tail;
                if (clause_list) {
                    p_exec_node *next = GC_NEW(p_exec_node);
                    if (next) {
                        next->goal = current->goal;
                        next->next_clause = clause_list;
                        next->success_node = current->success_node;
                        next->cut_node = current->cut_node;
                        next->catch_node = current->catch_node;
                        next->fail_marker = current->fail_marker;
                        current->goal = body;
                        context->fail_node = next;
                    } else {
                        current->goal = context->fail_atom;
                    }
                } else {
                    current->goal = body;
                }
            } else {
                current->goal = context->fail_atom;
            }
        }
        goal = p_term_deref(current->goal);

        /* Debugging: output the goal details, including next goals */
#ifdef P_GOAL_DEBUG
        {
            p_term_print(context, goal,
                         p_term_stdio_print_func, stdout);
            putc('\n', stdout);
            if (context->current_node->success_node) {
                fputs("\tsuccess: ", stdout);
                p_term_print
                    (context, context->current_node->success_node->goal,
                     p_term_stdio_print_func, stdout);
                putc('\n', stdout);
            } else {
                fputs("\tsuccess: top-level success\n", stdout);
            }
            if (context->fail_node) {
                fputs("\tfail: ", stdout);
                p_term_print
                    (context, context->fail_node->goal,
                     p_term_stdio_print_func, stdout);
                putc('\n', stdout);
            } else {
                fputs("\tfail: top-level fail\n", stdout);
            }
            if (context->current_node->cut_node) {
                fputs("\tcut: ", stdout);
                p_term_print
                    (context, context->current_node->cut_node->goal,
                     p_term_stdio_print_func, stdout);
                putc('\n', stdout);
            } else {
                fputs("\tcut: top-level fail\n", stdout);
            }
            if (context->current_node->catch_node) {
                fputs("\tcatch: ", stdout);
                p_term_print
                    (context, context->current_node->catch_node->goal,
                     p_term_stdio_print_func, stdout);
                putc('\n', stdout);
            }
        }
#endif

        /* Determine what needs to be done next for this goal */
        *error = 0;
        current->fail_marker = p_context_mark_trace(context);
        result = p_goal_execute_inner(context, goal, error);
        if (result == P_RESULT_TRUE) {
            /* Success of deterministic leaf goal */
#ifdef P_GOAL_DEBUG
            fputs("\tresult: true\n", stdout);
#endif
            current = context->current_node;
            context->current_node = current->success_node;
            if (!(context->current_node)) {
                /* Top-level success.  Set the current node to
                 * the fail node for re-executing the goal */
                context->current_node = context->fail_node;
                context->fail_node = 0;
                break;
            }
        } else if (result == P_RESULT_FAIL) {
            /* Failure of deterministic leaf goal */
#ifdef P_GOAL_DEBUG
            fputs("\tresult: fail\n", stdout);
#endif
            context->current_node = context->fail_node;
            if (!(context->current_node))
                break;      /* Final top-level goal failure */
            context->fail_node = context->current_node->cut_node;
            p_context_backtrack_trace
                (context, context->current_node->fail_marker);
        } else if (result == P_RESULT_ERROR) {
            /* Find an enclosing "catch" block to handle the error */
#ifdef P_GOAL_DEBUG
            fputs("\tresult: throw(", stdout);
            p_term_print
                (context, *error, p_term_stdio_print_func, stdout);
            fputs(")\n", stdout);
#endif
            if (!p_builtin_handle_catch(context, *error))
                break;
            *error = 0;
        } else if (result == P_RESULT_HALT) {
            /* Force execution to halt immediately */
#ifdef P_GOAL_DEBUG
            fputs("\tresult: halt(", stdout);
            p_term_print
                (context, *error, p_term_stdio_print_func, stdout);
            fputs(")\n", stdout);
#endif
            break;
        } else {
            /* Assumed to be P_RESULT_TREE_CHANGE, which has
             * already altered the current node */
        }
    }

    return result;
}

/**
 * \brief Executes \a goal against the current database state
 * of \a context.
 *
 * Returns a goal status of P_RESULT_FAIL, P_RESULT_TRUE,
 * P_RESULT_ERROR, or P_RESULT_HALT.  The previous goal, if any,
 * will be abandoned before execution of \a goal starts.
 *
 * If \a error is not null, then it will be set to the error
 * term for P_RESULT_ERROR.
 *
 * If the return value is P_RESULT_HALT, then \a error will be
 * set to an integer term corresponding to the requested exit value.
 *
 * \ingroup context
 * \sa p_context_reexecute_goal(), p_context_abandon_goal(),
 * \ref execution_model "Plang execution model"
 */
p_goal_result p_context_execute_goal
    (p_context *context, p_term *goal, p_term **error)
{
    p_term *error_term = 0;
    p_goal_result result;
    p_context_abandon_goal(context);
#ifdef P_GOAL_DEBUG
    fputs("top-level goal: ", stdout);
    p_term_print(context, goal, p_term_stdio_print_func, stdout);
    putc('\n', stdout);
#endif
    context->current_node = GC_NEW(p_exec_node);
    if (!context->current_node)
        return P_RESULT_FAIL;
    context->current_node->goal = goal;
    context->fail_node = 0;
    context->goal_active = 1;
    context->goal_marker = p_context_mark_trace(context);
    result = p_goal_execute(context, &error_term);
    if (error)
        *error = error_term;
    if (result != P_RESULT_TRUE) {
        context->current_node = 0;
        context->fail_node = 0;
    }
    return result;
}

/**
 * \brief Re-executes the current goal on \a context by forcing a
 * back-track to find a new solution.
 *
 * Returns a goal status of P_RESULT_FAIL, P_RESULT_TRUE,
 * P_RESULT_ERROR, or P_RESULT_HALT reporting the status of the
 * new solution found.  If P_RESULT_TRUE is returned, then further
 * solutions are possible.
 *
 * If \a error is not null, then it will be set to the error
 * term for P_RESULT_ERROR.
 *
 * If the return value is P_RESULT_HALT, then \a error will be
 * set to an integer term corresponding to the requested exit value.
 *
 * \ingroup context
 * \sa p_context_execute_goal(), p_context_abandon_goal(),
 * \ref execution_model "Plang execution model"
 */
p_goal_result p_context_reexecute_goal(p_context *context, p_term **error)
{
    p_term *error_term = 0;
    p_goal_result result;
    if (!context->current_node)
        return P_RESULT_FAIL;
    p_context_backtrack_trace
        (context, context->current_node->fail_marker);
    result = p_goal_execute(context, &error_term);
    if (error)
        *error = error_term;
    if (result != P_RESULT_TRUE) {
        context->current_node = 0;
        context->fail_node = 0;
    }
    return result;
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
        context->goal_marker = 0;
        context->current_node = 0;
        context->fail_node = 0;
    }
}

/* Calls a goal from the parser for immediate execution.
 * After execution, backtracks to the initial state */
void p_goal_call_from_parser(p_context *context, p_term *goal)
{
    p_term *error = 0;
    void *marker = p_context_mark_trace(context);
    p_goal_result result;
    p_exec_node *current = context->current_node;
    p_exec_node *fail = context->fail_node;
    p_exec_node *goal_node = GC_NEW(p_exec_node);
    if (goal_node) {
        goal_node->goal = goal;
        context->current_node = goal_node;
        context->fail_node = 0;
        result = p_goal_execute(context, &error);
        context->current_node = current;
        context->fail_node = fail;
    } else {
        result = P_RESULT_FAIL;
    }
    p_context_backtrack_trace(context, marker);
    if (result == P_RESULT_TRUE)
        return;
    goal = p_term_deref(goal);
    if (goal && goal->header.type == P_TERM_FUNCTOR &&
            goal->header.size == 3 &&
            goal->functor.functor_name == context->line_atom) {
        p_term_print_unquoted
            (context, p_term_arg(goal, 0),
             p_term_stdio_print_func, stderr);
        putc(':', stderr);
        p_term_print_unquoted
            (context, p_term_arg(goal, 1),
             p_term_stdio_print_func, stderr);
        putc(':', stderr);
        putc(' ', stderr);
        p_term_print
            (context, p_term_arg(goal, 2),
             p_term_stdio_print_func, stderr);
    } else {
        p_term_print(context, goal, p_term_stdio_print_func, stderr);
    }
    if (result == P_RESULT_ERROR) {
        fputs(": uncaught error: ", stderr);
        p_term_print(context, error, p_term_stdio_print_func, stderr);
        putc('\n', stderr);
    } else if (result == P_RESULT_HALT) {
        fputs(": ignoring halt during directive\n", stderr);
    } else {
        fputs(": fail\n", stderr);
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

/**
 * \brief Adds \a path to \a context as a directory to search for
 * source files imported by \ref import_1 "import/1".
 *
 * \ingroup context
 * \sa p_context_consult_file()
 */
void p_context_add_import_path(p_context *context, const char *path)
{
    p_context_add_path(context->user_imports, path);
}

/*\@}*/
