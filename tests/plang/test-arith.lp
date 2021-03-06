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

:- import(test).

eval(Expr, Expected)
{
    X is Expr;
    if (X !== Expected)
        fail(Expr == Expected, "failed");
}

eval_close(Expr, Expected)
{
    X is Expr;
    Y is abs(X - Expected);
    if (Y > 0.00001)
        fail(Expr == Expected, "failed");
}

eval_error(Expr, Expected)
{
    try {
        X is Expr;
    } catch (error(Error, _)) {
        if (Error !== Expected)
            fail(Expr, Error);
        SawError = true;
    }
    if (var(SawError))
        fail(Expr, "expecting an eval error");
}

test(constants)
{
    eval(e, 2.7182818284590452354);
    eval(pi, 3.14159265358979323846);
}

test(addition)
{
    eval(2 + 2, 4);
    eval(2 + -2, 0);
    eval(1.5 + 3, 4.5);
    eval(3 + 1.5, 4.5);
    eval(3.0 + 1.5, 4.5);
    eval("foo" + "bar", "foobar");
    eval("foo" + "", "foo");
    eval("" + "bar", "bar");

    eval_error(X + 1, instantiation_error);
    eval_error(1 + Y, instantiation_error);
    eval_error(2 + foo, type_error(evaluable, foo));
    eval_error(foo + 2, type_error(evaluable, foo));
    eval_error(1 + "foo", type_error(number, "foo"));
    eval_error("foo" + 1, type_error(string, 1));
    eval_error(1.5 + "foo", type_error(number, "foo"));
    eval_error("foo" + 1.5, type_error(string, 1.5));
}

test(negation)
{
    eval(- 2, -2);
    eval(- 1.5, -1.5);
    eval(--2, 2);
    eval(- - 2, 2);

    eval_error(-X, instantiation_error);
    eval_error(-foo, type_error(evaluable, foo));
    eval_error(-"foo", type_error(number, "foo"));
}

test(subtraction)
{
    eval(2 - 2, 0);
    eval(2 - -2, 4);
    eval(1.5 - 3, -1.5);
    eval(3 - 1.5, 1.5);
    eval(3.0 - 1.5, 1.5);

    eval_error(X - 1, instantiation_error);
    eval_error(1 - Y, instantiation_error);
    eval_error(2 - foo, type_error(evaluable, foo));
    eval_error(foo - 2, type_error(evaluable, foo));
    eval_error(1 - "foo", type_error(number, "foo"));
    eval_error("foo" - 1, type_error(number, "foo"));
    eval_error(1.5 - "foo", type_error(number, "foo"));
    eval_error("foo" - 1.5, type_error(number, "foo"));
}

test(multiplication)
{
    eval(2 * 2, 4);
    eval(2 * -2, -4);
    eval(1.5 * 3, 4.5);
    eval(3 * 1.5, 4.5);
    eval(3.0 * 1.5, 4.5);

    eval_error(X * 1, instantiation_error);
    eval_error(1 * Y, instantiation_error);
    eval_error(2 * foo, type_error(evaluable, foo));
    eval_error(foo * 2, type_error(evaluable, foo));
    eval_error(1 * "foo", type_error(number, "foo"));
    eval_error("foo" * 1, type_error(number, "foo"));
    eval_error(1.5 * "foo", type_error(number, "foo"));
    eval_error("foo" * 1.5, type_error(number, "foo"));
}

test(division)
{
    eval(2 / 2, 1);
    eval(2 / -2, -1);
    eval(1.5 / 3, 0.5);
    eval(3 / 1.5, 2.0);
    eval(3.0 / 1.5, 2.0);

    A is 42.0 / 0; verify(isinf(A) && A > 0);
    B is 42 / 0.0; verify(isinf(B) && B > 0);
    C is -42 / 0.0; verify(isinf(C) && C < 0);
    D is 0.0 / 0.0; verify(isnan(D));

    eval_error(X / 1, instantiation_error);
    eval_error(1 / Y, instantiation_error);
    eval_error(2 / foo, type_error(evaluable, foo));
    eval_error(foo / 2, type_error(evaluable, foo));
    eval_error(1 / "foo", type_error(number, "foo"));
    eval_error("foo" / 1, type_error(number, "foo"));
    eval_error(1.5 / "foo", type_error(number, "foo"));
    eval_error("foo" / 1.5, type_error(number, "foo"));
    eval_error(42 / 0, evaluation_error(zero_divisor));
}

test(modulus)
{
    eval(2 % 2, 0);
    eval(3 mod 2, 1);
    eval(1.5 % 3, 1.5);
    eval(3 mod 1.5, 0.0);
    eval(3.0 % 1.5, 0.0);

    A is 42.0 mod 0; verify(isnan(A));
    B is 42.0 % 0.0; verify(isnan(B));
    C is -42.0 mod 0.0; verify(isnan(C));

    eval_error(X % 1, instantiation_error);
    eval_error(1 mod Y, instantiation_error);
    eval_error(2 % foo, type_error(evaluable, foo));
    eval_error(foo mod 2, type_error(evaluable, foo));
    eval_error(1 % "foo", type_error(number, "foo"));
    eval_error("foo" mod 1, type_error(number, "foo"));
    eval_error(1.5 % "foo", type_error(number, "foo"));
    eval_error("foo" mod 1.5, type_error(number, "foo"));
    eval_error(42 % 0, evaluation_error(zero_divisor));
}

