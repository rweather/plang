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

#ifndef _TESTCASE_H
#define _TESTCASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <plang/context.h>

#define P_TEST_DECLARE()   \
    const char *_test_program; \
    const char *_test_name; \
    const char *_test_row_name; \
    jmp_buf _jump_back; \
    int _num_passed; \
    int _num_failed; \
    int _report_only_failures; \
    p_context *context;

#define P_FAIL(msg)   \
    do { \
        if (_test_row_name) \
            printf("%s: %s(%s): %s\n", _test_program, _test_name, \
                   _test_row_name, msg); \
        else \
            printf("%s: %s: %s\n", _test_program, _test_name, msg); \
        printf("\tfailed at %s:%d\n", __FILE__, __LINE__); \
        longjmp(_jump_back, 1); \
    } while (0)
#define P_WARNING(msg)   \
    do { \
        if (_test_row_name) \
            printf("%s: %s(%s): warning: %s\n", _test_program, _test_name, \
                   _test_row_name, msg); \
        else \
            printf("%s: %s: warning: %s\n", _test_program, _test_name, msg); \
    } while (0)
#define P_VERIFY(cond) \
    if (!(cond)) { \
        if (_test_row_name) \
            printf("%s: %s(%s): %s failed\n", _test_program, _test_name, \
                   _test_row_name, #cond); \
        else \
            printf("%s: %s: %s failed\n", _test_program, _test_name, #cond); \
        printf("\tfailed at %s:%d\n", __FILE__, __LINE__); \
        longjmp(_jump_back, 1); \
    }
#define P_COMPARE(actual, expected) \
    if ((actual) != (expected)) { \
        if (_test_row_name) \
            printf("%s: %s(%s): %s == %s failed\n", _test_program, _test_name, \
                   _test_row_name, #actual, #expected); \
        else \
            printf("%s: %s: %s == %s failed\n", _test_program, _test_name, \
                   #actual, #expected); \
        printf("\tfailed at %s:%d\n", __FILE__, __LINE__); \
        longjmp(_jump_back, 1); \
    }

#define P_TEST_RUN(name)   \
    do { \
        int jmpval; \
        _test_name = #name; \
        _test_row_name = 0; \
        if ((jmpval = setjmp(_jump_back)) == 0) { \
            test_##name(); \
            if (!_report_only_failures) \
                printf("%s: %s: ok\n", _test_program, _test_name); \
            ++_num_passed; \
        } else if (jmpval == 1) { \
            ++_num_failed; \
        } \
    } while (0)

#define P_TEST_INIT(program)  \
    do { \
        char *rep = getenv("P_REPORT_ONLY_FAILURES"); \
        _test_program = program; \
        _test_name = 0; \
        _num_passed = 0; \
        _num_failed = 0; \
        _report_only_failures = (rep != 0 && *rep == '1'); \
        if (!_report_only_failures) \
            printf("%s: starting tests\n", _test_program); \
    } while (0)

#define P_TEST_REPORT() \
    do { \
        if (!_report_only_failures || _num_failed > 0) \
            printf("%s: %d passed, %d failed\n", \
                   _test_program, _num_passed, _num_failed); \
        if (context) \
            p_context_free(context); \
    } while (0)

#define P_TEST_CREATE_CONTEXT() \
    do { \
        context = p_context_create(); \
    } while (0)

#define P_TEST_EXIT_CODE() (_num_failed ? 1 : 0)

#define P_TEST_SET_ROW(name)   _test_row_name = (name)

#ifdef __cplusplus
};
#endif

#endif
