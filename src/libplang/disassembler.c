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

#include "inst-priv.h"

/** @cond */

enum {
    P_ARG_NONE,
    P_ARG_X,
    P_ARG_Y,
    P_ARG_X_X,
    P_ARG_Y_X,
    P_ARG_X_Y,
    P_ARG_X_X_LARGE,
    P_ARG_Y_X_LARGE,
    P_ARG_X_Y_LARGE,
    P_ARG_FUNCTOR,
    P_ARG_FUNCTOR_LARGE,
    P_ARG_CONSTANT,
    P_ARG_CONSTANT_X,
    P_ARG_MEMBER,
    P_ARG_MEMBER_LARGE,
    P_ARG_RESET,
    P_ARG_RESET_LARGE,
    P_ARG_LABEL
};

enum {
    P_TYPE_GET,
    P_TYPE_STOP,
    P_TYPE_SKIP
};

typedef struct p_inst_info p_inst_info;
struct p_inst_info
{
    const char *name;
#if defined(P_TERM_64BIT)
    int arg_types;
    int get_put_type;
#else
    short arg_types;
    short get_put_type;
#endif
};
static p_inst_info const instructions[] = {
    {"put_variable",                P_ARG_X, P_TYPE_SKIP},
    {"put_variable2",               P_ARG_X_X, P_TYPE_SKIP},
    {"put_variable2_large",         P_ARG_X_X_LARGE, P_TYPE_SKIP},
    {"put_variable2",               P_ARG_Y_X, P_TYPE_SKIP},
    {"put_variable2_large",         P_ARG_Y_X_LARGE, P_TYPE_SKIP},
    {"put_value",                   P_ARG_X_X, P_TYPE_GET},
    {"put_value_large",             P_ARG_X_X_LARGE, P_TYPE_GET},
    {"put_value",                   P_ARG_Y_X, P_TYPE_SKIP},
    {"put_value_large",             P_ARG_Y_X_LARGE, P_TYPE_SKIP},
    {"put_functor",                 P_ARG_FUNCTOR, P_TYPE_STOP},
    {"put_functor_large",           P_ARG_FUNCTOR_LARGE, P_TYPE_STOP},
    {"put_list",                    P_ARG_X, P_TYPE_STOP},
    {"put_constant",                P_ARG_CONSTANT_X, P_TYPE_STOP},
    {"put_member_variable",         P_ARG_MEMBER, P_TYPE_SKIP},
    {"put_member_variable_large",   P_ARG_MEMBER_LARGE, P_TYPE_SKIP},
    {"put_member_variable_auto",    P_ARG_MEMBER, P_TYPE_SKIP},
    {"put_member_variable_auto_large", P_ARG_MEMBER_LARGE, P_TYPE_SKIP},

    {"set_variable",                P_ARG_X, P_TYPE_STOP},
    {"set_variable",                P_ARG_Y, P_TYPE_STOP},
    {"set_value",                   P_ARG_X, P_TYPE_STOP},
    {"set_value",                   P_ARG_Y, P_TYPE_STOP},
    {"set_functor",                 P_ARG_FUNCTOR, P_TYPE_STOP},
    {"set_functor_large",           P_ARG_FUNCTOR_LARGE, P_TYPE_STOP},
    {"set_list",                    P_ARG_X, P_TYPE_STOP},
    {"set_list_tail",               P_ARG_X, P_TYPE_STOP},
    {"set_nil_tail",                P_ARG_X, P_TYPE_STOP},
    {"set_constant",                P_ARG_CONSTANT, P_TYPE_STOP},
    {"set_void",                    P_ARG_NONE, P_TYPE_STOP},

    {"get_variable",                P_ARG_X_Y, P_TYPE_GET},
    {"get_variable_large",          P_ARG_X_Y_LARGE, P_TYPE_GET},
    {"get_value",                   P_ARG_X_X, P_TYPE_GET},
    {"get_value_large",             P_ARG_X_X_LARGE, P_TYPE_GET},
    {"get_value",                   P_ARG_Y_X, P_TYPE_GET},
    {"get_value_large",             P_ARG_Y_X_LARGE, P_TYPE_GET},
    {"get_functor",                 P_ARG_FUNCTOR, P_TYPE_GET},
    {"get_functor_large",           P_ARG_FUNCTOR_LARGE, P_TYPE_GET},
    {"get_list",                    P_ARG_X_X, P_TYPE_GET},
    {"get_list_large",              P_ARG_X_X_LARGE, P_TYPE_GET},
    {"get_atom",                    P_ARG_CONSTANT_X, P_TYPE_GET},
    {"get_constant",                P_ARG_CONSTANT_X, P_TYPE_GET},

    {"get_in_value",                P_ARG_X_X, P_TYPE_GET},
    {"get_in_value_large",          P_ARG_X_X_LARGE, P_TYPE_GET},
    {"get_in_value",                P_ARG_Y_X, P_TYPE_GET},
    {"get_in_value_large",          P_ARG_Y_X_LARGE, P_TYPE_GET},
    {"get_in_functor",              P_ARG_FUNCTOR, P_TYPE_GET},
    {"get_in_functor_large",        P_ARG_FUNCTOR_LARGE, P_TYPE_GET},
    {"get_in_list",                 P_ARG_X_X, P_TYPE_GET},
    {"get_in_list_large",           P_ARG_X_X_LARGE, P_TYPE_GET},
    {"get_in_atom",                 P_ARG_CONSTANT_X, P_TYPE_GET},
    {"get_in_constant",             P_ARG_CONSTANT_X, P_TYPE_GET},

    {"unify_variable",              P_ARG_X, P_TYPE_SKIP},
    {"unify_variable",              P_ARG_Y, P_TYPE_SKIP},
    {"unify_value",                 P_ARG_X, P_TYPE_SKIP},
    {"unify_value",                 P_ARG_Y, P_TYPE_SKIP},
    {"unify_functor",               P_ARG_FUNCTOR, P_TYPE_SKIP},
    {"unify_functor_large",         P_ARG_FUNCTOR_LARGE, P_TYPE_SKIP},
    {"unify_list",                  P_ARG_X, P_TYPE_SKIP},
    {"unify_list_tail",             P_ARG_X, P_TYPE_SKIP},
    {"unify_nil_tail",              P_ARG_X, P_TYPE_SKIP},
    {"unify_atom",                  P_ARG_CONSTANT, P_TYPE_SKIP},
    {"unify_constant",              P_ARG_CONSTANT, P_TYPE_SKIP},
    {"unify_void",                  P_ARG_NONE, P_TYPE_SKIP},

    {"unify_in_value",              P_ARG_X, P_TYPE_SKIP},
    {"unify_in_value",              P_ARG_Y, P_TYPE_SKIP},
    {"unify_in_functor",            P_ARG_FUNCTOR, P_TYPE_SKIP},
    {"unify_in_functor_large",      P_ARG_FUNCTOR_LARGE, P_TYPE_SKIP},
    {"unify_in_list",               P_ARG_X, P_TYPE_SKIP},
    {"unify_in_list_tail",          P_ARG_X, P_TYPE_SKIP},
    {"unify_in_nil_tail",           P_ARG_X, P_TYPE_SKIP},
    {"unify_in_atom",               P_ARG_CONSTANT, P_TYPE_SKIP},
    {"unify_in_constant",           P_ARG_CONSTANT, P_TYPE_SKIP},
    {"unify_in_void",               P_ARG_NONE, P_TYPE_SKIP},

    {"reset_argument",              P_ARG_RESET, P_TYPE_SKIP},
    {"reset_argument_large",        P_ARG_RESET_LARGE, P_TYPE_SKIP},
    {"reset_tail",                  P_ARG_X, P_TYPE_SKIP},

    {"jump",                        P_ARG_LABEL, P_TYPE_SKIP},

    {"proceed",                     P_ARG_NONE, P_TYPE_STOP},
    {"fail",                        P_ARG_NONE, P_TYPE_STOP},
    {"return",                      P_ARG_X, P_TYPE_STOP},
    {"return_true",                 P_ARG_NONE, P_TYPE_STOP},
    {"throw",                       P_ARG_X, P_TYPE_STOP},

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

    {"end",                         P_ARG_NONE, P_TYPE_STOP}
};

