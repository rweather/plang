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

#ifndef PLANG_INST_PRIV_H
#define PLANG_INST_PRIV_H

#include "term-priv.h"
#include "rbtree-priv.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */

typedef enum {
    P_OP_PUT_X_VARIABLE,
    P_OP_PUT_X_VARIABLE2,
    P_OP_PUT_X_VARIABLE2_LARGE,
    P_OP_PUT_Y_VARIABLE2,
    P_OP_PUT_Y_VARIABLE2_LARGE,
    P_OP_PUT_X_VALUE,
    P_OP_PUT_X_VALUE_LARGE,
    P_OP_PUT_Y_VALUE,
    P_OP_PUT_Y_VALUE_LARGE,
    P_OP_PUT_FUNCTOR,
    P_OP_PUT_FUNCTOR_LARGE,
    P_OP_PUT_LIST,
    P_OP_PUT_CONSTANT,
    P_OP_PUT_MEMBER_VARIABLE,
    P_OP_PUT_MEMBER_VARIABLE_LARGE,
    P_OP_PUT_MEMBER_VARIABLE_AUTO,
    P_OP_PUT_MEMBER_VARIABLE_AUTO_LARGE,

    P_OP_SET_X_VARIABLE,
    P_OP_SET_Y_VARIABLE,
    P_OP_SET_X_VALUE,
    P_OP_SET_Y_VALUE,
    P_OP_SET_FUNCTOR,
    P_OP_SET_FUNCTOR_LARGE,
    P_OP_SET_LIST,
    P_OP_SET_LIST_TAIL,
    P_OP_SET_NIL_TAIL,
    P_OP_SET_CONSTANT,
    P_OP_SET_VOID,

    P_OP_GET_Y_VARIABLE,
    P_OP_GET_Y_VARIABLE_LARGE,
    P_OP_GET_X_VALUE,
    P_OP_GET_X_VALUE_LARGE,
    P_OP_GET_Y_VALUE,
    P_OP_GET_Y_VALUE_LARGE,
    P_OP_GET_FUNCTOR,
    P_OP_GET_FUNCTOR_LARGE,
    P_OP_GET_LIST,
    P_OP_GET_LIST_LARGE,
    P_OP_GET_ATOM,
    P_OP_GET_CONSTANT,

    P_OP_GET_IN_X_VALUE,
    P_OP_GET_IN_X_VALUE_LARGE,
    P_OP_GET_IN_Y_VALUE,
    P_OP_GET_IN_Y_VALUE_LARGE,
    P_OP_GET_IN_FUNCTOR,
    P_OP_GET_IN_FUNCTOR_LARGE,
    P_OP_GET_IN_LIST,
    P_OP_GET_IN_LIST_LARGE,
    P_OP_GET_IN_ATOM,
    P_OP_GET_IN_CONSTANT,

    P_OP_UNIFY_X_VARIABLE,
    P_OP_UNIFY_Y_VARIABLE,
    P_OP_UNIFY_X_VALUE,
    P_OP_UNIFY_Y_VALUE,
    P_OP_UNIFY_FUNCTOR,
    P_OP_UNIFY_FUNCTOR_LARGE,
    P_OP_UNIFY_LIST,
    P_OP_UNIFY_LIST_TAIL,
    P_OP_UNIFY_NIL_TAIL,
    P_OP_UNIFY_ATOM,
    P_OP_UNIFY_CONSTANT,
    P_OP_UNIFY_VOID,

    P_OP_UNIFY_IN_X_VALUE,
    P_OP_UNIFY_IN_Y_VALUE,
    P_OP_UNIFY_IN_FUNCTOR,
    P_OP_UNIFY_IN_FUNCTOR_LARGE,
    P_OP_UNIFY_IN_LIST,
    P_OP_UNIFY_IN_LIST_TAIL,
    P_OP_UNIFY_IN_NIL_TAIL,
    P_OP_UNIFY_IN_ATOM,
    P_OP_UNIFY_IN_CONSTANT,
    P_OP_UNIFY_IN_VOID,

    P_OP_RESET_ARGUMENT,
    P_OP_RESET_ARGUMENT_LARGE,
    P_OP_RESET_TAIL,

    P_OP_JUMP,

    P_OP_PROCEED,
    P_OP_FAIL,
    P_OP_RETURN,
    P_OP_RETURN_TRUE,
    P_OP_THROW,

    P_OP_CALL,
    P_OP_EXECUTE,
    P_OP_TRY_ME_ELSE,

    P_OP_RETRY_ME_ELSE,
    P_OP_TRUST_ME,

    P_OP_NECK_CUT,
    P_OP_GET_LEVEL,
    P_OP_CUT,

    P_OP_END

} p_opcode;

