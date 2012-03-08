%{
/*
 * plang logic programming language
 * Copyright (C) 2011,2012  Southern Storm Software, Pty Ltd.
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

#include <plang/term.h>
#include <plang/context.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include "parser.h"
#include "term-priv.h"
#include "parser-priv.h"
#include "context-priv.h"

extern int yylex(YYSTYPE *lval, YYLTYPE *loc, p_context *context, yyscan_t yyscanner);
extern p_input_stream *p_term_get_extra(yyscan_t yyscanner);
extern p_term *p_term_lex_create_variable
    (p_context *context, p_input_stream *stream, const char *name);
extern unsigned int p_term_lex_variable_count
    (p_input_stream *stream, p_term *var);

#define input_stream (p_term_get_extra(yyscanner))

static void yyerror(YYLTYPE *loc, p_context *context, yyscan_t yyscanner, const char *msg)
{
    p_input_stream *stream = p_term_get_extra(yyscanner);
    if (stream->filename)
        fprintf(stderr, "%s:%d: %s\n", stream->filename, loc->first_line, msg);
    else
        fprintf(stderr, "%d: %s\n", loc->first_line, msg);
    ++(stream->error_count);
}

static void yyerror_printf(YYLTYPE *loc, p_context *context, p_input_stream *stream, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    if (stream && stream->filename)
        fprintf(stderr, "%s:%d: ", stream->filename, loc->first_line);
    else if (stream)
        fprintf(stderr, "%d: ", loc->first_line);
    vfprintf(stderr, format, va);
    putc('\n', stderr);
    if (stream)
        ++(stream->error_count);
    va_end(va);
}

/* The yylval stack contains pointers to p_term's which will need
 * to be visible to the garbage collector if it is triggered during
 * parse operations */
#define YYMALLOC    GC_MALLOC
#define YYFREE      GC_FREE

/* Construct a unary functor term */
static p_term *make_unary_term(p_context *context, const char *name, p_term *term)
{
    p_term *result = p_term_create_functor
        (context, p_term_create_atom(context, (name)), 1);
    p_term_bind_functor_arg(result, 0, (term));
    return result;
}
#define unary_term(name,term)   make_unary_term(context, (name), (term))

/* Construct a binary functor term */
static p_term *make_binary_term(p_context *context, const char *name, p_term *term1, p_term *term2)
{
    p_term *result = p_term_create_functor
        (context, p_term_create_atom(context, (name)), 2);
    p_term_bind_functor_arg(result, 0, (term1));
    p_term_bind_functor_arg(result, 1, (term2));
    return result;
}
#define binary_term(name,term1,term2) make_binary_term(context, (name), (term1), (term2))

/* Construct a ternary functor term */
static p_term *make_ternary_term(p_context *context, const char *name, p_term *term1, p_term *term2, p_term *term3)
{
    p_term *result = p_term_create_functor
        (context, p_term_create_atom(context, (name)), 3);
    p_term_bind_functor_arg(result, 0, (term1));
    p_term_bind_functor_arg(result, 1, (term2));
    p_term_bind_functor_arg(result, 2, (term3));
    return result;
}
#define ternary_term(name,term1,term2,term3) make_ternary_term(context, (name), (term1), (term2), (term3))

/* Convert the term from left-recursive in the grammar
 * into right-recursive in the constructed tree */
#define append_r_list(dest, src, new_term, oper) \
    do { \
        p_term *term; \
        if (src.r_hole) { \
            term = p_term_create_functor \
                (context, p_term_create_atom(context, oper), 2); \
            p_term_bind_functor_arg(term, 0, src.r_tail); \
            p_term_bind_functor_arg(src.r_hole, 1, term); \
            dest.r_head = src.r_head; \
            dest.r_hole = term; \
            dest.r_tail = new_term; \
        } else { \
            term = p_term_create_functor \
                (context, p_term_create_atom(context, oper), 2); \
            p_term_bind_functor_arg(term, 0, src.r_tail); \
            dest.r_head = term; \
            dest.r_hole = term; \
            dest.r_tail = new_term; \
        } \
    } while (0)
#define finalize_r_list(list) \
    (list.r_hole ? \
        (p_term_bind_functor_arg(list.r_hole, 1, list.r_tail), \
         list.r_head) : list.r_tail)
#define create_r_list(list,term) \
    do { \
        list.r_head = 0; \
        list.r_hole = 0; \
        list.r_tail = term; \
    } while (0)

/* Build a regular list */
#define append_list(dest,src,term) \
    do { \
        dest.l_head = src.l_head; \
        dest.l_tail = p_term_create_list(context, term, 0); \
        p_term_set_tail(src.l_tail, dest.l_tail); \
    } while (0)
#define append_lists(dest,src1,src2) \
    do { \
        if (src1.l_head && src2.l_head) { \
            dest.l_head = src1.l_head; \
            dest.l_tail = src2.l_tail; \
            p_term_set_tail(src1.l_tail, src2.l_head); \
        } else if (src1.l_head) { \
            dest.l_head = src1.l_head; \
            dest.l_tail = src1.l_tail; \
        } else if (src2.l_head) { \
            dest.l_head = src2.l_head; \
            dest.l_tail = src2.l_tail; \
        } else { \
            dest.l_head = 0; \
            dest.l_tail = 0; \
        } \
    } while (0)
#define create_list(list,term) \
    do { \
        list.l_head = p_term_create_list(context, term, 0); \
        list.l_tail = list.l_head; \
    } while (0)
#define create_empty_list(list) \
    do { \
        list.l_head = 0; \
        list.l_tail = 0; \
    } while (0)
#define finalize_list(list) \
    ((list).l_head ? \
        (p_term_set_tail((list).l_tail, p_term_nil_atom(context)), \
         (list).l_head) : p_term_nil_atom(context))
#define finalize_list_tail(list,tail) \
    (p_term_set_tail((list).l_tail, (tail)), (list).l_head)

/* Create a class declaration goal, which is equivalent to:
 * ?- new_class(class_name, parent, vars, clauses). */
static p_term *make_class_declaration
    (p_context *context, p_term *name, p_term *parent,
     p_term *vars, p_term *clauses)
{
    p_term *term;
    term = p_term_create_functor
        (context, p_term_create_atom(context, "new_class"), 4);
    p_term_bind_functor_arg(term, 0, name);
    p_term_bind_functor_arg(term, 1, parent);
    p_term_bind_functor_arg(term, 2, vars);
    p_term_bind_functor_arg(term, 3, clauses);
    return make_unary_term(context, "?-", term);
}

/* Clear the lexer's variable list */
#define clear_variables()   (input_stream->num_variables = 0)

/* Add debug line number information to a term */
static p_term *add_debug_line
    (p_context *context, p_input_stream *stream,
     YYLTYPE *loc, p_term *term)
{
    p_term *line;
    if (!context->debug)
        return term;
    if (!stream->filename_string) {
        stream->filename_string =
            p_term_create_string(context, stream->filename);
    }
    line = p_term_create_functor(context, context->line_atom, 3);
    p_term_bind_functor_arg(line, 0, stream->filename_string);
    p_term_bind_functor_arg
        (line, 1, p_term_create_integer(context, loc->first_line));
    p_term_bind_functor_arg(line, 2, term);
    return line;
}
#define add_debug(loc, term) \
    (add_debug_line(context, input_stream, &(loc), term))

static char *p_concat_strings
    (const char *str1, size_t len1, const char *str2, size_t len2,
     const char *str3, size_t len3, const char *str4, size_t len4)
{
    char *str = (char *)malloc(len1 + len2 + len3 + len4 + 1);
    if (len1 > 0)
        memcpy(str, str1, len1);
    if (len2 > 0)
        memcpy(str + len1, str2, len2);
    if (len3 > 0)
        memcpy(str + len1 + len2, str3, len3);
    if (len4 > 0)
        memcpy(str + len1 + len2 + len3, str4, len4);
    str[len1 + len2 + len3 + len4] = '\0';
    return str;
}

static int p_context_consult_file_in_path
    (p_context *context, const char *pathname, const char *name,
     int has_extn, p_input_stream *stream)
{
    int error;
    char *path;
    size_t len = strlen(pathname);
    const char *sep = 0;
    size_t sep_len = 0;
#if defined(P_WIN32)
    if (len > 0 && pathname[len - 1] != '/' && pathname[len - 1] != '\\') {
        sep = "\\";
        sep_len = 1;
    }
#else
    if (len > 0 && pathname[len - 1] != '/') {
        sep = "/";
        sep_len = 1;
    }
#endif
    if (has_extn) {
        path = p_concat_strings
            (pathname, strlen(pathname), sep, sep_len,
             name, strlen(name), 0, 0);
    } else {
        path = p_concat_strings
            (pathname, strlen(pathname), sep, sep_len,
             name, strlen(name), ".lp", 3);
    }
    error = p_context_consult_file(context, path, P_CONSULT_ONCE);
    if (error == 0) {
        /* File has been loaded successfully */
        free(path);
        return 1;
    } else if (error == EINVAL) {
        /* File exists but contained an error during loading */
        if (stream)
            ++(stream->error_count);
        free(path);
        return 0;
    }
    free(path);
    return -1;
}

static int p_context_import
    (YYLTYPE *loc, p_context *context, p_input_stream *stream,
     const char *name)
{
    const char *parent_filename;
    size_t len, len2;
    int has_extn, has_dir, is_root, error;
    char *path;
    int result;

    /* Find the directory that contains the parent source file */
    parent_filename = (stream ? stream->filename : 0);
    len = parent_filename ? strlen(parent_filename) : 0;
    while (len > 0) {
#if defined(P_WIN32)
        if (parent_filename[len - 1] == '/' ||
                parent_filename[len - 1] == '\\')
            break;
#else
        if (parent_filename[len - 1] == '/')
            break;
#endif
        --len;
    }
    if (!len) {
#if defined(P_WIN32)
        parent_filename = ".\\";
#else
        parent_filename = "./";
#endif
        len = 2;
    }

    /* Does the name need to have ".lp" appended to it?
     * Also determine if this is a real filename or abbreviated */
    len2 = strlen(name);
    if (!len2) {
        yyerror_printf(loc, context, stream, "empty import name");
        return 0;
    }
    has_extn = 0;
    while (len2 > 0) {
#if defined(P_WIN32)
        if (name[len2 - 1] == '/' || name[len2 - 1] == '\\')
            break;
#else
        if (name[len2 - 1] == '/')
            break;
#endif
        if (name[len2 - 1] == '.')
            has_extn = 1;
        --len2;
    }
    has_dir = (len2 > 0);

    /* Does the name start at the root directory, or is it relative? */
    is_root = 0;
    if (name[0] == '/') {
        is_root = 1;
#if defined(P_WIN32)
    } else if (name[0] == '\\') {
        is_root = 1;
    } else if (((name[0] >= 'a' && name[0] <= 'z') ||
                (name[0] >= 'A' && name[0] <= 'Z'))) {
        if (name[1] == ':' && (name[2] == '/' || name[2] == '\\'))
            is_root = 1;
#endif
    }

    /* Try the parent-relative filename first */
    if (is_root) {
        if (has_extn)
            path = p_concat_strings(name, strlen(name), 0, 0, 0, 0, 0, 0);
        else
            path = p_concat_strings(name, strlen(name), ".lp", 3, 0, 0, 0, 0);
    } else {
        if (has_extn) {
            path = p_concat_strings
                (parent_filename, len, name, strlen(name), 0, 0, 0, 0);
        } else {
            path = p_concat_strings
                (parent_filename, len, name, strlen(name), ".lp", 3, 0, 0);
        }
    }
    error = p_context_consult_file(context, path, P_CONSULT_ONCE);
    if (error == 0) {
        /* File has been loaded successfully */
        free(path);
        return 1;
    } else if (error == EINVAL) {
        /* File exists but contained an error during loading */
        if (stream)
            ++(stream->error_count);
        free(path);
        return 0;
    }
    free(path);

    /* Search the user and system import paths */
    if (!has_dir) {
        size_t index;
        for (index = 0; index < context->user_imports.num_paths; ++index) {
            result = p_context_consult_file_in_path
                (context, context->user_imports.paths[index],
                 name, has_extn, stream);
            if (result >= 0)
                return result;
        }
        for (index = 0; index < context->system_imports.num_paths; ++index) {
            result = p_context_consult_file_in_path
                (context, context->system_imports.paths[index],
                 name, has_extn, stream);
            if (result >= 0)
                return result;
        }
    }

    /* Cannot find the file to be imported */
    return -1;
}

int p_context_builtin_import(p_context *context, const char *name)
{
    return p_context_import(0, context, 0, name);
}

/* Create the head part of a class member clause */
static p_term *create_clause_head
    (p_context *context, p_input_stream *stream,
     p_term *member_name, p_term **args, size_t num_args,
     int is_static)
{
    size_t index;
    p_term *head = p_term_create_functor
        (context, p_term_create_member_name
            (context, stream->class_name, member_name),
         (int)(num_args + (is_static ? 0 : 1)));
    if (is_static) {
        for (index = 0; index < num_args; ++index)
            p_term_bind_functor_arg(head, index, args[index]);
    } else {
        p_term *self = p_term_lex_create_variable
            (context, stream, "Self");
        p_term_bind_functor_arg(head, 0, self);
        for (index = 0; index < num_args; ++index)
            p_term_bind_functor_arg(head, index + 1, args[index]);
    }
    if (args)
        GC_FREE(args);
    return head;
}

%}

