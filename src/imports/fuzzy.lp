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

// fuzzy/1 and set_fuzzy/1 are implemented in the engine.
:- '$$register_fuzzy_builtins'.

fuzzy(Expr, List)
{
    Value is Expr;
    if (!number(Value))
        throw(error(type_error(number, Expr), fuzzy/2));
    if (var(List))
        throw(error(instantiation_error, fuzzy/2));
    else if (!'$$fuzzy2_valid'(List))
        throw(error(type_error(interval_list, List), fuzzy/2));
    '$$fuzzy2'(Value, List, Fuzzy);
    fuzzy(Fuzzy);
}

'$$fuzzy2_valid'(List) { var(List); commit; fail; }
'$$fuzzy2_valid'([]) { commit; fail; }
'$$fuzzy2_valid'([H|T]) { (var(H) || var(T)); commit; fail; }
'$$fuzzy2_valid'([_, H|_]) { var(H); commit; fail; }
'$$fuzzy2_valid'([(Value1, Fuzzy1), (Value2, Fuzzy2)|Tail])
{
    number(Value1);
    number(Fuzzy1);
    number(Value2);
    number(Fuzzy2);
    commit;
    Value1 < Value2;
    '$$fuzzy2_valid'([(Value2, Fuzzy2)|Tail]);
}
'$$fuzzy2_valid'([(Value, Fuzzy)])
{
    number(Value);
    number(Fuzzy);
}

'$$fuzzy2'(_, [(_, Fuzzy)], Fuzzy)
{
    commit;
}
'$$fuzzy2'(Value, [(Value1, Fuzzy1), (Value2, Fuzzy2)|Tail], Fuzzy)
{
    if (Value <= Value1) {
        Fuzzy = Fuzzy1;
    } else if (Value <= Value2) {
        Fuzzy is Fuzzy1 + float(Fuzzy2 - Fuzzy1) * float(Value - Value1) / float(Value2 - Value1);
    } else {
        '$$fuzzy2'(Value, [(Value2, Fuzzy2)|Tail], Fuzzy);
    }
}

fuzzy_and(Goal1, Goal2)
{
    call(Goal1);
    call(Goal2);
}

fuzzy_findall(Term, Goal, List)
{
    if (!'$$fuzzy_findall_is_list'(List))
        throw(error(type_error(list, List), fuzzy_findall/3));
    '$$unique'(Tag);
    assertz('$$fuzzy_findall_list'(Tag, []));
    ('$$fuzzy_findall'(Tag, Term, Goal) || true);
    retract('$$fuzzy_findall_list'(Tag, L));
    '$$fuzzy_findall_reverse'(L, [], List);
}

fuzzy_findall_sorted(Term, Goal, List)
{
    if (!'$$fuzzy_findall_is_list'(List))
        throw(error(type_error(list, List), fuzzy_findall/3));
    '$$unique'(Tag);
    assertz('$$fuzzy_findall_list'(Tag, []));
    ('$$fuzzy_findall'(Tag, Term, Goal) || true);
    retract('$$fuzzy_findall_list'(Tag, L));
    rkeysortd(L, List);
}

fuzzy_not(Goal)
{
    fuzzy(Before);
    set_fuzzy(1);
    if (call(Goal)) {
        fuzzy(After);
        set_fuzzy(Before);
        fuzzy(1.0 - After);
    } else {
        set_fuzzy(Before);
    }
}

fuzzy_or(Goal1, Goal2)
{
    fuzzy(Before);
    set_fuzzy(1);
    if (call(Goal1)) {
        fuzzy(After1);
        set_fuzzy(1);
        if (call(Goal2))
            fuzzy(After2);
        else
            After2 = 0.0;
    } else {
        After1 = 0.0;
        set_fuzzy(1);
        call(Goal2);
        fuzzy(After2);
    }
    set_fuzzy(Before);
    if (After1 > After2)
        fuzzy(After1);
    else
        fuzzy(After2);
}

fuzzy_prod(Goal1, Goal2)
{
    fuzzy(Before);
    set_fuzzy(1);
    call(Goal1);
    fuzzy(After1);
    set_fuzzy(1);
    call(Goal2);
    fuzzy(After2);
    set_fuzzy(Before);
    fuzzy(After1 * After2);
}

fuzzy_prod_or(Goal1, Goal2)
{
    fuzzy(Before);
    set_fuzzy(1);
    if (call(Goal1)) {
        fuzzy(After1);
        set_fuzzy(1);
        if (call(Goal2))
            fuzzy(After2);
        else
            After2 = 0.0;
    } else {
        After1 = 0.0;
        set_fuzzy(1);
        call(Goal2);
        fuzzy(After2);
    }
    set_fuzzy(Before);
    fuzzy(After1 + After2 - After1 * After2);
}

'$$fuzzy_findall_is_list'(List)
{
    var(List);
    commit;
}
'$$fuzzy_findall_is_list'([])
{
    commit;
}
'$$fuzzy_findall_is_list'([_|List])
{
    '$$findall_is_list'(List);
}

'$$fuzzy_findall'(Tag, Term, Goal)
{
    call(Goal);
    fuzzy(Confidence);
    retract('$$fuzzy_findall_list'(Tag, List));
    assertz('$$fuzzy_findall_list'(Tag, [(Term, Confidence)|List]));
    fail;
}

'$$fuzzy_findall_reverse'([], L, L).
'$$fuzzy_findall_reverse'([H|T], L1, L2)
{
    '$$fuzzy_findall_reverse'(T, [H|L1], L2);
}
