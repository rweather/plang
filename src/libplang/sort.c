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

#include <plang/term.h>
#include <plang/errors.h>
#include "term-priv.h"
#include "context-priv.h"
#include "database-priv.h"

/**
 * \var P_SORT_ASCENDING
 * \ingroup term
 * Sort into ascending order.
 */

/**
 * \var P_SORT_DESCENDING
 * \ingroup term
 * Sort into descending order.
 */

/**
 * \var P_SORT_KEYED
 * \ingroup term
 * The elements have an arity-2 functor where the first argument
 * is the sort key and the second is the value.  If the element
 * does not have arity-2, then the element itself is used as the key.
 */

/**
 * \var P_SORT_REVERSE_KEYED
 * \ingroup term
 * The elements have an arity-2 functor where the first argument
 * is the value and the second is the sort key.  If the element
 * does not have arity-2, then the element itself is used as the key.
 */

/**
 * \var P_SORT_UNIQUE
 * \ingroup term
 * Remove duplicate keys to create a list of unique elements.
 */

P_INLINE int p_term_sort_compare
    (p_context *context, p_term *term1, p_term *term2, int flags)
{
    int cmp;
    if (flags & P_SORT_KEYED) {
        if (term1->header.type == P_TERM_FUNCTOR &&
                term1->header.size == 2)
            term1 = term1->functor.arg[0];
        if (term2->header.type == P_TERM_FUNCTOR &&
                term2->header.size == 2)
            term2 = term2->functor.arg[0];
    } else if (flags & P_SORT_REVERSE_KEYED) {
        if (term1->header.type == P_TERM_FUNCTOR &&
                term1->header.size == 2)
            term1 = term1->functor.arg[1];
        if (term2->header.type == P_TERM_FUNCTOR &&
                term2->header.size == 2)
            term2 = term2->functor.arg[1];
    }
    cmp = p_term_precedes(context, term1, term2);
    if (flags & P_SORT_DESCENDING)
        return -cmp;
    else
        return cmp;
}

/* Recursive merge sort within an array.  "Algorithms in C++",
 * Robert Sedgewick, Addison-Wesley, 1992 */
static void p_term_sort_section
    (p_context *context, p_term **array, p_term **temp,
     int left, int right, int flags)
{
    int middle, i, j, k;
    if (left >= right)
        return;
    middle = (left + right) / 2;
    p_term_sort_section(context, array, temp, left, middle, flags);
    p_term_sort_section(context, array, temp, middle + 1, right, flags);
    for (i = middle + 1; i > left; --i)
        temp[i - 1] = array[i - 1];
    for (j = middle; j < right; ++j)
        temp[right + middle - j] = array[j + 1];
    for (k = left; k <= right; ++k) {
        if (p_term_sort_compare(context, temp[i], temp[j], flags) <= 0)
            array[k] = temp[i++];
        else
            array[k] = temp[j--];
    }
}

/* Convert the contents of a non-empty array into a list */
static p_term *p_term_section_to_list
    (p_context *context, p_term **array, int size, int flags)
{
    p_term *list = p_term_create_list(context, array[0], 0);
    p_term *tail = list;
    p_term *new_tail;
    int index;
    if (!list)
        return 0;
    if ((flags & P_SORT_UNIQUE) == 0) {
        for (index = 1; index < size; ++index) {
            new_tail = p_term_create_list(context, array[index], 0);
            if (!new_tail)
                return 0;
            p_term_set_tail(tail, new_tail);
            tail = new_tail;
        }
    } else {
        for (index = 1; index < size; ++index) {
            if (!p_term_sort_compare
                    (context, array[index - 1], array[index], flags))
                continue;
            new_tail = p_term_create_list(context, array[index], 0);
            if (!new_tail)
                return 0;
            p_term_set_tail(tail, new_tail);
            tail = new_tail;
        }
    }
    p_term_set_tail(tail, context->nil_atom);
    return list;
}