/* Bison options */
%name-prefix="p_term_"
%pure-parser
%error-verbose
%locations
%parse-param {p_context *context}
%parse-param {yyscan_t yyscanner}
%lex-param {p_context *context}
%lex-param {yyscan_t yyscanner}

/* Define the structure of yylval */
%union {
    p_term *term;
    struct {
        p_term *l_head;
        p_term *l_tail;
    } list;
    struct {
        p_term **args;
        size_t num_args;
        size_t max_args;
    } arg_list;
    struct {
        p_term *r_head;
        p_term *r_tail;
        p_term *r_hole;
    } r_list;
    struct {
        p_term *vars;
        p_term *clauses;
        int has_constructor;
    } class_body;
    struct {
        struct {
            p_term *l_head;
            p_term *l_tail;
        } vars;
        struct {
            p_term *l_head;
            p_term *l_tail;
        } clauses;
        int has_constructor;
    } member_list;
    struct {
        p_term *object;
        p_term *name;
        int auto_create;
    } member_ref;
    struct {
        p_term *name;
        p_term *kind;
        p_term *head;
        p_term *body;
        int has_constructor;
    } clause;
    struct {
        p_term *l_head;
        p_term *l_tail;
        int is_default;
    } case_labels;
    struct {
        p_term *l_head;
        p_term *l_tail;
        p_term *default_case;
        int is_default;
    } switch_case;
    struct {
        p_term *case_list;
        p_term *default_case;
    } switch_body;
}

