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
#include <stddef.h>
#ifndef LOCAL_BUILD
#include <tmsolve/m_errors.h>
#include <tmsolve/tms_math_strs.h>
#else
#include "m_errors.h"
#include "tms_math_strs.h"
#endif

#define array_length(z) (sizeof(z) / sizeof(*z))

// Simple macro to ease dynamic resizing
#define DYNAMIC_RESIZE(ptr, current, max, type)                                                                        \
    if (current == max)                                                                                                \
    {                                                                                                                  \
        max *= 2;                                                                                                      \
        ptr = (type *)realloc(ptr, max * sizeof(type));                                                                \
    }

/// @brief Stores the answer of the last calculation.
extern double complex tms_g_ans;

/// @brief Int64 variant of ans.
extern int64_t tms_g_int_ans;

/// @brief Indicates if the library initialization function should be run.
extern bool _tms_do_init;

/// @brief Toggles additional debugging information.
extern bool _tms_debug;

/// @brief Names that should not be usable.
extern char *tms_g_illegal_names[];

/// @brief Number of illegal names (for use in loops);
extern const int tms_g_illegal_names_count;

/// @brief Mask used after every operation of int parser
extern uint64_t tms_int_mask;

/// @brief Mask width in bits.
extern int8_t tms_int_mask_size;

/// @brief Initializes the variables required for the proper operation of the calculator.
void tmsolve_init() __attribute__((constructor));

/// @brief Clears all user defined variables, functions and answers.
void tmsolve_reset();

/**
 * @brief Searches for a variable using its name.
 * @return Pointer to the variable in the internal hashmap, or NULL if no match is found.
 * @warning This function is not thread safe, and there is no guarantees that the pointer will remain valid on any hashmap modification.
 * @note This description applies to all `get_by_name` functions.
 */
const tms_var *tms_get_var_by_name(char *name);

const tms_int_var *tms_get_int_var_by_name(char *name);

const tms_rc_func *tms_get_rc_func_by_name(char *name);

const tms_extf *tms_get_extf_by_name(char *name);

const tms_int_func *tms_get_int_func_by_name(char *name);

const tms_int_extf *tms_get_int_extf_by_name(char *name);

const tms_ufunc *tms_get_ufunc_by_name(char *name);

const tms_int_ufunc *tms_get_int_ufunc_by_name(char *name);

/**
 * @brief Returns a malloc'd array with all of the currently defined variables .
 * @note This description applies to all `get_all_...` functions.
 */
tms_var *tms_get_all_vars(size_t *count, bool sort);

tms_int_var *tms_get_all_int_vars(size_t *count, bool sort);

tms_rc_func *tms_get_all_rc_func(size_t *count, bool sort);

tms_extf *tms_get_all_extf(size_t *count, bool sort);

tms_ufunc *tms_get_all_ufunc(size_t *count, bool sort);

tms_int_func *tms_get_all_int_func(size_t *count, bool sort);

tms_int_extf *tms_get_all_int_extf(size_t *count, bool sort);

tms_int_ufunc *tms_get_all_int_ufunc(size_t *count, bool sort);

/**
 * @brief Removes a runtime variable by its name.
 * @retval 0 on success.
 * @retval -1 on failure.
 * @retval 1 on variable found, but not removable.
 */
int tms_remove_var(char *name);

/**
 * @brief Removes a runtime integer variable by its name.
 * @retval 0 on success.
 * @retval -1 on failure.
 * @retval 1 on variable found, but not removable.
 */
int tms_remove_int_var(char *name);

/**
 * @brief Removes a user function by its name.
 * @retval 0 on success.
 * @retval -1 on failure.
 */
int tms_remove_ufunc(char *name);

/**
 * @brief Removes a user integer function by its name.
 * @retval 0 on success.
 * @retval -1 on failure.
 */
int tms_remove_int_ufunc(char *name);

/**
 * @brief Checks if a function with the specified name already exists.
 */
bool tms_function_exists(char *name);

/**
 * @brief Checks if a integer function with the specified name already exists.
 */
bool tms_int_function_exists(char *name);

/**
 * @brief Locks one of the parsers.
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
 * @brief Add/Update a user variable.
 * @param name Variable name.
 * @param value Variable value.
 * @param is_constant When set, the variable is treated read-only (can't be modified or removed).
 * @return 0 on success, -1 on failure.
 */
int tms_set_var(char *name, double complex value, bool is_constant);

/**
 * @brief Add/Update an integer user variable.
 * @param name Variable name.
 * @param value Variable value.
 * @param is_constant When set, the variable is treated read-only (can't be modified or removed).
 * @note The value is sign extended before being stored
 * @return 0 on success, -1 on failure.
 */
int tms_set_int_var(char *name, int64_t value, bool is_constant);

/**
 * @brief Add/Update a runtime function.
 * @param fname Function name.
 * @param function_args A comma separated string of labels names.
 * @param function Expression of the function in terms of its labels.
 * @return 0 on success, -1 on failure.
 */
int tms_set_ufunction(char *fname, char *function_args, char *function);

/**
 * @brief Add/Update a runtime integer function.
 * @param fname Function name.
 * @param labels_list A comma separated string of labels names.
 * @param function Expression of the function in terms of its labels.
 * @return 0 on success, -1 on failure.
 */
int tms_set_int_ufunction(char *fname, char *function_args, char *function);

bool _tms_validate_args_count(int expected, int actual, int facility_id);

bool _tms_validate_args_count_range(int actual, int min, int max, int facility_id);

/**
 * @brief Helper function for name autocompletion in interactive usage for "scientific" mode.
 * @param name Partial name to match.
 * @return An array of strings with all possible matches.
 * @note Matches are from functions and variable names.
 */
char **tms_smode_autocompletion_helper(const char *name);

/**
 * @brief Helper function for name autocompletion in interactive usage for "integer" mode.
 * @param name Partial name to match.
 * @return An array of strings with all possible matches.
 * @note Matches are from functions and variable names.
 */
char **tms_imode_autocompletion_helper(const char *name);

/// @brief It does what you think it does.
int find_min(int a, int b);

/// @brief Comparator function for use with qsort(), use to sort in increasing order.
int compare_ints(const void *a, const void *b);

/// @brief Comparator function for use with qsort(), use to sort in decreasing order.
int compare_ints_reverse(const void *a, const void *b);

// Returns a random weight in range [0;1]
double tms_random_weight();
#endif