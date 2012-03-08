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

#include "inst-priv.h"
#include "context-priv.h"

/** @cond */

#define P_INST_START_LOOP   \
    for (;;) { \
        switch (inst->header.opcode) {
#define P_INST_END_LOOP     \
        default: break; \
        } \
    }

#define P_INST_BEGIN(opcode)            case opcode: {
#define P_INST_BEGIN_LARGE(opcode)      case opcode + 1: {
#define P_INST_END(type)    \
    inst = (p_inst *)(((char *)inst) + sizeof(inst->type)); \
    break; }
#define P_INST_END_NO_ADVANCE   \
    break; }

#define P_INST_FAIL     return P_RESULT_FAIL /* FIXME */

void _p_code_set_xreg(p_context *context, int reg, p_term *value)
{
    p_term **xregs = context->xregs;
    if (reg >= context->num_xregs) {
        int num = (reg + 1 + 63) & ~63;
        xregs = (p_term **)GC_REALLOC(xregs, sizeof(p_term *) * num);
        if (!xregs)
            return;
        context->xregs = xregs;
        context->num_xregs = num;
    }
    xregs[reg] = value;
}

p_goal_result _p_code_run
    (p_context *context, const p_code_clause *clause, p_term **error)
{
    const p_inst *inst;
    p_term *term;
    p_term *term2;
    p_term **xregs;
    p_term **yregs = 0;
    p_term **put_ptr = 0;

    inst = (p_inst *)(clause->code->inst);

    /* Extend the X register array if this clause needs more */
    xregs = context->xregs;
    if (clause->num_xregs > context->num_xregs) {
        int num = (clause->num_xregs + 63) & ~63;
        xregs = (p_term **)GC_REALLOC(xregs, sizeof(p_term *) * num);
        if (!xregs)
            return P_RESULT_FAIL;
        context->xregs = xregs;
        context->num_xregs = num;
    }

    P_INST_START_LOOP

    /* put_variable Xn
     *      Create a new variable and place it into Xn */
    P_INST_BEGIN(P_OP_PUT_X_VARIABLE)
        term = p_term_create_variable(context);
        xregs[inst->one_reg.reg1] = term;
    P_INST_END(one_reg)

    /* put_variable2 Xn, Xm
     *      Create a new variable and place it into Xn and Xm */
    P_INST_BEGIN(P_OP_PUT_X_VARIABLE2)
        term = p_term_create_variable(context);
        xregs[inst->two_reg.reg1] = term;
        xregs[inst->two_reg.reg2] = term;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_PUT_X_VARIABLE2)
        term = p_term_create_variable(context);
        xregs[inst->large_two_reg.reg1] = term;
        xregs[inst->large_two_reg.reg2] = term;
    P_INST_END(large_two_reg)

    /* put_variable2 Yn, Xm
     *      Create a new variable and place it into Yn and Xm */
    P_INST_BEGIN(P_OP_PUT_Y_VARIABLE2)
        term = p_term_create_variable(context);
        yregs[inst->two_reg.reg1] = term;
        xregs[inst->two_reg.reg2] = term;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_PUT_Y_VARIABLE2)
        term = p_term_create_variable(context);
        yregs[inst->large_two_reg.reg1] = term;
        xregs[inst->large_two_reg.reg2] = term;
    P_INST_END(large_two_reg)

    /* put_value Xn, Xm
     *      Puts the value of Xn into Xm */
    P_INST_BEGIN(P_OP_PUT_X_VALUE)
        xregs[inst->two_reg.reg2] = xregs[inst->two_reg.reg1];
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_PUT_X_VALUE)
        xregs[inst->large_two_reg.reg2] =
            xregs[inst->large_two_reg.reg1];
    P_INST_END(large_two_reg)

    /* put_value Yn, Xm
     *      Puts the value of Yn into Xm */
    P_INST_BEGIN(P_OP_PUT_Y_VALUE)
        xregs[inst->two_reg.reg2] = yregs[inst->two_reg.reg1];
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_PUT_Y_VALUE)
        xregs[inst->large_two_reg.reg2] =
            yregs[inst->large_two_reg.reg1];
    P_INST_END(large_two_reg)

    /* put_functor Name/Arity, Xn
     *      Puts a new term with functor Name/Arity into Xn */
    P_INST_BEGIN(P_OP_PUT_FUNCTOR)
        term = p_term_create_functor
            (context, inst->functor.name, inst->functor.arity);
        put_ptr = &(term->functor.arg[0]);
        xregs[inst->functor.reg1] = term;
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_PUT_FUNCTOR)
        term = p_term_create_functor
            (context, inst->large_functor.name,
             inst->large_functor.arity);
        put_ptr = &(term->functor.arg[0]);
        xregs[inst->large_functor.reg1] = term;
    P_INST_END(large_functor)

    /* put_list Xn
     *      Puts a new list term into Xn */
    P_INST_BEGIN(P_OP_PUT_LIST)
        term = p_term_create_list(context, 0, 0);
        xregs[inst->one_reg.reg1] = term;
        put_ptr = &(term->list.head);
    P_INST_END(one_reg)

    /* put_constant Value, Xn
     *      Puts a constant Value into Xn */
    P_INST_BEGIN(P_OP_PUT_CONSTANT)
        xregs[inst->constant.reg1] = inst->constant.value;
    P_INST_END(constant)

    /* put_member_variable Xn, Name, Xm
     *      Puts a member variable reference for Xn.Name into Xm */
    P_INST_BEGIN(P_OP_PUT_MEMBER_VARIABLE)
        term = p_term_create_member_variable
            (context, xregs[inst->functor.reg1], inst->functor.name, 0);
        xregs[inst->functor.arity] = term;
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_PUT_MEMBER_VARIABLE)
        term = p_term_create_member_variable
            (context, xregs[inst->large_functor.reg1],
             inst->large_functor.name, 0);
        xregs[inst->large_functor.arity] = term;
    P_INST_END(large_functor)

    /* put_member_variable_auto Xn, Name, Xm
     *      Puts a member variable reference for Xn..Name into Xm */
    P_INST_BEGIN(P_OP_PUT_MEMBER_VARIABLE_AUTO)
        term = p_term_create_member_variable
            (context, xregs[inst->functor.reg1], inst->functor.name, 1);
        xregs[inst->functor.arity] = term;
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_PUT_MEMBER_VARIABLE_AUTO)
        term = p_term_create_member_variable
            (context, xregs[inst->large_functor.reg1],
             inst->large_functor.name, 1);
        xregs[inst->large_functor.arity] = term;
    P_INST_END(large_functor)

    /* set_variable Xn
     *      Sets a variable into the put pointer and Xn */
    P_INST_BEGIN(P_OP_SET_X_VARIABLE)
        term = p_term_create_variable(context);
        *put_ptr++ = term;
        xregs[inst->one_reg.reg1] = term;
    P_INST_END(one_reg)

    /* set_variable Yn
     *      Sets a variable into the put pointer and Yn */
    P_INST_BEGIN(P_OP_SET_Y_VARIABLE)
        term = p_term_create_variable(context);
        *put_ptr++ = term;
        yregs[inst->one_reg.reg1] = term;
    P_INST_END(one_reg)

    /* set_value Xn
     *      Sets the put pointer to the value in Xn */
    P_INST_BEGIN(P_OP_SET_X_VALUE)
        *put_ptr++ = xregs[inst->one_reg.reg1];
    P_INST_END(one_reg)

    /* set_value Yn
     *      Sets the put pointer to the value in Yn */
    P_INST_BEGIN(P_OP_SET_Y_VALUE)
        *put_ptr++ = yregs[inst->one_reg.reg1];
    P_INST_END(one_reg)

    /* set_functor Name/Arity, Xn
     *      Sets a new term with functor Name/Arity into Xn */
    P_INST_BEGIN(P_OP_SET_FUNCTOR)
        term = p_term_create_functor
            (context, inst->functor.name, inst->functor.arity);
        *put_ptr = term;
        xregs[inst->functor.reg1] = term;
        put_ptr = &(term->functor.arg[0]);
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_SET_FUNCTOR)
        term = p_term_create_functor
            (context, inst->large_functor.name,
             inst->large_functor.arity);
        *put_ptr = term;
        xregs[inst->large_functor.reg1] = term;
        put_ptr = &(term->functor.arg[0]);
    P_INST_END(large_functor)

    /* set_list Xn
     *      Sets a new list term into Xn */
    P_INST_BEGIN(P_OP_SET_LIST)
        term = p_term_create_list(context, 0, 0);
        *put_ptr = term;
        xregs[inst->one_reg.reg1] = term;
        put_ptr = &(term->list.head);
    P_INST_END(one_reg)

    /* set_list_tail Xn
     *      Sets a new list term into the tail of Xn and then replace
     *      Xn's value with the new list */
    P_INST_BEGIN(P_OP_SET_LIST_TAIL)
        term = p_term_create_list(context, 0, 0);
        xregs[inst->one_reg.reg1]->list.tail = term;
        xregs[inst->one_reg.reg1] = term;
        put_ptr = &(term->list.head);
    P_INST_END(one_reg)

    /* set_nil_tail Xn
     *      Sets the tail of Xn to nil */
    P_INST_BEGIN(P_OP_SET_NIL_TAIL)
        xregs[inst->one_reg.reg1]->list.tail = context->nil_atom;
    P_INST_END(one_reg)

    /* set_constant Value
     *      Sets the contents of the put pointer to Value */
    P_INST_BEGIN(P_OP_SET_CONSTANT)
        *put_ptr++ = inst->constant.value;
    P_INST_END(constant)

    /* set_void
     *      Sets the put pointer to an anonymous variable */
    P_INST_BEGIN(P_OP_SET_VOID)
        *put_ptr++ = p_term_create_variable(context);
    P_INST_END(header)

    /* get_variable Xn, Ym
     *      Moves the value in Xn to Yn.  We create an extra variable
     *      shell around the value because Y registers must be vars.
     *      Note: "get_x_variable" is the same as "put_x_value" so
     *      we don't need a special instruction for that */
    P_INST_BEGIN(P_OP_GET_Y_VARIABLE)
        term = p_term_create_variable(context);
        term->var.value = xregs[inst->two_reg.reg1];
        yregs[inst->two_reg.reg2] = term;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_Y_VARIABLE)
        term = p_term_create_variable(context);
        term->var.value = xregs[inst->large_two_reg.reg1];
        yregs[inst->large_two_reg.reg2] = term;
    P_INST_END(large_two_reg)

    /* get_value Xn, Xm
     *      Unify the contents of Xn and Xm */
    P_INST_BEGIN(P_OP_GET_X_VALUE)
        if (!p_term_unify(context, xregs[inst->two_reg.reg1],
                          xregs[inst->two_reg.reg2], P_BIND_DEFAULT))
            P_INST_FAIL;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_X_VALUE)
        if (!p_term_unify(context, xregs[inst->large_two_reg.reg1],
                          xregs[inst->large_two_reg.reg2],
                          P_BIND_DEFAULT))
            P_INST_FAIL;
    P_INST_END(large_two_reg)

    /* get_value Yn, Xm
     *      Unify the contents of Yn and Xm */
    P_INST_BEGIN(P_OP_GET_Y_VALUE)
        if (!p_term_unify(context, yregs[inst->two_reg.reg1],
                          xregs[inst->two_reg.reg2], P_BIND_DEFAULT))
            P_INST_FAIL;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_Y_VALUE)
        if (!p_term_unify(context, yregs[inst->large_two_reg.reg1],
                          xregs[inst->large_two_reg.reg2],
                          P_BIND_DEFAULT))
            P_INST_FAIL;
    P_INST_END(large_two_reg)

    /* get_functor Name/Arity, Xn
     *      Unifies Xn against the functor Name/Arity and sets the
     *      "current put pointer" to the first functor argument */
    P_INST_BEGIN(P_OP_GET_FUNCTOR)
        term = p_term_deref_member(context, xregs[inst->functor.reg1]);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->functor.arity &&
                term->functor.functor_name == inst->functor.name) {
            put_ptr = &(term->functor.arg[0]);
        } else if (term->header.type & P_TERM_VARIABLE) {
            term2 = p_term_create_functor
                (context, inst->functor.name, inst->functor.arity);
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
            put_ptr = &(term2->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_GET_FUNCTOR)
        term = p_term_deref_member(context, xregs[inst->large_functor.reg1]);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->large_functor.arity &&
                term->functor.functor_name ==
                        inst->large_functor.name) {
            put_ptr = &(term->functor.arg[0]);
        } else if (term->header.type & P_TERM_VARIABLE) {
            term2 = p_term_create_functor
                (context, inst->large_functor.name,
                 inst->large_functor.arity);
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
            put_ptr = &(term2->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(large_functor)

    /* get_list Xn, Xm
     *      Unifies Xn against a list term and copies the term into Xm.
     *      The "current put pointer" is set to the list head */
    P_INST_BEGIN(P_OP_GET_LIST)
        term = p_term_deref_member(context, xregs[inst->two_reg.reg1]);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->two_reg.reg2] = term;
            put_ptr = &(term->list.head);
        } else if (term->header.type & P_TERM_VARIABLE) {
            term2 = p_term_create_list(context, 0, 0);
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
            xregs[inst->two_reg.reg2] = term2;
            put_ptr = &(term2->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_LIST)
        term = p_term_deref_member(context, xregs[inst->large_two_reg.reg1]);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->large_two_reg.reg2] = term;
            put_ptr = &(term->list.head);
        } else if (term->header.type & P_TERM_VARIABLE) {
            term2 = p_term_create_list(context, 0, 0);
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
            xregs[inst->large_two_reg.reg2] = term2;
            put_ptr = &(term2->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(large_two_reg)

    /* get_atom Value, Xn
     *      Unifies Xn against the atom Value */
    P_INST_BEGIN(P_OP_GET_ATOM)
        term = p_term_deref_member(context, xregs[inst->constant.reg1]);
        if (term->header.type == P_TERM_ATOM) {
            if (term != inst->constant.value)
                P_INST_FAIL;
        } else if (term->header.type & P_TERM_VARIABLE) {
            if (!p_term_unify(context, term, inst->constant.value,
                              P_BIND_DEFAULT))
                P_INST_FAIL;
        } else {
            P_INST_FAIL;
        }
    P_INST_END(constant)

    /* get_constant Value, Xn
     *      Unifies Xn against the specified constant Value */
    P_INST_BEGIN(P_OP_GET_CONSTANT)
        term = p_term_deref_member(context, xregs[inst->constant.reg1]);
        term2 = inst->constant.value;
        if (term->header.type == term2->header.type ||
                (term->header.type & P_TERM_VARIABLE) != 0) {
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
        } else {
            P_INST_FAIL;
        }
    P_INST_END(constant)

    /* get_in_value Xn, Xm
     *      Unify the contents of Xn and Xm, without binding
     *      variables within Xm */
    P_INST_BEGIN(P_OP_GET_IN_X_VALUE)
        if (!p_term_unify(context, xregs[inst->two_reg.reg1],
                          xregs[inst->two_reg.reg2], P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_IN_X_VALUE)
        if (!p_term_unify(context, xregs[inst->large_two_reg.reg1],
                          xregs[inst->large_two_reg.reg2],
                          P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(large_two_reg)

    /* get_in_value Yn, Xm
     *      Unify the contents of Yn and Xm, without binding
     *      variables within Xm */
    P_INST_BEGIN(P_OP_GET_IN_Y_VALUE)
        if (!p_term_unify(context, yregs[inst->two_reg.reg1],
                          xregs[inst->two_reg.reg2], P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_IN_Y_VALUE)
        if (!p_term_unify(context, yregs[inst->large_two_reg.reg1],
                          xregs[inst->large_two_reg.reg2],
                          P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(large_two_reg)

    /* get_in_functor Name/Arity, Xn
     *      Unifies Xn against the functor Name/Arity and sets the
     *      "current put pointer" to the first functor argument.
     *      This instruction does not bind variables in Xn */
    P_INST_BEGIN(P_OP_GET_IN_FUNCTOR)
        term = p_term_deref_member(context, xregs[inst->functor.reg1]);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->functor.arity &&
                term->functor.functor_name == inst->functor.name) {
            put_ptr = &(term->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_GET_IN_FUNCTOR)
        term = p_term_deref_member(context, xregs[inst->large_functor.reg1]);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->large_functor.arity &&
                term->functor.functor_name ==
                        inst->large_functor.name) {
            put_ptr = &(term->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(large_functor)

    /* get_in_list Xn, Xm
     *      Unifies Xn against a list term and copies the term into Xm.
     *      The "current put pointer" is set to the list head.
     *      This instruction does not bind variables in Xn */
    P_INST_BEGIN(P_OP_GET_IN_LIST)
        term = p_term_deref_member(context, xregs[inst->two_reg.reg1]);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->two_reg.reg2] = term;
            put_ptr = &(term->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_GET_IN_LIST)
        term = p_term_deref_member(context, xregs[inst->large_two_reg.reg1]);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->large_two_reg.reg2] = term;
            put_ptr = &(term->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(large_two_reg)

    /* get_in_atom Value, Xn
     *      Unifies Xn against the atom Value, without binding
     *      variables in Xn */
    P_INST_BEGIN(P_OP_GET_IN_ATOM)
        term = p_term_deref_member(context, xregs[inst->constant.reg1]);
        if (term->header.type == P_TERM_ATOM) {
            if (term != inst->constant.value)
                P_INST_FAIL;
        } else {
            P_INST_FAIL;
        }
    P_INST_END(constant)

    /* get_in_constant Value, Xn
     *      Unifies Xn against the specified constant Value,
     *      without binding variables in Xn */
    P_INST_BEGIN(P_OP_GET_IN_CONSTANT)
        term = p_term_deref_member(context, xregs[inst->constant.reg1]);
        term2 = inst->constant.value;
        if (term->header.type == term2->header.type) {
            if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                P_INST_FAIL;
        } else {
            P_INST_FAIL;
        }
    P_INST_END(constant)

    /* unify_variable Xn
     *      Loads the contents of the put pointer into Xn or creates a
     *      new variable if the contents are null */
    P_INST_BEGIN(P_OP_UNIFY_X_VARIABLE)
        term = *put_ptr;
        if (term) {
            xregs[inst->one_reg.reg1] = term;
            ++put_ptr;
        } else {
            *put_ptr++ = term = p_term_create_variable(context);
            xregs[inst->one_reg.reg1] = term;
        }
    P_INST_END(one_reg)

    /* unify_variable Yn
     *      Loads the contents of the put pointer into Yn or creates a
     *      new variable if the contents are null */
    P_INST_BEGIN(P_OP_UNIFY_Y_VARIABLE)
        term = *put_ptr;
        if (term) {
            yregs[inst->one_reg.reg1] = term;
            ++put_ptr;
        } else {
            *put_ptr++ = term = p_term_create_variable(context);
            yregs[inst->one_reg.reg1] = term;
        }
    P_INST_END(one_reg)

    /* unify_value Xn
     *      Unifies the contents of the put pointer with Xn */
    P_INST_BEGIN(P_OP_UNIFY_X_VALUE)
        term = *put_ptr;
        if (term) {
            ++put_ptr;
            if (!p_term_unify(context, term, xregs[inst->one_reg.reg1],
                              P_BIND_DEFAULT))
                P_INST_FAIL;
        } else {
            *put_ptr++ = xregs[inst->one_reg.reg1];
        }
    P_INST_END(one_reg)

    /* unify_value Yn
     *      Unifies the contents of the put pointer with Yn */
    P_INST_BEGIN(P_OP_UNIFY_Y_VALUE)
        term = *put_ptr;
        if (term) {
            ++put_ptr;
            if (!p_term_unify(context, term, yregs[inst->one_reg.reg1],
                              P_BIND_DEFAULT))
                P_INST_FAIL;
        } else {
            *put_ptr++ = yregs[inst->one_reg.reg1];
        }
    P_INST_END(one_reg)

    /* unify_functor Name/Arity, Xn
     *      Unifies the contents of the put pointer with the
     *      function Name/Arity and copies the term into Xn */
    P_INST_BEGIN(P_OP_UNIFY_FUNCTOR)
        term = *put_ptr;
        if (term) {
            term = p_term_deref_member(context, term);
            if (term->header.type == P_TERM_FUNCTOR &&
                    term->header.size == inst->functor.arity &&
                    term->functor.functor_name
                            == inst->functor.name) {
                xregs[inst->functor.reg1] = term;
                put_ptr = &(term->functor.arg[0]);
            } else if (term->header.type & P_TERM_VARIABLE) {
                term2 = p_term_create_functor
                    (context, inst->functor.name, inst->functor.arity);
                if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                    P_INST_FAIL;
                xregs[inst->functor.reg1] = term2;
                put_ptr = &(term2->functor.arg[0]);
            } else {
                P_INST_FAIL;
            }
        } else {
            term = p_term_create_functor
                (context, inst->functor.name, inst->functor.arity);
            *put_ptr = term;
            xregs[inst->functor.reg1] = term;
            put_ptr = &(term->functor.arg[0]);
        }
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_UNIFY_FUNCTOR)
        term = *put_ptr;
        if (term) {
            term = p_term_deref_member(context, term);
            if (term->header.type == P_TERM_FUNCTOR &&
                    term->header.size == inst->large_functor.arity &&
                    term->functor.functor_name
                            == inst->large_functor.name) {
                xregs[inst->large_functor.reg1] = term;
                put_ptr = &(term->functor.arg[0]);
            } else if (term->header.type & P_TERM_VARIABLE) {
                term2 = p_term_create_functor
                    (context, inst->large_functor.name,
                     inst->large_functor.arity);
                if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                    P_INST_FAIL;
                xregs[inst->large_functor.reg1] = term2;
                put_ptr = &(term2->functor.arg[0]);
            } else {
                P_INST_FAIL;
            }
        } else {
            term = p_term_create_functor
                (context, inst->large_functor.name,
                 inst->large_functor.arity);
            *put_ptr = term;
            xregs[inst->large_functor.reg1] = term;
            put_ptr = &(term->functor.arg[0]);
        }
    P_INST_END(large_functor)

    /* unify_list Xn
     *      Unifies the contents of the put pointer with a list
     *      and copies the list term into Xn */
    P_INST_BEGIN(P_OP_UNIFY_LIST)
        term = *put_ptr;
        if (term) {
            term = p_term_deref_member(context, term);
            if (term->header.type == P_TERM_LIST) {
                xregs[inst->one_reg.reg1] = term;
                put_ptr = &(term->list.head);
            } else if (term->header.type & P_TERM_VARIABLE) {
                term2 = p_term_create_list(context, 0, 0);
                if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                    P_INST_FAIL;
                xregs[inst->one_reg.reg1] = term2;
                put_ptr = &(term2->list.head);
            } else {
                P_INST_FAIL;
            }
        } else {
            term = p_term_create_list(context, 0, 0);
            *put_ptr = term;
            xregs[inst->one_reg.reg1] = term;
            put_ptr = &(term->list.head);
        }
    P_INST_END(one_reg)

    /* unify_list_tail Xn
     *      Unifies the tail of Xn with a list and then replaces
     *      the contents of Xn with a reference to the new list */
    P_INST_BEGIN(P_OP_UNIFY_LIST_TAIL)
        term = xregs[inst->one_reg.reg1];
        if (term->list.tail) {
            term = p_term_deref_member(context, term->list.tail);
            if (term->header.type == P_TERM_LIST) {
                xregs[inst->one_reg.reg1] = term;
                put_ptr = &(term->list.head);
            } else if (term->header.type & P_TERM_VARIABLE) {
                term2 = p_term_create_list(context, 0, 0);
                if (!p_term_unify(context, term, term2, P_BIND_DEFAULT))
                    P_INST_FAIL;
                xregs[inst->one_reg.reg1] = term2;
                put_ptr = &(term2->list.head);
            } else {
                P_INST_FAIL;
            }
        } else {
            term2 = p_term_create_list(context, 0, 0);
            term->list.tail = term2;
            xregs[inst->one_reg.reg1] = term2;
            put_ptr = &(term2->list.head);
        }
    P_INST_END(one_reg)

    /* unify_nil_tail Xn
     *      Unifies the tail of Xn with nil */
    P_INST_BEGIN(P_OP_UNIFY_NIL_TAIL)
        term = xregs[inst->one_reg.reg1];
        if (term->list.tail) {
            term = p_term_deref_member(context, term->list.tail);
            if (term->header.type & P_TERM_VARIABLE) {
                if (!p_term_unify(context, term, context->nil_atom,
                                  P_BIND_DEFAULT))
                    P_INST_FAIL;
            } else if (term != context->nil_atom) {
                P_INST_FAIL;
            }
        } else {
            term->list.tail = context->nil_atom;
        }
    P_INST_END(one_reg)

    /* unify_atom Value
     *      Unifies the contents of the put pointer with Value */
    P_INST_BEGIN(P_OP_UNIFY_ATOM)
        term = p_term_deref_member(context, *put_ptr);
        if (term == inst->constant.value) {
            ++put_ptr;
        } else if (term) {
            if (!p_term_unify(context, term, inst->constant.value,
                              P_BIND_DEFAULT))
                P_INST_FAIL;
            ++put_ptr;
        } else {
            *put_ptr++ = inst->constant.value;
        }
    P_INST_END(constant)

    /* unify_constant Value
     *      Unifies the contents of the put pointer with Value */
    P_INST_BEGIN(P_OP_UNIFY_CONSTANT)
        term = *put_ptr;
        if (term) {
            if (!p_term_unify(context, term, inst->constant.value,
                              P_BIND_DEFAULT))
                P_INST_FAIL;
            ++put_ptr;
        } else {
            *put_ptr++ = inst->constant.value;
        }
    P_INST_END(constant)

    /* unify_void
     *      Unifies the contents of the put pointer with an
     *      anonymous variable */
    P_INST_BEGIN(P_OP_UNIFY_VOID)
        term = *put_ptr;
        if (!term)
            *put_ptr++ = p_term_create_variable(context);
        else
            ++put_ptr;
    P_INST_END(header)

    /* unify_in_value Xn
     *      Unifies the contents of the put pointer with Xn,
     *      without modifying the put pointer's contents */
    P_INST_BEGIN(P_OP_UNIFY_IN_X_VALUE)
        term = *put_ptr++;
        if (!p_term_unify(context, xregs[inst->one_reg.reg1], term,
                          P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(one_reg)

    /* unify_in_value Yn
     *      Unifies the contents of the put pointer with Yn,
     *      without modifying the put pointer's contents */
    P_INST_BEGIN(P_OP_UNIFY_IN_Y_VALUE)
        term = *put_ptr++;
        if (!p_term_unify(context, yregs[inst->one_reg.reg1], term,
                          P_BIND_ONE_WAY))
            P_INST_FAIL;
    P_INST_END(one_reg)

    /* unify_in_functor Name/Arity, Xn
     *      Unifies the contents of the put pointer with the
     *      function Name/Arity and copies the term into Xn.
     *      The put pointer's contents must not be modified */
    P_INST_BEGIN(P_OP_UNIFY_IN_FUNCTOR)
        term = p_term_deref_member(context, *put_ptr);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->functor.arity &&
                term->functor.functor_name == inst->functor.name) {
            xregs[inst->functor.reg1] = term;
            put_ptr = &(term->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(functor)
    P_INST_BEGIN_LARGE(P_OP_UNIFY_IN_FUNCTOR)
        term = p_term_deref_member(context, *put_ptr);
        if (term->header.type == P_TERM_FUNCTOR &&
                term->header.size == inst->large_functor.arity &&
                term->functor.functor_name
                        == inst->large_functor.name) {
            xregs[inst->large_functor.reg1] = term;
            put_ptr = &(term->functor.arg[0]);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(large_functor)

    /* unify_in_list Xn
     *      Unifies the contents of the put pointer with a list
     *      and copies the list term into Xn.  The contents of the
     *      put pointer must not be modified. */
    P_INST_BEGIN(P_OP_UNIFY_IN_LIST)
        term = p_term_deref_member(context, *put_ptr);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->one_reg.reg1] = term;
            put_ptr = &(term->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(one_reg)

    /* unify_in_list_tail Xn
     *      Unifies the tail of Xn with a list and then replaces
     *      the contents of Xn with a reference to the new list.
     *      The put pointer's contents must not be modified */
    P_INST_BEGIN(P_OP_UNIFY_IN_LIST_TAIL)
        term = xregs[inst->one_reg.reg1];
        term = p_term_deref_member(context, term->list.tail);
        if (term->header.type == P_TERM_LIST) {
            xregs[inst->one_reg.reg1] = term;
            put_ptr = &(term->list.head);
        } else {
            P_INST_FAIL;
        }
    P_INST_END(one_reg)

    /* unify_in_nil_tail Xn
     *      Unifies the tail of Xn with nil, without modifying
     *      the term pointed to by Xn */
    P_INST_BEGIN(P_OP_UNIFY_IN_NIL_TAIL)
        term = xregs[inst->one_reg.reg1];
        term = p_term_deref_member(context, term->list.tail);
        if (term != context->nil_atom)
            P_INST_FAIL;
    P_INST_END(one_reg)

    /* unify_in_atom Value
     *      Unifies the contents of the put pointer with Value,
     *      without modifying the contents of the put pointer */
    P_INST_BEGIN(P_OP_UNIFY_IN_ATOM)
        term = p_term_deref_member(context, *put_ptr);
        if (term == inst->constant.value)
            ++put_ptr;
        else
            P_INST_FAIL;
    P_INST_END(constant)

    /* unify_in_constant Value
     *      Unifies the contents of the put pointer with Value,
     *      without modifying the contents of the put pointer */
    P_INST_BEGIN(P_OP_UNIFY_IN_CONSTANT)
        term = p_term_deref_member(context, *put_ptr);
        if (term->header.type == inst->constant.value->header.type) {
            if (!p_term_unify(context, term, inst->constant.value,
                              P_BIND_DEFAULT))
                P_INST_FAIL;
            ++put_ptr;
        } else {
            P_INST_FAIL;
        }
    P_INST_END(constant)

    /* unify_in_void
     *      Unifies the contents of the put pointer with an
     *      anonymous variable, without modifying the incoming value */
    P_INST_BEGIN(P_OP_UNIFY_IN_VOID)
        ++put_ptr;
    P_INST_END(header)

    /* reset_argument Xn, ArgIndex
     *      Resets the put pointer to ArgIndex on functor Xn */
    P_INST_BEGIN(P_OP_RESET_ARGUMENT)
        term = p_term_deref_member(context, xregs[inst->two_reg.reg1]);
        put_ptr = &(term->functor.arg[inst->two_reg.reg2]);
    P_INST_END(two_reg)
    P_INST_BEGIN_LARGE(P_OP_RESET_ARGUMENT)
        term = p_term_deref_member(context, xregs[inst->large_two_reg.reg1]);
        put_ptr = &(term->functor.arg[inst->large_two_reg.reg2]);
    P_INST_END(large_two_reg)

    /* reset_tail Xn
     *      Resets the put pointer to the tail of Xn */
    P_INST_BEGIN(P_OP_RESET_TAIL)
        term = p_term_deref_member(context, xregs[inst->one_reg.reg1]);
        put_ptr = &(term->list.tail);
    P_INST_END(one_reg)

    /* jump Label
     *      Jumps to an instruction label */
    P_INST_BEGIN(P_OP_JUMP)
        inst = inst->label.label;
    P_INST_END_NO_ADVANCE

    /* proceed
     *      Returns from the current predicate and succeeds */
    P_INST_BEGIN(P_OP_PROCEED)
        /* FIXME */
        return P_RESULT_TRUE;
    P_INST_END_NO_ADVANCE

    /* fail
     *      Fails the current search path */
    P_INST_BEGIN(P_OP_FAIL)
        P_INST_FAIL;
    P_INST_END_NO_ADVANCE

    /* return Xn
     *      Returns from the current predicate with the value in Xn.
     *      This is used by dynamic clauses to return the body */
    P_INST_BEGIN(P_OP_RETURN)
        *error = xregs[inst->one_reg.reg1];
        return P_RESULT_RETURN_BODY;
    P_INST_END_NO_ADVANCE

    /* return_true
     *      Returns from the current predicate with success.
     *      This is used by dynamic clauses with no body */
    P_INST_BEGIN(P_OP_RETURN_TRUE)
        return P_RESULT_TRUE;
    P_INST_END_NO_ADVANCE

    /* throw Xn
     *      Throws the contents of Xn as an error */
    P_INST_BEGIN(P_OP_THROW)
        /* FIXME */
        *error = xregs[inst->one_reg.reg1];
        return P_RESULT_ERROR;
    P_INST_END_NO_ADVANCE

#if 0
    P_OP_CALL,
    P_OP_EXECUTE,

    P_OP_TRY_ME_ELSE,
    P_OP_RETRY_ME_ELSE,
    P_OP_TRUST_ME,

    P_OP_NECK_CUT,
    P_OP_GET_LEVEL,
    P_OP_CUT,
#endif

    /* end
     *      End of predicate marker that is used by the disassembler.
     *      Should never be executed because it will normally be
     *      preceded by a "return", "proceed", etc instruction. */
    P_INST_BEGIN(P_OP_END)
        P_INST_FAIL;
    P_INST_END_NO_ADVANCE

    P_INST_END_LOOP
}

/** @endcond */