/* Lexical tokens */
%token K_ATOM           "an atom"
%token K_INTEGER        "an integer"
%token K_REAL           "a floating-point number"
%token K_STRING         "a string"
%token K_VARIABLE       "a variable"
%token K_COLON_DASH     "`:-'"
%token K_QUEST_DASH     "`?-'"
%token K_TEST_GOAL      "`??--'"
%token K_READ_TERM      "`??-'"
%token K_ARROW          "`->'"
%token K_DARROW         "`-->'"
%token K_DOT_TERMINATOR "`.'"
%token K_OR             "`||'"
%token K_AND            "`&&'"
%token K_NOT            "`\\+'"
%token K_NE             "`!='"
%token K_TERM_EQ        "`=='"
%token K_TERM_NE        "`!=='"
%token K_TERM_LT        "`@<'"
%token K_TERM_LE        "`@<='"
%token K_TERM_GT        "`@>'"
%token K_TERM_GE        "`@>='"
%token K_UNIV           "`=..'"
%token K_NUM_EQ         "`=:='"
%token K_NUM_NE         "`=!='"
%token K_NUM_LT         "`<'"
%token K_NUM_LE         "`<='"
%token K_NUM_GT         "`>'"
%token K_NUM_GE         "`>='"
%token K_NUM_GETS       "`::='"
%token K_NUM_BT_GETS    "`::=='"
%token K_BITWISE_AND    "`/\\'"
%token K_BITWISE_OR     "`\\/'"
%token K_BITWISE_NOT    "`\\'"
%token K_SHIFT_LEFT     "`<<'"
%token K_SHIFT_RIGHT    "`>>'"
%token K_USHIFT_RIGHT    "`>>>'"
%token K_EXP            "`**'"
%token K_DOT_DOT        "`..'"
%token K_GETS           "`:='"
%token K_BT_GETS        "`:=='"
%token K_IMPLIES        "`=>'"
%token K_EQUIV          "`<=>'"
%token K_ABSTRACT       "`abstract'"
%token K_CASE           "`case'"
%token K_CATCH          "`catch'"
%token K_CLASS          "`class'"
%token K_DEFAULT        "`default'"
%token K_DO             "`do'"
%token K_ELSE           "`else'"
%token K_FOR            "`for'"
%token K_IF             "`if'"
%token K_IN             "`in'"
%token K_IS             "`is'"
%token K_MOD            "`mod'"
%token K_NEW            "`new'"
%token K_REM            "`rem'"
%token K_STATIC         "`static'"
%token K_SWITCH         "`switch'"
%token K_TRY            "`try'"
%token K_WHILE          "`while'"
%token K_VAR            "`var'"

/* Define the yylval types for tokens and rules */
%type <term>        K_ATOM K_INTEGER K_REAL K_STRING K_VARIABLE

%type <term>        declaration directive goal clause callable_term
%type <term>        class_declaration atom opt_parent member_name

%type <term>        term not_term compare_term additive_term
%type <term>        multiplicative_term power_term unary_term
%type <term>        primary_term condition if_term argument_term
%type <term>        new_term member_var bracketed_term head_argument_term
%type <term>        implies_term or_term
%type <term>        argument_implies_term argument_or_term

%type <term>        statement if_statement compound_statement
%type <term>        loop_statement unbind_vars try_statement
%type <term>        catch_clause switch_statement clause_body
%type <term>        confidence

%type <term>        dcg_clause dcg_body dcg_unary_term
%type <term>        dcg_primitive_term

%type <case_labels> case_label case_labels
%type <switch_case> switch_cases switch_case
%type <switch_body> switch_body

%type <list>        list_members declaration_list
%type <list>        member_vars unbind_var_list catch_clauses

%type <arg_list>    arguments
%type <r_list>      statements and_term argument_and_term dcg_term
%type <class_body>  class_body
%type <member_list> class_members class_member
%type <member_ref>  member_reference
%type <clause>      member_clause member_clause_head
%type <clause>      regular_member_clause_head

