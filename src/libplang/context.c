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
#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_LIBDL) && defined(HAVE_DLFCN_H) && defined(HAVE_DLOPEN)
#include <unistd.h>
#include <dlfcn.h>
#define P_HAVE_DLOPEN 1
#endif

/**
 * \defgroup context Native C API - Execution Contexts
 */
/*\@{*/

static void p_context_find_system_imports(p_context *context);

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
    context->in_atom = p_term_create_atom(context, "in");
    context->slash_atom = p_term_create_atom(context, "/");
    context->true_atom = p_term_create_atom(context, "true");
    context->fail_atom = p_term_create_atom(context, "fail");
    context->cut_atom = p_term_create_atom(context, "!");
    context->call_member_atom = p_term_create_atom(context, "$$call_member");
    context->call_args_atom = p_term_create_atom(context, "$$");
    context->unify_atom = p_term_create_atom(context, "=");
    context->trail_top = P_TRACE_SIZE;
    context->confidence = 1.0;
    _p_db_init(context);
    _p_db_init_builtins(context);
    _p_db_init_arith(context);
    _p_db_init_io(context);
    _p_db_init_fuzzy(context);
    _p_db_init_sort(context);
    p_context_find_system_imports(context);
    return context;
}

static void p_context_free_libraries(p_context *context);

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
    p_context_free_libraries(context);
    GC_FREE(context);
}

/* Push a word onto the trail */
P_INLINE int p_context_push_trail(p_context *context, void **word)
{
    if (context->trail_top >= P_TRACE_SIZE) {
        struct p_trail *trail = GC_NEW(struct p_trail);
        if (!trail)
            return 0;
        trail->next = context->trail;
        context->trail = trail;
        context->trail_top = 0;
    }
    context->trail->bindings[(context->trail_top)++] = word;
    return 1;
}

/* Pop a word from the trail */
P_INLINE void **p_context_pop_trail(p_context *context, void *marker)
{
    void **word;
    void ***wordp;

    /* Have we reached the marker? */
    if (!context->trail)
        return 0;
    wordp = &(context->trail->bindings[context->trail_top]);
    if (wordp == (void ***)marker)
        return 0;

    /* Pop the word and zero it out so that the garbage collector
     * will forget the reference to the popped word */
    --(context->trail_top);
    word = *(--wordp);
    *wordp = 0;

    /* Free the trail block if it is now empty */
    if (context->trail_top <= 0) {
        struct p_trail *trail = context->trail;
        context->trail = trail->next;
        context->trail_top = P_TRACE_SIZE;
        GC_FREE(trail);
    }
    return word;
}

/**
 * \brief Marks the current position in the backtrack trail
 * in \a context and returns a marker pointer.
 *
 * \ingroup context
 * \sa p_context_backtrack_trail()
 */
void *p_context_mark_trail(p_context *context)
{
    if (context->trail)
        return (void *)&(context->trail->bindings[context->trail_top]);
    else
        return 0;
}

/**
 * \brief Backtracks the trail in \a context, undoing variable
 * bindings until \a marker is reached.
 *
 * \ingroup context
 * \sa p_context_mark_trail()
 */
void p_context_backtrack_trail(p_context *context, void *marker)
{
    void **word;
    while ((word = p_context_pop_trail(context, marker)) != 0) {
        if (!(((long)word) & 1L)) {
            /* Reset a regular variable to unbound */
            *word = 0;
        } else {
            /* Restore a previous value from before an assignment */
            void *value = (void *)p_context_pop_trail(context, marker);
            word = (void **)(((long)word) & ~1L);
            *word = value;
        }
    }
}

int _p_context_record_in_trail(p_context *context, p_term *var)
{
    return p_context_push_trail(context, (void **)&(var->var.value));
}

int _p_context_record_contents_in_trail(p_context *context, void **location, void *prev_value)
{
    long loc = ((long)location) | 1L;
    if (!p_context_push_trail(context, (void **)prev_value))
        return 0;
    if (p_context_push_trail(context, (void **)loc))
        return 1;
    p_context_pop_trail(context, 0);
    return 0;
}

