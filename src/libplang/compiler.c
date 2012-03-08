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

/* Functions in this source file assist with compiling clauses
 * into virtual machine instuctions */

/* Special term types that are used to allocate regs to variables */
enum {
    P_TERM_X_REGISTER = 32,
    P_TERM_Y_REGISTER
};

#define P_REG_WORD_SIZE     ((int)(8 * sizeof(unsigned int)))

P_INLINE void p_inst_set_reg(unsigned int *regs, int reg)
{
    regs[reg / P_REG_WORD_SIZE] |=
        (((unsigned int)1) << (reg % P_REG_WORD_SIZE));
}

P_INLINE void p_inst_clear_reg(unsigned int *regs, int reg)
{
    regs[reg / P_REG_WORD_SIZE] &=
        ~(((unsigned int)1) << (reg % P_REG_WORD_SIZE));
}

P_INLINE int p_inst_is_reg_set(const unsigned int *regs, int reg)
{
    return (regs[reg / P_REG_WORD_SIZE] &
                (((unsigned int)1) << (reg % P_REG_WORD_SIZE))) != 0;
}

/* Allocate a new register */
static int p_inst_allocate_reg(p_code *code)
{
    int reg = code->blocked_regs;
    for (;;) {
        if (reg >= code->max_regs) {
            int new_max_regs = code->max_regs * 2;
            if (new_max_regs < P_REG_WORD_SIZE)
                new_max_regs = P_REG_WORD_SIZE;
            code->used_regs = GC_REALLOC
                (code->used_regs,
                 new_max_regs * sizeof(unsigned int) / P_REG_WORD_SIZE);
            code->temp_regs = GC_REALLOC
                (code->temp_regs,
                 new_max_regs * sizeof(unsigned int) / P_REG_WORD_SIZE);
            code->max_regs = new_max_regs;
        }
        if (reg >= code->num_regs)
            break;
        if (!p_inst_is_reg_set(code->used_regs, reg))
            break;
        ++reg;
    }
    p_inst_set_reg(code->used_regs, reg);
    if (reg >= code->num_regs)
        code->num_regs = reg + 1;
    return reg;
}

/* Allocate a new semi-permanent register.  This is typically
 * for variables that are expected to have more than one use */
static int p_inst_new_reg(p_code *code)
{
    int reg = p_inst_allocate_reg(code);
    p_inst_clear_reg(code->temp_regs, reg);
    return reg;
}

/* Allocate a new temporary register whose value can be
 * discarded as soon as it has been used.  That is, we expect
 * that there is only one use of the register's value */
static int p_inst_new_temp_reg(p_code *code)
{
    int reg = p_inst_allocate_reg(code);
    p_inst_set_reg(code->temp_regs, reg);
    return reg;
}

/* Mark a register as used.  If it was temporary then it will
 * be returned to the allocation pool */
static void p_inst_reg_used(p_code *code, int reg)
{
    if (p_inst_is_reg_set(code->temp_regs, reg)) {
        p_inst_clear_reg(code->used_regs, reg);
        p_inst_clear_reg(code->temp_regs, reg);
    }
}

/* Allocate a new instruction block */
static p_inst *_p_inst_new
    (p_code *code, p_opcode opcode, size_t inst_size)
{
    p_code_block *block;
    p_inst *inst;
    if ((code->posn + inst_size + sizeof(struct p_inst_label)) >
            P_CODE_BLOCK_SIZE) {
        /* This code block has overflowed - create another one
         * and output a "jump" instruction to reference it */
        block = GC_NEW(p_code_block);
        if (code->current_block) {
            inst = (p_inst *)
                (((char *)(code->current_block->inst)) + code->posn);
            inst->label.opcode = P_OP_JUMP;
            inst->label.label = (p_inst *)(block->inst);
            if (code->current_block == code->first_block) {
                code->first_block_size =
                    code->posn + sizeof(struct p_inst_label);
            }
            code->current_block = block;
        } else {
            code->first_block = block;
            code->current_block = block;
        }
        code->posn = 0;
    } else {
        /* There is enough room for the next instruction */
        block = code->current_block;
    }
    inst = (p_inst *)(((char *)(block->inst)) + code->posn);
    inst->header.opcode = opcode;
    code->posn += inst_size;
    return inst;
}
#define p_inst_new(code,opcode,type)    \
    _p_inst_new((code), (opcode), sizeof(type))

