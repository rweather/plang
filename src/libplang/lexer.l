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
#include "parser.h"
#include "parser-priv.h"
#include "term-priv.h"

/* Get the buffer of input from the input stream */
#define	YY_INPUT(buf,result,maxSize) \
    { \
        if (yyextra->stream) { \
            /* Read from a stdio-based file stream */ \
            result = fread((buf), 1, (maxSize), yyextra->stream); \
        } else { \
            /* Read from a memory buffer */ \
            size_t len = (maxSize); \
            if (len > yyextra->buffer_len) \
                len = yyextra->buffer_len; \
            if (len > 0) { \
                memcpy((buf), yyextra->buffer, len); \
                yyextra->buffer += len; \
                yyextra->buffer_len -= len; \
            } \
            result = (int)len; \
        } \
    }

/* Update the line number information and return a token code */
#if YYLTYPE_IS_DECLARED || defined(YYLTYPE)
#define	RETURNTOK(x)    \
    do { \
        loc->first_line = loc->last_line = yylineno; \
        loc->first_column = loc->last_column = yycolumn; \
        return (x); \
    } while (0)
#else
#define	RETURNTOK(x)    return (x)
#endif

/* Redeclare yylex to match the parser */
#define YY_DECL int yylex(YYSTYPE *lval, YYLTYPE *loc, \
                          p_context *context, yyscan_t *yyscanner)

#define YY_NO_INPUT 1

/* Create a named variable, reusing an existing variable if available */
p_term *p_term_lex_create_variable
    (p_context *context, p_input_stream *stream, const char *name)
{
    size_t index;
    p_term *var;
    struct p_input_var *vars;
    for (index = 0; index < stream->num_variables; ++index) {
        if (!strcmp(p_term_name(stream->variables[index].var), name)) {
            ++(stream->variables[index].count);
            return stream->variables[index].var;
        }
    }
    var = p_term_create_named_variable(context, name);
    if (stream->num_variables < stream->max_variables) {
        stream->variables[stream->num_variables].var = var;
        stream->variables[(stream->num_variables)++].count = 1;
        return var;
    }
    index = stream->max_variables * 2;
    if (index <= 16)
        index = 16;
    stream->max_variables = index;
    vars = (struct p_input_var *)GC_MALLOC_UNCOLLECTABLE(index * sizeof(struct p_input_var));
    if (stream->variables) {
        memcpy(vars, stream->variables,
               sizeof(struct p_input_var) * stream->num_variables);
        GC_FREE(stream->variables);
    }
    stream->variables = vars;
    vars[stream->num_variables].var = var;
    vars[(stream->num_variables)++].count = 1;
    return var;
}

/* Returns the number of times that a variable has been referenced */
unsigned int p_term_lex_variable_count(p_input_stream *stream, p_term *var)
{
    size_t index;
    for (index = 0; index < stream->num_variables; ++index) {
        if (stream->variables[index].var == var)
            return stream->variables[index].count;
    }
    return 0;
}

%}

%option outfile="lex.yy.c"
%option prefix="p_term_"
%option noyywrap
%option nounput
%option reentrant

%x MULTI_LINE_COMMENT