/* Header that appears on all instructions */
struct p_inst_header
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int arg1;
#else
    unsigned int opcode : 8;
    unsigned int arg1   : 24;
#endif
};

/* Instruction that takes a single register argument */
struct p_inst_one_reg
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
#endif
};

/* Instruction that takes two register arguments */
struct p_inst_two_reg
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1   : 16;
    unsigned int reg2   : 16;
#define P_MAX_SMALL_REG 65535
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 12;
    unsigned int reg2   : 12;
#define P_MAX_SMALL_REG 4095
#endif
};

/* Instruction that takes two register arguments, where at least
 * one of them is larger than P_INST_MAX_SMALL_REG */
struct p_inst_large_two_reg
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
    unsigned int reg2;
    unsigned int pad;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
    unsigned int reg2;
#endif
};

/* Instruction that takes a register, functor name, and arity */
struct p_inst_functor
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1   : 16;
    unsigned int arity  : 16;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 12;
    unsigned int arity  : 12;
#endif
    p_term *name;
};

/* Large version of functor instructions */
struct p_inst_large_functor
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
    unsigned int arity;
    unsigned int pad;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
    unsigned int arity;
#endif
    p_term *name;
};

/* Instruction that sets a functor argument to a register value */
struct p_inst_set_value
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
    unsigned int index;
    unsigned int reg2;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
    unsigned int index;
    unsigned int reg2;
#endif
};

/* Instruction that refers to a constant (atom, number, string) */
struct p_inst_constant
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
#endif
    p_term *value;
};

/* Instruction that refers to a label */
struct p_inst_label
{
#if defined(P_TERM_64BIT)
    unsigned int opcode;
    unsigned int reg1;
#else
    unsigned int opcode : 8;
    unsigned int reg1   : 24;
#endif
    p_inst *label;
};

union p_inst
{
    struct p_inst_header        header;
    struct p_inst_one_reg       one_reg;
    struct p_inst_two_reg       two_reg;
    struct p_inst_large_two_reg large_two_reg;
    struct p_inst_functor       functor;
    struct p_inst_large_functor large_functor;
    struct p_inst_set_value     set_value;
    struct p_inst_constant      constant;
    struct p_inst_label         label;
};

#define P_CODE_BLOCK_WORDS      64
#define P_CODE_BLOCK_SIZE       (P_CODE_BLOCK_WORDS * sizeof(void *))

typedef struct p_code_block p_code_block;
struct p_code_block
{
    void *inst[P_CODE_BLOCK_WORDS];
};

typedef struct p_code p_code;
struct p_code
{
    p_code_block *first_block;
    p_code_block *current_block;
    size_t posn;
    size_t first_block_size;

    unsigned int *used_regs;
    unsigned int *temp_regs;
    int num_regs;
    int max_regs;
    int blocked_regs;

    int num_yregs;

    int force_large_regs;
};

void _p_code_allocate_args(p_code *code, int arity);
int _p_code_generate_builder
    (p_context *context, p_term *term, p_code *code, int preferred_reg);
void _p_code_generate_return(p_code *code, int reg);
void _p_code_generate_matcher
    (p_context *context, p_term *term, p_code *code, int reg,
     int input_only);
void _p_code_generate_dynamic_clause
    (p_context *context, p_term *head, p_term *body, p_code *code);

p_code *_p_code_new(void);
void _p_code_finish(p_code *code, p_code_clause *clause);

void _p_code_set_xreg(p_context *context, int reg, p_term *value);
p_goal_result _p_code_run
    (p_context *context, const p_code_clause *clause, p_term **error);

void _p_code_disassemble
    (FILE *output, p_context *context, const p_code_clause *clause);
int _p_code_argument_key
    (p_rbkey *key, const p_code_clause *clause, unsigned int arg);

/** @endcond */

#ifdef __cplusplus
};
#endif

#endif
