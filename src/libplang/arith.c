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

#include <plang/database.h>
#include <plang/errors.h>
#include "term-priv.h"
#include "database-priv.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_FENV_H
#include <fenv.h>
#endif

/**
 * \addtogroup arithmetic
 *
 * Predicates and functions in this group are used to perform
 * arithmetic and string operations.
 *
 * Predicates: \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2",
 * \ref fperror_1 "fperror/1",
 * \ref isnan_1 "isnan/1",
 * \ref isinf_1 "isinf/1"
 *
 * Mathematical operators:
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2",
 * \ref func_pow_2 "(**)/2",
 * \ref func_mod_2 "mod/2",
 * \ref func_rem_2 "rem/2"
 *
 * Mathematical functions:
 * \ref func_abs_1 "abs/1",
 * \ref func_ceil_1 "ceil/1",
 * \ref func_exp_1 "exp/1",
 * \ref func_float_fractional_part_1 "float_fractional_part/1",
 * \ref func_float_integer_part_1 "float_integer_part/1",
 * \ref func_floor_1 "floor/1",
 * \ref func_log_1 "log/1",
 * \ref func_pow_2 "pow/2",
 * \ref func_round_1 "round/1",
 * \ref func_sign_1 "sign/1",
 * \ref func_sqrt_1 "sqrt/1",
 * \ref func_truncate_1 "truncate/1"
 *
 * Mathematical constants:
 * \ref func_e_0 "e/0",
 * \ref func_inf_0 "inf/0",
 * \ref func_nan_0 "nan/0",
 * \ref func_pi_0 "pi/0"
 *
 * Trigonometric functions:
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1"
 *
 * Bitwise operators:
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/2",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 *
 * Type conversion functions:
 * \ref func_float_1 "float/1",
 * \ref func_integer_1 "integer/1",
 * \ref func_string_1 "string/1",
 * \ref func_string_2 "string/2"
 *
 * String functions:
 * \ref func_left_2 "left/2",
 * \ref func_mid_2 "mid/2",
 * \ref func_mid_3 "mid/3",
 * \ref func_right_2 "right/2"
 */

