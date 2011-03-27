%{
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

static void yyerror(YYLTYPE *loc, p_context *context, yyscan_t yyscanner, const char *msg)
{
    p_input_stream *stream = p_term_get_extra(yyscanner);
    if (stream->filename)
        fprintf(stderr, "%s:%d: %s\n", stream->filename, loc->first_line, msg);
    else
        fprintf(stderr, "%d: %s\n", loc->first_line, msg);
    ++(stream->error_count);
}

static void yyerror_printf(YYLTYPE *loc, p_context *context, yyscan_t yyscanner, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    p_input_stream *stream = p_term_get_extra(yyscanner);
    if (stream->filename)
        fprintf(stderr, "%s:%d: ", stream->filename, loc->first_line);
    else
        fprintf(stderr, "%d: ", loc->first_line);
    vfprintf(stderr, format, va);
    putc('\n', stderr);
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
#define append_r_list(dest, src, new_term) \
    do { \
        p_term *term; \
        if (src.r_hole) { \
            term = p_term_create_functor \
                (context, p_term_create_atom(context, ","), 2); \
            p_term_bind_functor_arg(term, 0, src.r_tail); \
            p_term_bind_functor_arg(src.r_hole, 1, term); \
            dest.r_head = src.r_head; \
            dest.r_hole = term; \
            dest.r_tail = new_term; \
        } else { \
            term = p_term_create_functor \
                (context, p_term_create_atom(context, ","), 2); \
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
 * ?- new_class(class_name, parent, vars, clauses, X), assertz(X). */
static p_term *make_class_declaration
    (p_context *context, p_term *name, p_term *parent,
     p_term *vars, p_term *clauses)
{
    p_term *var;
    p_term *term;
    p_term *term2;
    var = p_term_create_variable(context);
    term = p_term_create_functor
        (context, p_term_create_atom(context, "new_class"), 5);
    p_term_bind_functor_arg(term, 0, name);
    p_term_bind_functor_arg(term, 1, parent);
    p_term_bind_functor_arg(term, 2, vars);
    p_term_bind_functor_arg(term, 3, clauses);
    p_term_bind_functor_arg(term, 4, var);
    term2 = p_term_create_functor_with_args
        (context, p_term_create_atom(context, "assertz"),
         &var, 1);
    return make_unary_term
        (context, "?-", make_binary_term(context, ",", term, term2));
}

/* Clear the lexer's variable list */
#define clear_variables()   \
    (p_term_get_extra(yyscanner)->num_variables = 0)

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
    (add_debug_line(context, p_term_get_extra(yyscanner), &(loc), term))

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
     int has_extn, yyscan_t yyscanner)
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
    error = p_context_consult_file(context, path);
    if (error == 0) {
        /* File has been loaded successfully */
        free(path);
        return 1;
    } else if (error == EINVAL) {
        /* File exists but contained an error during loading */
        ++(p_term_get_extra(yyscanner)->error_count);
        free(path);
        return 1;
    }
    free(path);
    return 0;
}

static void p_context_import
    (YYLTYPE *loc, p_context *context, yyscan_t yyscanner,
     const char *name)
{
    const char *parent_filename;
    size_t len, len2;
    int has_extn, has_dir, is_root, error;
    char *path;

    /* Find the directory that contains the parent source file */
    parent_filename = p_term_get_extra(yyscanner)->filename;
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
        yyerror_printf(loc, context, yyscanner, "empty import name");
        return;
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
        if (name[1] == '/' || name[1] == '\\')
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
    error = p_context_consult_file(context, path);
    if (error == 0) {
        /* File has been loaded successfully */
        free(path);
        return;
    } else if (error == EINVAL) {
        /* File exists but contained an error during loading */
        ++(p_term_get_extra(yyscanner)->error_count);
        free(path);
        return;
    }
    free(path);

    /* Search the user and system import paths */
    if (!has_dir) {
        size_t index;
        for (index = 0; index < context->user_imports.num_paths; ++index) {
            if (p_context_consult_file_in_path
                    (context, context->user_imports.paths[index],
                     name, has_extn, yyscanner))
                return;
        }
        for (index = 0; index < context->system_imports.num_paths; ++index) {
            if (p_context_consult_file_in_path
                    (context, context->system_imports.paths[index],
                     name, has_extn, yyscanner))
                return;
        }
    }

    /* Cannot find the file to be imported */
    yyerror_printf(loc, context, yyscanner,
                   "cannot locate import `%s'", name);
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
    } member_list;
    struct {
        p_term *object;
        p_term *name;
        int auto_create;
    } member_ref;
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
%token K_BITWISE_AND    "`/\\'"
%token K_BITWISE_OR     "`\\/'"
%token K_BITWISE_NOT    "`\\'"
%token K_SHIFT_LEFT     "`<<'"
%token K_SHIFT_RIGHT    "`>>'"
%token K_USHIFT_RIGHT    "`>>>'"
%token K_EXP            "`**'"
%token K_DOT_DOT        "`..'"
%token K_GETS           "`:='"
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
%token K_SWITCH         "`switch'"
%token K_TRY            "`try'"
%token K_WHILE          "`while'"
%token K_VAR            "`var'"

/* Define the yylval types for tokens and rules */
%type <term>        K_ATOM K_INTEGER K_REAL K_STRING K_VARIABLE

%type <term>        declaration directive goal clause callable_term
%type <term>        class_declaration object_declaration atom

%type <term>        term not_term compare_term additive_term
%type <term>        multiplicative_term power_term unary_term
%type <term>        primary_term condition if_term argument_term
%type <term>        new_term property property_bindings
%type <term>        opt_property_bindings member_var bracketed_term

%type <term>        statement if_statement compound_statement
%type <term>        loop_statement unbind_vars try_statement
%type <term>        catch_clause switch_statement

%type <case_labels> case_label case_labels
%type <switch_case> switch_cases switch_case
%type <switch_body> switch_body

%type <list>        list_members properties declaration_list
%type <list>        member_vars unbind_var_list catch_clauses

%type <arg_list>    arguments
%type <r_list>      statements and_term argument_and_term
%type <class_body>  class_body
%type <member_list> class_members class_member
%type <member_ref>  member_reference

/* Shift/reduce: dangling "else" in if_statement */
/* Shift/reduce: "!" used as cut vs "!" used as logical negation */
/* Shift/reduce: "catch" clause in "try" statements */
%expect 3

%start file
%%

file
    : declaration_list      {
            p_term_get_extra(yyscanner)->declarations =
                    finalize_list($1);
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
    | object_declaration        { $$ = $1; }
    | error K_DOT_TERMINATOR    {
            /* Replace the error term with "?- true." */
            $$ = unary_term("?-", p_term_create_atom(context, "true"));
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
                        (&(@2), context, yyscanner,
                         "import name is not an atom or string");
                } else {
                    p_context_import
                        (&(@2), context, yyscanner, p_term_name(name));
                }
                $$ = unary_term(":-", $2);
            } else {
                /* Execute the directive immediately */
                p_goal_call_from_parser(context, add_debug(@2, $2));
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
    : callable_term K_DOT_TERMINATOR    {
            $$ = binary_term
                (":-", $1, p_term_create_atom(context, "true"));
        }
    | callable_term K_COLON_DASH term K_DOT_TERMINATOR  {
            $$ = binary_term(":-", $1, $3);
        }
    | callable_term compound_statement  {
            $$ = binary_term(":-", $1, $2);
        }
    ;

callable_term
    : atom                          { $$ = $1; }
    | atom '(' ')'                  { $$ = $1; }
    | atom '(' arguments ')'        {
            if ($1 == context->dot_atom && $3.num_args == 2) {
                yyerror_printf
                    (&(@1), context, yyscanner,
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
    ;

arguments
    : arguments ',' argument_term   {
            $$ = $1;
            if ($$.num_args > $$.max_args) {
                $$.max_args *= 2;
                $$.args = GC_REALLOC($$.args, sizeof(p_term *) * $$.max_args);
            }
            $$.args[($$.num_args)++] = $3;
        }
    | argument_term {
            $$.args = GC_MALLOC(sizeof(p_term *) * 4);
            $$.args[0] = $1;
            $$.num_args = 1;
            $$.max_args = 4;
        }
    ;

bracketed_term
    : term                      { $$ = $1; }
    | term K_COLON_DASH term    { $$ = binary_term(":-", $1, $3); }
    | term K_DARROW term        { $$ = binary_term("-->", $1, $3); }
    | K_COLON_DASH term         { $$ = unary_term(":-", $2); }
    | K_QUEST_DASH term         { $$ = unary_term("?-", $2); }
    ;

term
    : term K_OR if_term         { $$ = binary_term("||", $1, $3); }
    | if_term                   { $$ = $1; }
    ;

if_term
    : and_term K_ARROW if_term  {
            $$ = binary_term("->", finalize_r_list($1), $3);
        }
    | and_term                  { $$ = finalize_r_list($1); }
    ;

and_term
    : and_term K_AND not_term   { append_r_list($$, $1, $3); }
    | and_term ',' not_term     { append_r_list($$, $1, $3); }
    | not_term                  { create_r_list($$, $1); }
    ;

argument_term
    : argument_term K_OR argument_and_term {
            $$ = binary_term("||", $1, finalize_r_list($3));
        }
    | argument_and_term         { $$ = finalize_r_list($1); }
    ;

argument_and_term
    : argument_and_term K_AND not_term { append_r_list($$, $1, $3); }
    | not_term                  { create_r_list($$, $1); }
    ;

not_term
    : K_NOT compare_term        {
            $$ = add_debug(@1, unary_term("!", $2));
        }
    | '!' compare_term          {
            $$ = add_debug(@1, unary_term("!", $2));
        }
    | compare_term              { $$ = add_debug(@1, $1); }
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
    | additive_term K_NUM_GETS additive_term {
            $$ = binary_term("::=", $1, $3);
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
            /* Turn "X.name(args)" into "$$call_member(X, name(args)) */
            p_term *term = p_term_create_functor_with_args
                (context, $1.name, $3.args, (int)($3.num_args));
            GC_FREE($3.args);
            $$ = p_term_create_functor
                (context, p_term_create_atom
                    (context, "$$call_member"), 2);
            p_term_bind_functor_arg($$, 0, $1.object);
            p_term_bind_functor_arg($$, 1, term);
        }
    | member_reference '(' ')'  {
            /* Turn "X.name()" into "$$call_member(X, name)" */
            $$ = p_term_create_functor
                (context, p_term_create_atom
                    (context, "$$call_member"), 2);
            p_term_bind_functor_arg($$, 0, $1.object);
            p_term_bind_functor_arg($$, 1, $1.name);
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
    | atom '.' atom     {
            $$.object = $1;
            $$.name = $3;
            $$.auto_create = 0;
        }
    | atom K_DOT_DOT atom     {
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
    : K_NEW atom '(' K_VARIABLE ')' opt_property_bindings {
            /* Convert the object construction into a predicate
             * call on new/2 or new/3, passing the property
             * bindings as a list to new/3 */
            if ($6 == p_term_nil_atom(context))
                $$ = binary_term("new", $2, $4);
            else
                $$ = ternary_term("new", $2, $6, $4);
        }
    ;

opt_property_bindings
    : property_bindings         { $$ = $1; }
    | /* empty */               { $$ = p_term_nil_atom(context); }
    ;

property_bindings
    : '{' properties '}'        { $$ = finalize_list($2); }
    | '{' properties ',' '}'    { $$ = finalize_list($2); }
    | '{' '}'                   { $$ = p_term_nil_atom(context); }
    | '{' error '}'             { $$ = p_term_nil_atom(context); }
    ;

properties
    : properties ',' property   { append_list($$, $1, $3); }
    | property                  { create_list($$, $1); }
    ;

property
    : atom ':' argument_term    { $$ = binary_term(":", $1, $3); }
    ;

statements
    : statements statement  {
            p_term *stmt = add_debug(@2, $2);
            append_r_list($$, $1, stmt);
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
            $$ = add_debug(@1, p_term_create_atom(context, "true"));
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
            p_term_bind_functor_arg
                ($$, 1, p_term_create_atom(context, "true"));
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
    : '(' term ')'      { $$ = $2; }
    | '(' error ')'     { $$ = p_term_create_atom(context, "fail"); }
    ;

compound_statement
    : '{' statements '}'        { $$ = finalize_r_list($2); }
    | '{' '}'           { $$ = p_term_create_atom(context, "true"); }
    | '{' error '}'     { $$ = p_term_create_atom(context, "fail"); }
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
            if (p_term_lex_variable_count
                    (p_term_get_extra(yyscanner), $4) > 1) {
                /* The loop variable cannot appear previously
                 * in the clause; it must be a new variable */
                yyerror_printf
                    (&(@4), context, yyscanner,
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
                $$.default_case = p_term_create_atom(context, "fail");
        }
    | /* empty */       {
            $$.case_list = p_term_nil_atom(context);
            $$.default_case = p_term_create_atom(context, "fail");
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
                        (&(@2), context, yyscanner,
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
                            (&(@2), context, yyscanner,
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
    : K_CLASS atom class_body opt_semi    {
            $$ = make_class_declaration
                (context, $2, p_term_nil_atom(context),
                 $3.vars, $3.clauses);
        }
    | K_CLASS atom ':' atom class_body opt_semi {
            $$ = make_class_declaration
                (context, $2, $4, $5.vars, $5.clauses);
        }
    ;

opt_semi
    : /* empty */
    | ';'
    ;

class_body
    : '{' class_members '}' {
            $$.vars = finalize_list($2.vars);
            $$.clauses = finalize_list($2.clauses);
        }
    | '{' '}'   {
            $$.vars = p_term_nil_atom(context);
            $$.clauses = p_term_nil_atom(context);
        }
    | '{' error '}' {
            $$.vars = p_term_nil_atom(context);
            $$.clauses = p_term_nil_atom(context);
        }
    ;

class_members
    : class_members class_member    {
            append_lists($$.vars, $1.vars, $2.vars);
            append_lists($$.clauses, $1.clauses, $2.clauses);
        }
    | class_member                  { $$ = $1; }
    ;

class_member
    : K_VAR member_vars opt_semi    {
            $$.vars.l_head = $2.l_head;
            $$.vars.l_tail = $2.l_tail;
            create_empty_list($$.clauses);
        }
    | clause    {
            /* Convert the ":-/2" clause form into ":-/3" with
             * the "Self" variable included in the arguments */
            p_term *self = p_term_lex_create_variable
                (context, p_term_get_extra(yyscanner), "Self");
            p_term *clause = p_term_create_functor
                (context, p_term_functor($1), 3);
            p_term_bind_functor_arg(clause, 0, p_term_arg($1, 0));
            p_term_bind_functor_arg(clause, 1, self);
            p_term_bind_functor_arg(clause, 2, p_term_arg($1, 1));
            create_empty_list($$.vars);
            clause = add_debug(@1, clause);
            create_list($$.clauses, clause);
            clear_variables();
        }
    ;

member_vars
    : member_vars ',' member_var    { append_list($$, $1, $3); }
    | member_var                    { create_list($$, $1); }
    ;

member_var
    : atom                          { $$ = add_debug(@1, $1); }
    ;

object_declaration
    : K_NEW atom property_bindings opt_semi     {
            /* An object declaration is equivalent to:
             * ?- new class_name (X) { properties }, assertz(X). */
            p_term *var;
            p_term *term;
            p_term *term2;
            var = p_term_create_variable(context);
            if ($3 == p_term_nil_atom(context))
                term = binary_term("new", $2, var);
            else
                term = ternary_term("new", $2, $3, var);
            term2 = p_term_create_functor_with_args
                (context, p_term_create_atom(context, "assertz"),
                 &var, 1);
            $$ = unary_term("?-", binary_term(",", term, term2));
        }
    ;
