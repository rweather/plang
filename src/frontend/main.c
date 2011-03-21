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
#include <plang/term.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    p_context *context;
    p_term *args;
    p_term *goal;
    p_term *error_term;
    int error, index;
    p_goal_result result;
    int exitval;

    /* Bail out if no input file provided */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s program [args ...]\n", argv[0]);
        return 1;
    }

    /* Load the contents of the input file */
    context = p_context_create();
    error = p_context_consult_file(context, argv[1]);
    if (error != 0) {
        fprintf(stderr, "%s: %s\n", argv[1], strerror(error));
        p_context_free(context);
        return 1;
    }

    /* Create the argument list to pass to main/1 */
    args = p_term_nil_atom(context);
    for (index = argc - 1; index >= 1; --index) {
        args = p_term_create_list
            (context, p_term_create_string(context, argv[index]), args);
    }

    /* Create and execute the main(Args) goal */
    goal = p_term_create_functor
        (context, p_term_create_atom(context, "main"), 1);
    p_term_bind_functor_arg(goal, 0, args);
    error_term = 0;
    result = p_context_execute_goal(context, goal, &error_term);
    if (result == P_RESULT_TRUE || result == P_RESULT_CUT_TRUE) {
        exitval = 0;
    } else if (result == P_RESULT_FAIL || result == P_RESULT_CUT_FAIL) {
        exitval = 1;
    } else {
        fprintf(stderr, "%s: main/1 threw uncaught error: ", argv[1]);
        p_term_print(context, error_term, p_term_stdio_print_func, stderr);
        putc('\n', stderr);
        exitval = 1;
    }

    /* Clean up and exit */
    p_context_free(context);
    return exitval;
}