/* Shift/reduce: dangling "else" in if_statement */
/* Shift/reduce: "!" used as cut vs "!" used as logical negation */
/* Shift/reduce: "catch" clause in "try" statements */
%expect 3

%start file
%%

file
    : declaration_list      {
            input_stream->declarations = finalize_list($1);
        }
    | K_READ_TERM term K_DOT_TERMINATOR {
            input_stream->declarations =
                p_term_create_list
                    (context, unary_term("\?\?-", $2),
                     context->nil_atom);
        }
    | /* empty */
    ;

declaration_list
    : declaration_list declaration  {
            p_term *decl = add_debug(@2, $2);
            append_list($$, $1, decl);
            clear_variables();
        }
    | declaration   {
            p_term *decl = add_debug(@1, $1);
            create_list($$, decl);
            clear_variables();
        }
    ;

declaration
    : clause                    { $$ = $1; }
    | directive                 { $$ = $1; }
    | goal                      { $$ = $1; }
    | class_declaration         { $$ = $1; }
    | dcg_clause                { $$ = $1; }
    | error K_DOT_TERMINATOR    {
            /* Replace the error term with "?- true." */
            $$ = unary_term("?-", context->true_atom);
        }
    ;

directive
    : K_COLON_DASH callable_term K_DOT_TERMINATOR   {
            if (p_term_type($2) == P_TERM_FUNCTOR &&
                    p_term_arg_count($2) == 1 &&
                    p_term_functor($2) ==
                        p_term_create_atom(context, "initialization")) {
                /* Turn "initialization(G)" directives into "?- G" */
                $$ = unary_term("?-", add_debug(@2, p_term_arg($2, 0)));
            } else if (p_term_type($2) == P_TERM_FUNCTOR &&
                       p_term_arg_count($2) == 1 &&
                       p_term_functor($2) ==
                       p_term_create_atom(context, "import")) {
                /* Import another source file at this location */
                p_term *name = p_term_deref(p_term_arg($2, 0));
                if (!name || (name->header.type != P_TERM_ATOM &&
                              name->header.type != P_TERM_STRING)) {
                    yyerror_printf
                        (&(@2), context, input_stream,
                         "import name is not an atom or string");
                } else {
                    int result = p_context_import
                        (&(@2), context, input_stream,
                         p_term_name(name));
                    if (result < 0) {
                        yyerror_printf
                            (&(@2), context, input_stream,
                             "cannot locate import `%s'", name);
                    }
                }
                $$ = unary_term(":-", $2);
            } else {
                /* Execute the directive immediately */
                if (p_goal_call_from_parser(context, add_debug(@2, $2))
                        != P_RESULT_TRUE) {
                    ++(input_stream->error_count);
                }
                $$ = unary_term(":-", $2);
            }
        }
    ;

goal
    : K_QUEST_DASH term K_DOT_TERMINATOR    {
            $$ = unary_term("?-", add_debug(@2, $2));
        }
    | K_QUEST_DASH compound_statement       {
            $$ = unary_term("?-", add_debug(@2, $2));
        }
    | K_TEST_GOAL term K_DOT_TERMINATOR     {
            /* Goal that is not executed during the consult but
             * which is saved for unit tests to execute separately */
            $$ = unary_term("\?\?--", $2);
        }
    | K_TEST_GOAL compound_statement        {
            $$ = unary_term("\?\?--", $2);
        }
    ;

clause
    : callable_term clause_body     { $$ = binary_term(":-", $1, $2); }
    ;

clause_body
    : K_DOT_TERMINATOR              { $$ = context->true_atom; }
    | compound_statement            { $$ = $1; }
    | K_SHIFT_LEFT confidence K_SHIFT_RIGHT K_DOT_TERMINATOR {
            $$ = unary_term("$$fuzzy", $2);
        }
    | K_SHIFT_LEFT confidence K_SHIFT_RIGHT compound_statement {
            $$ = binary_term(",", unary_term("$$fuzzy", $2), $4);
        }
    ;

confidence
    : K_INTEGER                     { $$ = $1; }
    | K_REAL                        { $$ = $1; }
    ;

callable_term
    : atom                          { $$ = $1; }
    | atom '(' ')'                  { $$ = $1; }
    | atom '(' arguments ')'   {
            if ($1 == context->dot_atom && $3.num_args == 2) {
                yyerror_printf
                    (&(@1), context, input_stream,
                     "(.)/2 cannot be used as a predicate name");
            }
            $$ = p_term_create_functor_with_args
                (context, $1, $3.args, (int)($3.num_args));
            GC_FREE($3.args);
        }
    ;

/* Atom or a keyword that is an atom in most circumstances */
atom
    : K_ATOM        { $$ = $1; }
    | K_VAR         { $$ = p_term_create_atom(context, "var"); }
    | K_CATCH       { $$ = p_term_create_atom(context, "catch"); }
    | K_CLASS       { $$ = p_term_create_atom(context, "class"); }
    | K_STATIC      { $$ = p_term_create_atom(context, "static"); }
    | K_ABSTRACT    { $$ = p_term_create_atom(context, "abstract"); }
    ;

arguments
    : arguments ',' head_argument_term   {
            $$ = $1;
            if ($$.num_args > $$.max_args) {
                $$.max_args *= 2;
                $$.args = GC_REALLOC($$.args, sizeof(p_term *) * $$.max_args);
            }
            $$.args[($$.num_args)++] = $3;
        }
    | head_argument_term {
            $$.args = GC_MALLOC(sizeof(p_term *) * 4);
            $$.args[0] = $1;
            $$.num_args = 1;
            $$.max_args = 4;
        }
    ;

head_argument_term
    : K_IN argument_term        { $$ = unary_term("in", $2); }
    | argument_term             { $$ = $1; }
    ;

bracketed_term
    : term                      { $$ = $1; }
    | term K_COLON_DASH term    { $$ = binary_term(":-", $1, $3); }
    | term K_DARROW term        { $$ = binary_term("-->", $1, $3); }
    | K_COLON_DASH term         { $$ = unary_term(":-", $2); }
    | K_QUEST_DASH term         { $$ = unary_term("?-", $2); }
    ;

term
    : term K_EQUIV implies_term { $$ = binary_term("<=>", $1, $3); }
    | implies_term              { $$ = $1; }
    ;

