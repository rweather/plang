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

#include <plang/context.h>
#include <plang/term.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <config.h>

static const char shell_main[] =
    ":- import(shell).\n"
    ":- import(stdout).\n"
    "shell::frontend_main()\n"
    "{\n"
    "    stdout::writeln(\"Plang version " VERSION "\");\n"
    "    stdout::writeln(\"Copyright (c) 2011,2012 Southern Storm Software, Pty Ltd.\");\n"
    "    stdout::writeln(\"Type 'help.' for help\");\n"
    "    stdout::writeln();\n"
    "    shell::main(\"| ?- \");\n"
    "}\n";

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    p_context *context;
    p_term *args;
    p_term *goal;
    p_term *error_term;
    p_term *main_atom;
    int error, index;
    p_goal_result result;
    int exitval;
    const char *filename;
    const char *main_pred = "main";

    /* Process leading options for the plang engine itself */
    context = p_context_create();
    while (argc > 1 && argv[1][0] == '-') {
        if (!strcmp(argv[1], "-I") || !strcmp(argv[1], "--import")) {
            ++argv;
            --argc;
            if (argc <= 1) {
                fprintf(stderr, "%s: missing import pathname\n",
                        progname);
                p_context_free(context);
                return 1;
            }
            p_context_add_import_path(context, argv[1]);
        } else if (!strncmp(argv[1], "-I", 2)) {
            p_context_add_import_path(context, argv[1] + 2);
        } else if (!strncmp(argv[1], "--import=", 9)) {
            p_context_add_import_path(context, argv[1] + 9);
        } else if (!strcmp(argv[1], "-L") || !strcmp(argv[1], "--import-lib")) {
            ++argv;
            --argc;
            if (argc <= 1) {
                fprintf(stderr, "%s: missing import library pathname\n",
                        progname);
                p_context_free(context);
                return 1;
            }
            p_context_add_library_path(context, argv[1]);
        } else if (!strncmp(argv[1], "-L", 2)) {
            p_context_add_library_path(context, argv[1] + 2);
        } else if (!strncmp(argv[1], "--import-lib=", 13)) {
            p_context_add_library_path(context, argv[1] + 9);
        } else if (!strcmp(argv[1], "-m") || !strcmp(argv[1], "--main")) {
            ++argv;
            --argc;
            if (argc <= 1) {
                fprintf(stderr, "%s: missing main predicate name\n",
                        progname);
                p_context_free(context);
                return 1;
            }
            main_pred = argv[1];
        } else if (!strncmp(argv[1], "-m", 2)) {
            main_pred = argv[1] + 2;
        } else if (!strncmp(argv[1], "--main=", 7)) {
            main_pred = argv[1] + 7;
        } else if (!strcmp(argv[1], "--")) {
            ++argv;
            --argc;
            break;
        } else {
            fprintf(stderr, "%s: unknown option `%s'\n",
                    progname, argv[1]);
            p_context_free(context);
            return 1;
        }
        ++argv;
        --argc;
    }

    /* Load the contents of the input file.  If no file supplied,
     * then load up an interactive shell */
    if (argc < 2) {
        error = p_context_consult_string(context, shell_main);
        filename = "shell.lp";
        main_pred = "shell::frontend_main";
    } else {
        filename = argv[1];
        error = p_context_consult_file
            (context, argv[1], P_CONSULT_DEFAULT);
    }
    if (error == EINVAL) {
        /* Syntax error that should have already been reported */
        p_context_free(context);
        return 1;
    } else if (error != 0) {
        /* Filesystem error */
        fprintf(stderr, "%s: %s\n", filename, strerror(error));
        p_context_free(context);
        return 1;
    }

    /* Create the argument list to pass to main/1 */
    args = p_term_nil_atom(context);
    for (index = argc - 1; index >= 1; --index) {
        args = p_term_create_list
            (context, p_term_create_string(context, argv[index]), args);
    }

    /* Create and execute the main(Args) or main() goal */
    main_atom = p_term_create_atom(context, main_pred);
    if (p_term_lookup_predicate(context, main_atom, 1)) {
        goal = p_term_create_functor(context, main_atom, 1);
        p_term_bind_functor_arg(goal, 0, args);
    } else if (p_term_lookup_predicate(context, main_atom, 0)) {
        goal = main_atom;
    } else {
        /* There is no main/1 or main/0 predicate, so force an error */
        goal = p_term_create_functor(context, main_atom, 1);
        p_term_bind_functor_arg(goal, 0, args);
    }
    error_term = 0;
    result = p_context_execute_goal(context, goal, &error_term);
    if (result == P_RESULT_TRUE) {
        exitval = 0;
    } else if (result == P_RESULT_FAIL) {
        exitval = 1;
    } else if (result == P_RESULT_HALT) {
        exitval = p_term_integer_value(error_term);
        if (exitval < 0 || exitval > 127)
            exitval = 127;
    } else {
        fprintf(stderr, "%s: %s/1 threw uncaught error: ",
                filename, main_pred);
        p_term_print(context, error_term, p_term_stdio_print_func, stderr);
        putc('\n', stderr);
        exitval = 1;
    }

    /* Clean up and exit */
    p_context_free(context);
    return exitval;
}
