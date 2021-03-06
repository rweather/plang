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
#include <errno.h>
#include <stdarg.h>
#include "parser.h"
#include "parser-priv.h"
#include "term-priv.h"

/* Get the buffer of input from the input stream */
#define	YY_INPUT(buf,result,maxSize) \
    { \
        result = (*(yyextra->read_func))(yyextra, (buf), (maxSize)); \
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

/* Create a named variable, reusing an existing variable if available */
p_term *p_term_lex_create_variable
    (p_context *context, p_input_stream *stream, const char *name)
{
    size_t index;
    p_term *name_term;
    p_term *var;
    struct p_input_var *vars;
    for (index = 0; index < stream->num_variables; ++index) {
        if (!strcmp(p_term_name(stream->variables[index].name), name)) {
            ++(stream->variables[index].count);
            return stream->variables[index].var;
        }
    }
    name_term = p_term_create_string(context, name);
    var = p_term_create_variable(context);
    if (stream->num_variables < stream->max_variables) {
        stream->variables[stream->num_variables].name = name_term;
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
    vars[stream->num_variables].name = name_term;
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

static p_term *p_term_read_quoted_string(p_context *context, yyscan_t scanner, int type, int quote);
static void yylex_error_printf(yyscan_t scanner, const char *format, ...);

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
NEG             (-?)

%%

":-"            { RETURNTOK(K_COLON_DASH); }
"?-"            { RETURNTOK(K_QUEST_DASH); }
"??--"          { RETURNTOK(K_TEST_GOAL); }
"??-"           { RETURNTOK(K_READ_TERM); }
"->"            { RETURNTOK(K_ARROW); }
"-->"           { RETURNTOK(K_DARROW); }
"."/{AFTERDOT}  { RETURNTOK(K_DOT_TERMINATOR); }
"||"            { RETURNTOK(K_OR); }
"&&"            { RETURNTOK(K_AND); }
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
"::=="          { RETURNTOK(K_NUM_BT_GETS); }
"/\\"           { RETURNTOK(K_BITWISE_AND); }
"\\/"           { RETURNTOK(K_BITWISE_OR); }
"\\"            { RETURNTOK(K_BITWISE_NOT); }
"<<"            { RETURNTOK(K_SHIFT_LEFT); }
">>"            { RETURNTOK(K_SHIFT_RIGHT); }
">>>"           { RETURNTOK(K_USHIFT_RIGHT); }
"**"            { RETURNTOK(K_EXP); }
".."            { RETURNTOK(K_DOT_DOT); }
":="            { RETURNTOK(K_GETS); }
":=="           { RETURNTOK(K_BT_GETS); }
"=>"            { RETURNTOK(K_IMPLIES); }
"<=>"           { RETURNTOK(K_EQUIV); }
"abstract"      { RETURNTOK(K_ABSTRACT); }
"case"          { RETURNTOK(K_CASE); }
"catch"         { RETURNTOK(K_CATCH); }
"class"         { RETURNTOK(K_CLASS); }
"default"       { RETURNTOK(K_DEFAULT); }
"do"            { RETURNTOK(K_DO); }
"else"          { RETURNTOK(K_ELSE); }
"for"           { RETURNTOK(K_FOR); }
"if"            { RETURNTOK(K_IF); }
"in"            { RETURNTOK(K_IN); }
"is"            { RETURNTOK(K_IS); }
"mod"           { RETURNTOK(K_MOD); }
"new"           { RETURNTOK(K_NEW); }
"rem"           { RETURNTOK(K_REM); }
"static"        { RETURNTOK(K_STATIC); }
"switch"        { RETURNTOK(K_SWITCH); }
"try"           { RETURNTOK(K_TRY); }
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

{NEG}{DIGIT}+"."{DIGIT}+({EXPONENT}?) {
        lval->term = p_term_create_real(context, atof(yytext));
        RETURNTOK(K_REAL);
    }
{NEG}{DIGIT}+{EXPONENT}     {
        lval->term = p_term_create_real(context, atof(yytext));
        RETURNTOK(K_REAL);
    }

"0'"        {
        lval->term = p_term_read_quoted_string
            (context, yyscanner, K_INTEGER, '\'');
        RETURNTOK(K_INTEGER);
    }

{NEG}"0"[xX]{HEX}+      {
        if (yytext[0] != '-') {
            lval->term = p_term_create_integer
                (context, (int)(strtol(yytext + 2, (char **)0, 16)));
        } else {
            lval->term = p_term_create_integer
                (context, (int)(-strtol(yytext + 3, (char **)0, 16)));
        }
        RETURNTOK(K_INTEGER);
    }
{NEG}"0"{OCT}+      {
        if (yytext[0] != '-') {
            lval->term = p_term_create_integer
                (context, (int)(strtol(yytext + 1, (char **)0, 8)));
        } else {
            lval->term = p_term_create_integer
                (context, (int)(-strtol(yytext + 2, (char **)0, 8)));
        }
        RETURNTOK(K_INTEGER);
    }
{NEG}{DIGIT}+        {
        lval->term = p_term_create_integer
            (context, (int)(strtol(yytext, (char **)0, 10)));
        RETURNTOK(K_INTEGER);
    }

"'"         {
        lval->term = p_term_read_quoted_string
            (context, yyscanner, K_ATOM, '\'');
        RETURNTOK(K_ATOM);
    }
"\""        {
        lval->term = p_term_read_quoted_string
            (context, yyscanner, K_STRING, '"');
        RETURNTOK(K_STRING);
    }

{WHITE}+        ;
\n              { ++yylineno; }
"//".*\n        { ++yylineno; }
"#!".*\n        { ++yylineno; }

"/*"            { BEGIN(MULTI_LINE_COMMENT); }

.	        { RETURNTOK(((int)(yytext[0])) & 0xFF); }

<MULTI_LINE_COMMENT>"*/"        { BEGIN(INITIAL); }
<MULTI_LINE_COMMENT>[^*\n]+     ;
<MULTI_LINE_COMMENT>"*"         ;
<MULTI_LINE_COMMENT>\n          { ++yylineno; }

%%

#define BUF_ADD_CHAR(ch)    \
    do { \
        if (buflen >= bufmax) { \
            char *temp_buf = (char *)malloc(bufmax * 2); \
            memcpy(temp_buf, buf, buflen); \
            if (buf != static_buf) \
                free(buf); \
            buf = temp_buf; \
            bufmax *= 2; \
        } \
        buf[buflen++] = (char)(ch); \
    } while (0)
#define BUF_ADD_UTF8_CHAR(ch)   \
    do { \
        if (ch < 0x80) { \
            BUF_ADD_CHAR(ch); \
        } else if (ch < (1 << (5 + 6))) { \
            BUF_ADD_CHAR(0xC0 | (ch >> 6)); \
            BUF_ADD_CHAR(0x80 | (ch & 0x3F)); \
        } else if (ch < (1 << (4 + 6 + 6))) { \
            BUF_ADD_CHAR(0xE0 | (ch >> 12)); \
            BUF_ADD_CHAR(0x80 | ((ch >> 6) & 0x3F)); \
            BUF_ADD_CHAR(0x80 | (ch & 0x3F)); \
        } else { \
            BUF_ADD_CHAR(0xF0 | (ch >> 18)); \
            BUF_ADD_CHAR(0x80 | ((ch >> 12) & 0x3F)); \
            BUF_ADD_CHAR(0x80 | ((ch >> 6) & 0x3F)); \
            BUF_ADD_CHAR(0x80 | (ch & 0x3F)); \
        } \
    } while (0)

/* Read a hexadecimal character sequence within a string */
static int p_term_read_hex(yyscan_t scanner, int num_digits, int *value)
{
    int ch;
    *value = 0;
    while (num_digits > 0) {
        ch = input(scanner);
        if (ch >= '0' && ch <= '9')
            *value = (*value << 4) + (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            *value = (*value << 4) + (ch - 'A' + 10);
        else if (ch >= 'a' && ch <= 'f')
            *value = (*value << 4) + (ch - 'a' + 10);
        else {
            yylex_error_printf
                (scanner, "insufficient digits for hex character");
            return ch;
        }
        --num_digits;
    }
    return -1;
}

/* Read a quoted string from the input stream */
static p_term *p_term_read_quoted_string(p_context *context, yyscan_t scanner, int type, int quote)
{
    struct yyguts_t *yyg = (struct yyguts_t *)scanner;
    char static_buf[128];
    char *buf = static_buf;
    size_t buflen = 0;
    size_t bufmax = sizeof(buf);
    int ch, preget_ch, value;
    p_term *term;
    ch = 0;
    preget_ch = 0;
    for (;;) {
        if (!preget_ch)
            ch = input(scanner);
        else
            preget_ch = 0;
        if (ch < 0 || ch == quote)
            break;
        if (ch == '\\') {
            ch = input(scanner);
            if (ch == 'n') {
                BUF_ADD_CHAR('\n');
            } else if (ch == 'r') {
                BUF_ADD_CHAR('\r');
            } else if (ch == 't') {
                BUF_ADD_CHAR('\t');
            } else if (ch == 'f') {
                BUF_ADD_CHAR('\f');
            } else if (ch == 'v') {
                BUF_ADD_CHAR('\v');
            } else if (ch == 'x') {
                /* Raw byte value */
                ch = p_term_read_hex(scanner, 2, &value);
                if (ch >= 0)
                    preget_ch = 1;
                BUF_ADD_CHAR(value);
            } else if (ch == 'u') {
                /* 4-digit unicode character */
                ch = p_term_read_hex(scanner, 4, &value);
                if (ch >= 0)
                    preget_ch = 1;
                BUF_ADD_UTF8_CHAR(value);
            } else if (ch == 'U') {
                /* 8-digit unicode character */
                ch = p_term_read_hex(scanner, 8, &value);
                if (ch >= 0)
                    preget_ch = 1;
                BUF_ADD_UTF8_CHAR(value);
            } else if (ch == '0') {
                BUF_ADD_CHAR('\0');
            } else {
                BUF_ADD_CHAR(ch);
            }
        } else if (ch == '\r' || ch == '\n') {
            yylex_error_printf
                (scanner, "unexpected end of line in string");
            if (ch == '\n')
                ++yylineno;
            break;
        } else {
            BUF_ADD_CHAR(ch);
        }
    }
    if (ch < 0)
        yylex_error_printf(scanner, "unexpected end of file in string");
    BUF_ADD_CHAR('\0');
    if (type == K_ATOM) {
        term = p_term_create_atom_n(context, buf, buflen - 1);
    } else if (type == K_STRING) {
        term = p_term_create_string_n(context, buf, buflen - 1);
    } else {
        size_t ch_size;
        ch = _p_term_next_utf8(buf, buflen - 1, &ch_size);
        if (ch < 0) {
            yylex_error_printf
                (scanner, "invalid character in 0' sequence");
        } else if (ch_size != (buflen - 1)) {
            yylex_error_printf
                (scanner, "single character expected in 0' sequence");
        }
        term = p_term_create_integer(context, ch);
    }
    if (buf != static_buf)
        free(buf);
    return term;
}

static void yylex_error_printf(yyscan_t scanner, const char *format, ...)
{
    struct yyguts_t *yyg = (struct yyguts_t *)scanner;
    va_list va;
    va_start(va, format);
    p_input_stream *stream = yyextra;
    if (stream->filename)
        fprintf(stderr, "%s:%d: ", stream->filename, yylineno);
    else
        fprintf(stderr, "%d: ", yylineno);
    vfprintf(stderr, format, va);
    putc('\n', stderr);
    ++(stream->error_count);
    va_end(va);
}