implies_term
    : implies_term K_IMPLIES or_term  {
            $$ = binary_term("=>", $1, $3);
        }
    | or_term                   { $$ = $1; }
    ;

or_term
    : or_term K_OR if_term      { $$ = binary_term("||", $1, $3); }
    | if_term                   { $$ = $1; }
    ;

if_term
    : and_term K_ARROW if_term  {
            $$ = binary_term("->", finalize_r_list($1), $3);
        }
    | and_term                  { $$ = finalize_r_list($1); }
    ;

and_term
    : and_term K_AND not_term   { append_r_list($$, $1, $3, "&&"); }
    | and_term ',' not_term     { append_r_list($$, $1, $3, ","); }
    | not_term                  { create_r_list($$, $1); }
    ;

argument_term
    : argument_term K_EQUIV argument_implies_term {
            $$ = binary_term("<=>", $1, $3);
        }
    | argument_implies_term     { $$ = $1; }
    ;

argument_implies_term
    : argument_implies_term K_IMPLIES argument_or_term {
            $$ = binary_term("=>", $1, $3);
        }
    | argument_or_term          { $$ = $1; }
    ;

argument_or_term
    : argument_or_term K_OR argument_and_term {
            $$ = binary_term("||", $1, finalize_r_list($3));
        }
    | argument_and_term         { $$ = finalize_r_list($1); }
    ;

argument_and_term
    : argument_and_term K_AND not_term { append_r_list($$, $1, $3, "&&"); }
    | not_term                  { create_r_list($$, $1); }
    ;

not_term
    : K_NOT not_term            { $$ = unary_term("!", $2); }
    | '!' not_term              { $$ = unary_term("!", $2); }
    | compare_term              { $$ = $1; }
    ;

compare_term
    : additive_term '=' additive_term   {
            $$ = binary_term("=", $1, $3);
        }
    | additive_term K_NE additive_term  {
            $$ = binary_term("!=", $1, $3);
        }
    | additive_term K_TERM_EQ additive_term {
            $$ = binary_term("==", $1, $3);
        }
    | additive_term K_TERM_NE additive_term {
            $$ = binary_term("!==", $1, $3);
        }
    | additive_term K_TERM_LT additive_term {
            $$ = binary_term("@<", $1, $3);
        }
    | additive_term K_TERM_LE additive_term {
            $$ = binary_term("@<=", $1, $3);
        }
    | additive_term K_TERM_GT additive_term {
            $$ = binary_term("@>", $1, $3);
        }
    | additive_term K_TERM_GE additive_term {
            $$ = binary_term("@>=", $1, $3);
        }
    | additive_term K_UNIV additive_term {
            $$ = binary_term("=..", $1, $3);
        }
    | additive_term K_IS additive_term {
            $$ = binary_term("is", $1, $3);
        }
    | additive_term K_NUM_EQ additive_term {
            $$ = binary_term("=:=", $1, $3);
        }
    | additive_term K_NUM_NE additive_term {
            $$ = binary_term("=!=", $1, $3);
        }
    | additive_term K_NUM_LT additive_term {
            $$ = binary_term("<", $1, $3);
        }
    | additive_term K_NUM_LE additive_term {
            $$ = binary_term("<=", $1, $3);
        }
    | additive_term K_NUM_GT additive_term {
            $$ = binary_term(">", $1, $3);
        }
    | additive_term K_NUM_GE additive_term {
            $$ = binary_term(">=", $1, $3);
        }
    | additive_term K_IN additive_term {
            $$ = binary_term("in", $1, $3);
        }
    | additive_term K_GETS additive_term {
            $$ = binary_term(":=", $1, $3);
        }
    | additive_term K_BT_GETS additive_term {
            $$ = binary_term(":==", $1, $3);
        }
    | additive_term K_NUM_GETS additive_term {
            $$ = binary_term("::=", $1, $3);
        }
    | additive_term K_NUM_BT_GETS additive_term {
            $$ = binary_term("::==", $1, $3);
        }
    | additive_term     { $$ = $1; }
    ;

additive_term
    : additive_term '+' multiplicative_term {
            $$ = binary_term("+", $1, $3);
        }
    | additive_term '-' multiplicative_term {
            $$ = binary_term("-", $1, $3);
        }
    | additive_term K_BITWISE_AND multiplicative_term {
            $$ = binary_term("/\\", $1, $3);
        }
    | additive_term K_BITWISE_OR multiplicative_term {
            $$ = binary_term("\\/", $1, $3);
        }
    | multiplicative_term   { $$ = $1; }
    ;

multiplicative_term
    : multiplicative_term '*' power_term {
            $$ = binary_term("*", $1, $3);
        }
    | multiplicative_term '/' power_term {
            $$ = binary_term("/", $1, $3);
        }
    | multiplicative_term '%' power_term {
            $$ = binary_term("%", $1, $3);
        }
    | multiplicative_term K_MOD power_term {
            $$ = binary_term("mod", $1, $3);
        }
    | multiplicative_term K_REM power_term {
            $$ = binary_term("rem", $1, $3);
        }
    | multiplicative_term K_SHIFT_LEFT power_term {
            $$ = binary_term("<<", $1, $3);
        }
    | multiplicative_term K_SHIFT_RIGHT power_term {
            $$ = binary_term(">>", $1, $3);
        }
    | multiplicative_term K_USHIFT_RIGHT power_term {
            $$ = binary_term(">>>", $1, $3);
        }
    | power_term    { $$ = $1; }
    ;

power_term
    : unary_term K_EXP unary_term {
            $$ = binary_term("**", $1, $3);
        }
    | unary_term '^' power_term {
            $$ = binary_term("^", $1, $3);
        }
    | unary_term    { $$ = $1; }
    ;

unary_term
    : '-' unary_term                { $$ = unary_term("-", $2); }
    | K_BITWISE_NOT unary_term      { $$ = unary_term("~", $2); }
    | '~' unary_term                { $$ = unary_term("~", $2); }
    | primary_term                  { $$ = $1; }
    ;

