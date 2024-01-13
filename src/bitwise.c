/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include "internals.h"
#include "string_tools.h"
#include "scientific.h"

int64_t tms_sign_extend(int64_t value)
{
    uint64_t inverse_mask = ~tms_int_mask;

    // Check the MSB relative to the current width (by masking the sign bit only)
    if ((((uint64_t)1 << (tms_int_mask_size - 1)) & value) != 0)
        return value | inverse_mask;
    else
        return value;
}

int get_two_operands(tms_arg_list *args, int64_t *op1, int64_t *op2)
{
    if (_tms_validate_args_count(2, args->count) == false)
        return -1;

    int status;
    status = tms_int_solve(args->arguments[0], op1);
    if (status != 0)
        return -1;

    status = tms_int_solve(args->arguments[1], op2);
    if (status != 0)
        return -1;
    else
        return 0;
}

int tms_not(int64_t value, int64_t *result)
{
    *result = (~value) & tms_int_mask;
    return 0;
}

int _tms_rotate_circular(tms_arg_list *args, char direction, int64_t *result)
{
    if (_tms_validate_args_count(2, args->count) == false)
        return -1;

    int64_t value, shift;
    if (get_two_operands(args, &value, &shift) == -1)
        return -1;

    shift %= tms_int_mask_size;
    switch (direction)
    {
    case 'r':
        *result = ((uint64_t)value >> shift | (uint64_t)value << (tms_int_mask_size - shift)) & tms_int_mask;
        return 0;

    case 'l':
        *result = ((uint64_t)value << shift | (uint64_t)value >> (tms_int_mask_size - shift)) & tms_int_mask;
        return 0;

    default:
        tms_error_handler(EH_SAVE, INTERNAL_ERROR, EH_FATAL_ERROR, NULL);
        return -1;
    }
}

int64_t tms_rr(tms_arg_list *args, int64_t *result)
{
    return _tms_rotate_circular(args, 'r', result);
}

int64_t tms_rl(tms_arg_list *args, int64_t *result)
{
    return _tms_rotate_circular(args, 'l', result);
}

int64_t tms_sr(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        // The cast to unsigned is necessary to avoid right shift sign extending
        *result = ((uint64_t)op1 >> op2) & tms_int_mask;
        return 0;
    }
}

int64_t tms_sra(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        op1 = tms_sign_extend(op1);
        *result = (op1 >> op2) & tms_int_mask;
        return 0;
    }
}

int64_t tms_sl(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (op1 << op2) & tms_int_mask;
        return 0;
    }
}

int64_t tms_nor(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (~(op1 | op2)) & tms_int_mask;
        return 0;
    }
}

int64_t tms_xor(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (op1 ^ op2) & tms_int_mask;
        return 0;
    }
}

int64_t tms_nand(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (~(op1 & op2)) & tms_int_mask;
        return 0;
    }
}

int64_t tms_and(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (op1 & op2) & tms_int_mask;
        return 0;
    }
}

int64_t tms_or(tms_arg_list *args, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2) == -1)
        return -1;
    else
    {
        *result = (op1 | op2) & tms_int_mask;
        return 0;
    }
}
