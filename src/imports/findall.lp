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

/**
 * \addtogroup module_findall
 * <hr>
 * \anchor bagof_3
 * <b>bagof/3</b> - finds all solutions to a goal and collects
 * them up into a list.
 *
 * \par Usage
 * <b>:- import</b>(<tt>findall</tt>).
 * \par
 * \b bagof(\em Template, \em Goal, \em List)
 *
 * \par Description
 * Form a \em Subgoal from \em Goal as follows:
 * \li If \em Goal does not have the form (\em A ^ \em B),
 *     then \em Subgoal is Goal.
 * \li Otherwise, the \em Subgoal of \em Goal is the subgoal of \em B.
 * \par
 * TBD
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Subgoal is a variable.
 * \li <tt>type_error(callable, \em Goal)</tt> - \em Subgoal or some
 *     part of \em Subgoal is not callable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a
 *     variable, list, or partial list.
 *
 * \par Examples
 * \code
 * TBD
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog", with the added requirement
 * to import the <tt>findall</tt> module to get the definition of
 * <b>bagof/3</b>.
 *
 * \par See Also
 * \ref findall_3 "findall/3",
 * \ref setof_3 "setof/3"
 */
bagof(Template, Goal, List)
{
    '$$witness'(Template ^ Goal, W, Subgoal);
    findall([W, Template], Subgoal, All);
    if (All == []) fail;
    // TODO
}

/**
 * \addtogroup module_findall
 * <hr>
 * \anchor findall_3
 * <b>findall/3</b> - finds all solutions to a goal and collects
 * them up into a list.
 *
 * \par Usage
 * <b>:- import</b>(<tt>findall</tt>).
 * \par
 * \b findall(\em Term, \em Goal, \em List)
 *
 * \par Description
 * TBD
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Goal is a variable.
 * \li <tt>type_error(callable, \em Goal)</tt> - \em Goal or some
 *     part of \em Goal is not callable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a
 *     variable, list, or partial list.
 *
 * \par Examples
 * \code
 * TBD
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog", with the added requirement
 * to import the <tt>findall</tt> module to get the definition of
 * <b>findall/3</b>.
 *
 * \par See Also
 * \ref bagof_3 "bagof/3",
 * \ref setof_3 "setof/3"
 */
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

/**
 * \addtogroup module_findall
 * <hr>
 * \anchor setof_3
 * <b>setof/3</b> - finds all solutions to a goal and collects
 * them up into a list.
 *
 * \par Usage
 * <b>:- import</b>(<tt>findall</tt>).
 * \par
 * \b setof(\em Term, \em Goal, \em List)
 *
 * \par Description
 * TBD
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em Goal is a variable.
 * \li <tt>type_error(callable, \em Goal)</tt> - \em Goal or some
 *     part of \em Goal is not callable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a
 *     variable, list, or partial list.
 *
 * \par Examples
 * \code
 * TBD
 * \endcode
 *
 * \par Compatibility
 * \ref standard "Standard Prolog", with the added requirement
 * to import the <tt>findall</tt> module to get the definition of
 * <b>setof/3</b>.
 *
 * \par See Also
 * \ref bagof_3 "bagof/3",
 * \ref findall_3 "findall/3"
 */
setof(Term, Goal, List)
{
    // TODO
}

/** @cond */
'$$findall_is_list'(List)
{
    var(List);
    !;
}
'$$findall_is_list'([])
{
    !;
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

/** @endcond */