test(remainder)
{
    eval(2 rem 2, 0);
    eval(3 rem 2, 1);
    eval(1.5 rem 3, 1.5);
    eval(3 rem 1.5, 0.0);
    eval(3.0 rem 1.5, 0.0);

    A is 42.0 rem 0; verify(isnan(A));
    B is 42.0 rem 0.0; verify(isnan(B));
    C is -42.0 rem 0.0; verify(isnan(C));

    eval_error(X rem 1, instantiation_error);
    eval_error(1 rem Y, instantiation_error);
    eval_error(2 rem foo, type_error(evaluable, foo));
    eval_error(foo rem 2, type_error(evaluable, foo));
    eval_error(1 rem "foo", type_error(number, "foo"));
    eval_error("foo" rem 1, type_error(number, "foo"));
    eval_error(1.5 rem "foo", type_error(number, "foo"));
    eval_error("foo" rem 1.5, type_error(number, "foo"));
    eval_error(42 rem 0, evaluation_error(zero_divisor));
}

test(arith_eq)
{
    verify(2 + 4 =:= 6);
    verify(1.5 =:= 3.0 / 2);
    verify(1 =:= 1.0);
    verify(2.0 + 2.0 =:= 4);
    verify("foobar" =:= "foo" + "bar");
    verify(!(2 + 2 =:= 5));

    verify_error(X =:= 1, instantiation_error);
    verify_error(1 =:= Y, instantiation_error);
    verify_error(2 =:= foo, type_error(evaluable, foo));
    verify_error(foo =:= 2, type_error(evaluable, foo));
    verify_error(1 =:= "foo", type_error(number, "foo"));
    verify_error("foo" =:= 1, type_error(string, 1));
    verify_error(1.5 =:= "foo", type_error(number, "foo"));
    verify_error("foo" =:= 1.5, type_error(string, 1.5));
}

test(arith_ne)
{
    verify(2 + 2 =!= 5);
    verify(1.5 =!= 3.0 * 2);
    verify(1 =!= 2.0);
    verify(2.0 + 2.0 =!= 5);
    verify("foobar" =!= "foo" + "ba");
    verify(!(2 + 2 =!= 4));

    verify_error(X =!= 1, instantiation_error);
    verify_error(1 =!= Y, instantiation_error);
    verify_error(2 =!= foo, type_error(evaluable, foo));
    verify_error(foo =!= 2, type_error(evaluable, foo));
    verify_error(1 =!= "foo", type_error(number, "foo"));
    verify_error("foo" =!= 1, type_error(string, 1));
    verify_error(1.5 =!= "foo", type_error(number, "foo"));
    verify_error("foo" =!= 1.5, type_error(string, 1.5));

    verify(2 + 2 =\= 5);
    verify(1.5 =\= 3.0 * 2);
    verify(1 =\= 2.0);
    verify(2.0 + 2.0 =\= 5);
    verify("foobar" =\= "foo" + "ba");
    verify(!(2 + 2 =\= 4));
}

test(arith_lt)
{
    verify(2 + 2 < 5);
    verify(1.5 < 3.0 * 2);
    verify(1 < 1.5);
    verify(2.0 + 2.0 < 5);
    verify("foo" + "ba" < "foobar");
    verify(!(2 + 2 < 4));

    verify_error(X < 1, instantiation_error);
    verify_error(1 < Y, instantiation_error);
    verify_error(2 < foo, type_error(evaluable, foo));
    verify_error(foo < 2, type_error(evaluable, foo));
    verify_error(1 < "foo", type_error(number, "foo"));
    verify_error("foo" < 1, type_error(string, 1));
    verify_error(1.5 < "foo", type_error(number, "foo"));
    verify_error("foo" < 1.5, type_error(string, 1.5));
}

test(arith_le)
{
    verify(2 + 2 <= 4);
    verify(1.5 <= 3.0 / 2);
    verify(1 <= 1.0);
    verify(2.0 + 2.0 <= 4);
    verify("foo" + "bar" <= "foobar");
    verify(!(2 + 2 <= 3));

    verify_error(X <= 1, instantiation_error);
    verify_error(1 <= Y, instantiation_error);
    verify_error(2 <= foo, type_error(evaluable, foo));
    verify_error(foo <= 2, type_error(evaluable, foo));
    verify_error(1 <= "foo", type_error(number, "foo"));
    verify_error("foo" <= 1, type_error(string, 1));
    verify_error(1.5 <= "foo", type_error(number, "foo"));
    verify_error("foo" <= 1.5, type_error(string, 1.5));

    verify(2 + 2 =< 4);
    verify(1.5 =< 3.0 / 2);
    verify(1 =< 1.0);
    verify(2.0 + 2.0 =< 4);
    verify("foo" + "bar" =< "foobar");
    verify(!(2 + 2 =< 3));
}

test(arith_gt)
{
    verify(2 + 2 > 3);
    verify(3.0 * 2 > 1.5);
    verify(1 > 0.5);
    verify(2.0 + 2.0 > 3);
    verify("foobar" > "foo" + "ba");
    verify(!(2 + 2 > 4));

    verify_error(X > 1, instantiation_error);
    verify_error(1 > Y, instantiation_error);
    verify_error(2 > foo, type_error(evaluable, foo));
    verify_error(foo > 2, type_error(evaluable, foo));
    verify_error(1 > "foo", type_error(number, "foo"));
    verify_error("foo" > 1, type_error(string, 1));
    verify_error(1.5 > "foo", type_error(number, "foo"));
    verify_error("foo" > 1.5, type_error(string, 1.5));
}

test(arith_ge)
{
    verify(2 + 2 >= 4);
    verify(3.0 / 2 >= 1.5);
    verify(1 >= 1.0);
    verify(2.0 + 2.0 >= 4);
    verify("foobar" >= "foo" + "bar");
    verify(!(2 + 2 >= 5));

    verify_error(X >= 1, instantiation_error);
    verify_error(1 >= Y, instantiation_error);
    verify_error(2 >= foo, type_error(evaluable, foo));
    verify_error(foo >= 2, type_error(evaluable, foo));
    verify_error(1 >= "foo", type_error(number, "foo"));
    verify_error("foo" >= 1, type_error(string, 1));
    verify_error(1.5 >= "foo", type_error(number, "foo"));
    verify_error("foo" >= 1.5, type_error(string, 1.5));
}

