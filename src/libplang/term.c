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
#include <plang/database.h>
#include "term-priv.h"
#include "context-priv.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Internal dereference algorithm, which we inline for maximum
 * performance in the functions that use it */
P_INLINE p_term *p_term_deref_non_null(const p_term *term)
{
    for(;;) {
        if (term->header.type & P_TERM_VARIABLE) {
            if (!term->var.value)
                break;
            term = term->var.value;
        } else {
            break;
        }
    }
    return (p_term *)term;
}

/**
 * \defgroup term Term Representation
 *
 * This module provides functions for creating Prolog terms
 * and operating on them.  Terms may have a number of special
 * forms, described by their p_term_type(): functors, lists,
 * atoms, strings, variables, integers, reals, and objects.
 */
/*\@{*/

/**
 * \var P_TERM_INVALID
 * \ingroup term
 * The term is invalid, usually because it is null.
 */

/**
 * \var P_TERM_FUNCTOR
 * \ingroup term
 * The term is a functor, with 1 or more arguments.
 * \sa p_term_create_functor()
 */

/**
 * \var P_TERM_LIST
 * \ingroup term
 * The term is a list, consisting of a head term and a tail list.
 * \sa p_term_create_list()
 */

/**
 * \var P_TERM_ATOM
 * \ingroup term
 * The term is an atom, or alphanumeric identifier.  Atoms are
 * unique within an execution context and can be compared by pointer.
 * \sa p_term_create_atom()
 */

/**
 * \var P_TERM_STRING
 * \ingroup term
 * The term is a string, containing zero or more UTF-8 encoded
 * characters.  Atoms and strings are not interchangeable.
 * \sa p_term_create_string()
 */

/**
 * \var P_TERM_VARIABLE
 * \ingroup term
 * The term is an unbound variable.  Any other type of term
 * may be bound to the variable.
 * \sa p_term_create_variable()
 */

/**
 * \var P_TERM_MEMBER_VARIABLE
 * \ingroup term
 * The term is a reference to a member variable within an object.
 * \sa p_term_create_member_variable()
 */

/**
 * \var P_TERM_INTEGER
 * \ingroup term
 * The term is a 32-bit signed integer.
 * \sa p_term_create_integer()
 */

/**
 * \var P_TERM_REAL
 * \ingroup term
 * The term is a double-precision floating-point number.
 * \sa p_term_create_real()
 */

/**
 * \var P_TERM_OBJECT
 * \ingroup term
 * The term is an object, consisting of zero or more property
 * bindings.  Each property has a name and a value.  The special
 * \c{prototype} property is used to implement prototype-based
 * inheritance, similar to ECMAScript.
 * \sa p_term_create_object()
 */

/**
 * \brief Creates a functor term within \a ontext with the specified
 * \a name and \a arg_count.  Returns the new functor.
 *
 * The arguments will be initially unbound.  This function should
 * be followed to calls to lc_term_bind_functor_arg() to bind the
 * arguments to specific terms.
 *
 * \ingroup term
 * \sa p_term_create_functor_with_args(), p_term_bind_functor_arg()
 * \sa p_term_functor(), p_term_arg()
 */
p_term *p_term_create_functor(p_context *context, p_term *name, int arg_count)
{
    struct p_term_functor *term;

    /* Bail out if the parameters are invalid */
    if (!name || arg_count < 0)
        return 0;
    if (name->header.type != P_TERM_ATOM) {
        name = p_term_deref_non_null(name);
        if (name->header.type != P_TERM_ATOM)
            return 0;
    }

    /* A functor with zero arguments is just an atom */
    if (!arg_count)
        return name;

    /* Create the functor term, with empty argument slots */
    term = p_term_malloc
        (context, struct p_term_functor,
            sizeof(struct p_term_functor) +
            (sizeof(p_term *) * (unsigned int)(arg_count - 1)));
    if (!term)
        return 0;
    term->header.type = P_TERM_FUNCTOR;
    term->header.size = (unsigned int)arg_count;
    term->functor_name = name;
    return (p_term *)term;
}

/**
 * \brief Binds the argument at \a index within the specified
 * functor \a term to \a value.
 *
 * Returns non-zero if the bind was successful, or zero if \a term
 * is not a functor, \a index is out of range, \a value is invalid,
 * or the argument has already been bound.
 *
 * \ingroup term
 * \sa p_term_create_functor(), p_term_arg()
 */
int p_term_bind_functor_arg(p_term *term, int index, p_term *value)
{
    if (!term || term->header.type != P_TERM_FUNCTOR || !value)
        return 0;
    if (((unsigned int)index) >= term->header.size)
        return 0;
    if (term->functor.arg[index])
        return 0;
    term->functor.arg[index] = value;
    return 1;
}

/**
 * \brief Creates a functor term within \a ontext with the specified
 * \a name and the \a arg_count members of \a args as arguments.
 * Returns the new functor.
 *
 * \ingroup term
 * \sa p_term_create_functor(), p_term_bind_functor_arg()
 */
p_term *p_term_create_functor_with_args(p_context *context, p_term *name, p_term **args, int arg_count)
{
    int index;
    p_term *term = p_term_create_functor(context, name, arg_count);
    if (!term)
        return 0;
    for (index = 0; index < arg_count; ++index)
        term->functor.arg[index] = args[index];
    return term;
}

/**
 * \brief Creates a list term from \a head and \a tail within
 * \a context.  Returns the new list.
 *
 * \ingroup term
 * \sa p_term_head(), p_term_tail(), p_term_set_tail()
 */
p_term *p_term_create_list(p_context *context, p_term *head, p_term *tail)
{
    struct p_term_list *term = p_term_new(context, struct p_term_list);
    if (!term)
        return 0;
    term->header.type = P_TERM_LIST;
    term->header.size = 2;  /* Arity indication for p_term_precedes */
    term->head = head;
    term->tail = tail;
    return (p_term *)term;
}

/**
 * \brief Sets the tail of \a list to \a tail.
 *
 * This function is intended for use by parsers that build lists
 * incrementally from the top down, where the tail of \a list
 * had previously been set to null.
 *
 * \ingroup term
 * \sa p_term_create_list(), p_term_tail()
 */
void p_term_set_tail(p_term *list, p_term *tail)
{
    if (!list || list->header.type != P_TERM_LIST)
        return;
    list->list.tail = tail;
}

/**
 * \brief Creates an atom within \a context with the specified \a name.
 *
 * Returns the atom term.  The same term will be returned every
 * time this function is called for the same \a name on \a context.
 * This allows atoms to be quickly compared for equality by
 * comparing their pointers.  By comparison, p_term_create_string()
 * creates a new term every time it is called.
 *
 * Atoms and strings are not unifiable as they are different types
 * of terms.  Atoms typically represent identifiers in the program,
 * whereas strings represent human-readable data for the program.
 *
 * The \a name should be encoded in the UTF-8 character set.
 * Use p_term_create_atom_n() for names with embedded NUL's.
 *
 * \ingroup term
 * \sa p_term_nil_atom(), p_term_create_string(), p_term_create_atom_n()
 */
p_term *p_term_create_atom(p_context *context, const char *name)
{
    return p_term_create_atom_n(context, name, name ? strlen(name) : 0);
}

/**
 * \brief Creates an atom within \a context with the \a len bytes
 * at \a name as its atom name.
 *
 * Returns the atom term.  The same term will be returned every
 * time this function is called for the same \a name on \a context.
 * This allows atoms to be quickly compared for equality by
 * comparing their pointers.  By comparison, p_term_create_string()
 * creates a new term every time it is called.
 *
 * Atoms and strings are not unifiable as they are different types
 * of terms.  Atoms typically represent identifiers in the program,
 * whereas strings represent human-readable data for the program.
 *
 * The \a name should be encoded in the UTF-8 character.
 *
 * \ingroup term
 * \sa p_term_create_atom()
 */