primary_term
    : atom                      { $$ = $1; }
    | K_INTEGER                 { $$ = $1; }
    | K_REAL                    { $$ = $1; }
    | K_STRING                  { $$ = $1; }
    | K_VARIABLE                { $$ = $1; }
    | '!'    {
            $$ = p_term_create_atom(context, "!");
        }
    | atom '(' arguments ')'  {
            if ($1 == context->dot_atom && $3.num_args == 2) {
                $$ = p_term_create_list
                    (context, $3.args[0], $3.args[1]);
            } else {
                $$ = p_term_create_functor_with_args
                    (context, $1, $3.args, (int)($3.num_args));
            }
            GC_FREE($3.args);
        }
    | atom '(' ')'              { $$ = $1; }
    | '[' ']'                   { $$ = p_term_nil_atom(context); }
    | '[' list_members ']'      { $$ = finalize_list($2); }
    | '[' list_members '|' term ']' { $$ = finalize_list_tail($2, $4); }
    | '(' bracketed_term ')'    { $$ = $2; }
    | member_reference          {
            $$ = p_term_create_member_variable
                (context, $1.object, $1.name, $1.auto_create);
        }
    | member_reference '(' arguments ')'    {
            /* Create '$$call_member'(X.name, '$$'(X, args)) */
            p_term *term = p_term_create_functor
                (context, p_term_create_atom(context, "$$"),
                 (int)($3.num_args + 1));
            size_t index;
            p_term_bind_functor_arg(term, 0, $1.object);
            for (index = 0; index < $3.num_args; ++index) {
                p_term_bind_functor_arg
                    (term, (int)(index + 1), $3.args[index]);
            }
            GC_FREE($3.args);
            $$ = p_term_create_functor
                (context, p_term_create_atom
                    (context, "$$call_member"), 2);
            p_term_bind_functor_arg
                ($$, 0, p_term_create_member_variable
                    (context, $1.object, $1.name, $1.auto_create));
            p_term_bind_functor_arg($$, 1, term);
        }
    | member_reference '(' ')'  {
            /* Create '$$call_member'(X.name, '$$'(X)) */
            p_term *term = p_term_create_functor
                (context, p_term_create_atom(context, "$$"), 1);
            p_term_bind_functor_arg(term, 0, $1.object);
            $$ = p_term_create_functor
                (context, p_term_create_atom
                    (context, "$$call_member"), 2);
            p_term_bind_functor_arg
                ($$, 0, p_term_create_member_variable
                    (context, $1.object, $1.name, $1.auto_create));
            p_term_bind_functor_arg($$, 1, term);
        }
    | new_term                  { $$ = $1; }
    ;

list_members
    : list_members ',' argument_term    { append_list($$, $1, $3); }
    | argument_term                     { create_list($$, $1); }
    ;

member_reference
    : K_VARIABLE '.' atom     {
            $$.object = $1;
            $$.name = $3;
            $$.auto_create = 0;
        }
    | K_VARIABLE K_DOT_DOT atom   {
            $$.object = $1;
            $$.name = $3;
            $$.auto_create = 1;
        }
    | member_reference '.' atom   {
            $$.object = p_term_create_member_variable
                (context, $1.object, $1.name, $1.auto_create);
            $$.name = $3;
            $$.auto_create = 0;
        }
    | member_reference K_DOT_DOT atom {
            $$.object = p_term_create_member_variable
                (context, $1.object, $1.name, $1.auto_create);
            $$.name = $3;
            $$.auto_create = 1;
        }
    ;

new_term
    : K_NEW atom '(' arguments ')' {
            /* Convert the object construction into the term
             * ($$new(class_name, Obj), class_name::new(Args))
             * where Obj is the first argument of Args */
            p_term *obj = $4.args[0];
            p_term *call_new = p_term_create_functor
                (context, p_term_create_atom(context, "$$new"), 2);
            p_term *ctor_name = p_term_create_member_name
                (context, $2, p_term_create_atom(context, "new"));
            p_term *call_ctor = p_term_create_functor_with_args
                (context, ctor_name, $4.args, (int)($4.num_args));
            GC_FREE($4.args);
            p_term_bind_functor_arg(call_new, 0, $2);
            p_term_bind_functor_arg(call_new, 1, obj);
            $$ = binary_term(",", call_new, call_ctor);
        }
    ;

statements
    : statements statement  {
            p_term *stmt = add_debug(@2, $2);
            append_r_list($$, $1, stmt, ",");
        }
    | statement             {
            p_term *stmt = add_debug(@1, $1);
            create_r_list($$, stmt);
        }
    ;

statement
    : argument_term ';'     { $$ = $1; }
    | if_statement          { $$ = $1; }
    | compound_statement    { $$ = $1; }
    | loop_statement        { $$ = $1; }
    | try_statement         { $$ = $1; }
    | switch_statement      { $$ = $1; }
    | ';'       {
            $$ = add_debug(@1, context->true_atom);
        }
    ;

if_statement
    : K_IF condition statement     {
            /* Convert the if statement into (A -> B || true) form */
            p_term *if_stmt = p_term_create_functor
                (context, p_term_create_atom(context, "->"), 2);
            p_term_bind_functor_arg(if_stmt, 0, $2);
            p_term_bind_functor_arg(if_stmt, 1, $3);
            $$ = p_term_create_functor
                (context, p_term_create_atom(context, "||"), 2);
            p_term_bind_functor_arg($$, 0, if_stmt);
            p_term_bind_functor_arg($$, 1, context->true_atom);
        }
    | K_IF condition statement K_ELSE statement {
            /* Convert the if statement into (A -> B || C) form */
            p_term *if_stmt = p_term_create_functor
                (context, p_term_create_atom(context, "->"), 2);
            p_term_bind_functor_arg(if_stmt, 0, $2);
            p_term_bind_functor_arg(if_stmt, 1, $3);
            $$ = p_term_create_functor
                (context, p_term_create_atom(context, "||"), 2);
            p_term_bind_functor_arg($$, 0, if_stmt);
            p_term_bind_functor_arg($$, 1, $5);
        }
    ;

condition
    : '(' term ')'          { $$ = $2; }
    | '(' error ')'         { $$ = context->fail_atom; }
    ;

compound_statement
    : '{' statements '}'    { $$ = finalize_r_list($2); }
    | '{' '}'               { $$ = context->true_atom; }
    | '{' error '}'         { $$ = context->true_atom; }
    ;