/* Merge two sorted lists */
static p_term *p_term_sort_merge
    (p_context *context, p_term *list1, p_term *list2, int flags)
{
    p_term *head = 0;
    p_term *tail = 0;
    int cmp;
    for (;;) {
        if (list1 == context->nil_atom) {
            if (head)
                p_term_set_tail(tail, list2);
            else
                head = list2;
            break;
        }
        if (list2 == context->nil_atom) {
            if (head)
                p_term_set_tail(tail, list1);
            else
                head = list1;
            break;
        }
        cmp = p_term_sort_compare(context, list1->list.head,
                                  list2->list.head, flags);
        if (cmp <= 0) {
            if (head)
                p_term_set_tail(tail, list1);
            else
                head = list1;
            tail = list1;
            list1 = list1->list.tail;
            if (!cmp && (flags & P_SORT_UNIQUE) != 0)
                list2 = list2->list.tail;
        } else {
            if (head)
                p_term_set_tail(tail, list2);
            else
                head = list2;
            tail = list2;
            list2 = list2->list.tail;
        }
    }
    return head;
}

/**
 * \brief Sorts \a list according to the \ref term_precedes
 * "term precedes" relationship.
 *
 * Returns the sorted version of the list, or null if some part
 * of \a list is not a valid list.  If \a list ends in a variable
 * tail, then the list will be sorted and the returned list will
 * end in nil.
 *
 * The \a flags indicate how to compare keys within the \a list.
 * The supported flags are P_SORT_ASCENDING, P_SORT_DESCENDING,
 * P_SORT_KEYED, P_SORT_REVERSE_KEYED, and P_SORT_UNIQUE.
 *
 * The sorting algorithm used is merge sort, which will preserve
 * the original ordering of elements that have identical keys.
 *
 * \ingroup term
 * \sa p_term_precedes()
 */
p_term *p_term_sort(p_context *context, p_term *list, int flags)
{
#define P_SORT_SECTION_SIZE     256
    p_term *array[P_SORT_SECTION_SIZE];
    p_term *temp[P_SORT_SECTION_SIZE];
    int size;
    p_term *section;
    p_term *sections;

    /* Bail out if not a valid list, or an empty list */
    list = p_term_deref(list);
    if (!list)
        return 0;
    if (list == context->nil_atom)
        return list;
    if (list->header.type != P_TERM_LIST)
        return 0;

    /* Break the list up into sections and sort each section
     * using an array-based merge sort procedure */
    size = 0;
    sections = 0;
    do {
        if ((array[size++] = p_term_deref(list->list.head)) == 0)
            return 0;
        if (size >= P_SORT_SECTION_SIZE) {
            p_term_sort_section
                (context, array, temp, 0, size - 1, flags);
            section = p_term_section_to_list
                (context, array, size, flags);
            if (!section)
                return 0;
            if (sections) {
                sections = p_term_sort_merge
                    (context, sections, section, flags);
            } else {
                sections = section;
            }
            size = 0;
        }
        list = p_term_deref(list->list.tail);
    } while (list && list->header.type == P_TERM_LIST);
    if (list && (list->header.type & P_TERM_VARIABLE) == 0 &&
            list != context->nil_atom)
        return 0;       /* Tail is not a variable or nil */
    if (size > 0) {
        p_term_sort_section(context, array, temp, 0, size - 1, flags);
        section = p_term_section_to_list(context, array, size, flags);
        if (!section)
            return 0;
        if (sections) {
            sections = p_term_sort_merge
                (context, sections, section, flags);
        } else {
            sections = section;
        }
    }
    return sections;
}

/**
 * \addtogroup sorting
 *
 * Predicates in this group are used to efficiently sort lists into
 * ascending or descending order.  The sorting algorithm used is
 * <a href="http://en.wikipedia.org/wiki/Merge_sort">merge sort</a>,
 * which provides a stable sort where elements with the same key
 * keep their relative ordering in the result.
 *
 * \ref keysort_2 "keysort/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref msort_2 "msort/2",
 * \ref msortd_2 "msortd/2",
 * \ref rkeysort_2 "rkeysort/2",
 * \ref rkeysortd_2 "rkeysortd/2",
 * \ref sort_2 "sort/2",
 * \ref sortd_2 "sortd/2"
 */