p_term *p_term_create_atom_n(p_context *context, const char *name, size_t len)
{
    unsigned int hash;
    const char *n;
    size_t nlen;
    p_term *atom;

    /* Look for the name in the context's atom hash */
    hash = 0;
    n = name;
    nlen = len;
    while (nlen > 0) {
        hash = hash * 5 + (((unsigned int)(*n++)) & 0xFF);
        --nlen;
    }
    hash %= P_CONTEXT_HASH_SIZE;
    atom = context->atom_hash[hash];
    while (atom != 0) {
        if (atom->header.size == len &&
                !memcmp(atom->atom.name, name, len))
            return atom;
        atom = atom->atom.next;
    }

    /* Create a new atom and add it to the hash */
    atom = p_term_malloc
        (context, p_term, sizeof(struct p_term_atom) + len);
    if (!atom)
        return 0;
    atom->header.type = P_TERM_ATOM;
    atom->header.size = (unsigned int)len;
    atom->atom.next = context->atom_hash[hash];
    if (len > 0)
        memcpy(atom->atom.name, name, len);
    atom->atom.name[len] = '\0';
    context->atom_hash[hash] = atom;
    return atom;
}

/**
 * \brief Creates a string within \a context with the specified
 * \a str value.
 *
 * Returns the string term.  Unlike p_term_create_atom(), a new term
 * is returned every time this function is called.
 *
 * Atoms and strings are not unifiable as they are different types
 * of terms.  Atoms typically represent identifiers in the program,
 * whereas strings represent human-readable data for the program.
 *
 * The \a str should be encoded in the UTF-8 character set.
 * Use p_term_create_string_n() for strings with embedded
 * NUL characters.
 *
 * \ingroup term
 * \sa p_term_create_atom(), p_term_concat_string()
 * \sa p_term_create_string_n()
 */
p_term *p_term_create_string(p_context *context, const char *str)
{
    size_t len = str ? strlen(str) : 0;
    struct p_term_string *term = p_term_malloc
        (context, struct p_term_string, sizeof(struct p_term_string) + len);
    if (!term)
        return 0;
    term->header.type = P_TERM_STRING;
    term->header.size = (unsigned int)len;
    if (len > 0)
        memcpy(term->name, str, len);
    term->name[len] = '\0';
    return (p_term *)term;
}

/**
 * \brief Creates a string within \a context with the \a len
 * bytes from the specified \a str buffer.
 *
 * Returns the string term.  Unlike p_term_create_atom(), a new term
 * is returned every time this function is called.
 *
 * Atoms and strings are not unifiable as they are different types
 * of terms.  Atoms typically represent identifiers in the program,
 * whereas strings represent human-readable data for the program.
 *
 * The \a str should be encoded in the UTF-8 character set.
 *
 * \ingroup term
 * \sa p_term_create_string(), p_term_name_length()
 */
p_term *p_term_create_string_n(p_context *context, const char *str, size_t len)
{
    struct p_term_string *term = p_term_malloc
        (context, struct p_term_string, sizeof(struct p_term_string) + len);
    if (!term)
        return 0;
    term->header.type = P_TERM_STRING;
    term->header.size = (unsigned int)len;
    if (len > 0)
        memcpy(term->name, str, len);
    term->name[len] = '\0';
    return (p_term *)term;
}

/**
 * \brief Creates an unbound variable within \a context.
 *
 * Returns the variable term.
 *
 * \ingroup term
 * \sa p_term_create_named_variable()
 */
p_term *p_term_create_variable(p_context *context)
{
    struct p_term_var *term = p_term_new(context, struct p_term_var);
    if (!term)
        return 0;
    term->header.type = P_TERM_VARIABLE;
    return (p_term *)term;
}

/**
 * \brief Creates an unbound variable within \a context and associates
 * it with \a name.
 *
 * Returns the variable term.  The \a name is for primarily for
 * debugging purposes.
 *
 * \ingroup term
 * \sa p_term_create_variable(), p_term_name()
 */
p_term *p_term_create_named_variable(p_context *context, const char *name)
{
    size_t len = name ? strlen(name) : 0;
    struct p_term_var *term;
    if (!len)
        return p_term_create_variable(context);
    term = p_term_malloc(context, struct p_term_var,
                         sizeof(struct p_term_var) + len + 1);
    if (!term)
        return 0;
    term->header.type = P_TERM_VARIABLE;
    term->header.size = (unsigned int)len;
    strcpy((char *)(term + 1), name);
    return (p_term *)term;
}

/**
 * \brief Creates an unbound member variable within \a context that
 * refers to the member \a name within \a object.
 *
 * Returns the member variable term.  The \a name must be an atom.
 *
 * If \a auto_create is non-zero, then the member \a name will be
 * added to the object during unification if it doesn't exist.
 * If \a auto_create is zero, then unification will fail if
 * the member \a name does not exist.
 *
 * \ingroup term
 * \sa p_term_object(), p_term_name()
 */
p_term *p_term_create_member_variable(p_context *context, p_term *object, p_term *name, int auto_create)
{
    struct p_term_member_var *term;
    if (!name || !object)
        return 0;
    name = p_term_deref_non_null(name);
    if (name->header.type != P_TERM_ATOM)
        return 0;
    term = p_term_new(context, struct p_term_member_var);
    if (!term)
        return 0;
    term->header.type = P_TERM_MEMBER_VARIABLE;
    term->header.size = (auto_create ? 1 : 0);
    term->object = object;
    term->name = name;
    return (p_term *)term;
}

/**
 * \brief Creates an integer within \a context with the specified
 * \a value.
 *
 * Returns the integer term.
 *
 * \ingroup term
 * \sa p_term_create_real(), p_term_integer_value()
 */
p_term *p_term_create_integer(p_context *context, int value)
{
    struct p_term_integer *term =
        p_term_new(context, struct p_term_integer);
    if (!term)
        return 0;
    term->header.type = P_TERM_INTEGER;
#if defined(P_TERM_64BIT)
    /* Pack the value into the header, to save memory */
    term->header.size = (unsigned int)value;
#else
    term->value = value;
#endif
    return (p_term *)term;
}

/**
 * \brief Creates a real within \a context with the specified
 * \a value.
 *
 * Returns the real term.
 *
 * \ingroup term
 * \sa p_term_create_integer(), p_term_real_value()
 */
p_term *p_term_create_real(p_context *context, double value)
{
    struct p_term_real *term = p_term_new(context, struct p_term_real);
    if (!term)
        return 0;
    term->header.type = P_TERM_REAL;
    term->value = value;
    return (p_term *)term;
}

/**
 * \brief Returns the special "nil" atom that represents
 * the empty list within \a context.
 *
 * Returns the atom with the name "[]".  This function is equivalent
 * to calling p_term_create_atom() with "[]" as the argument,
 * but is more efficient.
 *
 * \ingroup term
 * \sa p_term_create_atom(), p_term_prototype_atom()
 */
p_term *p_term_nil_atom(p_context *context)
{
    return context->nil_atom;
}

/**
 * \brief Returns the special "prototype" atom within \a context
 * that names the prototype for an object.
 *
 * Returns the atom with the name "prototype".  This function is
 * equivalent to calling p_term_create_atom() with "prototype"
 * as the argument, but is more efficient.
 *
 * \ingroup term
 * \sa p_term_create_atom(), p_term_class_name_atom()
 */
p_term *p_term_prototype_atom(p_context *context)
{
    return context->prototype_atom;
}

