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

#include "testcase.h"
#include <plang/database.h>

P_TEST_DECLARE();

static void test_operators()
{
    struct op_info
    {
        int priority;
        p_op_specifier specifier;
        int arity;
        const char *name;
    };
    static struct op_info const ops[] = {
        {1200, P_OP_XFX, 2, ":-"},
        {1200, P_OP_XFX, 2, "-->"},
        {1200, P_OP_FX,  1, ":-"},
        {1200, P_OP_FX,  1, "?-"},
        {1100, P_OP_XFY, 2, ";"},
        {1100, P_OP_XFY, 2, "||"},
        {1050, P_OP_XFY, 2, "->"},
        {1000, P_OP_XFY, 2, ","},
        {1000, P_OP_XFY, 2, "&&"},
        { 900, P_OP_FY,  1, "\\+"},
        { 900, P_OP_FY,  1, "!"},
        { 700, P_OP_XFX, 2, "="},
        { 700, P_OP_XFX, 2, "\\="},
        { 700, P_OP_XFX, 2, "!="},
        { 700, P_OP_XFX, 2, "=="},
        { 700, P_OP_XFX, 2, "\\=="},
        { 700, P_OP_XFX, 2, "!=="},
        { 700, P_OP_XFX, 2, "@<"},
        { 700, P_OP_XFX, 2, "@=<"},
        { 700, P_OP_XFX, 2, "@<="},
        { 700, P_OP_XFX, 2, "@>"},
        { 700, P_OP_XFX, 2, "@>="},
        { 700, P_OP_XFX, 2, "=.."},
        { 700, P_OP_XFX, 2, "is"},
        { 700, P_OP_XFX, 2, "in"},
        { 700, P_OP_XFX, 2, "=:="},
        { 700, P_OP_XFX, 2, "=\\="},
        { 700, P_OP_XFX, 2, "=!="},
        { 700, P_OP_XFX, 2, "<"},
        { 700, P_OP_XFX, 2, "=<"},
        { 700, P_OP_XFX, 2, "<="},
        { 700, P_OP_XFX, 2, ">"},
        { 700, P_OP_XFX, 2, ">="},
        { 700, P_OP_XFX, 2, ":="},
        { 700, P_OP_XFX, 2, "::="},
        { 500, P_OP_YFX, 2, "+"},
        { 500, P_OP_YFX, 2, "-"},
        { 500, P_OP_YFX, 2, "/\\"},
        { 500, P_OP_YFX, 2, "\\/"},
        { 400, P_OP_YFX, 2, "*"},
        { 400, P_OP_YFX, 2, "/"},
        { 400, P_OP_YFX, 2, "//"},
        { 400, P_OP_YFX, 2, "rem"},
        { 400, P_OP_YFX, 2, "mod"},
        { 400, P_OP_YFX, 2, "<<"},
        { 400, P_OP_YFX, 2, ">>"},
        { 200, P_OP_XFX, 2, "**"},
        { 200, P_OP_XFY, 2, "^"},
        { 200, P_OP_FY,  1, "-"},
        { 200, P_OP_FY,  1, "\\"},
        { 200, P_OP_FY,  1, "~"},
        { 100, P_OP_XFX, 2, ":"},
        {  50, P_OP_YFX, 2, "."},
        {   0, P_OP_NONE,0, 0},
    };
    int index, priority;
    p_op_specifier specifier;
    for (index = 0; ops[index].priority; ++index) {
        P_TEST_SET_ROW(ops[index].name);
        specifier = p_db_operator_info
            (p_term_create_atom(context, ops[index].name),
             ops[index].arity, &priority);
        P_COMPARE(specifier, ops[index].specifier);
        P_COMPARE(priority, ops[index].priority);
    }
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-database");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(operators);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