loop_statement
    : K_WHILE unbind_vars condition statement   {
            if ($2 == p_term_nil_atom(context))
                $$ = binary_term("$$while", $3, $4);
            else
                $$ = ternary_term("$$while", $2, $3, $4);
        }
    | K_DO unbind_vars compound_statement K_WHILE condition ';' {
            if ($2 == p_term_nil_atom(context))
                $$ = binary_term("$$do", $3, $5);
            else
                $$ = ternary_term("$$do", $2, $3, $5);
        }
    | K_FOR unbind_vars '(' K_VARIABLE  {
            if (p_term_lex_variable_count(input_stream, $4) > 1) {
                /* The loop variable cannot appear previously
                 * in the clause; it must be a new variable */
                yyerror_printf
                    (&(@4), context, input_stream,
                     "for loop variable `%s' has been referenced previously",
                     p_term_name($4));
            }
        }
      K_IN primary_term ')' statement  {
            $$ = p_term_create_functor
                (context, p_term_create_atom(context, "$$for"), 4);
            p_term_bind_functor_arg($$, 0, $2);
            p_term_bind_functor_arg($$, 1, unary_term("$$loopvar", $4));
            p_term_bind_functor_arg($$, 2, $7);
            p_term_bind_functor_arg($$, 3, $9);
        }
    ;

unbind_vars
    : '[' unbind_var_list ']'   { $$ = finalize_list($2); }
    | '[' ']'                   { $$ = p_term_nil_atom(context); }
    | /* empty */               { $$ = p_term_nil_atom(context); }
    ;

unbind_var_list
    : unbind_var_list ',' K_VARIABLE    { append_list($$, $1, $3); }
    | K_VARIABLE                        { create_list($$, $1); }
    ;

try_statement
    : K_TRY compound_statement catch_clauses    {
            $$ = binary_term("$$try", $2, finalize_list($3));
        }
    ;

catch_clauses
    : catch_clauses catch_clause        { append_list($$, $1, $2); }
    | catch_clause                      { create_list($$, $1); }
    ;

catch_clause
    : K_CATCH '(' argument_term ')' compound_statement  {
            $$ = binary_term("$$catch", $3, $5);
        }
    ;

switch_statement
    : K_SWITCH '(' argument_term ')' '{' switch_body '}' {
            $$ = ternary_term
                ("$$switch", $3, $6.case_list, $6.default_case);
        }
    ;

switch_body
    : switch_cases      {
            $$.case_list = finalize_list($1);
            if ($1.default_case)
                $$.default_case = $1.default_case;
            else
                $$.default_case = context->fail_atom;
        }
    | /* empty */       {
            $$.case_list = p_term_nil_atom(context);
            $$.default_case = context->fail_atom;
        }
    ;

switch_cases
    : switch_cases switch_case  {
            append_lists($$, $1, $2);
            if ($1.is_default) {
                $$.default_case = $1.default_case;
                if ($1.is_default == 2 || $2.is_default == 2) {
                    /* Multiple default cases already reported */
                    $$.is_default = 2;
                } else if ($1.is_default && $2.is_default) {
                    yyerror_printf
                        (&(@2), context, input_stream,
                         "multiple `default' cases in `switch'");
                    $$.is_default = 2;
                } else {
                    $$.is_default = $1.is_default;
                }
            } else if ($2.is_default) {
                $$.default_case = $2.default_case;
                $$.is_default = $2.is_default;
            } else {
                $$.default_case = 0;
                $$.is_default = 0;
            }
        }
    | switch_case               { $$ = $1; }
    ;

switch_case
    : case_labels statement     {
            p_term *case_list = finalize_list($1);
            if (case_list != context->nil_atom) {
                p_term *term = p_term_create_functor
                    (context, p_term_create_atom(context, "$$case"), 2);
                p_term_bind_functor_arg(term, 0, case_list);
                p_term_bind_functor_arg(term, 1, $2);
                create_list($$, term);
            } else {
                create_empty_list($$);
            }
            if ($1.is_default) {
                $$.default_case = $2;
                $$.is_default = $1.is_default;
            } else {
                $$.default_case = 0;
                $$.is_default = 0;
            }
        }
    ;

case_labels
    : case_labels case_label    {
                if ($2.is_default) {
                    append_lists($$, $1, $2);
                    if ($1.is_default) {
                        yyerror_printf
                            (&(@2), context, input_stream,
                             "multiple `default' cases in `switch'");
                        $$.is_default = 2;
                    } else {
                        $$.is_default = 1;
                    }
                } else {
                    append_lists($$, $1, $2);
                    $$.is_default = $1.is_default;
                }
            }
    | case_label                { $$ = $1; }
    ;

case_label
    : K_CASE argument_term ':'  {
            create_list($$, $2);
            $$.is_default = 0;
        }
    | K_DEFAULT ':'             {
            create_empty_list($$);
            $$.is_default = 1;
        }
    ;

class_declaration
    : K_CLASS atom      {
            input_stream->class_name = $2;
        } opt_parent class_body opt_semi    {
            p_term *clauses = $5.clauses;
            if (!($5.has_constructor)) {
                /* Create a default constructor for the class */
                p_term *ctor = create_clause_head
                    (context, input_stream,
                     p_term_create_atom(context, "new"), 0, 0, 0);
                ctor = binary_term(":-", ctor, context->true_atom);
                ctor = ternary_term
                    ("clause",
                     p_term_create_atom(context, "new"),
                     p_term_create_atom(context, "constructor"),
                     ctor);
                ctor = add_debug(@2, ctor);
                clauses = p_term_create_list(context, ctor, clauses);
                clear_variables();
            }
            $$ = make_class_declaration
                (context, $2, $4, $5.vars, clauses);
            input_stream->class_name = 0;
        }
    ;

opt_parent
    : /* empty */           { $$ = p_term_nil_atom(context); }
    | ':' '[' ']'           { $$ = p_term_nil_atom(context); }
    | ':' atom              { $$ = $2; }
    ;

opt_semi
    : /* empty */
    | ';'
    ;

class_body
    : '{' class_members '}' {
            $$.vars = finalize_list($2.vars);
            $$.clauses = finalize_list($2.clauses);
            $$.has_constructor = $2.has_constructor;
        }
    | '{' '}'   {
            $$.vars = p_term_nil_atom(context);
            $$.clauses = p_term_nil_atom(context);
            $$.has_constructor = 0;
        }
    | '{' error '}' {
            $$.vars = p_term_nil_atom(context);
            $$.clauses = p_term_nil_atom(context);
            $$.has_constructor = 0;
        }
    ;