/**
 * \brief Returns the special "className" atom within \a context
 * that names the class of an object.
 *
 * Returns the atom with the name "className".  This function is
 * equivalent to calling p_term_create_atom() with "className"
 * as the argument, but is more efficient.
 *
 * \ingroup term
 * \sa p_term_create_atom(), p_term_prototype_atom()
 */
p_term *p_term_class_name_atom(p_context *context)
{
    return context->class_name_atom;
}

/**
 * \brief Dereferences \a term to resolve bound variables.
 *
 * Returns the dereferenced version of \a term, or null if
 * \a term is null.  The result may be a variable if it is unbound.
 *
 * Dereferencing is needed when terms of type P_TERM_VARIABLE
 * or P_TERM_MEMBER_VARIABLE are bound to other terms during
 * unification.  Normally dereferencing is performed automatically
 * by term query functions such as p_term_type(), p_term_arg_count(),
 * p_term_name(), etc.
 *
 * \ingroup term
 * \sa p_term_type()
 */
p_term *p_term_deref(const p_term *term)
{
    return term ? p_term_deref_non_null(term) : 0;
}

/**
 * \brief Returns the type of \a term after dereferencing it.
 *
 * Returns one of P_TERM_FUNCTOR, P_TERM_VARIABLE, etc, or
 * P_TERM_INVALID if \a term is null.
 *
 * \ingroup term
 * \sa p_term_deref()
 */
int p_term_type(const p_term *term)
{
    if (term)
        return (int)(p_term_deref_non_null(term)->header.type);
    else
        return P_TERM_INVALID;
}

/**
 * \brief Returns the number of arguments for a functor \a term,
 * or zero if \a term is not a functor.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_functor(), p_term_deref(), p_term_arg()
 */
int p_term_arg_count(const p_term *term)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a functor */
    if (term->header.type == P_TERM_FUNCTOR)
        return (int)(term->header.size);
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_FUNCTOR)
        return (int)(term->header.size);
    else
        return 0;
}

/**
 * \brief Returns the name of the functor, atom, or variable
 * contained in \a term, or null if \a term does not have a name.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_type(), p_term_create_functor(), p_term_create_atom()
 * \sa p_term_name_length()
 */
const char *p_term_name(const p_term *term)
{
    if (!term)
        return 0;
    term = p_term_deref_non_null(term);
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        return p_term_name(term->functor.functor_name);
    case P_TERM_ATOM:
        return term->atom.name;
    case P_TERM_STRING:
        return term->string.name;
    case P_TERM_VARIABLE:
        if (term->header.size > 0)
            return (const char *)(&(term->var) + 1);
        break;
    case P_TERM_MEMBER_VARIABLE:
        return p_term_name(term->member_var.name);
    default: break;
    }
    return 0;
}

/**
 * \brief Returns the length of the name of the functor, atom,
 * or variable contained in \a term, or zero if \a term does
 * not have a name.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_name()
 */
size_t p_term_name_length(const p_term *term)
{
    if (!term)
        return 0;
    term = p_term_deref_non_null(term);
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        return p_term_name_length(term->functor.functor_name);
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_VARIABLE:
        return term->header.size;
    case P_TERM_MEMBER_VARIABLE:
        return p_term_name_length(term->member_var.name);
    default: break;
    }
    return 0;
}

/**
 * \brief Returns the atom name of the functor \a term, or null
 * if \a term is not a functor.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_functor(), p_term_arg()
 */
p_term *p_term_functor(const p_term *term)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a functor */
    if (term->header.type != P_TERM_FUNCTOR) {
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_FUNCTOR)
            return 0;
    }
    return term->functor.functor_name;
}

/**
 * \brief Returns the argument at position \a index within the
 * functor \a term, or null if \a term is not a functor or
 * \a index is out of range.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_functor(), p_term_functor()
 */
p_term *p_term_arg(const p_term *term, int index)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a functor */
    if (term->header.type != P_TERM_FUNCTOR) {
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_FUNCTOR)
            return 0;
    }
    if (((unsigned int)index) < term->header.size)
        return term->functor.arg[index];
    else
        return 0;
}

/**
 * \brief Returns the 32-bit signed integer value within \a term,
 * or zero if \a term is not an integer term.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_integer(), p_term_real_value()
 */
int p_term_integer_value(const p_term *term)
{
    if (!term)
        return 0;
#if defined(P_TERM_64BIT)
    /* The value is packed into the header, to save memory */
    if (term->header.type == P_TERM_INTEGER)
        return (int)(term->header.size);
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_INTEGER)
        return (int)(term->header.size);
    else
        return 0;
#else
    if (term->header.type == P_TERM_INTEGER)
        return term->integer.value;
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_INTEGER)
        return term->integer.value;
    else
        return 0;
#endif
}

/**
 * \brief Returns the double-precision floating point value within
 * \a term, or zero if \a term is not a real term.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_real(), p_term_integer_value()
 */
double p_term_real_value(const p_term *term)
{
    if (!term)
        return 0.0;
    /* Short-cut: avoid the dereference if already a real */
    if (term->header.type == P_TERM_REAL)
        return term->real.value;
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_REAL)
        return term->real.value;
    else
        return 0.0;
}

/**
 * \brief Returns the head of the specified list \a term, or null
 * if \a term is not a list term.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_list(), p_term_tail()
 */
p_term *p_term_head(const p_term *term)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a list */
    if (term->header.type == P_TERM_LIST)
        return term->list.head;
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_LIST)
        return term->list.head;
    else
        return 0;
}

/**
 * \brief Returns the tail of the specified list \a term, or null
 * if \a term is not a list term.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_list(), p_term_head()
 */
p_term *p_term_tail(const p_term *term)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a list */
    if (term->header.type == P_TERM_LIST)
        return term->list.tail;
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_LIST)
        return term->list.tail;
    else
        return 0;
}

/**
 * \brief Returns the object term associated with a member variable
 * reference \a term, or null if \a term is not a member variable
 * reference.
 *
 * The \a term is automatically dereferenced.  The p_term_name()
 * function can be used to retrive the name of the member variable.
 *
 * \ingroup term
 * \sa p_term_create_member_variable(), p_term_name()
 */
p_term *p_term_object(const p_term *term)
{
    if (!term)
        return 0;
    /* Short-cut: avoid the dereference if already a member var ref */
    if (term->header.type == P_TERM_MEMBER_VARIABLE)
        return term->member_var.object;
    term = p_term_deref_non_null(term);
    if (term->header.type == P_TERM_MEMBER_VARIABLE)
        return term->member_var.object;
    else
        return 0;
}

/**
 * \brief Creates an object term within \a context with the specified
 * class \a prototype.
 *
 * Returns the object term.  The \a prototype must be an object.
 *
 * \ingroup term
 * \sa p_term_create_class_object(), p_term_add_property()
 * \sa p_term_is_instance_object()
 */
p_term *p_term_create_object(p_context *context, p_term *prototype)
{
    struct p_term_object *term;
    if (!prototype)
        return 0;
    prototype = p_term_deref_non_null(prototype);
    if (prototype->header.type != P_TERM_OBJECT)
        return 0;
    term = p_term_new(context, struct p_term_object);
    if (!term)
        return 0;
    term->header.type = P_TERM_OBJECT;
    term->header.size = 1;
    term->properties[0].name = context->prototype_atom;
    term->properties[0].value = prototype;
    return (p_term *)term;
}

/**
 * \brief Creates a class object term within \a context for the
 * class called \a name with the specified base class \a prototype.
 *
 * Returns the class object term.  The \a class_name must be an atom,
 * and the \a prototype must be an object or null.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_add_property()
 * \sa p_term_is_class_object()
 */