/* Create a new two-register instruction */
P_INLINE void p_inst_new_two_reg
    (p_code *code, p_opcode opcode, int reg1, int reg2)
{
    p_inst *inst;
    if (reg1 <= P_MAX_SMALL_REG && reg2 <= P_MAX_SMALL_REG &&
            !code->force_large_regs) {
        inst = p_inst_new(code, opcode, struct p_inst_two_reg);
        inst->two_reg.reg1 = (unsigned int)reg1;
        inst->two_reg.reg2 = (unsigned int)reg2;
    } else {
        inst = p_inst_new
            (code, opcode + 1, struct p_inst_large_two_reg);
        inst->large_two_reg.reg1 = (unsigned int)reg1;
        inst->large_two_reg.reg2 = (unsigned int)reg2;
    }
}

/* Create a functor instruction */
P_INLINE void p_inst_new_functor
    (p_code *code, p_opcode opcode, int reg1,
     unsigned int arity, p_term *name)
{
    p_inst *inst;
    if (reg1 <= P_MAX_SMALL_REG && arity <= P_MAX_SMALL_REG &&
            !code->force_large_regs) {
        inst = p_inst_new(code, opcode, struct p_inst_functor);
        inst->functor.reg1 = (unsigned int)reg1;
        inst->functor.arity = arity;
        inst->functor.name = name;
    } else {
        inst = p_inst_new
            (code, opcode + 1, struct p_inst_large_functor);
        inst->large_functor.reg1 = (unsigned int)reg1;
        inst->large_functor.arity = arity;
        inst->large_functor.name = name;
    }
}

/* Allocate the first "arity" X registers as incoming arguments */
void _p_code_allocate_args(p_code *code, int arity)
{
    code->blocked_regs = 0;
    code->num_regs = 0;
    while (code->num_regs < arity)
        p_inst_allocate_reg(code);
    code->blocked_regs = arity;
}

/* Bind all unbound variables in a term to register terms,
 * count the number of references to the variable, and
 * determine which variables are used across goals */
static void p_code_analyze_variables
    (p_context *context, p_term *term, unsigned int goal_number)
{
    term = p_term_deref(term);
    if (!term)
        return;
    switch (term->header.type) {
    case P_TERM_FUNCTOR: {
        /* Analyze unbound variables within the functor arguments */
        unsigned int index;
        for (index = 0; index < term->header.size; ++index) {
            p_code_analyze_variables
                (context, term->functor.arg[index], goal_number);
        }
        break; }
    case P_TERM_LIST: {
        /* Analyze unbound variables within the list members */
        do {
            p_code_analyze_variables
                (context, term->list.head, goal_number);
            term = p_term_deref(term->list.tail);
            if (!term)
                return;
        } while (term->header.type == P_TERM_LIST);
        p_code_analyze_variables(context, term, goal_number);
        break; }
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
    case P_TERM_PREDICATE:
    case P_TERM_CLAUSE:
    case P_TERM_DATABASE:
        /* These terms are all treated as constants by the compiler */
        break;
    case P_TERM_VARIABLE: {
        /* Bind the variable to a register term */
        struct p_term_register *reg =
            p_term_new(context, struct p_term_register);
        if (!reg)
            return;
        reg->header.type = P_TERM_X_REGISTER;
        reg->usage_count = 1;
        reg->goal_number = goal_number;
        _p_context_record_in_trail(context, term);
        term->var.value = (p_term *)reg;
        break; }
    case P_TERM_MEMBER_VARIABLE:
        p_code_analyze_variables
            (context, term->member_var.object, goal_number);
        break;
    case P_TERM_X_REGISTER:
    case P_TERM_Y_REGISTER:
        /* We've already dealt with this variable before.  Update the
         * usage count and determine if the variable is cross-goal */
        ++(term->reg.usage_count);
        if (term->reg.goal_number != goal_number)
            term->header.type = P_TERM_Y_REGISTER;
        break;
    default: break;
    }
}

static void p_code_generate_list_setter
    (p_context *context, p_term *term, p_code *code,
     int list_reg, int preserve_reg);
static int p_code_generate_builder_inner
    (p_context *context, p_term *term, p_code *code, int preferred_reg);

/* Generate code to set a functor argument to "term.  The term
 * is assumed to be dereferenced.  Returns non-zero if the
 * "current put pointer" needs to be reset at the next higher level */
