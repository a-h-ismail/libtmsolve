/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_INTERNALS_H
#define _TMS_INTERNALS_H
/**
 * @file
 * @brief Declares general purpose functionality like library initialization, useful macros, runtime variables and functions.
 */

#include <complex.h>
#include <stdbool.h>
#ifndef LOCAL_BUILD
#include <tmsolve/m_errors.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "m_errors.h"
#include "tms_math_strs.h"
#endif

#define array_length(z) (sizeof(z) / sizeof(*z))

// Simple macro to ease dynamic resizing
#define DYNAMIC_RESIZE(ptr, current, max, type)         \
    if (current == max)                                 \
    {                                                   \
        max *= 2;                                       \
        ptr = (type *)realloc(ptr, max * sizeof(type)); \
    }

/// @brief Stores the answer of the last calculation.
extern double complex tms_g_ans;

/// @brief Int64 variant of ans.
extern int64_t tms_g_int_ans;

/// @brief Indicates if the library initialization function should be run.
extern bool _tms_do_init;

/// @brief Toggles additional debugging information.
extern bool _tms_debug;

/// @brief Contains all built in variables (like pi).
extern tms_var tms_g_builtin_vars[];

/// @brief Contains all variables, including built in ones, dynamically expanded on demand.
extern tms_var *tms_g_vars;

/// @brief Contains all int64 variables, dynamically expanded on demand.
extern tms_int_var *tms_g_int_vars;

/// @brief Contains all runtime functions.
extern tms_ufunc *tms_g_ufunc;

/// @brief Total number of runtime functions.
extern int tms_g_ufunc_count;

/// @brief Max number of runtime functions.
extern int tms_g_ufunc_max;

/// @brief Total number of variables, including built in ones.
extern int tms_g_var_count;

/// @brief Current maximum variable capacity, use to dynamically resize variable storage.
extern int tms_g_var_max;

/// @brief Total number of int64 variables.
extern int tms_g_int_var_count;

/// @brief Current maximum int64 variable capacity, use to dynamically resize variable storage.
extern int tms_g_int_var_max;

extern char **tms_g_all_func_names;

extern int tms_g_all_func_count, tms_g_all_func_max;

extern char **tms_g_all_int_func_names;

extern int tms_g_all_int_func_count, tms_g_all_int_func_max;

/// @brief Names that should not be usable.
extern char *tms_g_illegal_names[];

/// @brief Number of illegal names (for use in loops);
extern const int tms_g_illegal_names_count;

/// @brief Mask used after every operation of int parser
extern uint64_t tms_int_mask;

extern int8_t tms_int_mask_size;

/// @brief Initializes the variables required for the proper operation of the calculator.
void tmsolve_init() __attribute__((constructor));

/**
 * @brief Locks one of the parsers.
 * @details This function uses spinlocks to wait for a lock to be released.
 * @param variant Either TMS_PARSER or TMS_INT_PARSER
 */
void tms_lock_parser(int variant);

/**
 * @brief Releases locks required by the parser
 * @param variant Either TMS_PARSER or TMS_INT_PARSER
 */
void tms_unlock_parser(int variant);

/**
 * @brief Locks one of the evaluator
 * @details This function uses spinlocks to wait for a lock to be released.
 * @param variant Either TMS_EVALUATOR or TMS_INT_EVALUATOR
 */
void tms_lock_evaluator(int variant);

/**
 * @brief Unlocks one of the evaluators
 * @param variant 
 */
void tms_unlock_evaluator(int variant);

/**
 * @brief Sets the global mask used by integer parser and evaluator. Locks both of them while the mask is being modified.
 * @note This mask is what allows the library to emulate integers of width [1-64].
 * @param size Effective integer size when using this mask.
 */
void tms_set_int_mask(int size);

/**
 * @brief Creates a new variable if it doesn't exist
 * @param name Name of the variable
 * @param is_constant Sets the constant bit to protect the variable from being changed by the evaluator
 * @return The index of the variable in the all vars array (whether it already exists or is newly created)
 */
int tms_new_var(char *name, bool is_constant);

void tms_set_var(double complex value, int index);

int tms_new_int_var(char *name);

/**
 * @brief Add/Update a runtime function.
 * @param fname Function name.
 * @param unknowns_list A comma separated string of unknowns names.
 * @param function The expression with the unknowns.
 * @return 0 on success, -1 on failure.
 */
int tms_set_ufunction(char *fname, char *unknowns_list, char *function);

bool _tms_validate_args_count(int expected, int actual, int facility_id);

bool _tms_validate_args_count_range(int actual, int min, int max, int facility_id);

/**
 * @brief Simple function to find the minimum of 2 integers.
 * @return The minimum among the integers.
 */
int find_min(int a, int b);

/// @brief Comparator function for use with qsort(), use to sort in increasing order.
int compare_ints(const void *a, const void *b);

/// @brief Comparator function for use with qsort(), use to sort in decreasing order.
int compare_ints_reverse(const void *a, const void *b);

// Returns a random weight in range [0;1]
double tms_random_weight();
#endif