p_term *p_term_create_class_object(p_context *context, p_term *class_name, p_term *prototype)
{
    struct p_term_object *term;
    if (!class_name)
        return 0;
    class_name = p_term_deref_non_null(class_name);
    if (class_name->header.type != P_TERM_ATOM)
        return 0;
    if (prototype) {
        prototype = p_term_deref_non_null(prototype);
        if (prototype->header.type != P_TERM_OBJECT)
            return 0;
    }
    term = p_term_new(context, struct p_term_object);
    if (!term)
        return 0;
    term->header.type = P_TERM_OBJECT;
    if (prototype) {
        /* The prototype should be the first property if present */
        term->properties[0].name = context->prototype_atom;
        term->properties[0].value = prototype;
        term->properties[1].name = context->class_name_atom;
        term->properties[1].value = class_name;
        term->header.size = 2;
    } else {
        term->properties[0].name = context->class_name_atom;
        term->properties[0].value = class_name;
        term->header.size = 1;
    }
    return (p_term *)term;
}

/**
 * \brief Adds \a name and \a value as a property to \a term
 * within \a context.
 *
 * Returns non-zero if the property was added, or zero if
 * the parameters are invalid.  The \a term must be an object,
 * and \a name must be an atom other than "prototype" or
 * "className".
 *
 * This function does not check if \a name is already a property
 * on the \a term, so it should only be used to add new properties.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_property()
 */
int p_term_add_property(p_context *context, p_term *term, p_term *name, p_term *value)
{
    /* Validate the parameters */
    if (!term || !name)
        return 0;
    term = p_term_deref_non_null(term);
    if (term->header.type != P_TERM_OBJECT)
        return 0;
    name = p_term_deref_non_null(name);
    if (name->header.type != P_TERM_ATOM)
        return 0;
    if (name == context->prototype_atom ||
            name == context->class_name_atom)
        return 0;

    /* Find an object block with spare capacity to add the property */
    while (term->header.size >= P_TERM_MAX_PROPS && term->object.next)
        term = term->object.next;
    if (term->header.size >= P_TERM_MAX_PROPS) {
        /* Add a new extension block */
        p_term *block = p_term_malloc
            (context, p_term, sizeof(struct p_term_object));
        if (!block)
            return 0;
        block->header.type = P_TERM_OBJECT;
        term->object.next = block;
        term = block;
    }

    /* Add the new property */
    term->object.properties[term->header.size].name = name;
    term->object.properties[term->header.size].value = value;
    ++(term->header.size);
    return 1;
}

/**
 * \brief Returns the value associated with the property \a name
 * on \a term within \a context, or null if \a name is not present.
 *
 * If the property \a name is not present on \a term, then the
 * prototype object of \a term will be consulted for the property.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_add_property()
 * \sa p_term_own_property()
 */
p_term *p_term_property(p_context *context, const p_term *term, const p_term *name)
{
    const p_term *block;
    unsigned int index;

    /* Validate the parameters */
    if (!term || !name)
        return 0;
    term = p_term_deref_non_null(term);
    if (term->header.type != P_TERM_OBJECT)
        return 0;
    name = p_term_deref_non_null(name);
    if (name->header.type != P_TERM_ATOM)
        return 0;

    for (;;) {
        /* Locate the property in the object */
        block = term;
        do {
            for (index = 0; index < block->header.size; ++index) {
                if (block->object.properties[index].name == name)
                    return block->object.properties[index].value;
            }
            block = block->object.next;
        } while (block != 0);

        /* Try the prototype object instead */
        if (term->object.properties[0].name != context->prototype_atom)
            break;
        term = term->object.properties[0].value;
    }
    return 0;
}

/**
 * \brief Returns the value associated with the property \a name
 * on \a term within \a context, or null if \a name is not present.
 *
 * This function does not search prototype objects for \a name.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_add_property()
 * \sa p_term_property()
 */
p_term *p_term_own_property(p_context *context, const p_term *term, const p_term *name)
{
    unsigned int index;

    /* Validate the parameters */
    if (!term || !name)
        return 0;
    term = p_term_deref_non_null(term);
    if (term->header.type != P_TERM_OBJECT)
        return 0;
    name = p_term_deref_non_null(name);
    if (name->header.type != P_TERM_ATOM)
        return 0;

    /* Locate the property in the object */
    do {
        for (index = 0; index < term->header.size; ++index) {
            if (term->object.properties[index].name == name)
                return term->object.properties[index].value;
        }
        term = term->object.next;
    } while (term != 0);
    return 0;
}

/**
 * \brief Returns non-zero if \a term is an object within \a context
 * and \a term is not a class object, zero otherwise.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_is_class_object()
 * \sa p_term_inherits()
 */
int p_term_is_instance_object(p_context *context, const p_term *term)
{
    p_term *name;
    if (!term)
        return 0;
    if (term->header.type != P_TERM_OBJECT) {
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_OBJECT)
            return 0;
    }
    name = context->class_name_atom;
    return term->object.properties[0].name != name &&
           term->object.properties[1].name != name;
}

/**
 * \brief Returns non-zero if \a term is a class object within
 * \a context, zero otherwise.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_inherits()
 * \sa p_term_is_instance_object()
 */
int p_term_is_class_object(p_context *context, const p_term *term)
{
    p_term *name;
    if (!term)
        return 0;
    if (term->header.type != P_TERM_OBJECT) {
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_OBJECT)
            return 0;
    }
    name = context->class_name_atom;
    return term->object.properties[0].name == name ||
           term->object.properties[1].name == name;
}

/**
 * \brief Returns non-zero if \a term1 inherits from \a term2
 * within \a context, zero otherwise.
 *
 * The terms are automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_is_class_object(), p_term_is_instance_of()
 */
int p_term_inherits(p_context *context, const p_term *term1, const p_term *term2)
{
    p_term *pname = context->prototype_atom;
    if (!term1 || !term2)
        return 0;
    term2 = p_term_deref_non_null(term2);
    if (term2->header.type != P_TERM_OBJECT)
        return 0;
    do {
        term1 = p_term_deref_non_null(term1);
        if (term1 == term2)
            return 1;
        if (term1->header.type != P_TERM_OBJECT)
            break;
        if (term1->object.properties[0].name != pname)
            break;
        term1 = term1->object.properties[0].value;
    } while (term1 != 0);
    return 0;
}

/**
 * \brief Returns non-zero if \a term1 is an instance of \a term2,
 * zero otherwise.
 *
 * The terms are automatically dereferenced.  The \a term1 must be
 * an instance object, and \a term2 must be a class object.
 *
 * \ingroup term
 * \sa p_term_is_class_object(), p_term_is_instance_of()
 */
int p_term_is_instance_of(p_context *context, const p_term *term1, const p_term *term2)
{
    if (!p_term_is_instance_object(context, term1))
        return 0;
    if (!p_term_is_class_object(context, term2))
        return 0;
    return p_term_inherits(context, term1, term2);
}

/* Perform an occurs check */
static int p_term_occurs_in(const p_term *var, const p_term *value)
{
    unsigned int index;
    if (!value)
        return 0;
    value = p_term_deref_non_null(value);
    if (var == value)
        return 1;
    switch (value->header.type) {
    case P_TERM_FUNCTOR:
        /* Scan the functor arguments */
        for (index = 0; index < value->header.size; ++index) {
            if (p_term_occurs_in(var, value->functor.arg[index]))
                return 1;
        }
        break;
    case P_TERM_LIST:
        /* Try to reduce depth of recursion issues with long lists */
        do {
            if (p_term_occurs_in(var, value->list.head))
                return 1;
            value = value->list.tail;
            if (!value)
                return 0;
            value = p_term_deref_non_null(value);
        } while (value->header.type == P_TERM_LIST);
        if (value->header.type != P_TERM_ATOM)
            return p_term_occurs_in(var, value);
        break;
    case P_TERM_OBJECT:
        /* Scan the object's property values */
        do {
            for (index = 0; index < value->header.size; ++index) {
                if (p_term_occurs_in
                        (var, value->object.properties[index].value))
                    return 1;
            }
            value = value->object.next;
        }
        while (value);
        break;
    case P_TERM_MEMBER_VARIABLE:
        return p_term_occurs_in(var, value->member_var.object);
    default: break;
    }
    return 0;
}