void _p_code_disassemble
    (FILE *output, p_context *context, const p_code_clause *clause)
{
    p_opcode opcode;
    size_t size;
    const p_inst *inst = (p_inst *)(clause->code->inst);
    for (;;) {
        opcode = (p_opcode)(inst->header.opcode);
        if (opcode == P_OP_JUMP) {
            /* Jump to next continuation code block - not important */
            inst = inst->label.label;
            continue;
        } else if (opcode == P_OP_END) {
            /* We've reached the end of the predicate code */
            break;
        }
        fprintf(output, "%08lx: %s", (long)inst,
                instructions[opcode].name);
        switch (instructions[opcode].arg_types) {
        case P_ARG_NONE:
        default:
            size = sizeof(inst->header);
            break;
        case P_ARG_X:
            fprintf(output, " X%u", inst->one_reg.reg1);
            size = sizeof(inst->one_reg);
            break;
        case P_ARG_Y:
            fprintf(output, " Y%u", inst->one_reg.reg1);
            size = sizeof(inst->one_reg);
            break;
        case P_ARG_X_X:
            fprintf(output, " X%u, X%u", inst->two_reg.reg1,
                    inst->two_reg.reg2);
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_Y_X:
            fprintf(output, " Y%u, X%u", inst->two_reg.reg1,
                    inst->two_reg.reg2);
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_X_Y:
            fprintf(output, " X%u, Y%u", inst->two_reg.reg1,
                    inst->two_reg.reg2);
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_X_X_LARGE:
            fprintf(output, " X%u, X%u", inst->large_two_reg.reg1,
                    inst->large_two_reg.reg2);
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_Y_X_LARGE:
            fprintf(output, " Y%u, X%u", inst->large_two_reg.reg1,
                    inst->large_two_reg.reg2);
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_X_Y_LARGE:
            fprintf(output, " X%u, Y%u", inst->large_two_reg.reg1,
                    inst->large_two_reg.reg2);
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_FUNCTOR:
            putc(' ', output);
            p_term_print(context, inst->functor.name,
                         p_term_stdio_print_func, output);
            fprintf(output, "/%u, X%u",
                    inst->functor.arity, inst->functor.reg1);
            size = sizeof(inst->functor);
            break;
        case P_ARG_FUNCTOR_LARGE:
            putc(' ', output);
            p_term_print(context, inst->large_functor.name,
                         p_term_stdio_print_func, output);
            fprintf(output, "/%u, X%u",
                    inst->large_functor.arity,
                    inst->large_functor.reg1);
            size = sizeof(inst->large_functor);
            break;
        case P_ARG_CONSTANT:
            putc(' ', output);
            p_term_print(context, inst->constant.value,
                         p_term_stdio_print_func, output);
            size = sizeof(inst->constant);
            break;
        case P_ARG_CONSTANT_X:
            putc(' ', output);
            p_term_print(context, inst->constant.value,
                         p_term_stdio_print_func, output);
            fprintf(output, ", X%u", inst->constant.reg1);
            size = sizeof(inst->constant);
            break;
        case P_ARG_MEMBER:
            fprintf(output, " X%u, ", inst->functor.reg1);
            p_term_print(context, inst->functor.name,
                         p_term_stdio_print_func, output);
            fprintf(output, ", X%u", inst->functor.arity);
            size = sizeof(inst->functor);
            break;
        case P_ARG_MEMBER_LARGE:
            fprintf(output, " X%u, ", inst->large_functor.reg1);
            p_term_print(context, inst->large_functor.name,
                         p_term_stdio_print_func, output);
            fprintf(output, ", X%u", inst->large_functor.arity);
            size = sizeof(inst->large_functor);
            break;
        case P_ARG_RESET:
            fprintf(output, " X%u, %u", inst->two_reg.reg1,
                    inst->two_reg.reg2);
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_RESET_LARGE:
            fprintf(output, " X%u, %u", inst->large_two_reg.reg1,
                    inst->large_two_reg.reg2);
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_LABEL:
            fprintf(output, " %08lx", (long)(inst->label.label));
            size = sizeof(inst->label);
            break;
        }
        putc('\n', output);
        inst = (p_inst *)(((char *)inst) + size);
    }
}