test(float_conversion)
{
    eval(float(3), 3.0);
    eval(float(3.5), 3.5);
    eval(float(2 + 6), 8.0);
    eval(float("-3.5e02"), -350.0);
    eval(float("   -3.5e02   "), -350.0);
    eval(float("35"), 35.0);

    eval_error(float(X), instantiation_error);
    eval_error(float(foo), type_error(evaluable, foo));
    eval_error(float("foo"), type_error(number, "foo"));
    eval_error(float('3'), type_error(evaluable, '3'));
    eval_error(float(""), type_error(number, ""));
    eval_error(float("-35x"), type_error(number, "-35x"));
    eval_error(float("- 35"), type_error(number, "- 35"));

    // out of range
    eval_error(float("2.0e400"), type_error(number, "2.0e400"));
}

test(integer_conversion)
{
    eval(integer(3), 3);
    eval(integer(3.5), 3);
    eval(integer(-3.5), -3);
    eval(integer(2.0 + 6.0), 8);
    eval(integer("-35"), -35);
    eval(integer("  -35  "), -35);
    eval(integer(-2147483648.9), -2147483648);
    eval(integer(2147483647.9), 2147483647);

    eval_error(integer(X), instantiation_error);
    eval_error(integer(foo), type_error(evaluable, foo));
    eval_error(integer("foo"), type_error(integer, "foo"));
    eval_error(integer('3'), type_error(evaluable, '3'));
    eval_error(integer(""), type_error(integer, ""));
    eval_error(integer("-35x"), type_error(integer, "-35x"));
    eval_error(integer("- 35"), type_error(integer, "- 35"));
    eval_error(integer("11111111111111111111"), evaluation_error(int_overflow));
    eval_error(integer("2147483648"), evaluation_error(int_overflow));
    eval_error(integer("-2147483649"), evaluation_error(int_overflow));
    eval_error(integer(2147483648.0), evaluation_error(int_overflow));
    eval_error(integer(-2147483649.0), evaluation_error(int_overflow));
}

test(string_conversion)
{
    eval(string(3), "3");
    eval(string(-3.0), "-3.0");
    eval(string(3.5), "3.5");
    eval(string(350), "350");
    eval(string(350.0), "350.0");
    eval(string(-3.125), "-3.125");
    eval(string(2.0 + 6.0), "8.0");
    eval(string(2.0e35), "2e+35");
    eval(string("foo"), "foo");

    eval(string(3, 2), "3");
    eval(string(-3.0, 2), "-3.0");
    eval(string(3.5, 2), "3.5");
    eval(string(350.0, 20), "350.0");
    eval(string(-3.125, 2), "-3.1");
    eval(string(2.0 + 6.0, 2), "8.0");
    eval(string(2.0e35, 2), "2e+35");
    eval(string("foo", 2), "foo");

    eval_error(string(X), instantiation_error);
    eval_error(string(1, Y), instantiation_error);
    eval_error(string(foo), type_error(evaluable, foo));
    eval_error(string(1, 3.5), type_error(integer, 3.5));
    eval_error(string('3'), type_error(evaluable, '3'));
}

test(nan)
{
    verify(isnan(nan));
    verify(isnan(float("nan")));
    verify(!isnan(3));
    verify(!isnan(3.5));
    verify(!isnan(inf));

    eval(string(nan), "nan");

    verify_error(isnan("nan"), type_error(number, "nan"));
}

test(inf)
{
    verify(isinf(inf));
    verify(isinf(-inf));
    verify(isinf(float("inf")));
    verify(isinf(float("-inf")));
    verify(!isinf(3));
    verify(!isinf(3.5));
    verify(!isinf(nan));

    Inf is inf; verify(isinf(Inf) && Inf > 0);
    NegInf is -inf; verify(isinf(NegInf) && NegInf < 0);

    eval(string(inf), "inf");
    eval(string(-inf), "-inf");

    verify_error(isinf("inf"), type_error(number, "inf"));
}

test(bitwise_and)
{
    eval(22 /\ 5, 4);

    eval_error(X /\ 1, instantiation_error);
    eval_error(1 /\ Y, instantiation_error);
    eval_error(2 /\ foo, type_error(evaluable, foo));
    eval_error(foo /\ 2, type_error(evaluable, foo));
    eval_error(1 /\ "foo", type_error(integer, "foo"));
    eval_error("foo" /\ 1, type_error(integer, "foo"));
    eval_error(22.5 /\ 5, type_error(integer, 22.5));
    eval_error(22 /\ 5.0, type_error(integer, 5.0));
}

test(bitwise_or)
{
    eval(22 \/ 5, 23);

    eval_error(X \/ 1, instantiation_error);
    eval_error(1 \/ Y, instantiation_error);
    eval_error(2 \/ foo, type_error(evaluable, foo));
    eval_error(foo \/ 2, type_error(evaluable, foo));
    eval_error(1 \/ "foo", type_error(integer, "foo"));
    eval_error("foo" \/ 1, type_error(integer, "foo"));
    eval_error(22.5 \/ 5, type_error(integer, 22.5));
    eval_error(22 \/ 5.0, type_error(integer, 5.0));
}

test(bitwise_xor)
{
    eval(22 ^ 5, 19);

    eval_error(X ^ 1, instantiation_error);
    eval_error(1 ^ Y, instantiation_error);
    eval_error(2 ^ foo, type_error(evaluable, foo));
    eval_error(foo ^ 2, type_error(evaluable, foo));
    eval_error(1 ^ "foo", type_error(integer, "foo"));
    eval_error("foo" ^ 1, type_error(integer, "foo"));
    eval_error(22.5 ^ 5, type_error(integer, 22.5));
    eval_error(22 ^ 5.0, type_error(integer, 5.0));
}