/**
 * \var P_BIND_DEFAULT
 * \ingroup term
 * Default variable binding flags, which performs occurs checking
 * and recording of bindings for back-tracking.
 */

/**
 * \var P_BIND_NO_OCCURS_CHECK
 * \ingroup term
 * Flag indicating that the occurs check should not be performed.
 * This flag should be provided only when the caller is absolutely
 * certain that the variable will not occur in the term it is being
 * bound to; e.g. because it is a new variable that was created
 * after the term.
 */

/**
 * \var P_BIND_NO_RECORD
 * \ingroup term
 * Do not record the binding to be undone during back-tracking.
 */

/**
 * \var P_BIND_RECORD_ONE_WAY
 * \ingroup term
 * Variables bindings from the first term to the second are not
 * recorded to be undone during back-tracking, but variable bindings
 * from the second term to the first are.
 *
 * This is typically used when the first term is a newly formed
 * predicate instance whose free variables will be discarded
 * upon back-tracking without assistance from the trace.
 */

/**
 * \var P_BIND_EQUALITY
 * \ingroup term
 * Check the terms for equality but do not perform destructive
 * unification.
 */

/**
 * \brief Binds the variable \a var to \a value.
 *
 * Returns non-zero if the bind was successful, or zero if \a var
 * is already bound, if \a var is not a variable, or if creating
 * the binding would introduce a circularity into \a value.
 *
 * If \a flags does not contain P_BIND_NO_OCCURS_CHECK, then the
 * bind will fail if \a var occurs within \a value as performing
 * the binding will create a circularity.
 *
 * If \a flags does not contain P_BIND_NO_RECORD, then the binding
 * will be recorded in \a context for back-tracking purposes.
 *
 * \ingroup term
 * \sa p_term_create_variable(), p_term_unify()
 */
int p_term_bind_variable(p_context *context, p_term *var, p_term *value, int flags)
{
    if (!var)
        return 0;
    var = p_term_deref_non_null(var);
    if ((var->header.type & P_TERM_VARIABLE) == 0)
        return 0;
    if ((flags & P_BIND_NO_OCCURS_CHECK) == 0) {
        if (p_term_occurs_in(var, value))
            return 0;
    }
    if ((flags & P_BIND_NO_RECORD) == 0) {
        if (!_p_context_record_in_trace(context, var))
            return 0;
    }
    var->var.value = value;
    return 1;
}

/* Internal variable binding where "var" is known to be unbound */
P_INLINE int p_term_bind_var(p_context *context, p_term *var, p_term *value, int flags)
{
    if ((flags & P_BIND_NO_OCCURS_CHECK) == 0) {
        if (p_term_occurs_in(var, value))
            return 0;
    }
    if ((flags & P_BIND_NO_RECORD) == 0) {
        if (!_p_context_record_in_trace(context, var))
            return 0;
    }
    var->var.value = value;
    return 1;
}

/* Forward declaration */
static int p_term_unify_inner(p_context *context, p_term *term1, p_term *term2, int flags);

/* Resolve a member variable reference */
static p_term *p_term_resolve_member(p_context *context, p_term *term, int flags)
{
    p_term *object = term->member_var.object;
    p_term *value;
    if (!object)
        return 0;
    object = p_term_deref_non_null(object);
    if (object->header.type == P_TERM_MEMBER_VARIABLE) {
        /* Resolve a nested member reference */
        object = p_term_resolve_member(context, object, flags);
        if (!object)
            return 0;
        object = p_term_deref_non_null(object);
    } else if (object->header.type == P_TERM_ATOM) {
        /* Look up a global object reference */
        object = p_db_global_object(context, object);
        if (!object)
            return 0;
        object = p_term_deref_non_null(object);
    }
    if (object->header.type != P_TERM_OBJECT)
        return 0;
    value = p_term_property(context, object, term->member_var.name);
    if (!value && term->header.size && (flags & P_BIND_EQUALITY) == 0) {
        /* Add a new property to the object */
        value = p_term_create_variable(context);
        if (!p_term_add_property(context, object,
                                 term->member_var.name, value))
            return 0;
    }
    return value;
}

/* Unify an unbound variable against a term */
static int p_term_unify_variable(p_context *context, p_term *term1, p_term *term2, int flags)
{
    /* Resolve member variable references */
    if (term1->header.type == P_TERM_MEMBER_VARIABLE) {
        term1 = p_term_resolve_member(context, term1, flags);
        return p_term_unify_inner(context, term1, term2, flags);
    }
    if (term2->header.type == P_TERM_MEMBER_VARIABLE) {
        term2 = p_term_resolve_member(context, term2, flags);
        return p_term_unify_inner(context, term1, term2, flags);
    }

    /* Bail out if unification is supposed to be non-destructive */
    if (flags & P_BIND_EQUALITY)
        return 0;

    /* Bind the variable and return */
    if (flags & P_BIND_RECORD_ONE_WAY)
        flags |= P_BIND_NO_RECORD;
    return p_term_bind_var(context, term1, term2, flags);
}

/* Inner implementation of unification */
static int p_term_unify_inner(p_context *context, p_term *term1, p_term *term2, int flags)
{
    if (!term1 || !term2)
        return 0;
    term1 = p_term_deref_non_null(term1);
    term2 = p_term_deref_non_null(term2);
    if (term1 == term2)
        return 1;
    if (term1->header.type & P_TERM_VARIABLE)
        return p_term_unify_variable(context, term1, term2, flags);
    if (term2->header.type & P_TERM_VARIABLE) {
        return p_term_unify_variable
            (context, term2, term1, flags & ~P_BIND_RECORD_ONE_WAY);
    }
    switch (term1->header.type) {
    case P_TERM_FUNCTOR:
        /* Functor must have the same name and number of arguments */
        if (term2->header.type == P_TERM_FUNCTOR &&
                term1->header.size == term2->header.size &&
                term1->functor.functor_name ==
                        term2->functor.functor_name) {
            unsigned int index;
            for (index = 0; index < term1->header.size; ++index) {
                if (!p_term_unify_inner
                        (context, term1->functor.arg[index],
                         term2->functor.arg[index], flags))
                    return 0;
            }
            return 1;
        }
        break;
    case P_TERM_LIST:
        /* Unify the head and tail components of the list,
         * while trying to reduce recursion depth */
        if (term2->header.type != P_TERM_LIST)
            return 0;
        for (;;) {
            if (!p_term_unify_inner(context, term1->list.head,
                                    term2->list.head, flags))
                break;
            term1 = term1->list.tail;
            term2 = term2->list.tail;
            if (!term1 || !term2)
                break;
            term1 = p_term_deref_non_null(term1);
            term2 = p_term_deref_non_null(term2);
            if (term1->header.type != P_TERM_LIST ||
                    term2->header.type != P_TERM_LIST)
                return p_term_unify_inner(context, term1, term2, flags);
        }
        break;
    case P_TERM_ATOM:
        /* Atoms can unify only if their pointers are identical.
         * Identity has already been checked, so fail */
        break;
    case P_TERM_STRING:
        /* Compare two strings */
        if (term2->header.type == P_TERM_STRING &&
                term1->header.size == term2->header.size &&
                !memcmp(term1->string.name, term2->string.name,
                        term1->header.size))
            return 1;
        break;
    case P_TERM_INTEGER:
        /* Compare two integers */
        if (term2->header.type == P_TERM_INTEGER) {
#if defined(P_TERM_64BIT)
            /* Value is packed into the header, to save memory */
            return term1->header.size == term2->header.size;
#else
            return term1->integer.value == term2->integer.value;
#endif
        }
        break;
    case P_TERM_REAL:
        /* Compare two reals */
        if (term2->header.type == P_TERM_REAL)
            return term1->real.value == term2->real.value;
        break;
    case P_TERM_OBJECT:
        /* Objects can unify only if their pointers are identical.
         * Identity has already been checked, so fail */
        break;
    default: break;
    }
    return 0;
}