static int p_code_generate_setter
    (p_context *context, p_term *term, p_code *code)
{
    int reg;
    p_inst *inst;
    unsigned int index;
    p_term *arg;
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        /* Put the functor onto the heap */
        reg = p_inst_new_temp_reg(code);
        p_inst_new_functor
            (code, P_OP_SET_FUNCTOR, reg, term->header.size,
             term->functor.functor_name);

        /* Build the functor arguments */
        if (term->header.size == 2 &&
                term->functor.functor_name == context->comma_atom) {
            /* Try to reduce the recursion depth for comma operators,
             * which we assume to be right-recursive */
            for (;;) {
                arg = p_term_deref(term->functor.arg[0]);
                if (!arg)
                    return 0;
                if (p_code_generate_setter(context, arg, code)) {
                    p_inst_new_two_reg
                        (code, P_OP_RESET_ARGUMENT, reg, 1);
                }
                term = p_term_deref(term->functor.arg[1]);
                if (!term)
                    return 0;
                if (term->header.type != P_TERM_FUNCTOR ||
                        term->header.size != 2 ||
                        term->functor.functor_name !=
                                context->comma_atom)
                    break;
                p_inst_new_functor
                    (code, P_OP_SET_FUNCTOR, reg, term->header.size,
                     term->functor.functor_name);
            }
            p_code_generate_setter(context, term, code);
        } else {
            for (index = 0; index < term->header.size; ++index) {
                arg = p_term_deref(term->functor.arg[index]);
                if (!arg)
                    return 0;
                if (p_code_generate_setter(context, arg, code) &&
                        index < (term->header.size - 1)) {
                    p_inst_new_two_reg
                        (code, P_OP_RESET_ARGUMENT, reg, index + 1);
                }
            }
        }
        p_inst_reg_used(code, reg);

        /* Next level up will need to re-establish the put pointer */
        return 1;
    case P_TERM_LIST:
        /* Set the elements of a list */
        reg = p_inst_new_temp_reg(code);
        inst = p_inst_new(code, P_OP_SET_LIST, struct p_inst_one_reg);
        inst->one_reg.reg1 = reg;
        p_code_generate_list_setter(context, term, code, reg, 0);
        return 1;
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
    case P_TERM_PREDICATE:
    case P_TERM_CLAUSE:
    case P_TERM_DATABASE:
        /* Set the constant value directly */
        inst = p_inst_new
            (code, P_OP_SET_CONSTANT, struct p_inst_constant);
        inst->constant.value = term;
        break;
    case P_TERM_MEMBER_VARIABLE:
        /* Construct a member variable reference term */
        reg = p_code_generate_builder_inner(context, term, code, -1);
        inst = p_inst_new
            (code, P_OP_SET_X_VALUE, struct p_inst_one_reg);
        inst->one_reg.reg1 = reg;
        p_inst_reg_used(code, reg);
        break;
    case P_TERM_X_REGISTER:
        /* Variable that should be placed into an X register */
        if (term->reg.allocated) {
            reg = (int)(term->header.size);
            inst = p_inst_new
                (code, P_OP_SET_X_VALUE, struct p_inst_one_reg);
            inst->one_reg.reg1 = reg;
        } else if (term->reg.usage_count != 1) {
            reg = p_inst_new_reg(code);
            term->header.size = (unsigned int)reg;
            term->reg.allocated = 1;
            inst = p_inst_new
                (code, P_OP_SET_X_VARIABLE, struct p_inst_one_reg);
            inst->one_reg.reg1 = reg;
        } else {
            /* Only one reference, so no need for an X register */
            p_inst_new(code, P_OP_SET_VOID, struct p_inst_header);
        }
        break;
    case P_TERM_Y_REGISTER:
        /* Variable that should be placed into a Y register */
        if (term->reg.allocated) {
            reg = (int)(term->header.size);
            inst = p_inst_new
                (code, P_OP_SET_Y_VALUE, struct p_inst_one_reg);
            inst->one_reg.reg1 = reg;
        } else {
            reg = (code->num_yregs)++;
            term->header.size = (unsigned int)reg;
            term->reg.allocated = 1;
            inst = p_inst_new
                (code, P_OP_SET_Y_VARIABLE, struct p_inst_one_reg);
            inst->one_reg.reg1 = reg;
        }
        break;
    default: break;
    }
    return 0;
}

