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

bagof(Template, Goal, List)
{
    '$$witness'(Template ^ Goal, W, Subgoal);
    findall([W, Template], Subgoal, All);
    if (All == []) fail;
    // TODO
}

findall(Term, Goal, List)
{
    if (!'$$findall_is_list'(List))
        throw(error(type_error(list, List), findall/3));
    '$$unique'(Tag);
    assertz('$$findall_list'(Tag, []));
    ('$$findall'(Tag, Term, Goal) || true);
    retract('$$findall_list'(Tag, L));
    '$$findall_reverse'(L, [], List);
}

setof(Term, Goal, List)
{
    // TODO
}

'$$findall_is_list'(List)
{
    var(List);
    commit;
}
'$$findall_is_list'([])
{
    commit;
}
'$$findall_is_list'([_|List])
{
    '$$findall_is_list'(List);
}

'$$findall'(Tag, Term, Goal)
{
    call(Goal);
    retract('$$findall_list'(Tag, List));
    assertz('$$findall_list'(Tag, [Term|List]));
    fail;
}

'$$findall_reverse'([], L, L).
'$$findall_reverse'([H|T], L1, L2)
{
    '$$findall_reverse'(T, [H|L1], L2);
}
