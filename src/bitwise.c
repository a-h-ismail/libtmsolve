/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include "internals.h"
#include "string_tools.h"
#include "scientific.h"

void get_two_operands(tms_arg_list *args, int64_t *op1, int64_t *op2)
{
    if (_tms_validate_args_count(2, args->count) == false)
    {
        tms_error_bit = 1;
        return;
    }
    *op1 = tms_int_solve(args->arguments[0]);
    *op2 = tms_int_solve(args->arguments[1]);
}

int64_t tms_not(int64_t value)
{
    return (~value) & tms_int_mask;
}

int64_t _tms_rotate_circular(tms_arg_list *args, char direction)
{
    if (_tms_validate_args_count(2, args->count) == false)
    {
        tms_error_bit = 1;
        return -1;
    }
    int64_t value, shift;
    get_two_operands(args, &value, &shift);

    if (tms_error_bit == 1)
        return -1;

    shift %= tms_int_mask_size;
    switch (direction)
    {
    case 'r':
        return (value >> shift | value << (tms_int_mask_size - shift)) & tms_int_mask;

    case 'l':
        return (value << shift | value >> (tms_int_mask_size - shift)) & tms_int_mask;

    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, -1);
        tms_error_bit = 1;
        return -1;
    }
}

int64_t tms_rrc(tms_arg_list *args)
{
    return _tms_rotate_circular(args, 'r');
}

int64_t tms_rlc(tms_arg_list *args)
{
    return _tms_rotate_circular(args, 'l');
}

int64_t tms_rr(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (op1 >> op2) & tms_int_mask;
}

int64_t tms_rl(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (op1 << op2) & tms_int_mask;
}

int64_t tms_nor(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (~(op1 | op2)) & tms_int_mask;
}

int64_t tms_xor(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (op1 ^ op2) & tms_int_mask;
}

int64_t tms_nand(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (~(op1 & op2)) & tms_int_mask;
}

int64_t tms_and(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (op1 & op2) & tms_int_mask;
}

int64_t tms_or(tms_arg_list *args)
{
    int64_t op1, op2;
    get_two_operands(args, &op1, &op2);
    if (tms_error_bit == 1)
        return -1;
    return (op1 | op2) & tms_int_mask;
}
