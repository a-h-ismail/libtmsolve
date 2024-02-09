/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef TMS_BITWISE
#define TMS_BITWISE
#include "tms_math_strs.h"

/**
 * @file
 * @brief Contains bitwise regular and extended function declarations.
 */

/// @brief Sign extend to 64 bits
int64_t tms_sign_extend(int64_t value);

/// @brief Bitwise NOT
int tms_not(int64_t value, int64_t *result);

/// @brief Generates a mask of size "bits"
int tms_mask(int64_t bits, int64_t *result);

/// @brief Generates an inverted mask of size "bits"
int tms_inv_mask(int64_t bits, int64_t *result);

/// @brief Rotate Right
int tms_rr(tms_arg_list *args, int64_t *result);

/// @brief Rotate Left
int tms_rl(tms_arg_list *args, int64_t *result);

/// @brief Shift Right
int tms_sr(tms_arg_list *args, int64_t *result);

/// @brief Shift Right Arithmetic (sign extended)
int tms_sra(tms_arg_list *args, int64_t *result);

/// @brief Shift Left
int tms_sl(tms_arg_list *args, int64_t *result);

/// @brief Bitwise NOR
int tms_nor(tms_arg_list *args, int64_t *result);

/// @brief Bitwise XOR
int tms_xor(tms_arg_list *args, int64_t *result);

/// @brief Bitwise NAND
int tms_nand(tms_arg_list *args, int64_t *result);

/// @brief Bitwise AND
int tms_and(tms_arg_list *args, int64_t *result);

/// @brief Bitwise OR
int tms_or(tms_arg_list *args, int64_t *result);

/// @brief Reads an IPv4 in dotted decimal notation
int tms_ipv4(tms_arg_list *args, int64_t *result);
#endif