/* Sets the elements of a list */
static void p_code_generate_list_setter
    (p_context *context, p_term *term, p_code *code,
     int list_reg, int preserve_reg)
{
    p_term *arg;
    int need_reset;
    p_inst *inst;
    int start_reg = list_reg;

    /* Set the head */
    arg = p_term_deref(term->list.head);
    if (!arg)
        return;
    need_reset = p_code_generate_setter(context, arg, code);

    /* Set the remaining list elements */
    term = p_term_deref(term->list.tail);
    while (term && term->header.type == P_TERM_LIST) {
        if (list_reg == start_reg && preserve_reg) {
            /* Need to preserve the original list register so
             * copy it into a temporary for the rest of the list */
            list_reg = p_inst_new_temp_reg(code);
            p_inst_new_two_reg
                (code, P_OP_PUT_X_VALUE, start_reg, list_reg);
        }
        inst = p_inst_new(code, P_OP_SET_LIST_TAIL,
                          struct p_inst_one_reg);
        inst->one_reg.reg1 = list_reg;
        arg = p_term_deref(term->list.head);
        if (!arg)
            return;
        need_reset = p_code_generate_setter(context, arg, code);
        term = p_term_deref(term->list.tail);
    }

    /* Set the tail */
    if (!term || term == context->nil_atom) {
        inst = p_inst_new(code, P_OP_SET_NIL_TAIL,
                          struct p_inst_one_reg);
        inst->one_reg.reg1 = list_reg;
    } else {
        if (need_reset) {
            inst = p_inst_new(code, P_OP_RESET_TAIL,
                              struct p_inst_one_reg);
            inst->one_reg.reg1 = list_reg;
        }
        p_code_generate_setter(context, term, code);
    }

    /* Free the list register if we created a temporary */
    if (list_reg != start_reg)
        p_inst_reg_used(code, list_reg);
}

static int p_code_generate_builder_inner
    (p_context *context, p_term *term, p_code *code, int preferred_reg)
{
    int reg, arg_reg;
    p_inst *inst;
    unsigned int index;
    p_term *arg;
    term = p_term_deref(term);
    if (!term)
        return 0;
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        /* Put the functor onto the heap */
        if (preferred_reg != -1)
            reg = preferred_reg;
        else
            reg = p_inst_new_temp_reg(code);
        p_inst_new_functor
            (code, P_OP_PUT_FUNCTOR, reg, term->header.size,
             term->functor.functor_name);

        /* Build the functor arguments */
        for (index = 0; index < term->header.size; ++index) {
            arg = p_term_deref(term->functor.arg[index]);
            if (!arg)
                return 0;
            if (p_code_generate_setter(context, arg, code) &&
                    index < (term->header.size - 1)) {
                p_inst_new_two_reg
                    (code, P_OP_RESET_ARGUMENT, reg, index + 1);
            }
        }
        break;
    case P_TERM_LIST:
        /* Put the list term onto the heap */
        if (preferred_reg != -1)
            reg = preferred_reg;
        else
            reg = p_inst_new_temp_reg(code);
        inst = p_inst_new(code, P_OP_PUT_LIST, struct p_inst_one_reg);
        inst->one_reg.reg1 = reg;

        /* Set the list elements into place */
        p_code_generate_list_setter(context, term, code, reg, 1);
        break;
    case P_TERM_ATOM:
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
    case P_TERM_PREDICATE:
    case P_TERM_CLAUSE:
    case P_TERM_DATABASE:
        /* Put the constant value directly into a register */
        if (preferred_reg != -1)
            reg = preferred_reg;
        else
            reg = p_inst_new_temp_reg(code);
        inst = p_inst_new
            (code, P_OP_PUT_CONSTANT, struct p_inst_constant);
        inst->constant.reg1 = reg;
        inst->constant.value = term;
        break;
    case P_TERM_MEMBER_VARIABLE:
        /* Construct a member variable reference term */
        arg_reg = p_code_generate_builder_inner
            (context, term->member_var.object, code, -1);
        if (preferred_reg != -1)
            reg = preferred_reg;
        else
            reg = p_inst_new_temp_reg(code);
        p_inst_new_functor
            (code, (term->header.size ?
                        P_OP_PUT_MEMBER_VARIABLE_AUTO :
                        P_OP_PUT_MEMBER_VARIABLE), arg_reg, reg,
             term->member_var.name);
        p_inst_reg_used(code, arg_reg);
        break;
    case P_TERM_X_REGISTER:
        /* This is a variable that is allocated to an X register */
        if (term->reg.allocated) {
            /* Already in an X register - transfer to preferred */
            reg = (int)(term->header.size);
            if (preferred_reg != -1 && preferred_reg != reg) {
                p_inst_new_two_reg
                    (code, P_OP_PUT_X_VALUE, reg, preferred_reg);
            }
            if (preferred_reg != -1)
                reg = preferred_reg;
        } else if (term->reg.usage_count != 1 || preferred_reg == -1) {
            /* Allocate a new X register for the variable */
            reg = p_inst_new_reg(code);
            term->header.size = (unsigned int)reg;
            term->reg.allocated = 1;
            if (preferred_reg != -1) {
                p_inst_new_two_reg
                    (code, P_OP_PUT_X_VARIABLE2, reg, preferred_reg);
                reg = preferred_reg;
            } else {
                inst = p_inst_new
                    (code, P_OP_PUT_X_VARIABLE, struct p_inst_one_reg);
                inst->one_reg.reg1 = reg;
            }
        } else {
            /* Only one reference to the variable, so put it
             * straight into the preferred X register */
            inst = p_inst_new
                (code, P_OP_PUT_X_VARIABLE, struct p_inst_one_reg);
            inst->one_reg.reg1 = preferred_reg;
            reg = preferred_reg;
        }
        break;
    case P_TERM_Y_REGISTER:
        /* This is a variable that is allocated to a Y register.
         * Transfer its value to an X register, preferred or new */
        if (term->reg.allocated) {
            /* Already in a Y register - transfer to an X */
            reg = (int)(term->header.size);
            if (preferred_reg != -1) {
                p_inst_new_two_reg
                    (code, P_OP_PUT_Y_VALUE, reg, preferred_reg);
                reg = preferred_reg;
            } else {
                arg_reg = p_inst_new_temp_reg(code);
                p_inst_new_two_reg
                    (code, P_OP_PUT_Y_VALUE, reg, arg_reg);
                reg = arg_reg;
            }
        } else {
            /* Allocate a new Y register for the variable */
            reg = (code->num_yregs)++;
            term->header.size = (unsigned int)reg;
            term->reg.allocated = 1;
            if (preferred_reg != -1) {
                p_inst_new_two_reg
                    (code, P_OP_PUT_Y_VARIABLE2, reg, preferred_reg);
                reg = preferred_reg;
            } else {
                arg_reg = p_inst_new_temp_reg(code);
                p_inst_new_two_reg
                    (code, P_OP_PUT_Y_VARIABLE2, reg, arg_reg);
                reg = arg_reg;
            }
        }
        break;
    default: return 0;
    }
    return reg;
}