test(bitwise_not)
{
    eval(~22, -23);
    eval(\22, -23);

    eval_error(~X, instantiation_error);
    eval_error(~foo, type_error(evaluable, foo));
    eval_error(~"foo", type_error(integer, "foo"));
    eval_error(~22.5, type_error(integer, 22.5));
}

test(shift_left)
{
    eval(22 << 5, 704);

    eval_error(X << 1, instantiation_error);
    eval_error(1 << Y, instantiation_error);
    eval_error(2 << foo, type_error(evaluable, foo));
    eval_error(foo << 2, type_error(evaluable, foo));
    eval_error(1 << "foo", type_error(integer, "foo"));
    eval_error("foo" << 1, type_error(integer, "foo"));
    eval_error(22.5 << 5, type_error(integer, 22.5));
    eval_error(22 << 5.0, type_error(integer, 5.0));
}

test(shift_right)
{
    eval(22 >> 5, 0);
    eval(-22 >> 5, -1);

    eval_error(X >> 1, instantiation_error);
    eval_error(1 >> Y, instantiation_error);
    eval_error(2 >> foo, type_error(evaluable, foo));
    eval_error(foo >> 2, type_error(evaluable, foo));
    eval_error(1 >> "foo", type_error(integer, "foo"));
    eval_error("foo" >> 1, type_error(integer, "foo"));
    eval_error(22.5 >> 5, type_error(integer, 22.5));
    eval_error(22 >> 5.0, type_error(integer, 5.0));
}

test(shift_uright)
{
    eval(22 >>> 5, 0);
    eval(-22 >>> 5, 0x07ffffff);

    eval_error(X >>> 1, instantiation_error);
    eval_error(1 >>> Y, instantiation_error);
    eval_error(2 >>> foo, type_error(evaluable, foo));
    eval_error(foo >>> 2, type_error(evaluable, foo));
    eval_error(1 >>> "foo", type_error(integer, "foo"));
    eval_error("foo" >>> 1, type_error(integer, "foo"));
    eval_error(22.5 >>> 5, type_error(integer, 22.5));
    eval_error(22 >>> 5.0, type_error(integer, 5.0));
}

test(abs)
{
    eval(abs(3), 3);
    eval(abs(-35.125), 35.125);
    eval(abs(2147483648.0), 2147483648.0);
    eval(abs(-2147483648.0), 2147483648.0);

    AbsInf is abs(inf); verify(isinf(AbsInf) && AbsInf > 0);
    AbsNegInf is abs(-inf); verify(isinf(AbsNegInf) && AbsInf > 0);
    AbsNaN is abs(nan); verify(isnan(AbsNaN));

    eval_error(abs(X), instantiation_error);
    eval_error(abs(foo), type_error(evaluable, foo));
    eval_error(abs("foo"), type_error(number, "foo"));
    eval_error(abs(-2147483648), evaluation_error(int_overflow));
}

test(sign)
{
    eval(sign(3), 1);
    eval(sign(0), 0);
    eval(sign(-3), -1);
    eval(sign(35.125), 1);
    eval(sign(0.0), 0);
    eval(sign(-0.0), 0);
    eval(sign(-35.125), -1);
    eval(sign(inf), 1);
    eval(sign(-inf), -1);
    eval(sign(nan), 0);

    eval_error(sign(X), instantiation_error);
    eval_error(sign(foo), type_error(evaluable, foo));
    eval_error(sign("foo"), type_error(number, "foo"));
}

test(truncate)
{
    eval(truncate(3), 3);
    eval(truncate(3.5), 3);
    eval(truncate(-3.5), -3);
    eval(truncate(2.0 + 6.0), 8);
    eval(truncate("-35"), -35);
    eval(truncate("  -35  "), -35);
    eval(truncate(-2147483648.9), -2147483648);
    eval(truncate(2147483647.9), 2147483647);

    eval_error(truncate(X), instantiation_error);
    eval_error(truncate(foo), type_error(evaluable, foo));
    eval_error(truncate("foo"), type_error(integer, "foo"));
    eval_error(truncate('3'), type_error(evaluable, '3'));
    eval_error(truncate(""), type_error(integer, ""));
    eval_error(truncate("-35x"), type_error(integer, "-35x"));
    eval_error(truncate("- 35"), type_error(integer, "- 35"));
    eval_error(truncate("11111111111111111111"), evaluation_error(int_overflow));
    eval_error(truncate("2147483648"), evaluation_error(int_overflow));
    eval_error(truncate("-2147483649"), evaluation_error(int_overflow));
    eval_error(truncate(2147483648.0), evaluation_error(int_overflow));
    eval_error(truncate(-2147483649.0), evaluation_error(int_overflow));
}

test(round)
{
    eval(round(3), 3);
    eval(round(-3), -3);
    eval(round(35.125), 35.0);
    eval(round(35.5), 36.0);
    eval(round(35.625), 36.0);
    eval(round(36.5), 37.0);
    eval(round(-35.125), -35.0);
    eval(round(-35.5), -36.0);
    eval(round(-35.625), -36.0);
    eval(round(-36.5), -37.0);
    eval(round(0.0), 0.0);
    eval(round(-0.0), 0.0);

    RoundInf is round(inf); verify(isinf(RoundInf) && RoundInf > 0);
    RoundNegInf is round(-inf); verify(isinf(RoundNegInf) && RoundNegInf < 0);
    RoundNaN is round(nan); verify(isnan(RoundNaN));

    eval_error(round(X), instantiation_error);
    eval_error(round(foo), type_error(evaluable, foo));
    eval_error(round("foo"), type_error(number, "foo"));
    eval_error(round('3'), type_error(evaluable, '3'));
}