static p_goal_result p_builtin_common_sort
    (p_context *context, p_term **args, p_term **error, int flags)
{
    p_term *list = p_term_deref_member(context, args[0]);
    p_term *sorted;
    if (!list || (list->header.type & P_TERM_VARIABLE) != 0) {
        *error = p_create_instantiation_error(context);
        return P_RESULT_ERROR;
    }
    sorted = p_term_sort(context, list, flags);
    if (!sorted) {
        *error = p_create_type_error(context, "list", list);
        return P_RESULT_ERROR;
    }
    if (p_term_unify(context, args[1], sorted, P_BIND_DEFAULT))
        return P_RESULT_TRUE;
    else
        return P_RESULT_FAIL;
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor keysort_2
 * <b>keysort/2</b> - sorts a keyed list into ascending order.
 *
 * \par Usage
 * \b keysort(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * should be functors of arity 2.  The first argument to the functor
 * is used as a key to be ordered according to \ref term_lt_2 "(\@<)/2".
 * The tail of \em List can be a variable; the sorted version will
 * have nil as its tail.
 * \par
 * If an element is not a functor of arity 2, then the element itself
 * will be used as the key.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * keysort([a - 1, y - 2, b - 8, m - 6], [a - 1, b - 8, m - 6, y - 2])
 * \endcode
 *
 * \par Compatibility
 * \ref swi_prolog "SWI-Prolog" has a <b>keysort/2</b> predicate
 * that performs the same function as this predicate with some
 * minor changes.  Plang supports any arity-2 functor for the element
 * (or no arity-2 functor) whereas SWI-Prolog mandates the use
 * of <b>(-)/2</b>.  Plang's version is backwards compatible with
 * SWI-Prolog's.
 *
 * \par See Also
 * \ref term_lt_2 "(\@<)/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref rkeysort_2 "rkeysort/2",
 * \ref sort_2 "sort/2"
 */
static p_goal_result p_builtin_keysort
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_ASCENDING | P_SORT_KEYED);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor keysortd_2
 * <b>keysortd/2</b> - sorts a keyed list into descending order.
 *
 * \par Usage
 * \b keysortd(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * should be functors of arity 2.  The first argument to the functor
 * is used as a key to be ordered according to \ref term_gt_2 "(\@>)/2".
 * The tail of \em List can be a variable; the sorted version will
 * have nil as its tail.
 * \par
 * If an element is not a functor of arity 2, then the element itself
 * will be used as the key.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * keysortd([a - 1, y - 2, b - 8, m - 6], [y - 2, m - 6, b - 8, a - 1])
 * \endcode
 *
 * \par See Also
 * \ref term_gt_2 "(\@>)/2",
 * \ref keysort_2 "keysort/2",
 * \ref rkeysortd_2 "rkeysortd/2",
 * \ref sortd_2 "sortd/2"
 */
static p_goal_result p_builtin_keysortd
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_DESCENDING | P_SORT_KEYED);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor msort_2
 * <b>msort/2</b> - sorts a list into ascending order without
 * removing duplicates.
 *
 * \par Usage
 * \b msort(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * are ordered according to \ref term_lt_2 "(\@<)/2".  The tail of
 * \em List can be a variable; the sorted version will have nil
 * as its tail.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * msort([a, y, b, m], [a, b, m, y])
 * msort([a, y, a, m], [a, a, m, y])
 * \endcode
 *
 * \par Compatibility
 * \ref swi_prolog "SWI-Prolog"
 *
 * \par See Also
 * \ref term_lt_2 "(\@<)/2",
 * \ref keysort_2 "keysort/2",
 * \ref msortd_2 "msortd/2",
 * \ref sort_2 "sort/2"
 */
static p_goal_result p_builtin_msort
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_ASCENDING);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor msortd_2
 * <b>msortd/2</b> - sorts a list into descending order without
 * removing duplicates.
 *
 * \par Usage
 * \b msortd(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * are ordered according to \ref term_gt_2 "(\@>)/2".  The tail of
 * \em List can be a variable; the sorted version will have nil
 * as its tail.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * msortd([a, y, b, m], [y, m, b, a])
 * msortd([a, y, a, m], [y, m, a, a])
 * \endcode
 *
 * \par See Also
 * \ref term_gt_2 "(\@>)/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref msort_2 "msort/2",
 * \ref sortd_2 "sortd/2"
 */
static p_goal_result p_builtin_msortd
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_DESCENDING);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor rkeysort_2
 * <b>rkeysort/2</b> - sorts a keyed list into ascending order
 * with reversed keying.
 *
 * \par Usage
 * \b rkeysort(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * should be functors of arity 2.  The second argument to the functor
 * is used as a key to be ordered according to \ref term_lt_2 "(\@<)/2".
 * The tail of \em List can be a variable; the sorted version will
 * have nil as its tail.
 * \par
 * If an element is not a functor of arity 2, then the element itself
 * will be used as the key.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * rkeysort([a - 1, y - 2, b - 8, m - 6], [a - 1, y - 2, m - 6, b - 8])
 * \endcode
 *
 * \par See Also
 * \ref term_lt_2 "(\@<)/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref rkeysort_2 "rkeysort/2",
 * \ref sort_2 "sort/2"
 */
static p_goal_result p_builtin_rkeysort
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_ASCENDING | P_SORT_REVERSE_KEYED);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor rkeysortd_2
 * <b>rkeysortd/2</b> - sorts a keyed list into descending order
 * with reversed keying.
 *
 * \par Usage
 * \b rkeysortd(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * should be functors of arity 2.  The second argument to the functor
 * is used as a key to be ordered according to \ref term_gt_2 "(\@>)/2".
 * The tail of \em List can be a variable; the sorted version will
 * have nil as its tail.
 * \par
 * If an element is not a functor of arity 2, then the element itself
 * will be used as the key.
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * rkeysortd([a - 1, y - 2, b - 8, m - 6], [b - 8, m - 6, y - 2, a - 1])
 * \endcode
 *
 * \par See Also
 * \ref term_gt_2 "(\@>)/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref rkeysort_2 "rkeysort/2",
 * \ref sortd_2 "sortd/2"
 */
static p_goal_result p_builtin_rkeysortd
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_DESCENDING | P_SORT_REVERSE_KEYED);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor sort_2
 * <b>sort/2</b> - sorts a list into ascending order and
 * remove duplicates.
 *
 * \par Usage
 * \b sort(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * are ordered according to \ref term_lt_2 "(\@<)/2".  The tail of
 * \em List can be a variable; the sorted version will have nil
 * as its tail.  Duplicate elements in \em List will appear only
 * once in \em Sorted
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * sort([a, y, b, m], [a, b, m, y])
 * sort([a, y, a, m], [a, m, y])
 * \endcode
 *
 * \par Compatibility
 * \ref swi_prolog "SWI-Prolog"
 *
 * \par See Also
 * \ref term_lt_2 "(\@<)/2",
 * \ref keysort_2 "keysort/2",
 * \ref msort_2 "msort/2",
 * \ref sortd_2 "sortd/2"
 */
static p_goal_result p_builtin_sort
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_ASCENDING | P_SORT_UNIQUE);
}