/* Generate a build sequence for a term and return the X register
 * number that the term was placed into.  If "preferred_reg" is
 * not -1, then the value should be placed into that X register */
int _p_code_generate_builder
    (p_context *context, p_term *term, p_code *code, int preferred_reg)
{
    void *marker = p_context_mark_trail(context);
    int reg;
    p_code_analyze_variables(context, term, 0);
    reg = p_code_generate_builder_inner
        (context, term, code, preferred_reg);
    p_context_backtrack_trail(context, marker);
    return reg;
}

/* Generate a "return" instruction to return the value in "reg" */
void _p_code_generate_return(p_code *code, int reg)
{
    p_inst *inst;
    if (reg >= 0) {
        inst = p_inst_new(code, P_OP_RETURN, struct p_inst_one_reg);
        inst->one_reg.reg1 = reg;
    } else {
        p_inst_new(code, P_OP_RETURN_TRUE, struct p_inst_header);
    }
}

static void p_code_generate_list_unifier
    (p_context *context, p_term *term, p_code *code,
     int list_reg, int input_only);

/* Generate a unifier for a functor argument.  The "term" is assumed
 * to have already been dereferenced.  Returns non-zero if the
 * "current match pointer" needs to be re-established at the next
 * level up in the match hierarchy */
static int p_code_generate_unifier
    (p_context *context, p_term *term, p_code *code, int input_only)
{
    p_term *arg;
    p_inst *inst;
    int arg_reg;
    unsigned int index;
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        /* Unify the functor name and arity and copy the functor
         * reference into a new register */
        arg_reg = p_inst_new_temp_reg(code);
        p_inst_new_functor
            (code, input_only ? P_OP_UNIFY_IN_FUNCTOR
                              : P_OP_UNIFY_FUNCTOR,
             arg_reg, term->header.size, term->functor.functor_name);

        /* Unify the arguments */
        for (index = 0; index < term->header.size; ++index) {
            arg = p_term_deref(term->functor.arg[index]);
            if (!arg)
                return 0;
            if (p_code_generate_unifier
                        (context, arg, code, input_only) &&
                    index < (term->header.size - 1)) {
                p_inst_new_two_reg
                    (code, P_OP_RESET_ARGUMENT, arg_reg, index + 1);
            }
        }

        /* Functor is fully unified, so don't need arg_reg any more */
        p_inst_reg_used(code, arg_reg);

        /* Next level up will need to re-establish the match pointer */
        return 1;
    case P_TERM_LIST:
        /* Unify against a list */
        arg_reg = p_inst_new_temp_reg(code);
        inst = p_inst_new
            (code, input_only ? P_OP_UNIFY_IN_LIST : P_OP_UNIFY_LIST,
             struct p_inst_one_reg);
        inst->one_reg.reg1 = arg_reg;
        p_code_generate_list_unifier
            (context, term, code, arg_reg, input_only);
        p_inst_reg_used(code, arg_reg);

        /* Next level up will need to re-establish the match pointer */
        return 1;
    case P_TERM_ATOM:
        /* Unify against an atom value */
        inst = p_inst_new
            (code, input_only ? P_OP_UNIFY_IN_ATOM
                              : P_OP_UNIFY_ATOM,
             struct p_inst_constant);
        inst->constant.value = term;
        break;
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
    case P_TERM_PREDICATE:
    case P_TERM_CLAUSE:
    case P_TERM_DATABASE:
        /* Unify against a constant value */
        inst = p_inst_new
            (code, input_only ? P_OP_UNIFY_IN_CONSTANT
                              : P_OP_UNIFY_CONSTANT,
             struct p_inst_constant);
        inst->constant.value = term;
        break;
    case P_TERM_MEMBER_VARIABLE:
        /* Build the member variable term and then unify against it.
         * The unification will cause the member to be resolved */
        arg_reg = p_code_generate_builder_inner
            (context, term, code, -1);
        inst = p_inst_new
            (code, input_only ? P_OP_UNIFY_IN_X_VALUE
                              : P_OP_UNIFY_X_VALUE,
             struct p_inst_one_reg);
        inst->one_reg.reg1 = arg_reg;
        p_inst_reg_used(code, arg_reg);
        break;
    case P_TERM_X_REGISTER:
        /* Match against a variable that is assigned to an X register */
        if (term->reg.allocated) {
            inst = p_inst_new
                (code, input_only ? P_OP_UNIFY_IN_X_VALUE
                                  : P_OP_UNIFY_X_VALUE,
                 struct p_inst_one_reg);
            inst->one_reg.reg1 = (int)(term->header.size);
        } else if (term->reg.usage_count != 1) {
            arg_reg = p_inst_new_reg(code);
            term->header.size = (unsigned int)arg_reg;
            term->reg.allocated = 1;
            inst = p_inst_new
                (code, P_OP_UNIFY_X_VARIABLE, struct p_inst_one_reg);
            inst->one_reg.reg1 = arg_reg;
        } else {
            /* Only one reference, so unify with an anonymous var */
            p_inst_new(code, input_only ? P_OP_UNIFY_IN_VOID
                            : P_OP_UNIFY_VOID, struct p_inst_header);
        }
        break;
    case P_TERM_Y_REGISTER:
        /* Match against a variable that is assigned to a Y register */
        if (term->reg.allocated) {
            inst = p_inst_new
                (code, input_only ? P_OP_UNIFY_IN_Y_VALUE
                                  : P_OP_UNIFY_Y_VALUE,
                 struct p_inst_one_reg);
            inst->one_reg.reg1 = (int)(term->header.size);
        } else {
            arg_reg = (code->num_yregs)++;
            term->header.size = (unsigned int)arg_reg;
            term->reg.allocated = 1;
            inst = p_inst_new
                (code, P_OP_UNIFY_Y_VARIABLE, struct p_inst_one_reg);
            inst->one_reg.reg1 = arg_reg;
        }
        break;
    default: break;
    }
    return 0;
}

