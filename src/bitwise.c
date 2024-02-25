/*
Copyright (C) 2023-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include "bitwise.h"
#include "internals.h"
#include "scientific.h"
#include "string_tools.h"

#include <stdlib.h>
#include <string.h>

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
    if (_tms_validate_args_count(2, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int status;
    status = _tms_int_solve_unsafe(args->arguments[0], op1);
    if (status != 0)
        return -1;

    status = _tms_int_solve_unsafe(args->arguments[1], op2);
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

int tms_mask(int64_t bits, int64_t *result)
{
    if (bits < 0)
        return tms_inv_mask(-bits, result);

    if (bits > 63)
    {
        *result = ~(int64_t)0;
        return 0;
    }
    else
    // Shift a 1 by n bits then subtract by 1 to get the mask
    {
        *result = ((int64_t)1 << bits) - 1;
        return 0;
    }
}

// Just invert what you get by the regular mask
int tms_inv_mask(int64_t bits, int64_t *result)
{
    if (tms_mask(bits, result) != 0)
        return -1;
    else
    {
        *result = ~*result;
        return 0;
    }
}

int _tms_rotate_circular(tms_arg_list *args, char direction, int64_t *result)
{
    if (_tms_validate_args_count(2, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int64_t value, shift;
    if (get_two_operands(args, &value, &shift) == -1)
        return -1;

    shift = tms_sign_extend(shift);
    if (shift < 0)
    {
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, ROTATION_AMOUNT_NEGATIVE, EH_FATAL, NULL);
        return -1;
    }

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
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL);
        return -1;
    }
}

int tms_rr(tms_arg_list *args, int64_t *result)
{
    return _tms_rotate_circular(args, 'r', result);
}

int tms_rl(tms_arg_list *args, int64_t *result)
{
    return _tms_rotate_circular(args, 'l', result);
}

int tms_sr(tms_arg_list *args, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift) == -1)
        return -1;
    else
    {
        shift = tms_sign_extend(shift);
        if (shift < 0)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_AMOUNT_NEGATIVE, EH_FATAL, NULL);
            return -1;
        }
        else if (shift >= tms_int_mask_size)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_TOO_LARGE, EH_FATAL, NULL);
            return -1;
        }
        // The cast to unsigned is necessary to avoid right shift sign extending
        *result = ((uint64_t)value >> shift) & tms_int_mask;
        return 0;
    }
}

int tms_sra(tms_arg_list *args, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift) == -1)
        return -1;
    else
    {
        shift = tms_sign_extend(shift);
        if (shift < 0)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_AMOUNT_NEGATIVE, EH_FATAL, NULL);
            return -1;
        }
        else if (shift >= tms_int_mask_size)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_TOO_LARGE, EH_FATAL, NULL);
            return -1;
        }
        value = tms_sign_extend(value);
        *result = (value >> shift) & tms_int_mask;
        return 0;
    }
}

int tms_sl(tms_arg_list *args, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift) == -1)
        return -1;
    else
    {
        shift = tms_sign_extend(shift);
        if (shift < 0)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_AMOUNT_NEGATIVE, EH_FATAL, NULL);
            return -1;
        }
        if (shift >= tms_int_mask_size)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SHIFT_TOO_LARGE, EH_FATAL, NULL);
            return -1;
        }
        *result = ((uint64_t)value << shift) & tms_int_mask;
        return 0;
    }
}

int tms_nor(tms_arg_list *args, int64_t *result)
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

int tms_xor(tms_arg_list *args, int64_t *result)
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

int tms_nand(tms_arg_list *args, int64_t *result)
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

int tms_and(tms_arg_list *args, int64_t *result)
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

int tms_or(tms_arg_list *args, int64_t *result)
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

// Receives an arg list of octets, calculates the value corresponding to the dot decimal
int _tms_calculate_dot_decimal(tms_arg_list *L, int64_t *result)
{
    int i, status;
    int64_t tmp;
    *result = 0;
    if (L->count == 0)
        return -1;

    for (i = L->count - 1; i >= 0; --i)
    {
        status = _tms_read_int_helper(L->arguments[i], 10, &tmp);
        if (status == -1)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, SYNTAX_ERROR, EH_FATAL, NULL);
            return -1;
        }
        else if (tmp > 255 || tmp < 0)
        {
            tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, NOT_A_DOT_DECIMAL, EH_FATAL, NULL);
            return -1;
        }
        else
            // Dotted decimal here is read left to right, thus the right shift
            *result = *result | (tmp << (8 * (L->count - 1 - i)));
    }
    return 0;
}

int tms_dotted(tms_arg_list *args, int64_t *result)
{
    if (_tms_validate_args_count(1, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int status;
    char *input = args->arguments[0];

    // Replace the dots with commas to use get_args function
    while ((input = strchr(input, '.')) != NULL)
        *input = ',';

    // Get the beginning of the pointer back
    input = args->arguments[0];
    tms_arg_list *L = tms_get_args(input);

    status = _tms_calculate_dot_decimal(L, result);
    tms_free_arg_list(L);
    return status;
}

int tms_ipv4(tms_arg_list *args, int64_t *result)
{
    if (_tms_validate_args_count(1, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    if (tms_int_mask_size != 32)
    {
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, NOT_AN_IPV4_SIZE, EH_FATAL, NULL);
        return -1;
    }

    int status;
    char *input = args->arguments[0];

    // Replace the dots with commas to use get_args function
    while ((input = strchr(input, '.')) != NULL)
        *input = ',';

    // Get the beginning of the pointer back
    input = args->arguments[0];
    tms_arg_list *L = tms_get_args(input);

    if (L->count != 4)
    {
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, NOT_A_VALID_IPV4, EH_FATAL, NULL);
        return -1;
    }

    status = _tms_calculate_dot_decimal(L, result);
    tms_free_arg_list(L);
    return status;
}

int tms_ipv4_prefix(int64_t length, int64_t *result)
{
    if (tms_int_mask_size != 32)
    {
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, NOT_AN_IPV4_SIZE, EH_FATAL, NULL);
        return -1;
    }

    if (length < 0 || length > 32)
    {
        tms_error_handler(EH_SAVE, TMS_INT_EVALUATOR, NOT_A_VALID_IPV4_PREFIX, EH_FATAL, NULL);
        return -1;
    }

    return tms_inv_mask(32 - length, result);
}

int tms_zeros(int64_t value, int64_t *result)
{
    int zeros_count = 0;
    for (int i = 0; i < tms_int_mask_size; ++i)
    {
        if ((value & 1) == 0)
            ++zeros_count;
        value = (uint64_t)value >> 1;
    }
    *result = zeros_count;
    return 0;
}

int tms_ones(int64_t value, int64_t *result)
{
    int ones_count = 0;
    for (int i = 0; i < tms_int_mask_size; ++i)
    {
        if ((value & 1) == 1)
            ++ones_count;
        value = (uint64_t)value >> 1;
    }
    *result = ones_count;
    return 0;
}