/* Imports from the flex-generated lexer and bison-generated parser */
int p_term_lex_init_extra(p_input_stream *extra, yyscan_t *scanner);
int p_term_lex_destroy(yyscan_t scanner);
int p_term_parse(p_context *context, yyscan_t scanner);

int p_context_consult(p_context *context, p_input_stream *stream)
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

    /* Create a variable list if requested by "iostream::readTerm()" */
    if (stream->generate_vars) {
        size_t index;
        p_term *tail = 0;
        p_term *new_tail;
        for (index = 0; index < stream->num_variables; ++index) {
            p_term *head = p_term_create_functor
                (context, context->unify_atom, 2);
            p_term_bind_functor_arg
                (head, 0, stream->variables[index].name);
            p_term_bind_functor_arg
                (head, 1, stream->variables[index].var);
            new_tail = p_term_create_list(context, head, 0);
            if (tail)
                p_term_set_tail(tail, new_tail);
            else
                stream->vars = new_tail;
            tail = new_tail;
        }
        if (tail)
            p_term_set_tail(tail, context->nil_atom);
        else
            stream->vars = context->nil_atom;
    }

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
        p_term *read_atom = p_term_create_atom(context, "\?\?-");
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
                    if (p_goal_call_from_parser
                            (context, p_term_arg(decl, 0)) != P_RESULT_TRUE)
                        ok = 0;
                } else if (decl->functor.functor_name == test_goal_atom) {
                    /* Special goal that is used by unit tests.
                     * Ignored if unit testing is not active */
                    if (context->allow_test_goals)
                        context->test_goal = p_term_arg(decl, 0);
                } else if (decl->functor.functor_name == read_atom) {
                    stream->read_term = p_term_arg(decl, 0);
                }
            }
            list = list->list.tail;
        }
    }
    return ok ? 0 : EINVAL;
}

static int p_stdio_read_func
    (p_input_stream *stream, char *buf, size_t max_size)
{
    size_t result;
    errno = 0;
    while ((result = fread(buf, 1, max_size, stream->stream)) == 0
                && ferror(stream->stream)) {
        if (errno != EINTR)
            break;
        errno = 0;
        clearerr(stream->stream);
    }
    return (int)result;
}

/**
 * \enum p_consult_option
 * \ingroup context
 * This enum defines options for p_context_consult_file().
 */

/**
 * \var P_CONSULT_DEFAULT
 * \ingroup context
 * Default options.  The file will be loaded again every time
 * it is consulted.  This is used by the \ref consult_1 "consult/1"
 * directive.
 */

/**
 * \var P_CONSULT_ONCE
 * \ingroup context
 * The file will only be consulted once.  Subsequent attempts to
 * consult the file will quietly succeed.  This is used by
 * the \ref import_1 "import/1" directive.
 */

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
 * If \a option is P_CONSULT_ONCE and \a filename has already been
 * loaded into \a context previously, then this function does
 * nothing and returns zero.
 *
 * \ingroup context
 * \sa p_context_consult_string(), p_context_add_import_path()
 */