/**
 * \addtogroup sorting
 * <hr>
 * \anchor sortd_2
 * <b>sortd/2</b> - sorts a list into descending order and
 * remove duplicates.
 *
 * \par Usage
 * \b sortd(\em List, \em Sorted)
 *
 * \par Description
 * Unifies \em Sorted with a sorted version of \em List.  The elements
 * are ordered according to \ref term_gt_2 "(\@>)/2".  The tail of
 * \em List can be a variable; the sorted version will have nil
 * as its tail.  Duplicate elements in \em List will appear only
 * once in \em Sorted
 *
 * \par Errors
 *
 * \li <tt>instantiation_error</tt> - \em List is a variable.
 * \li <tt>type_error(list, \em List)</tt> - \em List is not a list
 *     or the tail of \em List is not nil or a variable.
 *
 * \par Examples
 * \code
 * sortd([a, y, b, m], [y, m, b, a])
 * sortd([a, y, a, m], [y, m, a])
 * \endcode
 *
 * \par See Also
 * \ref term_gt_2 "(\@>)/2",
 * \ref keysortd_2 "keysortd/2",
 * \ref msortd_2 "msortd/2",
 * \ref sort_2 "sort/2"
 */
static p_goal_result p_builtin_sortd
    (p_context *context, p_term **args, p_term **error)
{
    return p_builtin_common_sort
        (context, args, error, P_SORT_DESCENDING | P_SORT_UNIQUE);
}

void _p_db_init_sort(p_context *context)
{
    static struct p_builtin const builtins[] = {
        {"keysort", 2, p_builtin_keysort},
        {"keysortd", 2, p_builtin_keysortd},
        {"msort", 2, p_builtin_msort},
        {"msortd", 2, p_builtin_msortd},
        {"rkeysort", 2, p_builtin_rkeysort},
        {"rkeysortd", 2, p_builtin_rkeysortd},
        {"sort", 2, p_builtin_sort},
        {"sortd", 2, p_builtin_sortd},
        {0, 0, 0}
    };
    _p_db_register_builtins(context, builtins);
}