/* Unify the elements of a list */
static void p_code_generate_list_unifier
    (p_context *context, p_term *term, p_code *code,
     int list_reg, int input_only)
{
    p_term *arg;
    int need_reset;
    p_inst *inst;

    /* Unify the head */
    arg = p_term_deref(term->list.head);
    if (!arg)
        return;
    need_reset = p_code_generate_unifier
        (context, arg, code, input_only);

    /* Unify the remaining list elements */
    term = p_term_deref(term->list.tail);
    while (term && term->header.type == P_TERM_LIST) {
        inst = p_inst_new
            (code, input_only ? P_OP_UNIFY_IN_LIST_TAIL
                              : P_OP_UNIFY_LIST_TAIL,
             struct p_inst_one_reg);
        inst->one_reg.reg1 = list_reg;
        arg = p_term_deref(term->list.head);
        if (!arg)
            return;
        need_reset = p_code_generate_unifier
            (context, arg, code, input_only);
        term = p_term_deref(term->list.tail);
    }

    /* Unify the tail */
    if (!term || term == context->nil_atom) {
        if (need_reset) {
            inst = p_inst_new
                (code, input_only ? P_OP_UNIFY_IN_NIL_TAIL
                                  : P_OP_UNIFY_NIL_TAIL,
                 struct p_inst_one_reg);
            inst->one_reg.reg1 = list_reg;
        } else {
            inst = p_inst_new
                (code, input_only ? P_OP_UNIFY_IN_ATOM
                                  : P_OP_UNIFY_ATOM,
                 struct p_inst_constant);
            inst->constant.value = context->nil_atom;
        }
    } else {
        if (need_reset) {
            inst = p_inst_new(code, P_OP_RESET_TAIL,
                              struct p_inst_one_reg);
            inst->one_reg.reg1 = list_reg;
        }
        p_code_generate_unifier(context, term, code, input_only);
    }
}