test(ceil)
{
    eval(ceil(3), 3);
    eval(ceil(-3), -3);
    eval(ceil(35.125), 36.0);
    eval(ceil(35.5), 36.0);
    eval(ceil(35.625), 36.0);
    eval(ceil(36.5), 37.0);
    eval(ceil(-35.125), -35.0);
    eval(ceil(-35.5), -35.0);
    eval(ceil(-35.625), -35.0);
    eval(ceil(-36.5), -36.0);
    eval(ceil(0.0), 0.0);
    eval(ceil(-0.0), 0.0);

    eval(ceiling(3), 3);
    eval(ceiling(-3), -3);
    eval(ceiling(35.125), 36.0);
    eval(ceiling(35.5), 36.0);
    eval(ceiling(35.625), 36.0);
    eval(ceiling(36.5), 37.0);
    eval(ceiling(-35.125), -35.0);
    eval(ceiling(-35.5), -35.0);
    eval(ceiling(-35.625), -35.0);
    eval(ceiling(-36.5), -36.0);
    eval(ceiling(0.0), 0.0);
    eval(ceiling(-0.0), 0.0);

    CeilInf is ceil(inf); verify(isinf(CeilInf) && CeilInf > 0);
    CeilNegInf is ceil(-inf); verify(isinf(CeilNegInf) && CeilNegInf < 0);
    CeilNaN is ceil(nan); verify(isnan(CeilNaN));

    eval_error(ceil(X), instantiation_error);
    eval_error(ceil(foo), type_error(evaluable, foo));
    eval_error(ceil("foo"), type_error(number, "foo"));
    eval_error(ceil('3'), type_error(evaluable, '3'));
}

test(floor)
{
    eval(floor(3), 3);
    eval(floor(-3), -3);
    eval(floor(35.125), 35.0);
    eval(floor(35.5), 35.0);
    eval(floor(35.625), 35.0);
    eval(floor(36.5), 36.0);
    eval(floor(-35.125), -36.0);
    eval(floor(-35.5), -36.0);
    eval(floor(-35.625), -36.0);
    eval(floor(-36.5), -37.0);
    eval(floor(0.0), 0.0);
    eval(floor(-0.0), 0.0);

    FloorInf is floor(inf); verify(isinf(FloorInf) && FloorInf > 0);
    FloorNegInf is floor(-inf); verify(isinf(FloorNegInf) && FloorNegInf < 0);
    FloorNaN is floor(nan); verify(isnan(FloorNaN));

    eval_error(floor(X), instantiation_error);
    eval_error(floor(foo), type_error(evaluable, foo));
    eval_error(floor("foo"), type_error(number, "foo"));
    eval_error(floor('3'), type_error(evaluable, '3'));
}

test(float_fractional_part)
{
    eval(float_fractional_part(3), 0);
    eval(float_fractional_part(-3), 0);
    eval(float_fractional_part(35.125), 0.125);
    eval(float_fractional_part(35.5), 0.5);
    eval(float_fractional_part(35.625), 0.625);
    eval(float_fractional_part(36.5), 0.5);
    eval(float_fractional_part(-35.125), -0.125);
    eval(float_fractional_part(-35.5), -0.5);
    eval(float_fractional_part(-35.625), -0.625);
    eval(float_fractional_part(-36.5), -0.5);
    eval(float_fractional_part(0.0), 0.0);
    eval(float_fractional_part(-0.0), 0.0);
    eval(float_fractional_part(inf), 0.0);
    eval(float_fractional_part(-inf), 0.0);

    FractNaN is float_fractional_part(nan); verify(isnan(FractNaN));

    eval_error(float_fractional_part(X), instantiation_error);
    eval_error(float_fractional_part(foo), type_error(evaluable, foo));
    eval_error(float_fractional_part("foo"), type_error(number, "foo"));
    eval_error(float_fractional_part('3'), type_error(evaluable, '3'));
}

test(float_integer_part)
{
    eval(float_integer_part(3), 3);
    eval(float_integer_part(-3), -3);
    eval(float_integer_part(35.125), 35.0);
    eval(float_integer_part(35.5), 35.0);
    eval(float_integer_part(35.625), 35.0);
    eval(float_integer_part(36.5), 36.0);
    eval(float_integer_part(-35.125), -35.0);
    eval(float_integer_part(-35.5), -35.0);
    eval(float_integer_part(-35.625), -35.0);
    eval(float_integer_part(-36.5), -36.0);
    eval(float_integer_part(0.0), 0.0);
    eval(float_integer_part(-0.0), 0.0);

    PartInf is float_integer_part(inf); verify(isinf(PartInf) && PartInf > 0);
    PartNegInf is float_integer_part(-inf); verify(isinf(PartNegInf) && PartNegInf < 0);
    PartNaN is float_integer_part(nan); verify(isnan(PartNaN));

    eval_error(float_integer_part(X), instantiation_error);
    eval_error(float_integer_part(foo), type_error(evaluable, foo));
    eval_error(float_integer_part("foo"), type_error(number, "foo"));
    eval_error(float_integer_part('3'), type_error(evaluable, '3'));
}

test(exp)
{
    eval_close(exp(1), e);
    eval_close(exp(0), 1.0);
    eval_close(exp(2.0), e * e);
    eval_close(exp(-2.0), 1 / (e * e));
    eval_close(exp(-inf), 0.0);

    ExpInf is exp(inf); verify(isinf(ExpInf) && ExpInf > 0);
    ExpNaN is exp(nan); verify(isnan(ExpNaN));

    eval_error(exp(X), instantiation_error);
    eval_error(exp(foo), type_error(evaluable, foo));
    eval_error(exp("foo"), type_error(number, "foo"));
    eval_error(exp('3'), type_error(evaluable, '3'));
}

