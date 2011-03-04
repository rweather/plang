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
#include "term-priv.h"
#include "context-priv.h"
#include <string.h>

#if defined(__cplusplus)
#define P_INLINE inline
#elif defined(__GNUC_GNU_INLINE__) && !defined(__NO_INLINE__)
#define P_INLINE extern __inline__
#else
#define P_INLINE static
#endif

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
 * \var P_TERM_TYPED_VARIABLE
 * \ingroup term
 * The term is an unbound variable that has type restrictions on
 * what kind of terms may be bound to it.
 * \sa p_term_create_typed_variable()
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
 * \sa p_term_head(), p_term_tail()
 */
p_term *p_term_create_list(p_context *context, p_term *head, p_term *tail)
{
    struct p_term_list *term = p_term_new(context, struct p_term_list);
    if (!term)
        return 0;
    term->header.type = P_TERM_LIST;
    term->head = head;
    term->tail = tail;
    return (p_term *)term;
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
 * The \a name should be encoded in the UTF-8 character set with
 * embedded NUL characters encoded as the sequence 0xC0 0x80.
 *
 * \ingroup term
 * \sa p_term_nil_atom(), p_term_create_string()
 */
p_term *p_term_create_atom(p_context *context, const char *name)
{
    unsigned int hash;
    size_t len;
    const char *n;
    p_term *atom;

    /* Look for the name in the context's atom hash */
    hash = 0;
    len = 0;
    if (!name)
        name = "";
    n = name;
    while (*n != '\0') {
        hash = hash * 5 + (((unsigned int)(*n++)) & 0xFF);
        ++len;
    }
    hash %= P_CONTEXT_HASH_SIZE;
    atom = context->atom_hash[hash];
    while (atom != 0) {
        if (!strcmp(atom->atom.name, name))
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
    strcpy(atom->atom.name, name);
    context->atom_hash[hash] = atom;
    return atom;
}

/**
 * \brief Creates a string within \a context with the specified \a name.
 *
 * Returns the string term.  Unlike p_term_create_atom(), a new term
 * is returned every time this function is called.
 *
 * Atoms and strings are not unifiable as they are different types
 * of terms.  Atoms typically represent identifiers in the program,
 * whereas strings represent human-readable data for the program.
 *
 * The \a name should be encoded in the UTF-8 character set with
 * embedded NUL characters encoded as the sequence 0xC0 0x80.
 *
 * \ingroup term
 * \sa p_term_create_atom()
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
    if (str)
        strcpy(term->name, str);
    else
        term->name[0] = '\0';
    return (p_term *)term;
}

/**
 * \brief Creates an unbound variable within \a context.
 *
 * Returns the variable term.
 *
 * \ingroup term
 * \sa p_term_create_named_variable(), p_term_create_typed_variable()
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
 * \brief Creates an unbound variable within \a context that has a
 * type constraint.
 *
 * Returns the variable term.  Values that are bound to the variable
 * must be of the specified \a type or unification fails.  If \a type
 * is P_TERM_FUNCTOR or P_TERM_OBJECT, then \a functor_name specifies
 * further information to restrict the type.
 *
 * If \a arg_count is non-zero, and \a type is P_TERM_FUNCTOR,
 * then the term the variable is bound to must be a functor with
 * the specified number of arguments.
 *
 * If \a variable_name is not null, then it is added to the term
 * as the variable's name.  This is intended for debugging purposes.
 *
 * The \a functor_name must be an atom or null.
 *
 * \ingroup term
 * \sa p_term_create_named_variable(), p_term_create_typed_variable()
 */
p_term *p_term_create_typed_variable(p_context *context, int type, p_term *functor_name, int arg_count, const char *variable_name)
{
    size_t len = variable_name ? strlen(variable_name) : 0;
    struct p_term_typed_var *term;
    if (len) {
        term = p_term_malloc(context, struct p_term_typed_var,
                             sizeof(struct p_term_typed_var) + len + 1);
    } else {
        term = p_term_new(context, struct p_term_typed_var);
    }
    if (!term)
        return 0;
    term->header.type = P_TERM_TYPED_VARIABLE;
    term->header.size = (unsigned int)len;
    term->constraint.type = (unsigned int)type;
    term->constraint.size = (unsigned int)arg_count;
    term->functor_name = p_term_deref(functor_name);
    if (len)
        strcpy((char *)(term + 1), variable_name);
    return (p_term *)term;
}

/**
 * \brief Creates an unbound member variable within \a context that
 * refers to the member \a name within \a object.
 *
 * Returns the member variable term.  The \a name must be an atom.
 *
 * \ingroup term
 * \sa p_term_object(), p_term_name()
 */
p_term *p_term_create_member_variable(p_context *context, p_term *object, p_term *name)
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
 * Dereferencing is needed when terms of type P_TERM_VARIABLE,
 * P_TERM_TYPED_VARIABLE, or P_TERM_MEMBER_VARIABLE are bound to
 * other terms during unification.  Normally dereferencing is
 * performed automatically by term query functions such as
 * p_term_type(), p_term_arg_count(), p_term_name(), etc.
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
 * \sa p_term_create_functor(), p_term_deref()
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
        if (term->var.header.size > 0)
            return (const char *)(&(term->var) + 1);
        break;
    case P_TERM_MEMBER_VARIABLE:
        return p_term_name(term->member_var.name);
    case P_TERM_TYPED_VARIABLE:
        if (term->typed_var.header.size > 0)
            return (const char *)(&(term->typed_var) + 1);
    default: break;
    }
    return 0;
}

