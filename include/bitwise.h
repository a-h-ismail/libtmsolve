/*
Copyright (C) 2023-2026 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef _TMS_BITWISE_H
#define _TMS_BITWISE_H
#include "tms_math_strs.h"

/**
 * @file
 * @brief Contains bitwise regular and extended function declarations.
 * @note The functions intended for use in the integer evaluator need that their result get masked.
 */

/// @brief Sign extend to 64 bits
int64_t tms_sign_extend(int64_t value);

/// @brief Bitwise NOT
int tms_not(int64_t value, int64_t *result);

/// @brief Generates a mask of size "bits"
int tms_mask(int64_t bits, int64_t *result);

/// @brief Generates a mask for a specific bit.
int tms_mask_bit(int64_t bit, int64_t *result);

/// @brief Generates an inverted mask of size "bits"
int tms_inv_mask(int64_t bits, int64_t *result);

/// @brief Generates a random integer, supports specifing a range (min,max) or defaults to (0,max)
int _tms_int_rand(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

int _tms_rotate_circular_i(int64_t value, int64_t shift, char direction, int64_t *result);

int _tms_arithmetic_shift(int64_t value, int64_t shift, char direction, int64_t *result);

/// @brief Rotate Right
int _tms_rr(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Rotate Left
int _tms_rl(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Shift Right
int _tms_sr(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Shift Right Arithmetic (sign extended)
int _tms_sra(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Shift Left
int _tms_sl(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Bitwise NOR
int _tms_nor(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Bitwise XOR
int _tms_xor(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Bitwise NAND
int _tms_nand(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Bitwise AND
int _tms_and(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Bitwise OR
int _tms_or(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Reads an IPv4 in dot decimal notation
int _tms_ipv4(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Generates the subnet mask corresponding to the specified prefix length
int tms_ipv4_prefix(int64_t length, int64_t *result);

/// @brief Reads a dot decimal notation
int _tms_dotted(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Generates a mask for a range of bits
/// @details If the range start is larger than its end, the mask will wrap around.
int _tms_mask_range(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Returns the number of binary zeros in its argument
int tms_zeros(int64_t value, int64_t *result);

/// @brief Returns the number of binary ones in its argument
int tms_ones(int64_t value, int64_t *result);

/// @brief Calculate the parity bit
int tms_parity(int64_t value, int64_t *result);

/// @brief Finds the absolute value
int tms_int_abs(int64_t value, int64_t *result);

/// @brief Finds the minimum of its arguments
int _tms_int_min(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Finds the maximum of its arguments
int _tms_int_max(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Converts a floating point value/expression to its binary representation.
/// @note Supports only 32 and 64 bit floats and chooses between them according to int mask size.
int _tms_from_float(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Calculates hamming distance between its two arguments
int _tms_hamming_distance(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

/// @brief Calculates the multiplicative inverse of arg1 mod arg2
int _tms_multinv(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

int _tms_gcd(tms_arg_list *args, tms_arg_list *labels, int64_t *result);

int _tms_lcm(tms_arg_list *args, tms_arg_list *labels, int64_t *result);
#endif