test(log)
{
    eval_close(log(e), 1.0);
    eval_close(log(1.0), 0.0);
    eval_close(log(e * e), 2.0);
    eval_close(log(1 / (e * e)), -2.0);

    LogInf is log(inf); verify(isinf(LogInf) && LogInf > 0);
    LogZero is log(0.0); verify(isinf(LogZero) && LogZero < 0);
    LogNegInf is log(-inf); verify(isnan(LogNegInf));
    LogNaN is log(nan); verify(isnan(LogNaN));

    eval_error(log(X), instantiation_error);
    eval_error(log(foo), type_error(evaluable, foo));
    eval_error(log("foo"), type_error(number, "foo"));
    eval_error(log('3'), type_error(evaluable, '3'));
}

test(pow)
{
    eval(pow(2, 3), 8.0);
    eval(3.5 ** 0, 1.0);
    eval(pow(9, 0.5), 3.0);
    eval(1.0 ** 43, 1.0);
    eval(pow(1.0, nan), 1.0);
    eval(nan ** 0.0, 1.0);
    eval(pow(0.0, 20), 0.0);
    eval(-1.0 ** inf, 1.0);
    eval(pow(-1.0, -inf), 1.0);
    eval(1.5 ** -inf, 0.0);
    eval(pow(-inf, -3), 0.0);
    eval(inf ** -2, 0.0);

    Pow1 is pow(0.5, -inf); verify(isinf(Pow1) && Pow1 > 0);
    Pow2 is 1.5 ** inf; verify(isinf(Pow2) && Pow2 > 0);
    Pow3 is pow(-inf, 3); verify(isinf(Pow3) && Pow3 < 0);
    Pow4 is -inf ** 4; verify(isinf(Pow4) && Pow4 > 0);
    Pow5 is pow(inf, 2); verify(isinf(Pow5) && Pow5 > 0);
    Pow6 is 0.0 ** -2; verify(isinf(Pow6) && Pow6 > 0);
    Pow7 is pow(nan, nan); verify(isnan(Pow7));

    eval_error(pow(X, 1), instantiation_error);
    eval_error(1 ** Y, instantiation_error);
    eval_error(pow(2, foo), type_error(evaluable, foo));
    eval_error(foo ** 2, type_error(evaluable, foo));
    eval_error(pow(1, "foo"), type_error(number, "foo"));
    eval_error("foo" ** 1, type_error(number, "foo"));
    eval_error(pow(1.5, "foo"), type_error(number, "foo"));
    eval_error("foo" ** 1.5, type_error(number, "foo"));
}

test(sqrt)
{
    eval_close(sqrt(1), 1.0);
    eval_close(sqrt(0.0), 0.0);
    eval_close(sqrt(2.0), 1.41421356237309504880);
    eval_close(sqrt(256), 16.0);

    SqrtInf is sqrt(inf); verify(isinf(SqrtInf) && SqrtInf > 0);
    SqrtNeg2 is sqrt(-2.0); verify(isnan(SqrtNeg2));
    SqrtNegInf is sqrt(-inf); verify(isnan(SqrtNegInf));
    SqrtNaN is sqrt(nan); verify(isnan(SqrtNaN));

    eval_error(sqrt(X), instantiation_error);
    eval_error(sqrt(foo), type_error(evaluable, foo));
    eval_error(sqrt("foo"), type_error(number, "foo"));
    eval_error(sqrt('3'), type_error(evaluable, '3'));
}

test(acos)
{
    eval_close(acos(1), 0.0);
    eval_close(acos(0.0) * 2, pi);

    ACosInf is acos(inf); verify(isnan(ACosInf));
    ACosNegInf is acos(-inf); verify(isnan(ACosNegInf));
    ACosNaN is acos(nan); verify(isnan(ACosNaN));

    eval_error(acos(X), instantiation_error);
    eval_error(acos(foo), type_error(evaluable, foo));
    eval_error(acos("foo"), type_error(number, "foo"));
    eval_error(acos('3'), type_error(evaluable, '3'));
}

test(asin)
{
    eval_close(asin(0), 0.0);
    eval_close(asin(1.0) * 2, pi);

    ASinInf is asin(inf); verify(isnan(ASinInf));
    ASinNegInf is asin(-inf); verify(isnan(ASinNegInf));
    ASinNaN is asin(nan); verify(isnan(ASinNaN));

    eval_error(asin(X), instantiation_error);
    eval_error(asin(foo), type_error(evaluable, foo));
    eval_error(asin("foo"), type_error(number, "foo"));
    eval_error(asin('3'), type_error(evaluable, '3'));
}

test(atan)
{
    eval_close(atan(1.0) * 4, pi);
    eval_close(atan(0), 0.0);
    eval_close(atan(inf) * 2, pi);
    eval_close(atan(-inf) * 2, -pi);

    AtanNaN is atan(nan); verify(isnan(AtanNaN));

    eval_error(atan(X), instantiation_error);
    eval_error(atan(foo), type_error(evaluable, foo));
    eval_error(atan("foo"), type_error(number, "foo"));
    eval_error(atan('3'), type_error(evaluable, '3'));
}

test(atan2)
{
    eval_close(atan2(1.0, 1.0) * 4, pi);
    eval_close(atan2(0, 1), 0.0);
    eval_close(atan2(0, -1), pi);
    eval_close(atan2(inf, 2) * 2, pi);

    Atan2NaN1 is atan2(nan, 1); verify(isnan(Atan2NaN1));
    Atan2NaN2 is atan2(1, nan); verify(isnan(Atan2NaN2));

    eval_error(atan2(X, 1), instantiation_error);
    eval_error(atan2(1, Y), instantiation_error);
    eval_error(atan2(2, foo), type_error(evaluable, foo));
    eval_error(atan2(foo, 2), type_error(evaluable, foo));
    eval_error(atan2(1, "foo"), type_error(number, "foo"));
    eval_error(atan2("foo", 1), type_error(number, "foo"));
    eval_error(atan2(1.5, "foo"), type_error(number, "foo"));
    eval_error(atan2("foo", 1.5), type_error(number, "foo"));
}