/**
 * \brief Returns the argument at position \a index within the
 * functor \a term, or null if \a term is not a functor or
 * \a index is out of range.
 *
 * The \a term is automatically dereferenced.
 *
 * \ingroup term
 * \sa p_term_create_functor()
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
 * \sa p_term_is_instance()
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
 * \brief Returns the name of the class \a term is an instance of.
 * If \a term is a class object, then the class name is returned.
 *
 * This function is equivalent to fetching the "className"
 * property from \a term or its prototype.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_is_instance()
 */
p_term *p_term_class_name(p_context *context, const p_term *term)
{
    p_term *name = context->class_name_atom;
    p_term *pname = context->prototype_atom;
    while (term) {
        /* The class name will be either the first or second
         * property in the object, and the prototype will always
         * be the first property in the object if it is present */
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_OBJECT)
            break;
        if (term->object.properties[0].name == name)
            return term->object.properties[0].value;
        if (term->object.properties[1].name == name)
            return term->object.properties[1].value;
        if (term->object.properties[0].name != pname)
            break;
        term = term->object.properties[0].value;
    }
    return 0;
}

/**
 * \brief Returns non-zero if \a term is an instance of
 * \a class_name; zero otherwise.
 *
 * The \a class_name can be an atom, the class object for the class
 * being tested, or another object instance of the same class.
 *
 * \ingroup term
 * \sa p_term_create_object(), p_term_class_name()
 */
int p_term_is_instance(p_context *context, const p_term *term, const p_term *class_name)
{
    p_term *name = context->class_name_atom;
    p_term *pname = context->prototype_atom;
    if (!class_name)
        return 0;
    class_name = p_term_deref_non_null(class_name);
    if (class_name->header.type == P_TERM_OBJECT) {
        class_name = p_term_class_name(context, class_name);
        if (!class_name)
            return 0;
    }
    while (term) {
        /* The class name will be either the first or second
         * property in the object, and the prototype will always
         * be the first property in the object if it is present */
        term = p_term_deref_non_null(term);
        if (term->header.type != P_TERM_OBJECT)
            break;
        if (term->object.properties[0].name == name) {
            if (term->object.properties[0].value == class_name)
                return 1;
        } else if (term->object.properties[1].name == name) {
            if (term->object.properties[1].value == class_name)
                return 1;
        }
        if (term->object.properties[0].name != pname)
            break;
        term = term->object.properties[0].value;
    }
    return 0;
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

/* Record a variable binding within the trace.  The variable
 * will be set back to null upon back-tracking */
static int p_term_record_binding(p_context *context, p_term *var)
{
    /* TODO */
    return 1;
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
 * \sa p_term_create_variable()
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
        if (!p_term_record_binding(context, var))
            return 0;
    }
    var->var.value = value;
    return 1;
}

/*\@}*/