/* Extract the red-black key for a specific "get" argument */
int _p_code_argument_key
    (p_rbkey *key, const p_code_clause *clause, unsigned int arg)
{
    p_opcode opcode;
    size_t size;
    const p_inst *inst = (p_inst *)(clause->code->inst);
    for (;;) {
        opcode = (p_opcode)(inst->header.opcode);
        if (opcode == P_OP_JUMP) {
            /* Jump to next continuation code block */
            inst = inst->label.label;
            continue;
        } else if (opcode == P_OP_END) {
            /* We've reached the end of the predicate code */
            break;
        }
        switch (instructions[opcode].get_put_type) {
        case P_TYPE_GET:
            switch (opcode) {

            /* Variable arguments, which aren't indexable */
            case P_OP_PUT_X_VALUE:  /* Same as get_x_variable */
            case P_OP_GET_Y_VARIABLE:
                if (inst->two_reg.reg1 == arg)
                    return 0;
                break;
            case P_OP_PUT_X_VALUE_LARGE:
            case P_OP_GET_Y_VARIABLE_LARGE:
                if (inst->large_two_reg.reg1 == arg)
                    return 0;
                break;
            case P_OP_GET_X_VALUE:
            case P_OP_GET_Y_VALUE:
            case P_OP_GET_IN_X_VALUE:
            case P_OP_GET_IN_Y_VALUE:
                if (inst->two_reg.reg2 == arg)
                    return 0;
                break;
            case P_OP_GET_X_VALUE_LARGE:
            case P_OP_GET_Y_VALUE_LARGE:
            case P_OP_GET_IN_X_VALUE_LARGE:
            case P_OP_GET_IN_Y_VALUE_LARGE:
                if (inst->large_two_reg.reg2 == arg)
                    return 0;
                break;

            case P_OP_GET_FUNCTOR:
            case P_OP_GET_IN_FUNCTOR:
                if (inst->functor.reg1 != arg)
                    break;
                key->type = P_TERM_FUNCTOR;
                key->size = inst->functor.arity;
                key->name = inst->functor.name;
                return 1;
            case P_OP_GET_FUNCTOR_LARGE:
            case P_OP_GET_IN_FUNCTOR_LARGE:
                if (inst->large_functor.reg1 != arg)
                    break;
                key->type = P_TERM_FUNCTOR;
                key->size = inst->large_functor.arity;
                key->name = inst->large_functor.name;
                return 1;
            case P_OP_GET_LIST:
            case P_OP_GET_IN_LIST:
                if (inst->two_reg.reg1 != arg)
                    break;
                key->type = P_TERM_LIST;
                key->size = 0;
                key->name = 0;
                return 1;
            case P_OP_GET_LIST_LARGE:
            case P_OP_GET_IN_LIST_LARGE:
                if (inst->large_two_reg.reg1 != arg)
                    break;
                key->type = P_TERM_LIST;
                key->size = 0;
                key->name = 0;
                return 1;
            case P_OP_GET_ATOM:
            case P_OP_GET_IN_ATOM:
                if (inst->constant.reg1 != arg)
                    break;
                key->type = P_TERM_ATOM;
                key->size = 0;
                key->name = inst->constant.value;
                return 1;
            case P_OP_GET_CONSTANT:
            case P_OP_GET_IN_CONSTANT:
                if (inst->constant.reg1 != arg)
                    break;
                key->type = inst->constant.value->header.type;
#if defined(P_TERM_64BIT)
                if (key->type == P_TERM_INTEGER) {
                    key->size = p_term_integer_value
                        (inst->constant.value);
                    key->name = 0;
                } else
#endif
                {
                    key->size = 0;
                    key->name = inst->constant.value;
                }
                return 1;
            default: break;
            }
            break;

        case P_TYPE_STOP:
            /* No possibility of get instructions beyond this point */
            return 0;

        case P_TYPE_SKIP: break;
        }
        switch (instructions[opcode].arg_types) {
        case P_ARG_NONE:
        default:
            size = sizeof(inst->header);
            break;
        case P_ARG_X:
            size = sizeof(inst->one_reg);
            break;
        case P_ARG_Y:
            size = sizeof(inst->one_reg);
            break;
        case P_ARG_X_X:
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_Y_X:
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_X_Y:
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_X_X_LARGE:
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_Y_X_LARGE:
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_X_Y_LARGE:
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_FUNCTOR:
            size = sizeof(inst->functor);
            break;
        case P_ARG_FUNCTOR_LARGE:
            size = sizeof(inst->large_functor);
            break;
        case P_ARG_CONSTANT:
            size = sizeof(inst->constant);
            break;
        case P_ARG_CONSTANT_X:
            size = sizeof(inst->constant);
            break;
        case P_ARG_MEMBER:
            size = sizeof(inst->functor);
            break;
        case P_ARG_MEMBER_LARGE:
            size = sizeof(inst->large_functor);
            break;
        case P_ARG_RESET:
            size = sizeof(inst->two_reg);
            break;
        case P_ARG_RESET_LARGE:
            size = sizeof(inst->large_two_reg);
            break;
        case P_ARG_LABEL:
            size = sizeof(inst->label);
            break;
        }
        inst = (p_inst *)(((char *)inst) + size);
    }
    return 0;
}

/** @endcond */