test(cos)
{
    eval_close(cos(0), 1.0);
    eval_close(cos(pi / 2), 0.0);

    CosInf is cos(inf); verify(isnan(CosInf));
    CosNegInf is cos(-inf); verify(isnan(CosNegInf));
    CosNaN is cos(nan); verify(isnan(CosNaN));

    eval_error(cos(X), instantiation_error);
    eval_error(cos(foo), type_error(evaluable, foo));
    eval_error(cos("foo"), type_error(number, "foo"));
    eval_error(cos('3'), type_error(evaluable, '3'));
}

test(sin)
{
    eval_close(sin(0), 0.0);
    eval_close(sin(pi / 2), 1.0);

    SinInf is sin(inf); verify(isnan(SinInf));
    SinNegInf is sin(-inf); verify(isnan(SinNegInf));
    SinNaN is sin(nan); verify(isnan(SinNaN));

    eval_error(sin(X), instantiation_error);
    eval_error(sin(foo), type_error(evaluable, foo));
    eval_error(sin("foo"), type_error(number, "foo"));
    eval_error(sin('3'), type_error(evaluable, '3'));
}

test(tan)
{
    eval_close(tan(0), 0.0);
    eval_close(tan(1), 1.55740772465);

    TanInf is tan(inf); verify(isnan(TanInf));
    TanNegInf is tan(-inf); verify(isnan(TanNegInf));
    TanNaN is tan(nan); verify(isnan(TanNaN));

    eval_error(tan(X), instantiation_error);
    eval_error(tan(foo), type_error(evaluable, foo));
    eval_error(tan("foo"), type_error(number, "foo"));
    eval_error(tan('3'), type_error(evaluable, '3'));
}

