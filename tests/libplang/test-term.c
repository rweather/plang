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
#include <plang/term.h>

P_TEST_DECLARE();

static void test_atom()
{
    p_term *atom1;
    p_term *atom2;
    p_term *atom3;
    p_term *atom4;
    int value;
    char name[64];

    atom1 = p_term_create_atom(context, "foo");
    P_VERIFY(atom1 != 0);
    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_COMPARE(p_term_type(atom1), P_TERM_ATOM);

    atom2 = p_term_create_atom(context, "foo");
    P_VERIFY(atom1 == atom2);

    atom3 = p_term_create_atom(context, "bar");
    P_VERIFY(atom3 != 0);
    P_VERIFY(atom3 != atom1);
    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(atom3), "bar") == 0);

    atom4 = p_term_create_atom(context, 0);
    P_VERIFY(strcmp(p_term_name(atom4), "") == 0);
    P_VERIFY(p_term_create_atom(context, "") == atom4);
    P_VERIFY(p_term_create_atom(context, 0) == atom4);

    /* Load up the hash table to check overflow handling */
    for (value = 0; value < 1024; ++value) {
        sprintf(name, "%d", value);
        p_term_create_atom(context, name);
    }
    for (value = 0; value < 1024; ++value) {
        sprintf(name, "%d", value);
        atom4 = p_term_create_atom(context, name);
        P_VERIFY(strcmp(p_term_name(atom4), name) == 0);
        P_COMPARE(p_term_type(atom4), P_TERM_ATOM);
    }

    P_VERIFY(strcmp(p_term_name(atom1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(atom3), "bar") == 0);
}

static void test_standard_atoms()
{
    p_term *nil_atom = p_term_nil_atom(context);
    p_term *prototype_atom = p_term_prototype_atom(context);
    p_term *class_name_atom = p_term_class_name_atom(context);

    P_VERIFY(nil_atom != 0);
    P_VERIFY(prototype_atom != 0);
    P_VERIFY(class_name_atom != 0);

    P_VERIFY(strcmp(p_term_name(nil_atom), "[]") == 0);
    P_VERIFY(strcmp(p_term_name(prototype_atom), "prototype") == 0);
    P_VERIFY(strcmp(p_term_name(class_name_atom), "className") == 0);

    P_VERIFY(p_term_nil_atom(context) == nil_atom);
    P_VERIFY(p_term_prototype_atom(context) == prototype_atom);
    P_VERIFY(p_term_class_name_atom(context) == class_name_atom);
}

static void test_string()
{
    p_term *string1;
    p_term *string2;
    p_term *string3;
    p_term *string4;

    string1 = p_term_create_string(context, "foo");
    P_VERIFY(string1 != 0);
    P_VERIFY(strcmp(p_term_name(string1), "foo") == 0);
    P_COMPARE(p_term_type(string1), P_TERM_STRING);

    string2 = p_term_create_string(context, "foo");
    P_VERIFY(string1 != string2);   /* Strings are not hashed */
    P_VERIFY(strcmp(p_term_name(string2), "foo") == 0);

    string3 = p_term_create_string(context, "bar");
    P_VERIFY(string3 != 0);
    P_VERIFY(string3 != string1);
    P_VERIFY(string3 != string2);
    P_VERIFY(strcmp(p_term_name(string1), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(string2), "foo") == 0);
    P_VERIFY(strcmp(p_term_name(string3), "bar") == 0);

    string4 = p_term_create_string(context, 0);
    P_VERIFY(strcmp(p_term_name(string4), "") == 0);

    string4 = p_term_create_string(context, "");
    P_VERIFY(strcmp(p_term_name(string4), "") == 0);
}

static void test_integer()
{
    p_term *int1;
    p_term *int2;
    p_term *int3;
    p_term *int4;
    p_term *int5;

    int1 = p_term_create_integer(context, 0);
    P_VERIFY(int1 != 0);
    P_COMPARE(p_term_integer_value(int1), 0);
    P_COMPARE(p_term_type(int1), P_TERM_INTEGER);

    int2 = p_term_create_integer(context, 124);
    P_COMPARE(p_term_integer_value(int2), 124);

    int3 = p_term_create_integer(context, -124);
    P_COMPARE(p_term_integer_value(int3), -124);

    int4 = p_term_create_integer(context, 0x7fffffff);
    P_COMPARE(p_term_integer_value(int4), 0x7fffffff);

    int5 = p_term_create_integer(context, (int)(-0x7fffffff - 1));
    P_COMPARE(p_term_integer_value(int5), (int)(-0x7fffffff - 1));

    P_COMPARE(p_term_integer_value(int1), 0);
    P_COMPARE(p_term_integer_value(int2), 124);
    P_COMPARE(p_term_integer_value(int3), -124);
    P_COMPARE(p_term_integer_value(int4), 0x7fffffff);
    P_COMPARE(p_term_integer_value(int5), (int)(-0x7fffffff - 1));
}

static void test_real()
{
    p_term *real1;
    p_term *real2;
    p_term *real3;
    p_term *real4;
    p_term *real5;

    real1 = p_term_create_real(context, 0.0);
    P_VERIFY(real1 != 0);
    P_COMPARE(p_term_real_value(real1), 0.0);
    P_COMPARE(p_term_type(real1), P_TERM_REAL);

    real2 = p_term_create_real(context, 124.0);
    P_COMPARE(p_term_real_value(real2), 124.0);

    real3 = p_term_create_real(context, -124.5);
    P_COMPARE(p_term_real_value(real3), -124.5);

    real4 = p_term_create_real(context, 1e12);
    P_COMPARE(p_term_real_value(real4), 1e12);

    real5 = p_term_create_real(context, 1e-12);
    P_COMPARE(p_term_real_value(real5), 1e-12);

    P_COMPARE(p_term_real_value(real1), 0);
    P_COMPARE(p_term_real_value(real2), 124.0);
    P_COMPARE(p_term_real_value(real3), -124.5);
    P_COMPARE(p_term_real_value(real4), 1e12);
    P_COMPARE(p_term_real_value(real5), 1e-12)
}

static void test_list()
{
    p_term *member1;
    p_term *member2;
    p_term *member3;
    p_term *nil;
    p_term *list1;
    p_term *list2;

    member1 = p_term_create_atom(context, "foo");
    member2 = p_term_create_string(context, "bar");
    member3 = p_term_create_integer(context, 42);
    nil = p_term_nil_atom(context);

    list1 = p_term_create_list(context, member1, nil);
    P_VERIFY(p_term_head(list1) == member1);
    P_VERIFY(p_term_tail(list1) == nil);
    P_COMPARE(p_term_type(list1), P_TERM_LIST);

    list2 = p_term_create_list(context, member1,
                p_term_create_list(context, member2,
                    p_term_create_list(context, member3, nil)));
    P_VERIFY(p_term_head(list2) == member1);
    P_VERIFY(p_term_head(p_term_tail(list2)) == member2);
    P_VERIFY(p_term_head(p_term_tail(p_term_tail(list2))) == member3);
    P_VERIFY(p_term_tail(p_term_tail(p_term_tail(list2))) == nil);
}

int main(int argc, char *argv[])
{
    P_TEST_INIT("test-term");
    P_TEST_CREATE_CONTEXT();

    P_TEST_RUN(atom);
    P_TEST_RUN(standard_atoms);
    P_TEST_RUN(string);
    P_TEST_RUN(integer);
    P_TEST_RUN(real);
    P_TEST_RUN(list);

    P_TEST_REPORT();
    return P_TEST_EXIT_CODE();
}