/**
 * \brief Unifies \a term1 with \a term2 within \a context.
 *
 * Returns non-zero if the unify was successful, or zero on failure.
 * The \a flags control how variables are bound.
 *
 * \ingroup term
 * \sa p_term_bind_variable(), p_term_clone()
 */
int p_term_unify(p_context *context, p_term *term1, p_term *term2, int flags)
{
    void *marker = p_context_mark_trace(context);
    int result = p_term_unify_inner(context, term1, term2, flags);
    if (!result && (flags & P_BIND_NO_RECORD) == 0)
        p_context_backtrack_trace(context, marker);
    return result;
}

/**
 * \typedef p_term_print_func
 * \ingroup term
 *
 * The p_term_print_func function pointer type is used by
 * p_term_print() to output printf-style formatted data
 * to an output stream.
 *
 * \sa p_term_print(), p_term_stdio_print_func()
 */

/**
 * \brief Prints formatted output according to \a format
 * to the stdio FILE stream \a data.
 *
 * \ingroup term
 * \sa p_term_print(), p_term_print_func
 */
void p_term_stdio_print_func(void *data, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    vfprintf((FILE *)data, format, va);
    va_end(va);
}

/* Limited dereference that avoids recursing too far */
static const p_term *p_term_deref_limited(const p_term *term)
{
    int count = 32;
    if (!term)
        return 0;
    while (count-- > 0) {
        if (term->header.type & P_TERM_VARIABLE) {
            if (!term->var.value)
                break;
            term = term->var.value;
        } else {
            break;
        }
    }
    return term;
}

/* Print an atom name */
static void p_term_print_atom(const p_term *atom, p_term_print_func print_func, void *print_data)
{
    (*print_func)(print_data, "%s", p_term_name(atom));
}

static void p_term_print_inner(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data, int level, int prec)
{
    /* Bail out if we have exceeded the maximum recursion depth */
    if (level <= 0) {
        (*print_func)(print_data, "...");
        return;
    }

    /* Bail out if the term is invalid */
    if (!term) {
        (*print_func)(print_data, "NULL");
        return;
    }

    /* Determine how to print this type of term */
    switch (term->header.type) {
    case P_TERM_FUNCTOR: {
        unsigned int index;
        p_op_specifier spec;
        int priority;
        spec = p_db_operator_info
            (term->functor.functor_name,
             (int)(term->header.size), &priority);
        if (spec == P_OP_NONE) {
            p_term_print_atom(term->functor.functor_name,
                              print_func, print_data);
            (*print_func)(print_data, "(");
            for (index = 0; index < term->header.size; ++index) {
                if (index)
                    (*print_func)(print_data, ", ");
                p_term_print_inner
                    (context, term->functor.arg[index],
                     print_func, print_data, level - 1, 950);
            }
            (*print_func)(print_data, ")");
        } else {
            int bracketed = (priority > prec);
            if (bracketed) {
                (*print_func)(print_data, "(");
                priority = 1300;
            }
            switch (spec) {
            case P_OP_NONE: break;
            case P_OP_XF:
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority - 1);
                (*print_func)(print_data, " %s",
                              p_term_name(term->functor.functor_name));
                break;
            case P_OP_YF:
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority);
                (*print_func)(print_data, " %s",
                              p_term_name(term->functor.functor_name));
                break;
            case P_OP_XFX:
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority - 1);
                (*print_func)(print_data, " %s ",
                              p_term_name(term->functor.functor_name));
                p_term_print_inner
                    (context, term->functor.arg[1],
                     print_func, print_data, level - 1, priority - 1);
                break;
            case P_OP_XFY:
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority - 1);
                (*print_func)(print_data, " %s ",
                              p_term_name(term->functor.functor_name));
                p_term_print_inner
                    (context, term->functor.arg[1],
                     print_func, print_data, level - 1, priority);
                break;
            case P_OP_YFX:
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority);
                (*print_func)(print_data, " %s ",
                              p_term_name(term->functor.functor_name));
                p_term_print_inner
                    (context, term->functor.arg[1],
                     print_func, print_data, level - 1, priority - 1);
                break;
            case P_OP_FX:
                (*print_func)(print_data, "%s ",
                              p_term_name(term->functor.functor_name));
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority - 1);
                break;
            case P_OP_FY:
                (*print_func)(print_data, "%s ",
                              p_term_name(term->functor.functor_name));
                p_term_print_inner
                    (context, term->functor.arg[0],
                     print_func, print_data, level - 1, priority);
                break;
            }
            if (bracketed)
                (*print_func)(print_data, ")");
        }
        break; }
    case P_TERM_LIST:
        (*print_func)(print_data, "[");
        p_term_print_inner(context, term->list.head,
                           print_func, print_data, level - 1, 950);
        term = p_term_deref_limited(term->list.tail);
        while (term && term->header.type == P_TERM_LIST && level > 0) {
            (*print_func)(print_data, ", ");
            p_term_print_inner(context, term->list.head,
                               print_func, print_data, level - 1, 950);
            term = p_term_deref_limited(term->list.tail);
            --level;
        }
        if (level <= 0) {
            (*print_func)(print_data, "|...]");
            break;
        }
        if (term != context->nil_atom) {
            (*print_func)(print_data, "|");
            p_term_print_inner(context, term, print_func,
                               print_data, level - 1, 950);
        }
        (*print_func)(print_data, "]");
        break;
    case P_TERM_ATOM:
        p_term_print_atom(term, print_func, print_data);
        break;
    case P_TERM_STRING:
        (*print_func)(print_data, "\"%s\"", p_term_name(term));
        break;
    case P_TERM_INTEGER:
        (*print_func)(print_data, "%d", p_term_integer_value(term));
        break;
    case P_TERM_REAL:
        (*print_func)(print_data, "%g", p_term_real_value(term));
        break;
    case P_TERM_OBJECT: {
        p_term *name = p_term_property
            (context, term, context->class_name_atom);
        unsigned int index;
        int first = 1;
        if (p_term_is_class_object(context, term))
            (*print_func)(print_data, "class ");
        if (name)
            (*print_func)(print_data, "%s {", p_term_name(name));
        else
            (*print_func)(print_data, "unknown_class {");
        do {
            for (index = 0; index < term->header.size; ++index) {
                name = term->object.properties[index].name;
                if (name == context->class_name_atom)
                    continue;
                if (name == context->prototype_atom)
                    continue;
                if (!first)
                    (*print_func)(print_data, ", ");
                p_term_print_atom(name, print_func, print_data);
                (*print_func)(print_data, ": ");
                p_term_print_inner
                    (context, term->object.properties[index].value,
                     print_func, print_data, level - 1, 950);
                first = 0;
            }
            term = term->object.next;
        } while (term != 0);
        (*print_func)(print_data, "}");
        break; }
    case P_TERM_VARIABLE:
        if (term->var.value) {
            p_term_print_inner(context, term->var.value, print_func,
                               print_data, level - 1, prec);
        } else if (term->header.size > 0) {
            (*print_func)(print_data, "%s", p_term_name(term));
        } else {
            (*print_func)(print_data, "_%lx", (long)term);
        }
        break;
    case P_TERM_MEMBER_VARIABLE:
        if (term->var.value) {
            p_term_print_inner(context, term->var.value, print_func,
                               print_data, level - 1, prec);
            break;
        }
        p_term_print_inner(context, term->member_var.object, print_func,
                           print_data, level - 1, 0);
        (*print_func)(print_data, ".");
        p_term_print_atom(term->member_var.name, print_func, print_data);
        break;
    default: break;
    }
}