test(mid)
{
    eval(mid("foobar", 1, 4), "ooba");
    eval(mid("foobar", 1, 0), "");
    eval(mid("foobar", 10, 3), "");
    eval(mid("foobar", 0, 6), "foobar");
    eval(mid("foobar", 4, 3), "ar");
    eval(mid("", 0, 0), "");
    eval(mid("", 4, 2), "");
    eval(mid("a\u00AFbc", 1, 2), "\u00AFb");
    eval(mid("a\u00AFbc", 1, 4), "\u00AFbc");
    eval(mid("a\u00AFbc", 2, 4), "bc");

    eval(mid("foobar", 1), "oobar");
    eval(mid("foobar", 0), "foobar");
    eval(mid("a\u00AFbc", 1), "\u00AFbc");
    eval(mid("a\u00AFbc", 0), "a\u00AFbc");

    eval_error(mid(foobar, 1, 4), type_error(evaluable, foobar));
    eval_error(mid(1.5, 1, 4), type_error(string, 1.5));
    eval_error(mid("foobar", 1.0, 4), type_error(integer, 1.0));
    eval_error(mid("foobar", 1, 4.0), type_error(integer, 4.0));
    eval_error(mid("foobar", -1, 4), domain_error(not_less_than_zero, -1));
    eval_error(mid("foobar", 1, -4), domain_error(not_less_than_zero, -4));

    eval_error(mid(foobar, 1), type_error(evaluable, foobar));
    eval_error(mid(1.5, 1), type_error(string, 1.5));
    eval_error(mid("foobar", 1.0), type_error(integer, 1.0));
    eval_error(mid("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(mid_bytes)
{
    eval(mid_bytes("foobar", 1, 4), "ooba");
    eval(mid_bytes("foobar", 1, 0), "");
    eval(mid_bytes("foobar", 10, 3), "");
    eval(mid_bytes("foobar", 0, 6), "foobar");
    eval(mid("foobar", 4, 3), "ar");
    eval(mid_bytes("", 0, 0), "");
    eval(mid_bytes("", 4, 2), "");

    eval(mid_bytes("foobar", 1), "oobar");
    eval(mid_bytes("foobar", 0), "foobar");

    eval_error(mid_bytes(foobar, 1, 4), type_error(evaluable, foobar));
    eval_error(mid_bytes(1.5, 1, 4), type_error(string, 1.5));
    eval_error(mid_bytes("foobar", 1.0, 4), type_error(integer, 1.0));
    eval_error(mid_bytes("foobar", 1, 4.0), type_error(integer, 4.0));
    eval_error(mid_bytes("foobar", -1, 4), domain_error(not_less_than_zero, -1));
    eval_error(mid_bytes("foobar", 1, -4), domain_error(not_less_than_zero, -4));

    eval_error(mid_bytes(foobar, 1), type_error(evaluable, foobar));
    eval_error(mid_bytes(1.5, 1), type_error(string, 1.5));
    eval_error(mid_bytes("foobar", 1.0), type_error(integer, 1.0));
    eval_error(mid_bytes("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(left)
{
    eval(left("foobar", 3), "foo");
    eval(left("foobar", 10), "foobar");
    eval(left("foobar", 0), "");
    eval(left("a\u00AFbc", 2), "a\u00AF");
    eval(left("a\u00AFbc", 3), "a\u00AFb");

    eval_error(left(foobar, 1), type_error(evaluable, foobar));
    eval_error(left(1.5, 1), type_error(string, 1.5));
    eval_error(left("foobar", 1.0), type_error(integer, 1.0));
    eval_error(left("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(left_bytes)
{
    eval(left_bytes("foobar", 3), "foo");
    eval(left_bytes("foobar", 10), "foobar");
    eval(left_bytes("foobar", 0), "");
    eval(left_bytes("a\xC1\x81b", 2), "a\xC1");

    eval_error(left_bytes(foobar, 1), type_error(evaluable, foobar));
    eval_error(left_bytes(1.5, 1), type_error(string, 1.5));
    eval_error(left_bytes("foobar", 1.0), type_error(integer, 1.0));
    eval_error(left_bytes("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(right)
{
    eval(right("foobar", 3), "bar");
    eval(right("foobar", 10), "foobar");
    eval(right("foobar", 0), "");
    eval(right("a\u00AFbc", 3), "\u00AFbc");
    eval(right("a\u00AFbc", 4), "a\u00AFbc");

    eval_error(right(foobar, 1), type_error(evaluable, foobar));
    eval_error(right(1.5, 1), type_error(string, 1.5));
    eval_error(right("foobar", 1.0), type_error(integer, 1.0));
    eval_error(right("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(right_bytes)
{
    eval(right_bytes("foobar", 3), "bar");
    eval(right_bytes("foobar", 10), "foobar");
    eval(right_bytes("foobar", 0), "");

    eval_error(right_bytes(foobar, 1), type_error(evaluable, foobar));
    eval_error(right_bytes(1.5, 1), type_error(string, 1.5));
    eval_error(right_bytes("foobar", 1.0), type_error(integer, 1.0));
    eval_error(right_bytes("foobar", -1), domain_error(not_less_than_zero, -1));
}

test(length)
{
    eval(length("foobar"), 6);
    eval(length(""), 0);
    eval(length("foo" + "bar"), 6);
    eval(length("a\xC1\x81b"), 3);
    eval(length("a\xC4\x81b"), 3);
    eval(length("a\u00AFb"), 3);

    eval_error(length(foobar), type_error(evaluable, foobar));
    eval_error(length(1.5), type_error(string, 1.5));
}

test(length_bytes)
{
    eval(length_bytes("foobar"), 6);
    eval(length_bytes(""), 0);
    eval(length_bytes("foo" + "bar"), 6);
    eval(length_bytes("a\xC1\x81b"), 4);
    eval(length_bytes("a\xC4\x81b"), 4);
    eval(length_bytes("a\u00AFb"), 4);

    eval_error(length_bytes(foobar), type_error(evaluable, foobar));
    eval_error(length_bytes(1.5), type_error(string, 1.5));
}

test(char)
{
    eval(char("foobar", 3), 98);
    eval(char("foobar", 0), 102);
    eval(char("a\u00AFb", 0), 0'a');
    eval(char("a\u00AFb", 1), 0xAF);
    eval(char("a\u00AFb", 2), 0'b');
    eval(char("a\uFFEFb", 1), 0xFFEF);

    eval_error(char(foobar, 2), type_error(evaluable, foobar));
    eval_error(char("foobar", foobar), type_error(evaluable, foobar));
    eval_error(char(1.5, 2), type_error(string, 1.5));
    eval_error(char("foobar", -1), domain_error(string_index, -1));
    eval_error(char("foobar", 6), domain_error(string_index, 6));
}

test(char_to_string)
{
    eval(char_to_string(98), "b");
    eval(char_to_string(0xAF), "\u00AF");
    eval(char_to_string(0xFFEF), "\uFFEF");

    eval_error(char_to_string(foobar), type_error(evaluable, foobar));
    eval_error(char_to_string(1.5), type_error(integer, 1.5));
    eval_error(char_to_string(-1), representation_error(character_code));
    eval_error(char_to_string(0x110000), representation_error(character_code));
}

test(byte)
{
    eval(byte("foobar", 3), 98);
    eval(byte("foobar", 0), 102);
    eval(byte("a\xC1\x81b", 0), 0'a');
    eval(byte("a\xC1\x81b", 1), 0xC1);
    eval(byte("a\xC1\x81b", 2), 0x81);
    eval(byte("a\xC1\x81b", 3), 0'b');

    eval_error(byte(foobar, 2), type_error(evaluable, foobar));
    eval_error(byte("foobar", foobar), type_error(evaluable, foobar));
    eval_error(byte(1.5, 2), type_error(string, 1.5));
    eval_error(byte("foobar", -1), domain_error(string_index, -1));
    eval_error(byte("foobar", 6), domain_error(string_index, 6));
}

test(byte_to_string)
{
    eval(byte_to_string(98), "b");
    eval(byte_to_string(0), "\0");
    eval(byte_to_string(255), "\xff");

    eval_error(byte_to_string(foobar), type_error(evaluable, foobar));
    eval_error(byte_to_string(1.5), type_error(integer, 1.5));
    eval_error(byte_to_string(-1), type_error(byte, -1));
    eval_error(byte_to_string(256), type_error(byte, 256));
}

test(atom_name)
{
    verify(atom_name(foobar, S1) && S1 == "foobar");
    verify(atom_name('hi there!', S2) && S2 == "hi there!");
    verify(atom_name(A1, "foobar") && A1 == foobar);
    verify(atom_name(A2, "") && A2 == '');
    verify(!atom_name(foobar, "barfoo"));

    verify_error(atom_name(A3, S3), instantiation_error);
    verify_error(atom_name(1.5, S4), type_error(atom, 1.5));
    verify_error(atom_name(A4, foobar), type_error(string, foobar));
}

test(other)
{
    verify(!(2 is 1 + 3));
    verify(!(-2 == - 2));

    eval_error(X, instantiation_error);
    eval_error(foo, type_error(evaluable, foo));
    eval_error(foo(1), type_error(evaluable, foo(1)));
    eval_error(foo(1, 2), type_error(evaluable, foo(1, 2)));

    verify(fperror(clear));
    verify(!fperror(zero_divisor));
}
