/*
Copyright (C) 2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/

#ifndef TMS_BITWISE
#define TMS_BITWISE
#include "tms_math_strs.h"

/**
 * @file
 * @brief Contains bitwise extended functions declarations.
 */

/// @brief Bitwise NOT
int64_t tms_not(int64_t value);

/// @brief Rotate Right
int64_t tms_rr(tms_arg_list *args);

/// @brief Rotate Left
int64_t tms_rl(tms_arg_list *args);

/// @brief Shift Right
int64_t tms_sr(tms_arg_list *args);

/// @brief Shift Right Arithmetic (sign extended)
int64_t tms_sra(tms_arg_list *args);

/// @brief Shift Left
int64_t tms_sl(tms_arg_list *args);

/// @brief Bitwise NOR
int64_t tms_nor(tms_arg_list *args);

/// @brief Bitwise XOR
int64_t tms_xor(tms_arg_list *args);

/// @brief Bitwise NAND
int64_t tms_nand(tms_arg_list *args);

/// @brief Bitwise AND
int64_t tms_and(tms_arg_list *args);

/// @brief Bitwise OR
int64_t tms_or(tms_arg_list *args);
#endif