/**
 * \brief Prints \a term within \a context to the output stream
 * defined by \a print_func and \a print_data.
 *
 * This function is intended for debugging purposes.  It may refuse
 * to print some parts of \a term if the recursion depth is too high.
 *
 * \ingroup term
 * \sa p_term_stdio_print_func(), p_term_print_func
 * \sa p_term_print_unquoted()
 */
void p_term_print(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data)
{
    p_term_print_inner(context, term, print_func, print_data, 1000, 1300);
}

/**
 * \brief Prints \a term within \a context to the output stream
 * defined by \a print_func and \a print_data.
 *
 * If \a term is an atom or string, then it will be printed
 * without quoting.  Otherwise, the behavior is the same as
 * p_term_print().
 *
 * This function is intended for debugging purposes.  It may refuse
 * to print some parts of \a term if the recursion depth is too high.
 *
 * \ingroup term
 * \sa p_term_print()
 */
void p_term_print_unquoted(p_context *context, const p_term *term, p_term_print_func print_func, void *print_data)
{
    term = p_term_deref(term);
    if (term) {
        if (term->header.type == P_TERM_ATOM ||
                term->header.type == P_TERM_STRING) {
            (*print_func)(print_data, "%s", p_term_name(term));
            return;
        }
    }
    p_term_print_inner(context, term, print_func, print_data, 1000, 1300);
}

/**
 * \brief Returns -1, 0, or 1 depending upon whether \a term1 is
 * less than, equal to, or greater than \a term2 using the
 * "precedes" relationship.  The terms are compared within \a context.
 *
 * Variables precede all floating-point reals, which precede
 * all integers, which precede all strings, which precede all
 * atoms, which precede all functors (including lists), which
 * precede all objects.
 *
 * Variables and objects are compared by pointer.  Reals, integers,
 * strings, and atoms are compared by value.  Functors order on
 * arity, then name, and then the arguments from left-to-right.
 * Lists are assumed to have arity 2 and "." as their functor name.
 *
 * \ingroup term
 */
int p_term_precedes(p_context *context, const p_term *term1, const p_term *term2)
{
    int group1, group2, cmp;
    static unsigned char const precedes_ordering[] = {
        0, /* 0: P_TERM_INVALID */
        6, /* 1: P_TERM_FUNCTOR */
        6, /* 2: P_TERM_LIST */
        5, /* 3: P_TERM_ATOM */
        4, /* 4: P_TERM_STRING */
        3, /* 5: P_TERM_INTEGER */
        2, /* 6: P_TERM_REAL */
        7, /* 7: P_TERM_OBJECT */
        0, 0, 0, 0, 0, 0, 0, 0,
        1, /* 16: P_TERM_VARIABLE */
        1  /* 17: P_TERM_MEMBER_VARIABLE */
    };

    /* Dereference the terms */
    if (!term1)
        return term2 ? -1 : 0;
    if (!term2)
        return 1;
    term1 = p_term_deref_non_null(term1);
    term2 = p_term_deref_non_null(term2);
    if (term1 == term2)
        return 0;

    /* Determine which groups the terms fall within */
    group1 = precedes_ordering[term1->header.type];
    group2 = precedes_ordering[term2->header.type];
    if (group1 < group2)
        return -1;
    else if (group1 > group2)
        return 1;

    /* Compare based on the term type */
    switch (term1->header.type) {
    case P_TERM_FUNCTOR:
    case P_TERM_LIST: {
        p_term *name1;
        p_term *name2;
        unsigned int index;
        if (term1->header.size < term2->header.size)
            return -1;
        else if (term1->header.size > term2->header.size)
            return 1;
        if (term1->header.type == P_TERM_FUNCTOR)
            name1 = term1->functor.functor_name;
        else
            name1 = context->dot_atom;
        if (term2->header.type == P_TERM_FUNCTOR)
            name2 = term2->functor.functor_name;
        else
            name2 = context->dot_atom;
        cmp = p_term_strcmp(name1, name2);
        if (cmp < 0)
            return -1;
        else if (cmp > 0)
            return 1;
        if (term1->header.type == P_TERM_FUNCTOR &&
                term2->header.type == P_TERM_FUNCTOR) {
            for (index = 0; index < term1->header.size; ++index) {
                cmp = p_term_precedes(context,
                                      term1->functor.arg[index],
                                      term2->functor.arg[index]);
                if (cmp != 0)
                    return cmp;
            }
        } else if (term1->header.type == P_TERM_LIST &&
                   term2->header.type == P_TERM_LIST) {
            do {
                cmp = p_term_precedes
                    (context, term1->list.head, term2->list.head);
                if (cmp != 0)
                    return cmp;
                term1 = term1->list.tail;
                term2 = term2->list.tail;
                if (!term1 || !term2)
                    break;
                term1 = p_term_deref_non_null(term1);
                term2 = p_term_deref_non_null(term2);
            } while (term1->header.type == P_TERM_LIST &&
                     term2->header.type == P_TERM_LIST);
            return p_term_precedes(context, term1, term2);
        } else {
            /* Shouldn't get here, because parsers will normally
             * convert '.' functors into list terms.  Have to do
             * something, so order the terms on pointer */
            return (term1 < term2) ? -1 : 1;
        }
        break; }
    case P_TERM_ATOM:
    case P_TERM_STRING:
        return p_term_strcmp(term1, term2);
    case P_TERM_INTEGER:
#if defined(P_TERM_64BIT)
        if (((int)(term1->header.size)) < ((int)(term2->header.size)))
            return -1;
        else if (((int)(term1->header.size)) > ((int)(term2->header.size)))
            return 1;
#else
        if (term1->integer.value < term2->integer.value)
            return -1;
        else if (term1->integer.value > term2->integer.value)
            return 1;
#endif
        break;
    case P_TERM_REAL:
        if (term1->real.value < term2->real.value)
            return -1;
        else if (term1->real.value > term2->real.value)
            return 1;
        break;
    case P_TERM_OBJECT:
    case P_TERM_VARIABLE:
    case P_TERM_MEMBER_VARIABLE:
        return (term1 < term2) ? -1 : 1;
    default: break;
    }
    return 0;
}

/**
 * \brief Returns non-zero if \a term is a ground term without
 * any unbound variables; zero otherwise.
 *
 * Note: objects are considered ground terms, even if their
 * properties contain unbound variables.
 *
 * \ingroup term
 */
int p_term_is_ground(const p_term *term)
{
    if (!term)
        return 0;
    term = p_term_deref_non_null(term);
    switch (term->header.type) {
    case P_TERM_FUNCTOR: {
        unsigned int index;
        for (index = 0; index < term->header.size; ++index) {
            if (!p_term_is_ground(term->functor.arg[index]))
                return 0;
        }
        return 1; }
    case P_TERM_LIST:
        do {
            if (!p_term_is_ground(term->list.head))
                return 0;
            term = term->list.tail;
            if (!term)
                return 0;
            term = p_term_deref_non_null(term);
        }
        while (term->header.type == P_TERM_LIST);
        return p_term_is_ground(term);
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
        return 1;
    case P_TERM_VARIABLE:
    case P_TERM_MEMBER_VARIABLE:
    case P_TERM_RENAME:
        return 0;
    default: break;
    }
    return 0;
}