class_members
    : class_members class_member    {
            append_lists($$.vars, $1.vars, $2.vars);
            append_lists($$.clauses, $1.clauses, $2.clauses);
            $$.has_constructor =
                ($1.has_constructor || $2.has_constructor);
        }
    | class_member                  { $$ = $1; }
    ;

class_member
    : K_VAR member_vars opt_semi    {
            $$.vars.l_head = $2.l_head;
            $$.vars.l_tail = $2.l_tail;
            create_empty_list($$.clauses);
            $$.has_constructor = 0;
        }
    | member_clause {
            p_term *clause = ternary_term
                ("clause", $1.name, $1.kind,
                 binary_term(":-", $1.head, $1.body));
            clause = add_debug(@1, clause);
            create_empty_list($$.vars);
            create_list($$.clauses, clause);
            $$.has_constructor = $1.has_constructor;
            clear_variables();
        }
    ;

member_clause
    : member_clause_head clause_body    {
            $$ = $1;
            $$.body = $2;
        }
    | K_ABSTRACT regular_member_clause_head K_DOT_TERMINATOR {
            /* Construct a body for the abstract predicate that
             * will throw an existence_error at runtime */
            p_term *pred;
            p_term *error;
            pred = p_term_create_functor
                (context, context->slash_atom, 2);
            p_term_bind_functor_arg(pred, 0, p_term_functor($2.head));
            p_term_bind_functor_arg
                (pred, 1, p_term_create_integer
                    (context, p_term_arg_count($2.head)));
            error = binary_term
                ("existence_error",
                 p_term_create_atom(context, "member_predicate"), pred);
            error = binary_term("error", error, pred);
            $$ = $2;
            $$.body = unary_term("throw", error);
        }
    ;

member_clause_head
    : regular_member_clause_head                { $$ = $1; }
    | K_STATIC member_name                      {
            $$.name = $2;
            $$.kind = p_term_create_atom(context, "static");
            $$.head = create_clause_head
                (context, input_stream, $2, 0, 0, 1);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    | K_STATIC member_name '(' ')'              {
            $$.name = $2;
            $$.kind = p_term_create_atom(context, "static");
            $$.head = create_clause_head
                (context, input_stream, $2, 0, 0, 1);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    | K_STATIC member_name '(' arguments ')'    {
            $$.name = $2;
            $$.kind = p_term_create_atom(context, "static");
            $$.head = create_clause_head
                (context, input_stream, $2, $4.args, $4.num_args, 1);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    | K_NEW                                     {
            $$.name = p_term_create_atom(context, "new");
            $$.kind = p_term_create_atom(context, "constructor");
            $$.head = create_clause_head
                (context, input_stream,
                 p_term_create_atom(context, "new"), 0, 0, 0);
            $$.body = 0;
            $$.has_constructor = 1;
        }
    | K_NEW '(' ')'                             {
            $$.name = p_term_create_atom(context, "new");
            $$.kind = p_term_create_atom(context, "constructor");
            $$.head = create_clause_head
                (context, input_stream,
                 p_term_create_atom(context, "new"), 0, 0, 0);
            $$.body = 0;
            $$.has_constructor = 1;
        }
    | K_NEW '(' arguments ')'                   {
            $$.name = p_term_create_atom(context, "new");
            $$.kind = p_term_create_atom(context, "constructor");
            $$.head = create_clause_head
                (context, input_stream,
                 p_term_create_atom(context, "new"),
                 $3.args, $3.num_args, 0);
            $$.body = 0;
            $$.has_constructor = 1;
        }
    ;

regular_member_clause_head
    : member_name                               {
            $$.name = $1;
            $$.kind = p_term_create_atom(context, "member");
            $$.head = create_clause_head
                (context, input_stream, $1, 0, 0, 0);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    | member_name '(' ')'                       {
            $$.name = $1;
            $$.kind = p_term_create_atom(context, "member");
            $$.head = create_clause_head
                (context, input_stream, $1, 0, 0, 0);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    | member_name '(' arguments ')'             {
            $$.name = $1;
            $$.kind = p_term_create_atom(context, "member");
            $$.head = create_clause_head
                (context, input_stream, $1, $3.args, $3.num_args, 0);
            $$.body = 0;
            $$.has_constructor = 0;
        }
    ;

member_vars
    : member_vars ',' member_var    { append_list($$, $1, $3); }
    | member_var                    { create_list($$, $1); }
    ;

member_var
    : member_name                   { $$ = add_debug(@1, $1); }
    ;

member_name
    : atom          {
            if ($1 == context->class_name_atom ||
                    $1 == context->prototype_atom) {
                yyerror_printf
                    (&(@1), context, input_stream,
                     "`%s' is not allowed as a member name",
                     p_term_name($1));
            }
            $$ = $1;
        }
    ;

dcg_clause
    : callable_term K_DARROW dcg_body K_DOT_TERMINATOR {
            $$ = p_term_expand_dcg(context, binary_term("-->", $1, $3));
        }
    | callable_term K_DARROW dcg_body K_SHIFT_LEFT confidence
        K_SHIFT_RIGHT K_DOT_TERMINATOR {
            p_term *body = unary_term("{}", unary_term("$$fuzzy", $5));
            body = binary_term(",", $3, body);
            $$ = p_term_expand_dcg
                (context, binary_term("-->", $1, body));
        }
    ;

dcg_body
    : dcg_body K_OR dcg_term        {
            $$ = binary_term("||", $1, finalize_r_list($3));
        }
    | dcg_term                      {
            $$ = finalize_r_list($1);
        }
    ;

dcg_term
    : dcg_term ',' dcg_unary_term   { append_r_list($$, $1, $3, ","); }
    | dcg_unary_term                { create_r_list($$, $1); }
    ;

dcg_unary_term
    : '!' dcg_primitive_term    { $$ = unary_term("!", $2); }
    | dcg_primitive_term        { $$ = $1; }
    ;

dcg_primitive_term
    : callable_term             { $$ = $1; }
    | compound_statement        { $$ = unary_term("{}", $1); }
    | '[' ']'                   { $$ = context->nil_atom; }
    | '[' list_members ']'      { $$ = finalize_list($2); }
    | K_STRING                  { $$ = $1; }
    | '(' dcg_body ')'          { $$ = $2; }
    | '!'                       { $$ = context->commit_atom; }
    ;
