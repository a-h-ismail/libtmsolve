/*
Copyright (C) 2023-2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#include "bitwise.h"
#include "error_handler.h"
#include "internals.h"
#include "m_errors.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_math_strs.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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

int get_two_operands(tms_arg_list *args, int64_t *op1, int64_t *op2, tms_arg_list *labels)
{
    if (_tms_validate_args_count(2, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int status;
    status = tms_int_solve_e(args->arguments[0], op1, NO_LOCK, labels);
    if (status != 0)
        return -1;

    status = tms_int_solve_e(args->arguments[1], op2, NO_LOCK, labels);
    if (status != 0)
        return -1;
    else
        return 0;
}

int get_all_arguments(tms_arg_list *args, int64_t **numerical_args, tms_arg_list *labels)
{
    *numerical_args = malloc(args->count * sizeof(int64_t));
    int status;
    for (int i = 0; i < args->count; ++i)
    {
        status = tms_int_solve_e(args->arguments[i], *numerical_args + i, NO_LOCK, labels);
        if (status != 0)
        {
            free(*numerical_args);
            return -1;
        }
    }
    return 0;
}

int tms_not(int64_t value, int64_t *result)
{
    *result = ~value;
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

int tms_mask_bit(int64_t bit, int64_t *result)
{
    if (bit < 0 || bit >= tms_int_mask_size)
    {
        tms_save_error(TMS_INT_EVALUATOR, BIT_OUT_OF_RANGE, EH_FATAL, NULL, 0);
        return -1;
    }

    *result = (int64_t)1 << bit;
    return 0;
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

int _tms_rotate_circular_i(int64_t value, int64_t shift, char direction, int64_t *result)
{
    value &= tms_int_mask;
    if (shift < 0)
    {
        tms_save_error(TMS_INT_EVALUATOR, ROTATION_AMOUNT_NEGATIVE, EH_FATAL, NULL, 0);
        return -1;
    }

    shift %= tms_int_mask_size;
    switch (direction)
    {
    case 'r':
        *result = ((uint64_t)value >> shift | (uint64_t)value << (tms_int_mask_size - shift));
        break;

    case 'l':
        *result = ((uint64_t)value << shift | (uint64_t)value >> (tms_int_mask_size - shift));
        break;

    default:
        tms_save_error(TMS_INT_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL, 0);
        return -1;
    }
    // Mask away any additional bits to the left due to shifting, then sign extend
    *result &= tms_int_mask;
    *result = tms_sign_extend(*result);
    return 0;
}

int _tms_rotate_circular(tms_arg_list *args, char direction, int64_t *result, tms_arg_list *labels)
{
    if (_tms_validate_args_count(2, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int64_t value, shift;
    if (get_two_operands(args, &value, &shift, labels) == -1)
        return -1;

    return _tms_rotate_circular_i(value, shift, direction, result);
}

int _tms_int_rand(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count_range(args->count, 0, 2, TMS_INT_EVALUATOR) == false)
        return -1;

    int64_t random_64 = 0;
    // Generate an 64 bit random number
    for (int i = 0; i < 8 / sizeof(int); ++i)
        random_64 |= (int64_t)rand() << (i * 8 * sizeof(int));

    // Mask then sign extend to ensure that it fits the current size
    random_64 = tms_sign_extend(tms_int_mask & random_64);

    if (args->count == 0)
    {
        *result = random_64;
        return 0;
    }
    else if (args->count == 1)
    {
        tms_save_error(TMS_INT_EVALUATOR, INCOMPLETE_RANGE, EH_FATAL, NULL, 0);
        return -1;
    }
    else if (args->count == 2)
    {
        int64_t min, max;
        if (tms_int_solve_e(args->arguments[0], &min, NO_LOCK, labels) != 0 ||
            tms_int_solve_e(args->arguments[1], &max, NO_LOCK, labels) != 0)
            return -1;

        if (min >= max)
        {
            tms_save_error(TMS_INT_EVALUATOR, INVALID_RANGE, EH_FATAL, NULL, 0);
            return -1;
        }
        // The int mask is the largest value for the current int width
        *result = (random_64 % (max - min + 1)) + min;
        return 0;
    }
    return -1;
}

int _tms_rr(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    return _tms_rotate_circular(args, 'r', result, labels);
}

int _tms_rl(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    return _tms_rotate_circular(args, 'l', result, labels);
}

int _tms_sr(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift, labels) == -1)
        return -1;
    else
    {
        // Mask needed so that right shifting works properly
        value &= tms_int_mask;
        if (shift < 0)
        {
            tms_save_error(TMS_INT_EVALUATOR, SHIFT_AMOUNT_NEGATIVE, EH_FATAL, NULL, 0);
            return -1;
        }
        else if (shift >= tms_int_mask_size)
        {
            tms_save_error(TMS_INT_EVALUATOR, SHIFT_TOO_LARGE, EH_FATAL, NULL, 0);
            return -1;
        }
        // The cast to unsigned is necessary to avoid right shift sign extending (not an arithmetic shift)
        *result = (uint64_t)value >> shift;
        return 0;
    }
}

int _tms_arithmetic_shift(int64_t value, int64_t shift, char direction, int64_t *result)
{
    if (shift < 0)
    {
        tms_save_error(TMS_INT_EVALUATOR, SHIFT_AMOUNT_NEGATIVE, EH_FATAL, NULL, 0);
        return -1;
    }
    if (shift >= tms_int_mask_size)
    {
        tms_save_error(TMS_INT_EVALUATOR, SHIFT_TOO_LARGE, EH_FATAL, NULL, 0);
        return -1;
    }
    switch (direction)
    {
    case 'l':
        *result = value << shift;
        break;
    case 'r':
        *result = value >> shift;
    default:
        break;
    }

    return 0;
}

int _tms_sra(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift, labels) == -1)
        return -1;
    else
        return _tms_arithmetic_shift(value, shift, 'r', result);
}

int _tms_sl(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t value, shift;
    if (get_two_operands(args, &value, &shift, labels) == -1)
        return -1;
    else
        return _tms_arithmetic_shift(value, shift, 'l', result);
}

int _tms_nor(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        *result = ~(op1 | op2);
        return 0;
    }
}

int _tms_xor(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        *result = op1 ^ op2;
        return 0;
    }
}

int _tms_nand(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        *result = ~(op1 & op2);
        return 0;
    }
}

int _tms_and(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        *result = op1 & op2;
        return 0;
    }
}

int _tms_or(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        *result = op1 | op2;
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
            tms_save_error(TMS_INT_EVALUATOR, SYNTAX_ERROR, EH_FATAL, NULL, 0);
            return -1;
        }
        else if (tmp > 255 || tmp < 0)
        {
            tms_save_error(TMS_INT_EVALUATOR, NOT_A_DOT_DECIMAL, EH_FATAL, NULL, 0);
            return -1;
        }
        else
            // Dotted decimal here is read left to right, thus the right shift
            *result = *result | (tmp << (8 * (L->count - 1 - i)));
    }
    *result = tms_sign_extend(*result);
    return 0;
}

int _tms_dotted(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
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

int _tms_mask_range(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count(2, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    int64_t start, end;
    int status;
    status = tms_int_solve_e(args->arguments[0], &start, NO_LOCK, labels);
    if (status == -1)
    {
        tms_clear_errors(TMS_INT_EVALUATOR | TMS_INT_PARSER);
        return -1;
    }
    status = tms_int_solve_e(args->arguments[1], &end, NO_LOCK, labels);
    if (status == -1)
    {
        tms_clear_errors(TMS_INT_EVALUATOR | TMS_INT_PARSER);
        return -1;
    }

    if (start < 0 || start >= tms_int_mask_size || end < 0 || end >= tms_int_mask_size)
    {
        tms_save_error(TMS_INT_EVALUATOR, BIT_OUT_OF_RANGE, EH_FATAL, NULL, 0);
        return -1;
    }

    if (start == end)
        *result = (uint64_t)1 << start;
    else if (start < end)
    {
        tms_mask(end - start + 1, result);
        *result = *result << start;
    }
    else
    {
        tms_mask(start - end + 1, result);
        *result = ~(*result << end);
    }
    return 0;
}

int _tms_ipv4(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count(1, args->count, TMS_INT_EVALUATOR) == false)
        return -1;

    if (tms_int_mask_size != 32)
    {
        tms_save_error(TMS_INT_EVALUATOR, NOT_AN_IPV4_SIZE, EH_FATAL, NULL, 0);
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
        tms_save_error(TMS_INT_EVALUATOR, NOT_A_VALID_IPV4, EH_FATAL, NULL, 0);
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
        tms_save_error(TMS_INT_EVALUATOR, NOT_AN_IPV4_SIZE, EH_FATAL, NULL, 0);
        return -1;
    }

    if (length < 0 || length > 32)
    {
        tms_save_error(TMS_INT_EVALUATOR, NOT_A_VALID_IPV4_PREFIX, EH_FATAL, NULL, 0);
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

int tms_parity(int64_t value, int64_t *result)
{
    tms_ones(value, result);
    *result %= 2;
    return 0;
}

int tms_int_abs(int64_t value, int64_t *result)
{
    if (value > 0)
        *result = value;
    else
        *result = -value;
    return 0;
}

int _tms_int_min(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_INT_EVALUATOR) == false)
        return -1;

    // Set min to the largest possible value so it would always be overwritten in the first iteration
    int64_t min = INT64_MAX, tmp;
    int status;

    for (int i = 0; i < args->count; ++i)
    {
        status = tms_int_solve_e(args->arguments[i], &tmp, NO_LOCK, labels);
        if (status == -1)
            return -1;

        if (tmp < min)
            min = tmp;
    }

    *result = min;
    return 0;
}

int _tms_int_max(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_INT_EVALUATOR) == false)
        return -1;

    // Set min to the smallest possible value so it would always be overwritten in the first iteration
    int64_t max = INT64_MIN, tmp;
    int status;

    for (int i = 0; i < args->count; ++i)
    {
        status = tms_int_solve_e(args->arguments[i], &tmp, NO_LOCK, labels);
        if (status == -1)
            return -1;

        if (tmp > max)
            max = tmp;
    }

    *result = max;
    return 0;
}

int _tms_from_float(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (_tms_validate_args_count_range(args->count, 1, -1, TMS_INT_EVALUATOR) == false)
        return -1;

    if (tms_int_mask_size != 32 && tms_int_mask_size != 64)
    {
        tms_save_error(TMS_INT_EVALUATOR, NOT_A_FLOAT_OR_DOUBLE, EH_FATAL, NULL, -1);
        return -1;
    }

    double tmp = tms_solve_e(args->arguments[0], 0, NULL);
    if (isnan(tmp))
    {
        tms_error_data *last_error = tms_get_last_error(TMS_PARSER | TMS_EVALUATOR);
        char *err_msg = last_error->message;
        tms_save_error(TMS_INT_EVALUATOR, err_msg, EH_FATAL, NULL, -1);
        tms_clear_errors(TMS_PARSER | TMS_EVALUATOR);
        return -1;
    }
    else
    {
        switch (tms_int_mask_size)
        {
        case 32: {
            // Use a union to reinterpret a float as unsigned int32
            const union {
                float f;
                uint32_t i;
            } u = {tmp};
            *result = u.i;
            break;
        }
        case 64: {
            // Use a union to reinterpret a double as unsigned int64
            const union {
                double f;
                uint64_t i;
            } u = {tmp};
            *result = u.i;
            break;
        }
        default:
            break;
        }
    }
    return 0;
}

int _tms_hamming_distance(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        // XOR leaves ones where the values are different, count them and you get the hamming distance
        int64_t tmp = op1 ^ op2;
        return tms_ones(tmp, result);
    }
}

int _tms_gcd(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (!_tms_validate_args_count_range(args->count, 2, -1, TMS_INT_EVALUATOR))
        return -1;

    int64_t *operands;
    if (get_all_arguments(args, &operands, labels) == -1)
        return -1;
    // Check if all values are within permitted range
    for (int i = 0; i < args->count; ++i)
    {
        if (operands[i] == INT64_MIN)
        {
            // Overflow because abs(INT64_MIN) = INT64_MAX + 1
            tms_save_error(TMS_INT_EVALUATOR, INTEGER_OVERFLOW, EH_FATAL, NULL, -1);
            free(operands);
            return -1;
        }
    }

    int64_t tmp = operands[0];
    for (int i = 1; i < args->count; ++i)
        tmp = tms_gcd(tmp, operands[i]);

    *result = tmp;
    free(operands);
    return 0;
}

int _tms_lcm(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    if (!_tms_validate_args_count_range(args->count, 2, -1, TMS_INT_EVALUATOR))
        return -1;

    int64_t *operands;
    if (get_all_arguments(args, &operands, labels) == -1)
        return -1;
    // Check if all values are within permitted range
    for (int i = 0; i < args->count; ++i)
    {
        if (operands[i] == INT64_MIN)
        {
            // Overflow because abs(INT64_MIN) = INT64_MAX + 1
            tms_save_error(TMS_INT_EVALUATOR, INTEGER_OVERFLOW, EH_FATAL, NULL, -1);
            free(operands);
            return -1;
        }
    }

    int64_t lcm = operands[0], tmp;
    for (int i = 1; i < args->count; ++i)
    {
        tmp = lcm / tms_gcd(lcm, operands[i]);
        bool overflow = __builtin_mul_overflow(tmp, operands[i], &lcm);
        if (overflow || tms_sign_extend(lcm & tms_int_mask) != lcm)
        {
            tms_save_error(TMS_INT_EVALUATOR, INTEGER_OVERFLOW, EH_FATAL, NULL, -1);
            free(operands);
            return -1;
        }
    }
    *result = lcm;
    free(operands);
    return 0;
}

int _tms_multinv(tms_arg_list *args, tms_arg_list *labels, int64_t *result)
{
    int64_t op1, op2;
    if (get_two_operands(args, &op1, &op2, labels) == -1)
        return -1;
    else
    {
        bool op1_negative;
        if (op1 < 0)
        {
            op1_negative = true;
            op1 = -op1;
        }
        else
            op1_negative = false;

        if (op1 > INT32_MAX || op2 > INT32_MAX)
        {
            tms_save_error(TMS_INT_EVALUATOR, VALUE_OUT_OF_RANGE_FOR_MULINV, EH_FATAL, NULL, -1);
            return -1;
        }
        if (op2 < 0)
        {
            tms_save_error(TMS_INT_EVALUATOR, MULTINV_NO_NEGATIVE_MODULUS, EH_FATAL, NULL, -1);
            return -1;
        }

        if (tms_gcd(op1, op2) != 1)
        {
            tms_save_error(TMS_INT_EVALUATOR, MULINV_NEEDS_COPRIMES, EH_FATAL, NULL, -1);
            return -1;
        }

        // Applying the extended Euclidian algorithm
        int q, r1, r2, r, s1, s2, s, t1, t2, t;
        // Initialization
        if (op1 > op2)
        {
            r1 = op1;
            r2 = op2;
        }
        else
        {
            r1 = op2;
            r2 = op1;
        }
        q = r1 / r2;
        s = t2 = 1;
        s2 = 0;
        t = -q;
        r = r1 % r2;
        // Applying an iteration
        while (r != 1)
        {
            // Shifting values like in the table method
            r1 = r2;
            r2 = r;
            s1 = s2;
            s2 = s;
            t1 = t2;
            t2 = t;
            // Calculating new q, r, s, t
            q = r1 / r2;
            r = r1 % r2;
            s = s1 - s2 * q;
            t = t1 - t2 * q;
        }
        // Stop condition met, return the appropriate inverse
        // Remember that we want the inverse of op1 mod op2
        // We may have inverted them in the table
        if (op1 > op2)
            *result = s;
        else
            *result = t;

        // Prefer a positive answer
        if (*result < 0)
            *result += (-*result / op2 + 1) * op2;

        int64_t tmp;
        // For negative input, the result is mod - positive multinv
        if (op1_negative)
        {
            *result = op2 - *result;
            // Restoring the value of op1 as it was made positive earlier
            op1 = -op1;
            tmp = *result * op1;
        }
        else
            tmp = *result * op1;

        // The check below is expecting the answer of the modulus to be one, so we push the tmp value to the positive side
        if (tmp < 0)
            tmp += (-tmp / op2 + 1) * op2;

        // Making sure that the multiplicative inverse is correct
        if (tmp % op2 != 1)
        {
            tms_save_error(TMS_INT_EVALUATOR, INTERNAL_ERROR, EH_FATAL, NULL, -1);
            return -1;
        }

        return 0;
    }
}