/* Generate code to match the contents of "reg" against "term" */
static void p_code_generate_matcher_inner
    (p_context *context, p_term *term, p_code *code, int reg,
     int input_only)
{
    int arg_reg;
    p_inst *inst;
    p_term *arg;
    unsigned int index;
    term = p_term_deref(term);
    if (!term)
        return;
    switch (term->header.type) {
    case P_TERM_FUNCTOR:
        /* Match the functor name and arity */
        p_inst_new_functor
            (code, input_only ? P_OP_GET_IN_FUNCTOR : P_OP_GET_FUNCTOR,
             reg, term->header.size, term->functor.functor_name);

        /* Unify the arguments */
        for (index = 0; index < term->header.size; ++index) {
            arg = p_term_deref(term->functor.arg[index]);
            if (!arg)
                return;
            if (p_code_generate_unifier(context, arg, code, input_only) &&
                    index < (term->header.size - 1)) {
                p_inst_new_two_reg
                    (code, P_OP_RESET_ARGUMENT, reg, index + 1);
            }
        }
        break;
    case P_TERM_LIST:
        /* Match a list of elements */
        arg_reg = p_inst_new_temp_reg(code);
        p_inst_new_two_reg
            (code, input_only ? P_OP_GET_IN_LIST : P_OP_GET_LIST,
             reg, arg_reg);
        p_code_generate_list_unifier
            (context, term, code, arg_reg, input_only);
        p_inst_reg_used(code, arg_reg);
        break;
    case P_TERM_ATOM:
        /* Match an atom value */
        inst = p_inst_new
            (code, input_only ? P_OP_GET_IN_ATOM : P_OP_GET_ATOM,
             struct p_inst_constant);
        inst->constant.reg1 = reg;
        inst->constant.value = term;
        break;
    case P_TERM_STRING:
    case P_TERM_INTEGER:
    case P_TERM_REAL:
    case P_TERM_OBJECT:
    case P_TERM_PREDICATE:
    case P_TERM_CLAUSE:
    case P_TERM_DATABASE:
        /* Match a constant value */
        inst = p_inst_new
            (code, input_only ? P_OP_GET_IN_CONSTANT : P_OP_GET_CONSTANT,
             struct p_inst_constant);
        inst->constant.reg1 = reg;
        inst->constant.value = term;
        break;
    case P_TERM_MEMBER_VARIABLE:
        /* Build the member variable term and then unify against it.
         * The unification will cause the member to be resolved */
        arg_reg = p_code_generate_builder_inner
            (context, term, code, -1);
        p_inst_new_two_reg
            (code, input_only ? P_OP_GET_IN_X_VALUE : P_OP_GET_X_VALUE,
             arg_reg, reg);
        break;
    case P_TERM_X_REGISTER:
        /* Match against a variable that is assigned to an X register */
        if (term->reg.allocated) {
            arg_reg = (int)(term->header.size);
            if (reg != arg_reg) {
                p_inst_new_two_reg
                    (code, input_only ? P_OP_GET_IN_X_VALUE
                                      : P_OP_GET_X_VALUE, arg_reg, reg);
            }
        } else if (term->reg.usage_count != 1) {
            /* Allocate a new X register outside of the argument
             * area and copy the value so that it will not be
             * overwritten when generating the next call site.
             * If there is only one usage, then there is no point
             * doing the following as the copy will never be used */
            arg_reg = p_inst_new_reg(code);
            term->header.size = (unsigned int)arg_reg;
            term->reg.allocated = 1;
            p_inst_new_two_reg(code, P_OP_PUT_X_VALUE, reg, arg_reg);
        }
        break;
    case P_TERM_Y_REGISTER:
        /* Match against a variable that is assigned to a Y register */
        if (term->reg.allocated) {
            arg_reg = (int)(term->header.size);
            p_inst_new_two_reg
                (code, input_only ? P_OP_GET_IN_Y_VALUE
                                  : P_OP_GET_Y_VALUE, arg_reg, reg);
        } else {
            arg_reg = (code->num_yregs)++;
            term->header.size = (unsigned int)arg_reg;
            term->reg.allocated = 1;
            p_inst_new_two_reg(code, P_OP_GET_Y_VARIABLE, reg, arg_reg);
        }
        break;
    default: break;
    }
}

