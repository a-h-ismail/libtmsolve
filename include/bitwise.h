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

int64_t tms_not(int64_t value);

int64_t tms_rrc(tms_arg_list *args);

int64_t tms_rlc(tms_arg_list *args);

int64_t tms_rr(tms_arg_list *args);

int64_t tms_rl(tms_arg_list *args);

int64_t tms_nor(tms_arg_list *args);

int64_t tms_xor(tms_arg_list *args);

int64_t tms_nand(tms_arg_list *args);

int64_t tms_and(tms_arg_list *args);

int64_t tms_or(tms_arg_list *args);
#endif