DIGIT           [0-9]
HEX             [a-fA-F0-9]
OCT             [0-7]
BIN             [01]
ATOMPREFIX      [a-z]
VARPREFIX       [A-Z_]
IDENTCHAR       ([a-zA-Z_0-9]|"::")
EXPONENT        [Ee][+-]?{DIGIT}+
WHITE           [ \t\r\f\v\032]
AFTERDOT        ({WHITE}|[\n%]|"/*")
SQCHAR          ("''"|[^'\\]|\\{OCT}+\\|\\x{HEX}+\\|\\[abrftnv]|\\)
DQCHAR          ("\"\""|[^"\\]|\\{OCT}+\\|\\x{HEX}+\\|\\[abrftnv]|\\)

%%

":-"            { RETURNTOK(K_COLON_DASH); }
"?-"            { RETURNTOK(K_QUEST_DASH); }
"->"            { RETURNTOK(K_ARROW); }
"."/{AFTERDOT}  { RETURNTOK(K_DOT_TERMINATOR); }
"||"            { RETURNTOK(K_OR); }
"&&"            { RETURNTOK(K_OR); }
"\\+"           { RETURNTOK(K_NOT); }
"\\="           { RETURNTOK(K_NE); }
"!="            { RETURNTOK(K_NE); }
"=="            { RETURNTOK(K_TERM_EQ); }
"\\=="          { RETURNTOK(K_TERM_NE); }
"!=="           { RETURNTOK(K_TERM_NE); }
"@<"            { RETURNTOK(K_TERM_LT); }
"@=<"           { RETURNTOK(K_TERM_LE); }
"@<="           { RETURNTOK(K_TERM_LE); }
"@>"            { RETURNTOK(K_TERM_GT); }
"@>="           { RETURNTOK(K_TERM_GE); }
"=.."           { RETURNTOK(K_UNIV); }
"=:="           { RETURNTOK(K_NUM_EQ); }
"=\\="          { RETURNTOK(K_NUM_NE); }
"=!="           { RETURNTOK(K_NUM_NE); }
"<"             { RETURNTOK(K_NUM_LT); }
"=<"            { RETURNTOK(K_NUM_LE); }
"<="            { RETURNTOK(K_NUM_LE); }
">"             { RETURNTOK(K_NUM_GT); }
">="            { RETURNTOK(K_NUM_GE); }
"::="           { RETURNTOK(K_NUM_GETS); }
"/\\"           { RETURNTOK(K_BITWISE_AND); }
"\\/"           { RETURNTOK(K_BITWISE_OR); }
"\\"            { RETURNTOK(K_BITWISE_NOT); }
"//"            { RETURNTOK(K_INT_DIV); }
"<<"            { RETURNTOK(K_SHIFT_LEFT); }
">>"            { RETURNTOK(K_SHIFT_RIGHT); }
"**"            { RETURNTOK(K_EXP); }
".."            { RETURNTOK(K_DOT_DOT); }
":="            { RETURNTOK(K_GETS); }
"accept"        { RETURNTOK(K_ACCEPT); }
"class"         { RETURNTOK(K_CLASS); }
"do"            { RETURNTOK(K_DO); }
"else"          { RETURNTOK(K_ELSE); }
"fail"          { RETURNTOK(K_FAIL); }
"for"           { RETURNTOK(K_FOR); }
"if"            { RETURNTOK(K_IF); }
"in"            { RETURNTOK(K_IN); }
"is"            { RETURNTOK(K_IS); }
"loop"          { RETURNTOK(K_LOOP); }
"mod"           { RETURNTOK(K_MOD); }
"new"           { RETURNTOK(K_NEW); }
"rem"           { RETURNTOK(K_REM); }
"step"          { RETURNTOK(K_STEP); }
"stop"          { RETURNTOK(K_STOP); }
"to"            { RETURNTOK(K_TO); }
"while"         { RETURNTOK(K_WHILE); }
"var"           { RETURNTOK(K_VAR); }

"_"             {
        /* Anonymous variable: fresh variable each time it is used */
        lval->term = p_term_create_variable(context);
        RETURNTOK(K_VARIABLE);
    }

{ATOMPREFIX}{IDENTCHAR}*    {
        lval->term = p_term_create_atom(context, yytext);
        RETURNTOK(K_ATOM);
    }
{VARPREFIX}{IDENTCHAR}*     {
        lval->term = p_term_lex_create_variable
            (context, yyextra, yytext);
        RETURNTOK(K_VARIABLE);
    }

{DIGIT}+"."{DIGIT}+({EXPONENT}) {
        lval->term = p_term_create_real(context, atof(yytext));
        RETURNTOK(K_REAL);
    }

"0'"{SQCHAR}    { /* TODO */ }

"0x"{HEX}+      {
        lval->term = p_term_create_integer
            (context, strtol(yytext + 2, (char **)0, 16));
        RETURNTOK(K_INTEGER);
    }
"0o"{OCT}+      {
        lval->term = p_term_create_integer
            (context, strtol(yytext + 2, (char **)0, 8));
        RETURNTOK(K_INTEGER);
    }
"0b"{BIN}+      {
        lval->term = p_term_create_integer
            (context, strtol(yytext + 2, (char **)0, 2));
        RETURNTOK(K_INTEGER);
    }
{DIGIT}+        {
        lval->term = p_term_create_integer
            (context, strtol(yytext, (char **)0, 10));
        RETURNTOK(K_INTEGER);
    }

"'"{SQCHAR}*"'"         { /* TODO */ }
"\""{DQCHAR}*"\""       { /* TODO */ }

{WHITE}+        ;
\n              { ++yylineno; }
"%".*\n         { ++yylineno; }
"#!".*\n        { ++yylineno; }

"/*"            { BEGIN(MULTI_LINE_COMMENT); }

.	        { RETURNTOK(((int)(yytext[0])) & 0xFF); }

<MULTI_LINE_COMMENT>"*/"        { BEGIN(INITIAL); }
<MULTI_LINE_COMMENT>[^*\n]+     ;
<MULTI_LINE_COMMENT>"*"         ;
<MULTI_LINE_COMMENT>\n          { ++yylineno; }

%%