void _p_code_generate_matcher
    (p_context *context, p_term *term, p_code *code, int reg,
     int input_only)
{
    void *marker = p_context_mark_trail(context);
    p_code_analyze_variables(context, term, 0);
    p_code_generate_matcher_inner
        (context, term, code, reg, input_only);
    p_context_backtrack_trail(context, marker);
}

/* Generate code for a dynamic clause that matches the "head"
 * and then builds and returns the "body" */
void _p_code_generate_dynamic_clause
    (p_context *context, p_term *head, p_term *body, p_code *code)
{
    void *marker = p_context_mark_trail(context);
    int arity, index, reg;
    p_term *arg;

    /* Assign registers to the variables in the terms.  For dynamic
     * clauses, the body is built straight after matching the head
     * so it is still technically within the head goal's scope */
    p_code_analyze_variables(context, head, 0);
    p_code_analyze_variables(context, body, 0);

    /* Allocate and match the arguments */
    arity = p_term_arg_count(head);
    _p_code_allocate_args(code, arity);
    for (index = 0; index < arity; ++index) {
        arg = p_term_deref(p_term_arg(head, index));
        if (!arg)
            continue;
        if (arg->header.type == P_TERM_FUNCTOR &&
                arg->header.size == 1 &&
                arg->functor.functor_name == context->in_atom) {
            p_code_generate_matcher_inner
                (context, p_term_arg(arg, 0), code, index, 1);
        } else {
            p_code_generate_matcher_inner(context, arg, code, index, 0);
        }
    }

    /* Build the clause body and return it.  If there is no body
     * then succeed without constructing a body term */
    if (body != context->true_atom) {
        reg = p_code_generate_builder_inner(context, body, code, -1);
        _p_code_generate_return(code, reg);
    } else {
        _p_code_generate_return(code, -1);
    }

    /* Backtrack out the register assignments */
    p_context_backtrack_trail(context, marker);
}

p_code *_p_code_new(void)
{
    p_code *code = GC_NEW(p_code);
    code->posn = P_CODE_BLOCK_SIZE;
    return code;
}

void _p_code_finish(p_code *code, p_code_clause *clause)
{
    p_code_block *block;
    p_inst *inst;

    /* Tag the end of the code with P_OP_END for the disassembler */
    block = code->current_block;
    if (block) {
        inst = (p_inst *)(((char *)(block->inst)) + code->posn);
        inst->header.opcode = P_OP_END;
    } else {
        p_inst_new(code, P_OP_END, struct p_inst_header);
    }

    /* Detach the code and create a clause block for it */
    clause->num_xregs = code->num_regs;
    clause->num_yregs = code->num_yregs;
    clause->code = code->first_block;

    /* Clean up and exit */
    if (code->used_regs)
        GC_FREE(code->used_regs);
    if (code->temp_regs)
        GC_FREE(code->temp_regs);
    GC_FREE(code);
}

/** @endcond */