int p_context_consult_file
    (p_context *context, const char *filename, p_consult_option option)
{
    p_input_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.context = context;
    stream.read_func = p_stdio_read_func;
    if (!strcmp(filename, "-")) {
        stream.stream = stdin;
        stream.filename = "(standard-input)";
        stream.close_stream = 0;
    } else {
        if (option == P_CONSULT_ONCE) {
            size_t index;
            for (index = 0; index < context->loaded_files.num_paths; ++index) {
                if (!strcmp(filename, context->loaded_files.paths[index]))
                    return 0;
            }
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

int p_string_read_func(p_input_stream *stream, char *buf, size_t max_size)
{
    size_t len = max_size;
    if (len > stream->buffer_len)
        len = stream->buffer_len;
    if (len > 0) {
        memcpy(buf, stream->buffer, len);
        stream->buffer += len;
        stream->buffer_len -= len;
    }
    return (int)len;
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
    stream.context = context;
    stream.buffer = str;
    stream.buffer_len = strlen(str);
    stream.read_func = p_string_read_func;
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

/* Basic fail handling function which only unwinds the trail */
void _p_context_basic_fail_func
    (p_context *context, p_exec_fail_node *node)
{
    p_context_backtrack_trail(context, node->fail_marker);
    context->confidence = node->confidence;
    context->catch_node = node->catch_node;
}

/* Fail handling function for going to the next clause of a
 * dynamic predicate */
void _p_context_clause_fail_func
    (p_context *context, p_exec_fail_node *node)
{
    p_exec_clause_node *current = (p_exec_clause_node *)node;
    p_term *clause_list;
    p_term *body;
    p_exec_node *new_current;

    /* Perform the basic backtracking logic */
    _p_context_basic_fail_func(context, node);

    /* We have backtracked into a new clause of a predicate.
     * See if the clause, or one of the following clauses
     * matches the current goal.  If no match, then fail */
    clause_list = current->next_clause;
    body = 0;
    while (clause_list != 0) {
        body = p_term_unify_clause
            (context, current->parent.parent.goal, clause_list->list.head);
        if (body)
            break;
        clause_list = clause_list->list.tail;
    }
    if (body) {
        clause_list = clause_list->list.tail;
        if (clause_list) {
            p_exec_clause_node *next = GC_NEW(p_exec_clause_node);
            if (next) {
                next->parent.parent.goal = current->parent.parent.goal;
                next->parent.parent.success_node = current->parent.parent.success_node;
                next->parent.parent.cut_node = current->parent.parent.cut_node;
                _p_context_init_fail_node
                    (context, &(next->parent), _p_context_clause_fail_func);
                next->parent.fail_marker = current->parent.fail_marker;
                next->next_clause = clause_list;
                context->fail_node = &(next->parent);
            } else {
                body = context->fail_atom;
            }
        }
    } else {
        body = context->fail_atom;
    }
    new_current = GC_NEW(p_exec_node);
    if (new_current) {
        new_current->goal = body;
        new_current->success_node = current->parent.parent.success_node;
        new_current->cut_node = current->parent.parent.cut_node;
        context->current_node = new_current;
    } else {
        current->parent.parent.goal = context->fail_atom;
    }
}

void _p_context_init_fail_node
    (p_context *context, p_exec_fail_node *node,
     p_exec_fail_func fail_func)
{
    node->parent.fail_func = fail_func;
    node->fail_marker = context->fail_marker;
    node->confidence = context->confidence;
    node->catch_node = context->catch_node;
}

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
    p_exec_node *new_current;

    /* Bail out if the goal is a variable.  It is assumed that
     * the goal has already been dereferenced by the caller */
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
             * right-recursive.  Create two new nodes for the
             * left and right parts of the comma term */
            current = context->current_node;
            next = GC_NEW(p_exec_node);
            new_current = GC_NEW(p_exec_node);
            if (!next || !new_current)
                return P_RESULT_FAIL;
            new_current->goal = goal->functor.arg[0];
            new_current->success_node = next;
            new_current->cut_node = current->cut_node;
            next->goal = goal->functor.arg[1];
            next->success_node = current->success_node;
            next->cut_node = current->cut_node;
            context->current_node = new_current;
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
                    p_exec_clause_node *next = GC_NEW(p_exec_clause_node);
                    new_current = GC_NEW(p_exec_node);
                    if (!next || !new_current)
                        return P_RESULT_FAIL;
                    next->parent.parent.goal = current->goal;
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
        goal = p_term_deref_member(context, current->goal);

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
            if (context->catch_node) {
                fputs("\tcatch: ", stdout);
                p_term_print
                    (context, context->catch_node->goal,
                     p_term_stdio_print_func, stdout);
                putc('\n', stdout);
            }
        }
#endif

        /* Determine what needs to be done next for this goal */
        *error = 0;
        context->fail_marker = p_context_mark_trail(context);
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
                context->current_node = &(context->fail_node->parent);
                if (context->current_node)
                    context->fail_node = context->current_node->cut_node;
                else
                    context->fail_node = 0;
                break;
            }
        } else if (result == P_RESULT_FAIL) {
            /* Failure of deterministic leaf goal */
#ifdef P_GOAL_DEBUG
            fputs("\tresult: fail\n", stdout);
#endif
            context->current_node = &(context->fail_node->parent);
            if (!(context->current_node))
                break;      /* Final top-level goal failure */
            context->fail_node = context->current_node->cut_node;
            (*(context->current_node->fail_func))
                (context, (p_exec_fail_node *)(context->current_node));
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
 * p_context_fuzzy, confidence(),
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
    context->catch_node = 0;
    context->confidence = 1.0;
    context->goal_active = 1;
    context->goal_marker = p_context_mark_trail(context);
    result = p_goal_execute(context, &error_term);
    if (error)
        *error = error_term;
    if (result != P_RESULT_TRUE) {
        context->current_node = 0;
        context->fail_node = 0;
        context->confidence = 0.0;
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
 * p_context_fuzzy_confidence(),
 * \ref execution_model "Plang execution model"
 */
p_goal_result p_context_reexecute_goal(p_context *context, p_term **error)
{
    p_term *error_term = 0;
    p_goal_result result;
    if (!context->current_node)
        return P_RESULT_FAIL;
    (*(context->current_node->fail_func))
        (context, (p_exec_fail_node *)(context->current_node));
    result = p_goal_execute(context, &error_term);
    if (error)
        *error = error_term;
    if (result != P_RESULT_TRUE) {
        context->current_node = 0;
        context->fail_node = 0;
        context->confidence = 0.0;
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
 */
void p_context_abandon_goal(p_context *context)
{
    if (context->goal_active) {
        p_context_backtrack_trail(context, context->goal_marker);
        context->goal_active = 0;
        context->goal_marker = 0;
        context->current_node = 0;
        context->fail_node = 0;
        context->catch_node = 0;
        context->confidence = 1.0;
    }
}

/**
 * \brief Returns the fuzzy confidence factor for the last top-level
 * solution that was returned on \a context.
 *
 * The confidence factor is between 0 and 1 and indicates how
 * confident the application is of the solution when it involves
 * \ref fuzzy_logic "fuzzy reasoning".  For example, 0.8 indicates
 * that the application is 80% confident about the returned solution.
 *
 * The value will be 0 if a top-level failure or thrown error has
 * occurred.  The value will be 1 if a top-level success has
 * occurred with normal confidence.  The values will be between
 * 0 and 1 if a top-level success has occurred but the confidence
 * is less than total
 *
 * \ingroup context
 * \sa p_context_execute_goal(), p_context_reexecute_goal(),
 * p_context_set_fuzzy_confidence(), \ref fuzzy_logic "Fuzzy logic".
 */
double p_context_fuzzy_confidence(p_context *context)
{
    return context->confidence;
}

/**
 * \brief Sets the fuzzy confidence factor for \a context to \a value.
 *
 * The confidence factor is between 0 and 1 and indicates how
 * confident the application is of the solution when it involves
 * \ref fuzzy_logic "fuzzy reasoning".
 *
 * The \a value will be clamped to between 0.00001 and 1.  It is
 * not possible to set \a value to 0, as that value should be
 * indicated by P_RESULT_FAIL instead.
 *
 * \ingroup context
 * \sa p_context_fuzzy_confidence(), \ref fuzzy_logic "Fuzzy logic".
 */
void p_context_set_fuzzy_confidence(p_context *context, double value)
{
    if (value < 0.00001)
        value = 0.00001;
    else if (value > 1.0)
        value = 1.0;
    context->confidence = value;
}

/**
 * \brief Calls \a goal once on \a context.  Returns a result code
 * and an optional error term in \a error.
 *
 * This function is intended for calling back from a builtin
 * function into the Plang execution engine.  Back-tracking of
 * the top level of \a goal is not supported.
 *
 * \ingroup context
 * \sa p_context_execute_goal(), p_context_fuzzy_confidence()
 */
p_goal_result p_context_call_once
    (p_context *context, p_term *goal, p_term **error)
{
    p_goal_result result;
    p_exec_node *current = context->current_node;
    p_exec_fail_node *fail = context->fail_node;
    p_exec_catch_node *catch_node = context->catch_node;
    double confidence = context->confidence;
    p_exec_node *goal_node = GC_NEW(p_exec_node);
    p_term *error_node = 0;
    if (goal_node) {
        goal_node->goal = goal;
        context->current_node = goal_node;
        context->fail_node = 0;
        context->catch_node = 0;
        context->confidence = 1.0;
        result = p_goal_execute(context, &error_node);
        if (result == P_RESULT_TRUE && context->confidence != 1.0) {
            // Propagate the goal's fuzzy confidence to the parent.
            if (context->confidence < confidence)
                confidence = context->confidence;
        }
        context->current_node = current;
        context->fail_node = fail;
        context->catch_node = catch_node;
        context->confidence = confidence;
    } else {
        result = P_RESULT_FAIL;
    }
    if (error)
        *error = error_node;
    return result;
}

/* Calls a goal from the parser for immediate execution.
 * After execution, backtracks to the initial state */
p_goal_result p_goal_call_from_parser(p_context *context, p_term *goal)
{
    p_term *error = 0;
    void *marker = p_context_mark_trail(context);
    p_goal_result result;
    p_exec_node *current = context->current_node;
    p_exec_fail_node *fail = context->fail_node;
    p_exec_catch_node *catch_node = context->catch_node;
    double confidence = context->confidence;
    p_exec_node *goal_node = GC_NEW(p_exec_node);
    if (goal_node) {
        goal_node->goal = goal;
        context->current_node = goal_node;
        context->fail_node = 0;
        context->catch_node = 0;
        context->confidence = 1.0;
        result = p_goal_execute(context, &error);
        context->current_node = current;
        context->fail_node = fail;
        context->catch_node = catch_node;
        context->confidence = confidence;
    } else {
        result = P_RESULT_FAIL;
    }
    p_context_backtrack_trail(context, marker);
    if (result == P_RESULT_TRUE)
        return result;
    goal = p_term_deref_member(context, goal);
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
        fputs(": halt during directive\n", stderr);
    } else {
        fputs(": fail\n", stderr);
    }
    return result;
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
 * \sa p_context_consult_file(), p_context_add_library_path()
 */
void p_context_add_import_path(p_context *context, const char *path)
{
    p_context_add_path(context->user_imports, path);
}

/**
 * \brief Adds \a path to \a context as a directory to search for
 * library files loaded by \ref load_library_1 "load_library/1".
 *
 * \ingroup context
 * \sa p_context_add_library_path()
 */
void p_context_add_library_path(p_context *context, const char *path)
{
    p_context_add_path(context->user_libs, path);
}

static p_term *p_create_load_library_error
    (p_context *context, p_term *name, const char *message)
{
    p_term *error = p_term_create_functor
        (context, p_term_create_atom(context, "load_library_error"), 2);
    p_term_bind_functor_arg(error, 0, name);
    p_term_bind_functor_arg
        (error, 1, p_term_create_string(context, message));
    return p_create_generic_error(context, error);
}

#if defined(HAVE_DLOPEN)

static char *p_context_library_path
    (const char *path, const char *prefix,
     const char *base_name, const char *suffix)
{
    char *lib_path = (char *)malloc
        (strlen(path) + 1 + strlen(prefix) + strlen(base_name) + strlen(suffix) + 1);
    if (!lib_path)
        return 0;
    strcpy(lib_path, path);
    strcat(lib_path, "/");
    strcat(lib_path, prefix);
    strcat(lib_path, base_name);
    strcat(lib_path, suffix);
    if (access(lib_path, 0) >= 0)
        return lib_path;
    free(lib_path);
    return 0;
}

#endif

p_goal_result _p_context_load_library(p_context *context, p_term *name, p_term **error)
{
#if defined(HAVE_DLOPEN)
    const char *base_name = p_term_name(name);
    size_t index;
    char *lib_path;
    void *handle;
    p_library_entry_func setup_func;
    p_library_entry_func shutdown_func;
    p_library *library;

    /* Validate the name: must not be empty or contain
     * directory separators */
    if (*base_name == '\0' || strchr(base_name, '/') != 0 ||
            strchr(base_name, '\\') != 0) {
        *error = p_create_type_error(context, "library_name", name);
        return P_RESULT_ERROR;
    }

    /* Search for the library */
    lib_path = 0;
    for (index = 0; !lib_path && index < context->user_libs.num_paths; ++index) {
        lib_path = p_context_library_path
            (context->user_libs.paths[index], "lib", base_name, ".so");
    }
    for (index = 0; !lib_path && index < context->system_libs.num_paths; ++index) {
        lib_path = p_context_library_path
            (context->system_libs.paths[index], "lib", base_name, ".so");
    }
    if (!lib_path) {
        *error = p_create_existence_error(context, "library", name);
        return P_RESULT_ERROR;
    }

    /* Open the library and fetch the Plang entry points */
    dlerror();
    handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        *error = p_create_load_library_error(context, name, dlerror());
        free(lib_path);
        return P_RESULT_ERROR;
    }
    setup_func = (p_library_entry_func)dlsym(handle, "plang_module_setup");
    shutdown_func = (p_library_entry_func)dlsym(handle, "plang_module_shutdown");
    if (!setup_func) {
        *error = p_create_load_library_error
            (context, name, 
             "plang_module_setup() entry point not found");
        free(lib_path);
        return P_RESULT_ERROR;
    }
    free(lib_path);

    /* Initialize the library for this context */
    (*setup_func)(context);

    /* Create a library information block for the context */
    library = GC_NEW(p_library);
    library->handle = handle;
    library->shutdown_func = shutdown_func;
    library->next = context->libraries;
    context->libraries = library;

    /* Library is ready to go */
    return P_RESULT_TRUE;
#else
    *error = p_create_load_library_error
        (context, name, "do not know how to load libraries");
    return P_RESULT_ERROR;
#endif
}

static void p_context_free_libraries(p_context *context)
{
#if defined(HAVE_DLOPEN)
    p_library *library = context->libraries;
    while (library != 0) {
        if (library->shutdown_func)
            (*(library->shutdown_func))(context);
        dlclose(library->handle);
        library = library->next;
    }
#endif
}

/* Find the system import directories */
static void p_context_find_system_imports(p_context *context)
{
#if !defined(P_WIN32)
#if defined(P_SYSTEM_IMPORT_PATH)
    p_context_add_path(context->system_imports, P_SYSTEM_IMPORT_PATH);
#else
    p_context_add_path(context->system_imports, "/usr/local/share/plang/imports");
    p_context_add_path(context->system_imports, "/opt/local/share/plang/imports");
    p_context_add_path(context->system_imports, "/usr/share/plang/imports");
    p_context_add_path(context->system_imports, "/opt/share/plang/imports");
#endif

#if defined(P_SYSTEM_LIB_PATH)
    p_context_add_path(context->system_libs, P_SYSTEM_LIB_PATH);
#else
    p_context_add_path(context->system_libs, "/usr/local/lib/plang");
    p_context_add_path(context->system_libs, "/opt/local/lib/plang");
    p_context_add_path(context->system_libs, "/usr/lib/plang");
    p_context_add_path(context->system_libs, "/opt/lib/plang");
#endif
#else
    /* TODO: Win32 system import path */
#endif
}

/*\@}*/