/* Internal expression evaluator */
static p_goal_result p_arith_eval
    (p_context *context, p_arith_value *result,
     p_term *expr, p_term **error)
{
    expr = p_term_deref(expr);
    if (!expr || (expr->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    switch (expr->header.type) {
    case P_TERM_ATOM: {
        p_db_arith func = p_db_builtin_arith(expr, 0);
        if (!func)
            break;
        return (*func)(context, result, 0, 0, error); }
    case P_TERM_FUNCTOR: {
        p_arith_value args[2];
        p_goal_result goal_result;
        p_db_arith func = p_db_builtin_arith
            (expr->functor.functor_name, (int)(expr->header.size));
        if (!func)
            break;
        if (expr->header.size == 2) {
            goal_result = p_arith_eval
                (context, args, expr->functor.arg[0], error);
            if (goal_result != P_RESULT_TRUE)
                return goal_result;
            goal_result = p_arith_eval
                (context, args + 1, expr->functor.arg[1], error);
            if (goal_result != P_RESULT_TRUE)
                return goal_result;
            return (*func)(context, result, args, expr->functor.arg, error);
        } else if (expr->header.size == 1) {
            goal_result = p_arith_eval
                (context, args, expr->functor.arg[0], error);
            if (goal_result != P_RESULT_TRUE)
                return goal_result;
            return (*func)(context, result, args, expr->functor.arg, error);
        } else {
            unsigned int index;
            p_arith_value *args = GC_MALLOC
                (sizeof(p_arith_value) * expr->header.size);
            if (!args)
                break;
            for (index = 0; index < expr->header.size; ++index) {
                goal_result = p_arith_eval
                    (context, args + index,
                     expr->functor.arg[index], error);
                if (goal_result != P_RESULT_TRUE)
                    return goal_result;
            }
            goal_result = (*func)
                (context, result, args, expr->functor.arg, error);
            GC_FREE(args);
            return goal_result;
        }
        break; }
    case P_TERM_INTEGER:
        result->type = P_TERM_INTEGER;
        result->integer_value = p_term_integer_value(expr);
        return P_RESULT_TRUE;
    case P_TERM_REAL:
        result->type = P_TERM_REAL;
        result->real_value = p_term_real_value(expr);
        return P_RESULT_TRUE;
    case P_TERM_STRING:
        result->type = P_TERM_STRING;
        result->string_value = expr;
        return P_RESULT_TRUE;
    default: break;
    }
    *error = p_create_type_error(context, "evaluable", expr);
    return P_RESULT_ERROR;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor is_2
 * <b>is/2</b> - unifies the result of an arithmetic expression
 * with a variable.
 *
 * \par Usage
 * \em Var \b is \em Expr
 *
 * \par Description
 * \em Expr is evaluated as an arithmetic expression and the result
 * is unified with \em Var.  Expressions are evaluated as follows:
 *
 * \li Integer, real, and string terms evaluate to themselves.
 * \li An atom will cause a call to an internal function that
 *     returns a constant value if there is an arithmatic constant
 *     associated with the atom; for example \ref func_pi_0 "pi/0".
 * \li A compound functor term will evaluate each of the arguments
 *     as arithmetic expressions and then call the corresponding
 *     arithmetic function.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - An unbound variable was
 *     encountered in a subexpression.
 * \li <tt>type_error(evaluable, \em Expr)</tt> - \em Expr is not
 *     an integer constant, floating-point constant, string
 *     constant, or defined arithmetic function.
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is not a
 *     number but it was used as an argument to an arithmetic
 *     function that requires a number.
 * \li <tt>type_error(integer, \em Expr)</tt> - \em Expr is not an
 *     integer but it was used as an argument to an arithmetic
 *     function that requires an integer.
 * \li <tt>type_error(string, \em Expr)</tt> - \em Expr is not a
 *     string but it was used as an argument to an arithmetic
 *     function that requires a string.
 * \li <tt>evaluation_error(zero_divisor)</tt> - \em Expr attempted
 *     to divide by zero.
 * \li <tt>evaluation_error(int_overflow)</tt> - a conversion to
 *     the integer type resulted in an overflow because the
 *     incoming value was too large.  Note: ordinary mathematical
 *     and bitwise operators on integers like \ref func_add_2 "(+)/2",
 *     \ref func_mul_2 "(*)/2", \ref func_lshift_2 "(&lt;&lt;)/2", etc
 *     do not report overflow.
 *
 * \par Examples
 * \code
 * X is Y + 1
 * S is sin(Angle * pi / 180)
 * \endcode
 *
 * \par Compatibility
 * Evaluation and error reporting are mostly compatible with
 * \ref standard "Standard Prolog".  Floating-point exceptions
 * such as overflow and divide-by-zero are not reported as
 * thrown errors; use \ref fperror_1 "fperror/1" instead.
 *
 * \par See Also
 * \ref num_eq_2 "(=:=)/2",
 * \ref fperror_1 "fperror/1"
 */
static p_goal_result p_builtin_is
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value;
    p_goal_result result;
    p_term *value_term;
    result = p_arith_eval(context, &value, args[1], error);
    if (result != P_RESULT_TRUE)
        return result;
    switch (value.type) {
    case P_TERM_INTEGER:
        value_term = p_term_create_integer(context, value.integer_value);
        break;
    case P_TERM_REAL:
        value_term = p_term_create_real(context, value.real_value);
        break;
    case P_TERM_STRING:
        value_term = value.string_value;
        break;
    default: return P_RESULT_FAIL;
    }
    if (p_term_unify(context, args[0], value_term, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_eq_2
 * <b>(=:=)/2</b> - arithmetic equality.
 *
 * \par Usage
 * \em \em Expr1 <b>=:=</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the two values are
 * identical, then \em Expr1 <b>=:=</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>=:=</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings are compared for identity.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 4 =:= 6
 * 1.5 =:= 3.0 / 2
 * "foo" + "bar" =:= "foobar"
 * \endcode
 *
 * \par Compatibility
 * Compatible with \ref standard "Standard Prolog" for integer
 * and floating-point comparisons.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2"
 */
static int p_builtin_num_cmp
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value1;
    p_arith_value value2;
    if (p_arith_eval(context, &value1, args[0], error) != P_RESULT_TRUE)
        return -2;
    if (p_arith_eval(context, &value2, args[1], error) != P_RESULT_TRUE)
        return -2;
    if (value1.type == P_TERM_INTEGER) {
        if (value2.type == P_TERM_INTEGER) {
            if (value1.integer_value < value2.integer_value)
                return -1;
            else if (value1.integer_value > value2.integer_value)
                return 1;
            else
                return 0;
        } else if (value2.type == P_TERM_REAL) {
            double val1 = value1.integer_value;
            if (val1 < value2.real_value)
                return -1;
            else if (val1 > value2.real_value)
                return 1;
            else
                return 0;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return -2;
        }
    } else if (value1.type == P_TERM_REAL) {
        if (value2.type == P_TERM_REAL) {
            if (value1.real_value < value2.real_value)
                return -1;
            else if (value1.real_value > value2.real_value)
                return 1;
            else
                return 0;
        } else if (value2.type == P_TERM_INTEGER) {
            double val2 = value2.integer_value;
            if (value1.real_value < val2)
                return -1;
            else if (value1.real_value > val2)
                return 1;
            else
                return 0;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return -2;
        }
    } else if (value1.type == P_TERM_STRING) {
        if (value2.type == P_TERM_STRING) {
            int cmp = p_term_strcmp
                (value1.string_value, value2.string_value);
            if (cmp < 0)
                return -1;
            else if (cmp > 0)
                return 1;
            else
                return 0;
        } else {
            *error = p_create_type_error(context, "string", args[1]);
            return -2;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return -2;
    }
}
static p_goal_result p_builtin_num_eq
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp == 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_ne_2
 * <b>(=!=)/2</b>, <b>(=\\=)/2</b> - arithmetic inequality.
 *
 * \par Usage
 * \em \em Expr1 <b>=!=</b> \em Expr2
 * \par
 * \em \em Expr1 <b>=\\=</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the two values are
 * not identical, then \em Expr1 <b>=!=</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>=!=</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings are compared for non-identity.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 2 =!= 5
 * 1.5 =!= 3.0 * 2
 * "foo" + "ba" =!= "foobar"
 * \endcode
 *
 * \par Compatibility
 * The <b>(=\\=)/2</b> predicate is compatible with
 * \ref standard "Standard Prolog".  The new name <b>(=!=)/2</b>
 * is the recommended spelling.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2"
 */
static p_goal_result p_builtin_num_ne
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp != 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_lt_2
 * <b>(&lt;)/2</b> - arithmetic less than comparison.
 *
 * \par Usage
 * \em \em Expr1 <b>&lt;</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the value of \em Expr1
 * is less than the value of \em Expr2, then
 * \em Expr1 <b>&lt;</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>&lt;</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings, which may contain embedded NUL's, are compared using
 * the C function memcmp().
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 2 < 5
 * 1.5 < 3.0 * 2
 * "foo" + "ba" < "foobar"
 * \endcode
 *
 * \par Compatibility
 * Compatible with \ref standard "Standard Prolog" for integer
 * and floating-point comparisons.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2"
 */
static p_goal_result p_builtin_num_lt
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp < 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_le_2
 * <b>(&lt;=)/2</b>, <b>(=&lt;)/2</b> - arithmetic less than or
 * equal comparison.
 *
 * \par Usage
 * \em \em Expr1 <b>&lt;=</b> \em Expr2
 * \par
 * \em \em Expr1 <b>=&lt;</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the value of \em Expr1
 * is less than or equal to the value of \em Expr2, then
 * \em Expr1 <b>&lt;=</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>&lt;=</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings, which may contain embedded NUL's, are compared using
 * the C function memcmp().
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 2 <= 4
 * 1.5 <= 3.0 * 2
 * 1.5 <= 3.0 * 0.5
 * "foo" + "ba" <= "foobar"
 * \endcode
 *
 * \par Compatibility
 * The <b>(=&lt;)/2</b> predicate is compatible with
 * \ref standard "Standard Prolog".  The new name <b>(&lt;=)/2</b>
 * is the recommended spelling.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_gt_2 "(&gt;)/2",
 * \ref num_ge_2 "(&gt;=)/2"
 */
static p_goal_result p_builtin_num_le
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp <= 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_gt_2
 * <b>(&gt;)/2</b> - arithmetic greater than comparison.
 *
 * \par Usage
 * \em \em Expr1 <b>&gt;</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the value of \em Expr1
 * is greater than the value of \em Expr2, then
 * \em Expr1 <b>&gt;</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>&gt;</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings, which may contain embedded NUL's, are compared using
 * the C function memcmp().
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 2 > 3
 * 3.0 * 2 > 1.5
 * "foobar" > "foo" + "ba"
 * \endcode
 *
 * \par Compatibility
 * Compatible with \ref standard "Standard Prolog" for integer
 * and floating-point comparisons.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_ge_2 "(&gt;=)/2"
 */
static p_goal_result p_builtin_num_gt
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp > 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor num_ge_2
 * <b>(&gt;=)/2</b> - arithmetic greater than or equal comparison.
 *
 * \par Usage
 * \em \em Expr1 <b>&gt;=</b> \em Expr2
 *
 * \par Description
 * \em Expr1 and \em Expr2 are evaluated as described in the
 * documentation for \ref is_2 "is/2".  If the value of \em Expr1
 * is greater than or equal to the value of \em Expr2, then
 * \em Expr1 <b>&gt;=</b> \em Expr2 succeeds.
 * Otherwise, \em Expr1 <b>&gt;=</b> \em Expr2 fails.
 * \par
 * If one of \em Expr1 or \em Expr2 evaluates to an integer and
 * the other evaluates to a floating-point value, then both will
 * be converted into floating-point values prior to comparison.
 * Strings, which may contain embedded NUL's, are compared using
 * the C function memcmp().
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * 2 + 2 >= 4
 * 3.0 * 0.5 >= 1.5
 * "foobar" >= "foo" + "ba"
 * \endcode
 *
 * \par Compatibility
 * Compatible with \ref standard "Standard Prolog" for integer
 * and floating-point comparisons.  String comparisons are a
 * Plang extension.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref num_eq_2 "(=:=)/2",
 * \ref num_ne_2 "(=!=)/2",
 * \ref num_lt_2 "(&lt;)/2",
 * \ref num_le_2 "(&lt;=)/2",
 * \ref num_gt_2 "(&gt;)/2"
 */
static p_goal_result p_builtin_num_ge
    (p_context *context, p_term **args, p_term **error)
{
    int cmp = p_builtin_num_cmp(context, args, error);
    if (cmp == -2)
        return P_RESULT_ERROR;
    else if (cmp >= 0)
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor fperror_1
 * <b>fperror/1</b> - floating-point exception handling.
 *
 * \par Usage
 * \b fperror(\em Type)
 *
 * \par Description
 * If \em Type is the atom \c clear, then clear all of the
 * floating-point exception flags in the system and succeed.
 * Otherwise, test for a specific floating-point exception
 * and succeed if it is present.  If \em Type is not recognized,
 * or the exception has not been raised, then \b fperror(\em Type)
 * fails.
 *
 * \par
 * \li \c inexact - result could not be represented exactly.
 * \li \c overflow - overflow occurred.
 * \li \c undefined - result was undefined (not-a-number).
 * \li \c underflow - underflow occurred.
 * \li \c zero_divisor - division by zero.
 *
 * \par
 * Note: <b>fperror/1</b> relies upon the presence of the C99
 * floating-point exception functions feclearexcept() and
 * fetestexcept().  If these functions are not present, then
 * <b>fperror/1</b> will succeed for \c clear and fail for
 * all other arguments.  The \ref isnan_1 "isnan/1" and
 * \ref isinf_1 "isinf/1" predicates may be used to detect
 * certain error conditions.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Type is a variable.
 * \li <tt>type_error(atom, \em Type)</tt> - \em Type is not an atom.
 *
 * \par Examples
 * \code
 * fperror(clear);
 * X is Y / Z;
 * if (fperror(zero_divisor))
 *     stderr::writeln("division by zero occurred");
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref isnan_1 "isnan/1",
 * \ref isinf_1 "isinf/1"
 */
static p_goal_result p_builtin_fperror
    (p_context *context, p_term **args, p_term **error)
{
    p_term *type = p_term_deref(args[0]);
    if (!type || (type->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    if (type->header.type != P_TERM_ATOM) {
        *error = p_create_type_error(context, "atom", args[0]);
        return P_RESULT_ERROR;
    }
#if defined(HAVE_FENV_H) && defined(HAVE_FECLEAREXCEPT) && \
        defined(HAVE_FETESTEXCEPT)
    if (type == p_term_create_atom(context, "clear")) {
        feclearexcept(FE_ALL_EXCEPT);
        return P_RESULT_TRUE;
    } else {
        int excepts = 0;
        if (type == p_term_create_atom(context, "inexact"))
            excepts = FE_INEXACT;
        else if (type == p_term_create_atom(context, "overflow"))
            excepts = FE_OVERFLOW;
        else if (type == p_term_create_atom(context, "undefined"))
            excepts = FE_INVALID;
        else if (type == p_term_create_atom(context, "underflow"))
            excepts = FE_UNDERFLOW;
        else if (type == p_term_create_atom(context, "zero_divisor"))
            excepts = FE_DIVBYZERO;
        else
            return P_RESULT_FAIL;
        if (fetestexcept(excepts))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    }
#else
    if (type == p_term_create_atom(context, "clear"))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
#endif
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor isnan_1
 * <b>isnan/1</b> - test an arithmetic value for not-a-number.
 *
 * \par Usage
 * \b isnan(\em Expr)
 *
 * \par Description
 * \em Expr is as described in the documentation for \ref is_2 "is/2".
 * If the value of \em Expr is the IEEE not-a-number indicator,
 * then \b isnan(\em Expr) succeeds.  Otherwise,
 * \b isnan(\em Expr) fails.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * isnan(nan)               succeeds
 * isnan(float("nan"))      succeeds
 * isnan(3)                 fails
 * isnan(inf)               fails
 * isnan("nan")             type_error(number, "nan")
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref isinf_1 "isinf/1",
 * \ref func_nan_0 "nan/0",
 * \ref fperror_1 "fperror/1"
 */
static p_goal_result p_builtin_isnan
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value;
    p_goal_result result;
    result = p_arith_eval(context, &value, args[0], error);
    if (result != P_RESULT_TRUE)
        return result;
    if (value.type == P_TERM_INTEGER) {
        return P_RESULT_FAIL;
    } else if (value.type == P_TERM_REAL) {
        if (isnan(value.real_value))
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor isinf_1
 * <b>isinf/1</b> - test an arithmetic value for infinity.
 *
 * \par Usage
 * \b isinf(\em Expr)
 *
 * \par Description
 * \em Expr is as described in the documentation for \ref is_2 "is/2".
 * If the value of \em Expr is an IEEE infinity indicator,
 * then \b isinf(\em Expr) succeeds.  Otherwise,
 * \b isinf(\em Expr) fails.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * isinf(inf)               succeeds
 * isinf(-inf)              succeeds
 * isinf(float("inf"))      succeeds
 * isinf(float("-inf"))     succeeds
 * isinf(3)                 fails
 * isinf(nan)               fails
 * isinf("inf")             type_error(number, "inf")
 * isinf(X); X < 0          checks for negative infinity
 * isinf(X); X > 0          checks for positive infinity
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref isnan_1 "isnan/1",
 * \ref func_inf_0 "inf/0",
 * \ref fperror_1 "fperror/1"
 */
static p_goal_result p_builtin_isinf
    (p_context *context, p_term **args, p_term **error)
{
    p_arith_value value;
    p_goal_result result;
    result = p_arith_eval(context, &value, args[0], error);
    if (result != P_RESULT_TRUE)
        return result;
    if (value.type == P_TERM_INTEGER) {
        return P_RESULT_FAIL;
    } else if (value.type == P_TERM_REAL) {
        if (isinf(value.real_value) != 0)
            return P_RESULT_TRUE;
        else
            return P_RESULT_FAIL;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_add_2
 * <b>(+)/2</b> - addition of two arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b + \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 + \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value \em Expr1 + \em Expr2.
 * If both are strings, then the result is the concatenation
 * of \em Expr1 and \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr1 is a
 *     number, but \em Expr2 is not.
 * \li <tt>type_error(string, \em Expr2)</tt> - \em Expr1 is a
 *     string, but \em Expr2 is not.
 *
 * \par Examples
 * \code
 * X is 2 + 2               sets X to the integer value 4
 * X is 1.5 + 3             sets X to the floating-point value 4.5
 * X is "foo" + "bar"       sets X to the string "foobar"
 * X is 1 + "foo"           type_error(number, "foo")
 * X is "foo" + 1           type_error(string, 1)
 * X is "foo" + string(1)   sets X to the string "foo1"
 * \endcode
 *
 * \par Compatibility
 * The <b>(+)/2</b> function is compatible with
 * \ref standard "Standard Prolog" for integer and floating-point
 * arguments.  String concatenation is a Plang extension.
 *
 * \par See Also
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2"
 */
static p_goal_result p_arith_add
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value + values[1].integer_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].integer_value + values[1].real_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value + values[1].real_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value + values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_STRING) {
        if (values[1].type == P_TERM_STRING) {
            result->type = P_TERM_STRING;
            result->string_value = p_term_concat_string
                (context, values[0].string_value,
                 values[1].string_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "string", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        return P_RESULT_FAIL;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_neg_1
 * <b>(-)/1</b> - negation of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b - \em Expr
 *
 * \par Description
 * Evaluates \em Expr.  If the result is an integer, then the result
 * is the integer value - \em Expr.  If the result is a floating-point
 * number, then the result is the floating-point value - \em Expr.
 * \par
 * Note: integer and floating-point constants that start with "-"
 * will be recognized as negative numbers by the parser if there is
 * no space between the "-" and the digits.  If there is a space,
 * then a term with <b>(-)/1</b> as its functor will be created.
 * Thus, <tt>-2 == - 2</tt> will fail, but <tt>-2 =:= - 2</tt>
 * will succeed.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is - 2                 sets X to the integer value -2
 * X is - 1.5               sets X to the floating-point value -1.5
 * X is - "foo"             type_error(number, "foo")
 * X is --2                 sets X to the integer value 2
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2",
 * \ref func_abs_1 "abs/1"
 */
static p_goal_result p_arith_neg
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = -(values[0].integer_value);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = -(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_sub_2
 * <b>(-)/2</b> - subtraction of two arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b - \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 - \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value \em Expr1 - \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is 2 - 2               sets X to the integer value 0
 * X is 1.5 - 3             sets X to the floating-point value -1.5
 * X is 1 - "foo"           type_error(number, "foo")
 * X is "foo" - 1           type_error(number, "foo")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2"
 */
static p_goal_result p_arith_sub
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value - values[1].integer_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].integer_value - values[1].real_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value - values[1].real_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value - values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_mul_2
 * <b>(*)/2</b> - multiplication of two arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b * \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 * \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value \em Expr1 * \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is 2 * 2               sets X to the integer value 4
 * X is 1.5 * 3             sets X to the floating-point value 4.5
 * X is 1 * "foo"           type_error(number, "foo")
 * X is "foo" * 1           type_error(number, "foo")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2"
 */
static p_goal_result p_arith_mul
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value * values[1].integer_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].integer_value * values[1].real_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value * values[1].real_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value * values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_div_2
 * <b>(/)/2</b> - division of two arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b / \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 / \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value \em Expr1 / \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 * \li <tt>evaluation_error(zero_divisor)</tt> - \em Expr2 is zero
 *     and an integer division is being performed.  Floating-point
 *     division will produce not-a number (\ref func_nan_0 "nan/0")
 *     or an infinity (\ref func_inf_0 "inf/0") if \em Expr2 is zero.
 *
 * \par Examples
 * \code
 * X is 2 / 2               sets X to the integer value 1
 * X is 1.5 / 3             sets X to the floating-point value 0.5
 * X is 1 / "foo"           type_error(number, "foo")
 * X is "foo" / 1           type_error(number, "foo")
 * X is 42 / 0              evaluation_error(zero_divisor)
 * X is 42 / 0.0            sets X to positive infinity
 * X is 0.0 / 0.0           sets X to not-a-number
 * \endcode
 *
 * \par Compatibility
 * The <b>(/)/2</b> function is part of \ref standard "Standard Prolog",
 * but it has a slightly different interpretation when mixing integer
 * and floating-point values.  In Standard Prolog, <b>(/)/2</b> always
 * produces a floating-point result, even if both arguments are
 * integers.  Standard Prolog has a separate <b>(//)/2</b> function
 * for integer division.  The Standard Prolog forms can be translated
 * into Plang using explicit type conversions:
 * \code
 * X is Y / Z               X is float(Y) / float(Z)
 * X is Y // Z              X is integer(Y) / integer(Z)
 * \endcode
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_mod_2 "(%)/2"
 */
static p_goal_result p_arith_div
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            if (values[1].integer_value != 0) {
                result->type = P_TERM_INTEGER;
                result->integer_value =
                    values[0].integer_value / values[1].integer_value;
                return P_RESULT_TRUE;
            }
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].integer_value / values[1].real_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value / values[1].real_value;
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                values[0].real_value / values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
    *error = p_create_evaluation_error(context, "zero_divisor");
    return P_RESULT_ERROR;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_mod_2
 * <b>(%)/2</b>, <b>mod/2</b> - modulus of dividing two
 * arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b % \em Expr2
 * \par
 * \em Var \b is \em Expr1 \b mod \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 % \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value fmod(\em Expr1,
 * \em Expr2).
 * \par
 * The floating-point modulus uses the C function fmod().
 * For the IEEE remainder, use \ref func_rem_2 "remainder/2".
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 * \li <tt>evaluation_error(zero_divisor)</tt> - \em Expr2 is zero
 *     and an integer division is being performed.  Floating-point
 *     remainder will produce not-a number (\ref func_nan_0 "nan/0")
 *     if \em Expr2 is zero.
 *
 * \par Examples
 * \code
 * X is 2 % 2               sets X to the integer value 0
 * X is 3 % 2               sets X to the integer value 1
 * X is 1.5 % 3             sets X to the floating-point value 1.5
 * X is 1 % "foo"           type_error(number, "foo")
 * X is "foo" % 1           type_error(number, "foo")
 * X is 42 % 0              evaluation_error(zero_divisor)
 * X is 42 % 0.0            sets X to not-a-number
 * \endcode
 *
 * \par Compatibility
 * The <b>mod/2</b> function is from \ref standard "Standard Prolog".
 * The new name <b>(%)/2</b> is the recommended spelling.
 * In Standard Prolog, <b>mod/2</b> only works on integer values.
 * Plang extends the definition to floating-point.
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_rem_2 "remainder/2"
 */
P_INLINE double p_arith_fmod(double x, double y)
{
#if defined(HAVE_FMOD)
    return fmod(x, y);
#else
    /* Return NaN if we have no way to calculate the remainder */
    return 0.0 / 0.0;
#endif
}
static p_goal_result p_arith_mod
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            if (values[1].integer_value != 0) {
                result->type = P_TERM_INTEGER;
                result->integer_value =
                    values[0].integer_value % values[1].integer_value;
                return P_RESULT_TRUE;
            }
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_fmod(values[0].integer_value,
                             values[1].real_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_fmod(values[0].real_value,
                             values[1].real_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_fmod(values[0].real_value,
                             values[1].integer_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
    *error = p_create_evaluation_error(context, "zero_divisor");
    return P_RESULT_ERROR;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_rem_2
 * <b>rem/2</b> - IEEE remainder of dividing two
 * arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \em Expr1 \b rem \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value \em Expr1 % \em Expr2.
 * If both are numbers (integers or floating-point), then
 * the result is the floating-point value remainder(\em Expr1,
 * \em Expr2).
 * \par
 * The floating-point remainder uses the C function remainder().
 * For the C function fmod(), use \ref func_mod_2 "(%)/2".
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 * \li <tt>evaluation_error(zero_divisor)</tt> - \em Expr2 is zero
 *     and an integer division is being performed.  Floating-point
 *     remainder will produce not-a number (\ref func_nan_0 "nan/0")
 *     if \em Expr2 is zero.
 *
 * \par Examples
 * \code
 * X is 2 rem 2             sets X to the integer value 0
 * X is 3 rem 2             sets X to the integer value 1
 * X is 1.5 rem 3           sets X to the floating-point value 1.5
 * X is 1 rem "foo"         type_error(number, "foo")
 * X is "foo" rem 1         type_error(number, "foo")
 * X is 42 rem 0            evaluation_error(zero_divisor)
 * X is 42 rem 0.0          sets X to not-a-number
 * \endcode
 *
 * \par Compatibility
 * The <b>rem/2</b> function is from \ref standard "Standard Prolog".
 * The new name <b>remainder/2</b> is the recommended spelling.
 * In Standard Prolog, <b>rem/2</b> only works on integer values.
 * Plang extends the definition to floating-point.
 *
 * \par See Also
 * \ref func_add_2 "(+)/2",
 * \ref func_neg_1 "(-)/1",
 * \ref func_sub_2 "(-)/2",
 * \ref func_mul_2 "(*)/2",
 * \ref func_div_2 "(/)/2",
 * \ref func_mod_2 "(%)/2"
 */
P_INLINE double p_arith_drem(double x, double y)
{
#if defined(HAVE_REMAINDER)
    return remainder(x, y);
#elif defined(HAVE_DREM)
    return drem(x, y);
#else
    if (isnan(x) || isnan(y) || y == 0.0) {
        return 0.0 / 0.0;
    } else {
        double quotient = x / y;
        if (quotient >= 0.0)
            return x - ceil(quotient) * y;
        else
            return x - floor(quotient) * y;
    }
#endif
}
static p_goal_result p_arith_rem
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            if (values[1].integer_value != 0) {
                result->type = P_TERM_INTEGER;
                result->integer_value =
                    values[0].integer_value % values[1].integer_value;
                return P_RESULT_TRUE;
            }
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_drem(values[0].integer_value,
                             values[1].real_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_drem(values[0].real_value,
                             values[1].real_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                p_arith_drem(values[0].real_value,
                             values[1].integer_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
    *error = p_create_evaluation_error(context, "zero_divisor");
    return P_RESULT_ERROR;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_and_2
 * <b>(/\\)/2</b> - bitwise-and of two integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <tt>/\\</tt> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing a
 * bitwise-and of \em Expr1 and \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 /\\ 5            X is set to 4
 * X is 22.0 /\\ 5          type_error(integer, 22.0)
 * X is 22 /\\ 5.0          type_error(integer, 5.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_and
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value & values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_or_2
 * <b>(\\/)/2</b> - bitwise-or of two integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <tt>\\/</tt> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing a
 * bitwise-or of \em Expr1 and \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 \\/ 5            X is set to 23
 * X is 22.0 \\/ 5          type_error(integer, 22.0)
 * X is 22 \\/ 5.0          type_error(integer, 5.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_or
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value | values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_xor_2
 * <b>(^)/2</b> - bitwise-xor of two integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <tt>^</tt> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing a
 * bitwise-xor of \em Expr1 and \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 ^ 5              X is set to 19
 * X is 22.0 ^ 5            type_error(integer, 22.0)
 * X is 22 ^ 5.0            type_error(integer, 5.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_xor
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value ^ values[1].integer_value;
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_not_1
 * <b>(~)/1</b>, <b>(\\)/1</b> - bitwise-not of an integer value.
 *
 * \par Usage
 * \em Var \b is <tt>~</tt> \em Expr
 * \par
 * \em Var \b is <tt>\\</tt> \em Expr
 *
 * \par Description
 * Evaluates \em Expr.  If it is an integers, then the result is
 * the integer value of performing a bitwise-not on \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr)</tt> - \em Expr is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is ~22                 X is set to -23
 * X is ~22.0               type_error(integer, 22.0)
 * \endcode
 *
 * \par Compatibility
 * The <b>(\\)/1</b> function is from \ref standard "Standard Prolog".
 * The new name <b>(~)/1</b> is the recommended spelling.
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_not
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = ~(values[0].integer_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_lshift_2
 * <b>(&lt;&lt;)/2</b> - bitwise shift left of two integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <b>&lt;&lt;</b> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing a
 * left shift of \em Expr1 by the value in the bottom 5
 * bits of \em Expr2.  The top bits of \em Expr2 are ignored.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 << 5             X is set to 704
 * X is 22.0 << 5           type_error(integer, 22.0)
 * X is 22 << 5.0           type_error(integer, 5.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_rshift_2 "(&gt;&gt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_lshift
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value <<
                    (values[1].integer_value & 31);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_rshift_2
 * <b>(&gt;&gt;)/2</b> - bitwise signed shift right of two
 * integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <b>&gt;&gt;</b> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing a signed
 * left shift of \em Expr1 by the value in the bottom 5
 * bits of \em Expr2.  The top bits of \em Expr2 are ignored.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 >> 5             X is set to 0
 * X is -22 >> 5            X is set to -1
 * X is 22.0 >> 5           type_error(integer, 22.0)
 * X is 22 >> 5.0           type_error(integer, 5.0)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog" leaves it unspecified whether
 * <b>(&gt;&gt;)/2</b> is signed or unsigned.  In Plang,
 * <b>(&gt;&gt;)/2</b> is defined as signed, with
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2" provided for
 * the unsigned case.
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rushift_2 "(&gt;&gt;&gt;)/2"
 */
static p_goal_result p_arith_rshift
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                values[0].integer_value >>
                    (values[1].integer_value & 31);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_rushift_2
 * <b>(&gt;&gt;&gt;)/2</b> - bitwise unsigned shift right of two
 * integer values.
 *
 * \par Usage
 * \em Var \b is \em Expr1 <b>&gt;&gt;&gt;</b> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2.  If both are integers,
 * then the result is the integer value of performing an unsigned
 * left shift of \em Expr1 by the value in the bottom 5
 * bits of \em Expr2.  The top bits of \em Expr2 are ignored.
 *
 * \par Errors
 *
 * \li <tt>type_error(integer, \em Expr1)</tt> - \em Expr1 is
 *     not an integer.
 * \li <tt>type_error(integer, \em Expr2)</tt> - \em Expr2 is
 *     not an integer.
 *
 * \par Examples
 * \code
 * X is 22 >>> 5            X is set to 0
 * X is -22 >>> 5           X is set to 134217727 (0x07ffffff)
 * X is 22.0 >>> 5          type_error(integer, 22.0)
 * X is 22 >>> 5.0          type_error(integer, 5.0)
 * \endcode
 *
 * \par See Also
 * \ref func_and_2 "(/\\)/2",
 * \ref func_or_2 "(\\/)/2",
 * \ref func_xor_2 "(^)/2",
 * \ref func_not_1 "(~)/1",
 * \ref func_lshift_2 "(&lt;&lt;)/2",
 * \ref func_rshift_2 "(&gt;&gt;)/2"
 */
static p_goal_result p_arith_rushift
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_INTEGER;
            result->integer_value =
                (int)(((unsigned int)(values[0].integer_value)) >>
                            (values[1].integer_value & 31));
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "integer", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_abs_1
 * <b>abs/1</b> - absolute value of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b abs(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and produces its absolute value as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 * \li <tt>evaluation_error(int_overflow)</tt> - \em Expr is
 *     the integer -2147483648, which cannot be negated to
 *     produce a positive integer value.
 *
 * \par Examples
 * \code
 * X is abs(3)              X is set to 3
 * X is abs(-35.125)        X is set to 35.125
 * X is abs(-inf)           X is set to inf
 * X is abs(nan)            X is set to nan
 * X is abs("-35")          type_error(number, "-35")
 * X is abs(-2147483648)    evaluation_error(int_overflow)
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_neg_1 "(-)/1",
 * \ref func_sign_1 "sign/1"
 */
static p_goal_result p_arith_abs
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        if (values[0].integer_value == (int)(-0x7fffffff - 1)) {
            *error = p_create_evaluation_error(context, "int_overflow");
            return P_RESULT_ERROR;
        } else if (values[0].integer_value < 0) {
            result->integer_value = -(values[0].integer_value);
        } else {
            result->integer_value = values[0].integer_value;
        }
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = fabs(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_acos_1
 * <b>acos/1</b> - arc cosine of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b acos(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and produces its arc cosine as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is acos(1)             sets X to 0.0
 * X is acos(0.0)           sets X to pi / 2
 * X is acos(inf)           sets X to nan
 * X is acos(nan)           sets X to nan
 * X is acos("1.0")         type_error(number, "1.0")
 * \endcode
 *
 * \par See Also
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_acos
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = acos((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = acos(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_asin_1
 * <b>asin/1</b> - arc sine of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b asin(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and produces its arc sine as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is asin(0)             sets X to 0.0
 * X is asin(1.0)           sets X to pi / 2
 * X is asin(inf)           sets X to nan
 * X is asin(nan)           sets X to nan
 * X is asin("1.0")         type_error(number, "1.0")
 * \endcode
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_asin
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = asin((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = asin(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_atan_1
 * <b>atan/1</b> - arc tangent of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b atan(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and produces its arc tangent as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is atan(1.0)           sets X to pi / 4
 * X is atan(0)             sets X to 0.0
 * X is atan(inf)           sets X to pi / 2
 * X is atan(nan)           sets X to nan
 * X is atan("1.0")         type_error(number, "1.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_atan
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = atan((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = atan(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_atan2_2
 * <b>atan2/2</b> - arc tangent of two arithmetic terms.
 *
 * \par Usage
 * \em Var \b is \b atan2(\em Expr1, \em Expr2)
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2 and produces the
 * arc tangent of \em Expr1 / \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is atan2(1.0, 1.0)     sets X to pi / 4
 * X is atan2(0, 1)         sets X to 0.0
 * X is atan2(0, -1)        sets X to pi
 * X is atan2(inf, 2)       sets X to pi / 2
 * X is atan2(nan, 1)       sets X to nan
 * X is atan2(1, nan)       sets X to nan
 * X is atan2("2.0", 1.0)   type_error(number, "2.0")
 * X is atan2(2.0, "1.0")   type_error(number, "1.0")
 * \endcode
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_atan2
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                atan2(values[0].integer_value, values[1].integer_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                atan2(values[0].integer_value, values[1].real_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                atan2(values[0].real_value, values[1].real_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                atan2(values[0].real_value, values[1].integer_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_ceil_1
 * <b>ceil/1</b>, <b>ceiling/1</b> - smallest integer not smaller
 * than an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b ceil(\em Expr)
 * \par
 * \em Var \b is \b ceiling(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and rounds the smallest integer that is
 * not smaller than \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is ceil(3)             X is set to 3
 * X is ceil(35.125)        X is set to 36.0
 * X is ceil(-35.125)       X is set to -35.0
 * X is ceil(0.0)           X is set to 0.0
 * X is ceil(-inf)          X is set to -inf
 * X is ceil(nan)           X is set to nan
 * X is ceil("-35")         type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * The <b>ceiling/1</b> function is compatible with
 * \ref standard "Standard Prolog".  The new name <b>ceil/1</b>
 * is the recommended spelling.
 *
 * \par See Also
 * \ref func_floor_1 "floor/1",
 * \ref func_round_1 "round/1",
 * \ref func_float_integer_part_1 "float_integer_part/1"
 */
static p_goal_result p_arith_ceil
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = ceil(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_cos_1
 * <b>cos/1</b> - cosine of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b cos(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr as a value in radians and produces its
 * cosine as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is cos(0)              sets X to 1.0
 * X is cos(pi / 2)         sets X to 0.0
 * X is cos(inf)            sets X to nan
 * X is cos(nan)            sets X to nan
 * X is cos("1.0")          type_error(number, "1.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_sin_1 "sin/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_cos
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = cos((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = cos(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_exp_1
 * <b>exp/1</b> - base-\em e exponential of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b exp(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and computes the base-\em e exponential
 * of \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is exp(1)              X is set to e
 * X is exp(0)              X is set to 1.0
 * X is exp(2.0)            X is set to e * e
 * X is exp(-2.0)           X is set to 1 / (e * e)
 * X is exp(inf)            X is set to inf
 * X is exp(-inf)           X is set to 0.0
 * X is exp(nan)            X is set to nan
 * X is exp("2.0")          type_error(number, "2.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_log_1 "log/1",
 * \ref func_pow_2 "pow/2",
 * \ref func_sqrt_1 "sqrt/1",
 * \ref func_e_0 "e/0"
 */
static p_goal_result p_arith_exp
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = exp((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = exp(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_e_0
 * <b>e/0</b> - mathematical constant for \em e.
 *
 * \par Usage
 * \em Var \b is \b e
 *
 * \par Description
 * This function evaluates to the floating-point mathematical
 * constant for \em e, 2.7182818284590452354.
 *
 * \par Examples
 * \code
 * E2 is e * 2
 * \endcode
 *
 * \par See Also
 * \ref func_inf_0 "inf/0",
 * \ref func_nan_0 "nan/0",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_e
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    result->type = P_TERM_REAL;
    result->real_value = 2.7182818284590452354;
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_float_1
 * <b>float/1</b> - conversion to floating-point.
 *
 * \par Usage
 * \em Var \b is \b float(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and converts it into a floating-point value.
 * Strings are converted using the C function strtod().
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * F is float(3)            F is set to 3.0
 * F is float(3.5)          F is set to 3.5 (no change)
 * F is float(2 + 6)        F is set to 8.0
 * F is float("-3.5e02")    F is set to -350.0 (string conversion)
 * F is float("foo")        type_error(number, "foo")
 * F is float('3.5')        type_error(evaluable, '3.5')
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref func_integer_1 "integer/1",
 * \ref func_string_1 "string/1"
 */
static p_goal_result p_arith_float
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = values[0].real_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_STRING) {
        const char *str = p_term_name(values[0].string_value);
        char *endptr;
        double val;
        errno = 0;
        val = strtod(str, &endptr);
        if (errno == ERANGE || endptr == str) {
            /* No conversion performed or value is out of range */
            *error = p_create_type_error(context, "number", args[0]);
            return P_RESULT_ERROR;
        }
        while (*endptr == ' ' || *endptr == '\t' ||
               *endptr == '\r' || *endptr == '\n')
            ++endptr;
        if (*endptr != '\0') {
            /* Trailing characters other than whitespace */
            *error = p_create_type_error(context, "number", args[0]);
            return P_RESULT_ERROR;
        }
        result->type = P_TERM_REAL;
        result->real_value = val;
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_float_fractional_part_1
 * <b>float_fractional_part/1</b> - fractional part of a
 * floating-point arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b float_fractional_part(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and returns the fractional part of the value.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is float_fractional_part(3)        X is set to 0
 * X is float_fractional_part(35.125)   X is set to 0.125
 * X is float_fractional_part(-35.125)  X is set to -0.125
 * X is float_fractional_part(0.0)      X is set to 0.0
 * X is float_fractional_part(-inf)     X is set to 0.0
 * X is float_fractional_part(nan)      X is set to nan
 * X is float_fractional_part("-35")    type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_float_integer_part_1 "float_integer_part/1",
 * \ref func_floor_1 "floor/1",
 * \ref func_round_1 "round/1"
 */
static p_goal_result p_arith_float_fractional_part
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = 0;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        double ipart;
        result->type = P_TERM_REAL;
        result->real_value = modf(values[0].real_value, &ipart);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_float_integer_part_1
 * <b>float_integer_part/1</b> - integer part of a
 * floating-point arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b float_integer_part(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and returns the integer part of the value.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is float_integer_part(3)           X is set to 3
 * X is float_integer_part(35.125)      X is set to 35.0
 * X is float_integer_part(-35.125)     X is set to -35.0
 * X is float_integer_part(0.0)         X is set to 0.0
 * X is float_integer_part(-inf)        X is set to -inf
 * X is float_integer_part(nan)         X is set to nan
 * X is float_integer_part("-35")       type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_float_fractional_part_1 "float_fractional_part/1",
 * \ref func_floor_1 "floor/1",
 * \ref func_round_1 "round/1"
 */
static p_goal_result p_arith_float_integer_part
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        modf(values[0].real_value, &(result->real_value));
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_floor_1
 * <b>floor/1</b> - largest integer not greater than an
 * arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b floor(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and returns the largest integer that is
 * not larger than \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is floor(3)            X is set to 3
 * X is floor(35.125)       X is set to 35.0
 * X is floor(-35.125)      X is set to -36.0
 * X is floor(0.0)          X is set to 0.0
 * X is floor(-inf)         X is set to -inf
 * X is floor(nan)          X is set to nan
 * X is floor("-35")        type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_ceil_1 "ceil/1",
 * \ref func_round_1 "round/1",
 * \ref func_float_integer_part_1 "float_integer_part/1"
 */
static p_goal_result p_arith_floor
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = floor(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_inf_0
 * <b>inf/0</b> - mathematical constant for positive infinity.
 *
 * \par Usage
 * \em Var \b is \b inf
 *
 * \par Description
 * This function evaluates to the floating-point mathematical
 * constant for positive infinity.
 *
 * \par Examples
 * \code
 * Inf is inf
 * NegInf is -inf
 * \endcode
 *
 * \par See Also
 * \ref func_e_0 "e/0",
 * \ref func_nan_0 "nan/0",
 * \ref func_pi_0 "pi/0",
 * \ref isinf_1 "isinf/1"
 */
static p_goal_result p_arith_inf
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    result->type = P_TERM_REAL;
    result->real_value = 1.0 / 0.0;
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_integer_1
 * \anchor func_truncate_1
 * <b>integer/1</b>, <b>truncate/1</b> - conversion to integer.
 *
 * \par Usage
 * \em Var \b is \b integer(\em Expr)
 * \par
 * \em Var \b is \b truncate(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and converts it into an integer value
 * by truncating the fractional part.  Strings are converted
 * using the C function strtol().
 *
 * \par Errors
 *
 * \li <tt>evaluation_error(int_overflow)</tt> - a conversion to
 *     the integer type resulted in an overflow because the incoming
 *     floating-point or string value was too large.
 *
 * \par Examples
 * \code
 * I is integer(3)              I is set to 3 (no change)
 * I is integer(3.5)            I is set to 3
 * I is integer(2.0 + 6.0)      I is set to 8
 * I is integer("-35")          I is set to -35 (string conversion)
 * I is integer("foo")          type_error(integer, "foo")
 * I is integer('3')            type_error(evaluable, '3')
 * I is integer(2147483648.0)   evaluation_error(int_overflow)
 * \endcode
 *
 * \par Compatibility
 * The <b>truncate/1</b> function is compatible with
 * \ref standard "Standard Prolog".  The new name <b>integer/1</b>
 * is the recommended spelling.
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref func_float_1 "float/1",
 * \ref func_round_1 "round/1",
 * \ref func_string_1 "string/1",
 * \ref func_float_integer_part_1 "float_integer_part/1"
 */
static p_goal_result p_arith_integer
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        if (values[0].real_value >= 2147483648.0 ||
                values[0].real_value <= -2147483649.0) {
            *error = p_create_evaluation_error(context, "int_overflow");
            return P_RESULT_ERROR;
        }
        result->type = P_TERM_INTEGER;
        result->integer_value = (int)(values[0].real_value);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_STRING) {
        const char *str = p_term_name(values[0].string_value);
        char *endptr;
        long val;
        errno = 0;
        val = strtol(str, &endptr, 0);
        if (errno == ERANGE) {
            /* Value is out of range */
            *error = p_create_evaluation_error(context, "int_overflow");
            return P_RESULT_ERROR;
        } else if (endptr == str) {
            /* No conversion performed - data is not an integer */
            *error = p_create_type_error(context, "integer", args[0]);
            return P_RESULT_ERROR;
        }
        while (*endptr == ' ' || *endptr == '\t' ||
               *endptr == '\r' || *endptr == '\n')
            ++endptr;
        if (*endptr != '\0') {
            /* Trailing characters other than whitespace */
            *error = p_create_type_error(context, "integer", args[0]);
            return P_RESULT_ERROR;
        }
        if (val != (int)val) {
            /* Value is out of range for a 32-bit integer */
            *error = p_create_evaluation_error(context, "int_overflow");
            return P_RESULT_ERROR;
        }
        result->type = P_TERM_INTEGER;
        result->integer_value = (int)val;
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "integer", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_left_2
 * <b>left/2</b> - extract the left portion of a string.
 *
 * \par Usage
 * \em Var \b is \b left(\em Expr, \em Length)
 *
 * \par Description
 * Evaluates \em Expr \em Length.  Returns the \em Length bytes
 * at the beginning of the string.
 * \par
 * If \em Length is greater than the length of \em Expr,
 * then the whole string is returned.
 *
 * \par Errors
 *
 * \li <tt>type_error(string, \em Expr)</tt> - \em Expr is
 *     not a string.
 * \li <tt>type_error(integer, \em Length)</tt> - \em Length is
 *     not an integer.
 * \li <tt>domain_error(not_less_than_zero, \em Length)</tt> -
 *     \em Start is an integer that is less than zero.
 *
 * \par Examples
 * \code
 * X is left("foobar", 3)       X is set to "foo"
 * X is left("foobar", 10)      X is set to "foobar"
 * X is left("foobar", 0)       X is set to ""
 * X is left(1.5, 1)            type_error(string, 1.5)
 * X is left("foobar", 1.0)     type_error(integer, 1.0)
 * X is left("foobar", -1)      domain_error(not_less_than_zero, -1)
 * \endcode
 *
 * \par See Also
 * \ref func_mid_2 "mid/2",
 * \ref func_mid_3 "mid/3",
 * \ref func_right_2 "right/2"
 */
static p_term *p_arith_mid
    (p_context *context, p_term *str, unsigned int start,
     unsigned int length)
{
    if (!str->header.size)
        return str;
    if (start >= str->header.size)
        return p_term_create_string_n(context, "", 0);
    if (start == 0 && length >= str->header.size)
        return str;
    if (length > (str->header.size - start))
        length = str->header.size - start;
    return p_term_create_string_n
        (context, str->string.name + start, length);
}
static p_goal_result p_arith_left
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    p_term *str;
    int length;
    if (values[0].type != P_TERM_STRING) {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
    if (values[1].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[1]);
        return P_RESULT_ERROR;
    }
    str = values[0].string_value;
    length = values[1].integer_value;
    if (length < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", args[1]);
        return P_RESULT_ERROR;
    }
    result->type = P_TERM_STRING;
    result->string_value = p_arith_mid
        (context, str, 0, (unsigned int)length);
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_log_1
 * <b>log/1</b> - base-\em e logarithm of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b log(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and computes the base-\em e logarithm
 * of \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is log(e)              X is set to 1
 * X is log(1)              X is set to 0.0
 * X is log(e * e)          X is set to 2.0
 * X is log(1 / (e * e))    X is set to -2.0
 * X is log(0.0)            X is set to -inf
 * X is log(inf)            X is set to inf
 * X is log(-inf)           X is set to nan
 * X is log(nan)            X is set to nan
 * X is log("2.0")          type_error(number, "2.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_exp_1 "exp/1",
 * \ref func_pow_2 "pow/2",
 * \ref func_sqrt_1 "sqrt/1",
 * \ref func_e_0 "e/0"
 */
static p_goal_result p_arith_log
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = log((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = log(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_mid_2
 * \anchor func_mid_3
 * <b>mid/2</b>, <b>mid/3</b> - extract the middle portion of a string.
 *
 * \par Usage
 * \em Var \b is \b mid(\em Expr, \em Start, \em Length)
 * \par
 * \em Var \b is \b mid(\em Expr, \em Start)
 *
 * \par Description
 * Evaluates \em Expr, \em Start, and \em Length.  Returns the
 * \em Length bytes starting at index \em Start within \em Expr.
 * The first byte is at index 0.
 * \par
 * If \em Length is omited, then the returned string starts at
 * \em Start and extends to the end of \em Expr.
 * \par
 * If \em Start is beyond the end of \em Expr, then the empty
 * string is returned.
 * \par
 * If (\em Start + \em Length) is greater than the length of
 * \em Expr, then as many bytes as are available are returned,
 * starting at \em Start.
 *
 * \par Errors
 *
 * \li <tt>type_error(string, \em Expr)</tt> - \em Expr is
 *     not a string.
 * \li <tt>type_error(integer, \em Start)</tt> - \em Start is
 *     not an integer.
 * \li <tt>type_error(integer, \em Length)</tt> - \em Length is
 *     not an integer.
 * \li <tt>domain_error(not_less_than_zero, \em Start)</tt> -
 *     \em Start is an integer that is less than zero.
 * \li <tt>domain_error(not_less_than_zero, \em Length)</tt> -
 *     \em Start is an integer that is less than zero.
 *
 * \par Examples
 * \code
 * X is mid("foobar", 1, 4)     X is set to "ooba"
 * X is mid("foobar", 1, 0)     X is set to ""
 * X is mid("foobar", 10, 3)    X is set to ""
 * X is mid("foobar", 4, 3)     X is set to "ar"
 * X is mid("foobar", 4)        X is set to "ar"
 * X is mid(1.5, 1)             type_error(string, 1.5)
 * X is mid("foobar", 1.0, 4)   type_error(integer, 1.0)
 * X is mid("foobar", 1, 4.0)   type_error(integer, 4.0)
 * X is mid("foobar", -1)       domain_error(not_less_than_zero, -1)
 * X is mid("foobar", 1, -4)    domain_error(not_less_than_zero, -4)
 * \endcode
 *
 * \par See Also
 * \ref func_left_2 "left/2",
 * \ref func_right_2 "right/2"
 */
static p_goal_result p_arith_mid_2
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    p_term *str;
    int start;
    if (values[0].type != P_TERM_STRING) {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
    if (values[1].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[1]);
        return P_RESULT_ERROR;
    }
    str = values[0].string_value;
    start = values[1].integer_value;
    if (start < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", args[1]);
        return P_RESULT_ERROR;
    }
    result->type = P_TERM_STRING;
    result->string_value = p_arith_mid
        (context, str, (unsigned int)start, str->header.size);
    return P_RESULT_TRUE;
}
static p_goal_result p_arith_mid_3
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    p_term *str;
    int start;
    int length;
    if (values[0].type != P_TERM_STRING) {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
    if (values[1].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[1]);
        return P_RESULT_ERROR;
    }
    if (values[2].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[2]);
        return P_RESULT_ERROR;
    }
    str = values[0].string_value;
    start = values[1].integer_value;
    length = values[2].integer_value;
    if (start < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", args[1]);
        return P_RESULT_ERROR;
    }
    if (length < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", args[2]);
        return P_RESULT_ERROR;
    }
    result->type = P_TERM_STRING;
    result->string_value = p_arith_mid
        (context, str, (unsigned int)start, (unsigned int)length);
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_nan_0
 * <b>nan/0</b> - mathematical constant for the IEEE not-a-number
 * indication.
 *
 * \par Usage
 * \em Var \b is \b nan
 *
 * \par Description
 * This function evaluates to the floating-point mathematical
 * constant for the IEEE not-a-number indication.
 *
 * \par Examples
 * \code
 * NaN is nan
 * \endcode
 *
 * \par See Also
 * \ref func_e_0 "e/0",
 * \ref func_inf_0 "inf/0",
 * \ref func_pi_0 "pi/0",
 * \ref isnan_1 "isnan/1"
 */
static p_goal_result p_arith_nan
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    result->type = P_TERM_REAL;
    result->real_value = 0.0 / 0.0;
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_pi_0
 * <b>pi/0</b> - mathematical constant for \em pi.
 *
 * \par Usage
 * \em Var \b is \b pi
 *
 * \par Description
 * This function evaluates to the floating-point mathematical
 * constant for \em pi, 3.14159265358979323846.
 *
 * \par Examples
 * \code
 * PIOver2 is pi / 2
 * \endcode
 *
 * \par See Also
 * \ref func_e_0 "e/0",
 * \ref func_inf_0 "inf/0",
 * \ref func_nan_0 "nan/0"
 */
static p_goal_result p_arith_pi
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    result->type = P_TERM_REAL;
    result->real_value = 3.14159265358979323846;
    return P_RESULT_TRUE;
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_pow_2
 * <b>pow/2</b>, <b>(**)/2</b> - raises one arithmetic term
 * to the power of another.
 *
 * \par Usage
 * \em Var \b is \b pow(\em Expr1, \em Expr2)
 * \par
 * \em Var \b is \em Expr1 <b>**</b> \em Expr2
 *
 * \par Description
 * Evaluates \em Expr1 and \em Expr2 and computes the result
 * of raising \em Expr1 to the power of \em Expr2.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr1)</tt> - \em Expr1 is
 *     not a number.
 * \li <tt>type_error(number, \em Expr2)</tt> - \em Expr2 is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is pow(2, 3)           X is set to 8
 * X is pow(3.5, 0)         X is set to 1.0
 * X is pow(9, 0.5)         X is set to 3.0
 * X is pow(1.0, 43)        X is set to 1.0
 * X is pow(1.0, nan)       X is set to 1.0
 * X is pow(nan, 0)         X is set to 1.0
 * X is pow(0.0, 20)        X is set to 0.0
 * X is pow(-1, inf)        X is set to 1.0
 * X is pow(-1, -inf)       X is set to 1.0
 * X is pow(0.5, -inf)      X is set to inf
 * X is pow(1.5, -inf)      X is set to 0.0
 * X is pow(1.5, inf)       X is set to inf
 * X is pow(-inf, -3)       X is set to 0.0
 * X is pow(-inf, 3)        X is set to -inf
 * X is pow(-inf, 4)        X is set to inf
 * X is pow(inf, -2)        X is set to 0.0
 * X is pow(inf, 2)         X is set to inf
 * X is pow(0.0, -2)        X is set to inf
 * X is pow(nan, nan)       X is set to nan
 * X is pow("2.0", 2)       type_error(number, "2.0")
 * X is pow(2, "2.0")       type_error(number, "2.0")
 * \endcode
 *
 * \par Compatibility
 * The <b>(**)/2</b> function is compatible with
 * \ref standard "Standard Prolog".  The new name <b>pow/2</b>
 * is the recommended spelling.
 *
 * \par See Also
 * \ref func_exp_1 "exp/1",
 * \ref func_log_1 "log/2",
 * \ref func_sqrt_1 "sqrt/1"
 */
static p_goal_result p_arith_pow
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                pow(values[0].integer_value, values[1].integer_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                pow(values[0].integer_value, values[1].real_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else if (values[0].type == P_TERM_REAL) {
        if (values[1].type == P_TERM_REAL) {
            result->type = P_TERM_REAL;
            result->real_value =
                pow(values[0].real_value, values[1].real_value);
            return P_RESULT_TRUE;
        } else if (values[1].type == P_TERM_INTEGER) {
            result->type = P_TERM_REAL;
            result->real_value =
                pow(values[0].real_value, values[1].integer_value);
            return P_RESULT_TRUE;
        } else {
            *error = p_create_type_error(context, "number", args[1]);
            return P_RESULT_ERROR;
        }
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_right_2
 * <b>right/2</b> - extract the right portion of a string.
 *
 * \par Usage
 * \em Var \b is \b right(\em Expr, \em Length)
 *
 * \par Description
 * Evaluates \em Expr \em Length.  Returns the \em Length bytes
 * at the end of the string.
 * \par
 * If \em Length is greater than the length of \em Expr,
 * then the whole string is returned.
 *
 * \par Errors
 *
 * \li <tt>type_error(string, \em Expr)</tt> - \em Expr is
 *     not a string.
 * \li <tt>type_error(integer, \em Length)</tt> - \em Length is
 *     not an integer.
 * \li <tt>domain_error(not_less_than_zero, \em Length)</tt> -
 *     \em Start is an integer that is less than zero.
 *
 * \par Examples
 * \code
 * X is right("foobar", 3)      X is set to "bar"
 * X is right("foobar", 10)     X is set to "foobar"
 * X is right("foobar", 0)      X is set to ""
 * X is right(1.5, 1)           type_error(string, 1.5)
 * X is right("foobar", 1.0)    type_error(integer, 1.0)
 * X is right("foobar", -1)     domain_error(not_less_than_zero, -1)
 * \endcode
 *
 * \par See Also
 * \ref func_left_2 "left/2",
 * \ref func_mid_2 "mid/2",
 * \ref func_mid_3 "mid/3"
 */
static p_goal_result p_arith_right
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    p_term *str;
    int length;
    if (values[0].type != P_TERM_STRING) {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
    if (values[1].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[1]);
        return P_RESULT_ERROR;
    }
    str = values[0].string_value;
    length = values[1].integer_value;
    if (length < 0) {
        *error = p_create_domain_error
            (context, "not_less_than_zero", args[1]);
        return P_RESULT_ERROR;
    }
    result->type = P_TERM_STRING;
    if (length >= str->header.size) {
        result->string_value = str;
    } else {
        result->string_value = p_arith_mid
            (context, str, str->header.size - (unsigned int)length,
             (unsigned int)length);
    }
    return P_RESULT_TRUE;
}

/* Standardized rounding function that avoids system-specific
 * variations in round() and rint() */
P_INLINE double p_arith_round_nearest(double x)
{
    if (x >= 0.0) {
        double y = floor(x);
        if ((x - y) >= 0.5)
            return ceil(x);
        else
            return y;
    } else {
        double y = ceil(x);
        if ((x - y) <= -0.5)
            return floor(x);
        else
            return y;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_round_1
 * <b>round/1</b> - rounds an arithmetic term to the nearest integer.
 *
 * \par Usage
 * \em Var \b is \b round(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and rounds the value to the nearest integer.
 * If \em Expr is halfway between two integers (at 0.5), then
 * <b>round/1</b> will round away from zero.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is round(3)            X is set to 3
 * X is round(35.125)       X is set to 35.0
 * X is round(35.5)         X is set to 36.0
 * X is round(35.625)       X is set to 36.0
 * X is round(-35.125)      X is set to -35.0
 * X is round(-35.5)        X is set to -36.0
 * X is round(-35.625)      X is set to -36.0
 * X is round(0.0)          X is set to 0.0
 * X is round(-inf)         X is set to -inf
 * X is round(nan)          X is set to nan
 * X is round("-35")        type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * The <b>round/1</b> function in \ref standard "Standard Prolog"
 * produces an integer result, which means it does not work on
 * large floating-point values.  Use \b integer(\b round(\em Expr))
 * to achieve the Standard Prolog behavior.
 *
 * \par See Also
 * \ref func_integer_1 "integer/1",
 * \ref func_ceil_1 "ceil/1",
 * \ref func_floor_1 "floor/1"
 */
static p_goal_result p_arith_round
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        result->integer_value = values[0].integer_value;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value =
            p_arith_round_nearest(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_sign_1
 * <b>sign/1</b> - calculate the sign (-1, 0, or 1) of an
 * arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b sign(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and produces the integer values -1, 0, or 1
 * dependening upon whether \em Expr is negative, zero, or positive.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is sign(3)             X is set to 1
 * X is sign(-35.125)       X is set to -1
 * X is sign(0.0)           X is set to 0
 * X is sign(-0.0)          X is set to 0
 * X is sign(-inf)          X is set to -1
 * X is sign(nan)           X is set to 0
 * X is sign("-35")         type_error(number, "-35")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_neg_1 "(-)/1",
 * \ref func_abs_1 "abs/1"
 */
static p_goal_result p_arith_sign
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_INTEGER;
        if (values[0].integer_value < 0)
            result->integer_value = -1;
        else if (values[0].integer_value > 0)
            result->integer_value = 1;
        else
            result->integer_value = 0;
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_INTEGER;
        if (values[0].real_value < 0.0)
            result->integer_value = -1;
        else if (values[0].real_value > 0.0)
            result->integer_value = 1;
        else
            result->integer_value = 0;
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_sin_1
 * <b>sin/1</b> - sine of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b sin(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr as a value in radians and produces its
 * sine as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is sin(0)              sets X to 0.0
 * X is sin(pi / 2)         sets X to 1.0
 * X is sin(inf)            sets X to nan
 * X is sin(nan)            sets X to nan
 * X is sin("1.0")          type_error(number, "1.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_tan_1 "tan/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_sin
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = sin((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = sin(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_sqrt_1
 * <b>sqrt/1</b> - square root of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b sqrt(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and computes the square root of \em Expr.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is sqrt(1)             X is set to 1.0
 * X is sqrt(0.0)           X is set to 0.0
 * X is sqrt(2.0)           X is set to 1.41421...
 * X is sqrt(256)           X is set to 16.0
 * X is sqrt(inf)           X is set to inf
 * X is sqrt(-2.0)          X is set to nan
 * X is sqrt(-inf)          X is set to nan
 * X is sqrt(nan)           X is set to nan
 * X is sqrt("2.0")         type_error(number, "2.0")
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog"
 *
 * \par See Also
 * \ref func_exp_1 "exp/1",
 * \ref func_log_1 "log/1",
 * \ref func_pow_2 "pow/2"
 */
static p_goal_result p_arith_sqrt
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = sqrt((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = sqrt(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_string_1
 * <b>string/1</b> - conversion to string.
 *
 * \par Usage
 * \em Var \b is \b string(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr and converts it into a string value
 * using the C function snprintf().  Floating point values
 * use the format "<tt>%.10g</tt>", adding a trailing "<tt>.0</tt>"
 * if necessary to make the value a valid Plang floating-point
 * constant.  Use \ref func_string_2 "string/2" to specify a
 * different precision from 10.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * S is string(3)           S is set to "3"
 * S is string(3.5)         S is set to "3.5"
 * S is string(2.0 + 6.0)   S is set to "8.0"
 * S is string("foo")       S is set to "foo" (no change)
 * S is string(2.0e35)      S is set to "2e+35"
 * S is string('3')         type_error(evaluable, '3')
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref func_float_1 "float/1",
 * \ref func_integer_1 "integer/1",
 * \ref func_string_2 "string/2"
 */
static p_goal_result p_arith_string
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    char buffer[128];
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_STRING;
        snprintf(buffer, sizeof(buffer), "%d", values[0].integer_value);
        result->string_value = p_term_create_string(context, buffer);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_STRING;
        if (isnan(values[0].real_value)) {
            result->string_value = p_term_create_string(context, "nan");
            return P_RESULT_TRUE;
        } else if (isinf(values[0].real_value) != 0) {
            if (values[0].real_value < 0)
                result->string_value = p_term_create_string(context, "-inf");
            else
                result->string_value = p_term_create_string(context, "inf");
            return P_RESULT_TRUE;
        }
        snprintf(buffer, sizeof(buffer) - 8,
                 "%.10g", values[0].real_value);
        if (strchr(buffer, '.') == 0 && strchr(buffer, 'e') == 0) {
            /* Must have a decimal spec for floating-point values */
            strcat(buffer, ".0");
        }
        result->string_value = p_term_create_string(context, buffer);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_STRING) {
        result->type = P_TERM_STRING;
        result->string_value = values[0].string_value;
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_string_2
 * <b>string/2</b> - conversion to string with specified
 * floating-point precision.
 *
 * \par Usage
 * \em Var \b is \b string(\em Expr, \em Precision)
 *
 * \par Description
 * Evaluates \em Expr and converts it into a string value
 * using the C function snprintf().  Floating point values
 * use the format "<tt>%.Pg</tt>" where <tt>P</tt> is the
 * result of evaluating \em Precision.  A trailing "<tt>.0</tt>"
 * will be added if necessary to make the value a valid Plang
 * floating-point constant.
 *
 * \par Errors
 * Same as for \ref is_2 "is/2".
 *
 * \par Examples
 * \code
 * S is string(3, 2)        S is set to "3"
 * S is string(3.5, 2)      S is set to "3.5"
 * S is string(3.125, 2)    S is set to "3.1"
 * S is string("foo", 2)    S is set to "foo" (no change)
 * S is string('3')         type_error(evaluable, '3')
 * \endcode
 *
 * \par See Also
 * \ref is_2 "is/2",
 * \ref func_float_1 "float/1",
 * \ref func_integer_1 "integer/1",
 * \ref func_string_1 "string/1"
 */
static p_goal_result p_arith_string_2
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    char prec[64];
    char buffer[128];
    if (values[1].type != P_TERM_INTEGER) {
        *error = p_create_type_error(context, "integer", args[1]);
        return P_RESULT_ERROR;
    }
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_STRING;
        snprintf(buffer, sizeof(buffer), "%d", values[0].integer_value);
        result->string_value = p_term_create_string(context, buffer);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_STRING;
        if (isnan(values[0].real_value)) {
            result->string_value = p_term_create_string(context, "nan");
            return P_RESULT_TRUE;
        } else if (isinf(values[0].real_value) != 0) {
            if (values[0].real_value < 0)
                result->string_value = p_term_create_string(context, "-inf");
            else
                result->string_value = p_term_create_string(context, "inf");
            return P_RESULT_TRUE;
        }
        snprintf(prec, sizeof(prec), "%%.%dg", values[1].integer_value);
        snprintf(buffer, sizeof(buffer) - 8,
                 prec, values[0].real_value);
        if (strchr(buffer, '.') == 0 && strchr(buffer, 'e') == 0) {
            /* Must have a decimal spec for floating-point values */
            strcat(buffer, ".0");
        }
        result->string_value = p_term_create_string(context, buffer);
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_STRING) {
        result->type = P_TERM_STRING;
        result->string_value = values[0].string_value;
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "string", args[0]);
        return P_RESULT_ERROR;
    }
}

/**
 * \addtogroup arithmetic
 * <hr>
 * \anchor func_tan_1
 * <b>tan/1</b> - tangent of an arithmetic term.
 *
 * \par Usage
 * \em Var \b is \b tan(\em Expr)
 *
 * \par Description
 * Evaluates \em Expr as a value in radians and produces its
 * tangent as the result.
 *
 * \par Errors
 *
 * \li <tt>type_error(number, \em Expr)</tt> - \em Expr is
 *     not a number.
 *
 * \par Examples
 * \code
 * X is tan(0)              sets X to 0.0
 * X is tan(1.0)            sets X to 1.557407...
 * X is tan(inf)            sets X to nan
 * X is tan(nan)            sets X to nan
 * X is tan("1.0")          type_error(number, "1.0")
 * \endcode
 *
 * \par See Also
 * \ref func_acos_1 "acos/1",
 * \ref func_asin_1 "asin/1",
 * \ref func_atan_1 "atan/1",
 * \ref func_atan2_2 "atan2/2",
 * \ref func_cos_1 "cos/1",
 * \ref func_sin_1 "sin/1",
 * \ref func_pi_0 "pi/0"
 */
static p_goal_result p_arith_tan
    (p_context *context, p_arith_value *result,
     const p_arith_value *values, p_term **args, p_term **error)
{
    if (values[0].type == P_TERM_INTEGER) {
        result->type = P_TERM_REAL;
        result->real_value = tan((double)(values[0].integer_value));
        return P_RESULT_TRUE;
    } else if (values[0].type == P_TERM_REAL) {
        result->type = P_TERM_REAL;
        result->real_value = tan(values[0].real_value);
        return P_RESULT_TRUE;
    } else {
        *error = p_create_type_error(context, "number", args[0]);
        return P_RESULT_ERROR;
    }
}

void _p_db_init_arith(p_context *context)
{
    static struct p_builtin const builtins[] = {
        /* Predicates */
        {"is", 2, p_builtin_is},
        {"=:=", 2, p_builtin_num_eq},
        {"=!=", 2, p_builtin_num_ne},
        {"=\\=", 2, p_builtin_num_ne},
        {"<", 2, p_builtin_num_lt},
        {"<=", 2, p_builtin_num_le},
        {"=<", 2, p_builtin_num_le},
        {">", 2, p_builtin_num_gt},
        {">=", 2, p_builtin_num_ge},
        {"fperror", 1, p_builtin_fperror},
        {"isnan", 1, p_builtin_isnan},
        {"isinf", 1, p_builtin_isinf},
        {0, 0, 0}
    };
    static struct p_arith const ariths[] = {
        /* Functions */
        {"+", 2, p_arith_add},
        {"-", 1, p_arith_neg},
        {"-", 2, p_arith_sub},
        {"*", 2, p_arith_mul},
        {"/", 2, p_arith_div},
        {"%", 2, p_arith_mod},
        {"**", 2, p_arith_pow},
        {"/\\", 2, p_arith_and},
        {"\\/", 2, p_arith_or},
        {"^", 2, p_arith_xor},
        {"~", 1, p_arith_not},
        {"\\", 1, p_arith_not},
        {"<<", 2, p_arith_lshift},
        {">>", 2, p_arith_rshift},
        {">>>", 2, p_arith_rushift},
        {"abs", 1, p_arith_abs},
        {"acos", 1, p_arith_acos},
        {"asin", 1, p_arith_asin},
        {"atan", 1, p_arith_atan},
        {"atan2", 2, p_arith_atan2},
        {"ceil", 1, p_arith_ceil},
        {"ceiling", 1, p_arith_ceil},
        {"cos", 1, p_arith_cos},
        {"e", 0, p_arith_e},
        {"exp", 1, p_arith_exp},
        {"float", 1, p_arith_float},
        {"float_fractional_part", 1, p_arith_float_fractional_part},
        {"float_integer_part", 1, p_arith_float_integer_part},
        {"floor", 1, p_arith_floor},
        {"inf", 0, p_arith_inf},
        {"integer", 1, p_arith_integer},
        {"left", 2, p_arith_left},
        {"log", 1, p_arith_log},
        {"mid", 2, p_arith_mid_2},
        {"mid", 3, p_arith_mid_3},
        {"mod", 2, p_arith_mod},
        {"nan", 0, p_arith_nan},
        {"pi", 0, p_arith_pi},
        {"pow", 2, p_arith_pow},
        {"rem", 2, p_arith_rem},
        {"right", 2, p_arith_right},
        {"round", 1, p_arith_round},
        {"sign", 1, p_arith_sign},
        {"sin", 1, p_arith_sin},
        {"sqrt", 1, p_arith_sqrt},
        {"string", 1, p_arith_string},
        {"string", 2, p_arith_string_2},
        {"tan", 1, p_arith_tan},
        {"truncate", 1, p_arith_integer},
        {0, 0, 0}
    };
    _p_db_register_builtins(context, builtins);
    _p_db_register_ariths(context, ariths);
}