static p_term *p_term_clone_inner(p_context *context, p_term *term)
{
    p_term *clone;
    p_term *rename;
    if (!term)
        return 0;
    term = p_term_deref_non_null(term);
    switch (term->header.type) {
    case P_TERM_FUNCTOR: {
        /* Clone a functor term */
        unsigned int index;
        clone = p_term_create_functor
            (context, term->functor.functor_name,
             (int)(term->header.size));
        if (!clone)
            return 0;
        for (index = 0; index < term->header.size; ++index) {
            p_term *arg = p_term_clone_inner
                (context, term->functor.arg[index]);
            if (!arg)
                return 0;
            p_term_bind_functor_arg(clone, (int)index, arg);
        }
        return clone; }
    case P_TERM_LIST: {
        /* Clone a list term */
        p_term *head;
        p_term *tail = 0;
        clone = 0;
        do {
            head = p_term_clone_inner(context, term->list.head);
            if (!head)
                return 0;
            head = p_term_create_list(context, head, 0);
            if (tail)
                tail->list.tail = head;
            else
                clone = head;
            tail = head;
            term = term->list.tail;
            if (!term)
                return 0;
            term = p_term_deref_non_null(term);
        }
        while (term->header.type == P_TERM_LIST);
        head = p_term_clone_inner(context, term);
        if (!head)
            return 0;
        tail->list.tail = head;
        return clone; }
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
        /* Constant and object terms are cloned as themselves */
        break;
    case P_TERM_VARIABLE:
        /* Create a new variable and bind the current one
         * to a rename term so that future references can
         * quickly reuse the cloned variable */
        if (term->header.size > 0)
            clone = p_term_create_named_variable(context, p_term_name(term));
        else
            clone = p_term_create_variable(context);
        if (!clone)
            return 0;
        _p_context_record_in_trace(context, term);
        rename = p_term_malloc
            (context, p_term, sizeof(struct p_term_rename));
        if (!rename)
            return 0;
        rename->header.type = P_TERM_RENAME;
        rename->rename.var = clone;
        term->var.value = rename;
        return clone;
    case P_TERM_MEMBER_VARIABLE:
        /* Clone a member variable reference */
        clone = p_term_clone_inner(context, term->member_var.object);
        if (!clone)
            return 0;
        clone = p_term_create_member_variable
            (context, clone, term->member_var.name,
             (int)(term->header.size));
        if (!clone)
            return 0;
        _p_context_record_in_trace(context, term);
        rename = p_term_malloc
            (context, p_term, sizeof(struct p_term_rename));
        if (!rename)
            return 0;
        rename->header.type = P_TERM_RENAME;
        rename->rename.var = clone;
        term->var.value = rename;
        return clone;
    case P_TERM_RENAME:
        /* Variable that has already been renamed */
        return term->rename.var;
    default: break;
    }
    return term;
}

/**
 * \brief Clones \a term within \a context to create a new
 * term that has freshly renamed versions of the variables
 * within \a term.
 *
 * \ingroup term
 * \sa p_term_unify(), p_term_unify_clause()
 */
p_term *p_term_clone(p_context *context, p_term *term)
{
    /* We use the trace to record temporary bindings of variables
     * to P_TERM_RENAME terms, and then back them out at the end.
     * This won't be safe in concurrent environments, so we will
     * need to come up with a better solution later */
    void *marker = p_context_mark_trace(context);
    p_term *clone = p_term_clone_inner(context, term);
    p_context_backtrack_trace(context, marker);
    return clone;
}

/**
 * \brief Unifies \a term with the renamed head of \a clause.
 *
 * If the unification succeeds, then this function returns the
 * renamed body of \a clause.  Returns null if the unification fails.
 *
 * The return value will be the atom \c true if \a clause
 * does not have a body and \a clause unifies with \a term.
 *
 * \ingroup term
 * \sa p_term_unify(), p_term_clone()
 */
p_term *p_term_unify_clause(p_context *context, p_term *term, p_term *clause)
{
    /* This implementation is currently inefficient.  We should change
     * this to an alternative that only clones the body if the head
     * could be successfully unified with the term */
    p_term *clone = p_term_clone(context, clause);
    if (!clone)
        return 0;
    if (clone->header.type == P_TERM_FUNCTOR &&
            clone->header.size == 2 &&
            clone->functor.functor_name == context->clause_atom) {
        if (p_term_unify(context, term, clone->functor.arg[0],
                         P_BIND_DEFAULT))
            return clone->functor.arg[1];
    } else {
        if (p_term_unify(context, term, clone, P_BIND_DEFAULT))
            return context->true_atom;
    }
    return 0;
}

/**
 * \brief Compares \a str1 and \a str2 and returns a comparison code.
 *
 * The \a str1 and \a str2 terms are assumed to be either atoms
 * or strings.  The result is undefined if this assumption does
 * not hold.
 * 
 * This comparison function is safe on strings that have embedded NUL
 * characters as it uses memcmp() internally.
 *
 * \ingroup term
 * \sa p_term_create_string()
 */
int p_term_strcmp(const p_term *str1, const p_term *str2)
{
    const char *s1;
    unsigned int s1len;
    const char *s2;
    unsigned int s2len;
    int cmp;
    if (!str1 || !str2)
        return 0;
    str1 = p_term_deref_non_null(str1);
    str2 = p_term_deref_non_null(str2);
    if (str1->header.type == P_TERM_ATOM) {
        s1 = str1->atom.name;
        s1len = str1->header.size;
    } else if (str1->header.type == P_TERM_STRING) {
        s1 = str1->string.name;
        s1len = str1->header.size;
    } else {
        return 0;
    }
    if (str2->header.type == P_TERM_ATOM) {
        s2 = str2->atom.name;
        s2len = str2->header.size;
    } else if (str2->header.type == P_TERM_STRING) {
        s2 = str2->string.name;
        s2len = str2->header.size;
    } else {
        return 0;
    }
    if (!s1len)
        return s2len ? -1 : 0;
    if (!s2len)
        return s1len ? 1 : 0;
    if (s1len == s2len) {
        return memcmp(s1, s2, s1len);
    } else if (s1len < s2len) {
        cmp = memcmp(s1, s2, s1len);
        if (cmp != 0)
            return cmp;
        else
            return -1;
    } else {
        cmp = memcmp(s1, s2, s2len);
        if (cmp != 0)
            return cmp;
        else
            return 1;
    }
}

/**
 * \brief Concatenates \a str1 and \a str2 to create a new string.
 *
 * Returns the concatenated string, or null if \a str1 or \a str2
 * is not a string.
 *
 * \ingroup term
 * \sa p_term_create_string()
 */
p_term *p_term_concat_string(p_context *context, p_term *str1, p_term *str2)
{
    size_t len;
    struct p_term_string *term;
    str1 = p_term_deref(str1);
    str2 = p_term_deref(str2);
    if (!str1 || str1->header.type != P_TERM_STRING)
        return 0;
    if (!str2 || str2->header.type != P_TERM_STRING)
        return 0;
    if (str1->header.size == 0)
        return str2;
    if (str2->header.size == 0)
        return str1;
    len = str1->header.size + str2->header.size;
    term = p_term_malloc
        (context, struct p_term_string, sizeof(struct p_term_string) + len);
    if (!term)
        return 0;
    term->header.type = P_TERM_STRING;
    term->header.size = (unsigned int)len;
    memcpy(term->name, str1->string.name, str1->header.size);
    memcpy(term->name + str1->header.size, str2->string.name, str2->header.size);
    return (p_term *)term;
}

/*\@}*/
