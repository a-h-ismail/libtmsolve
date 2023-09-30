/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef INTERNALS_H
#define INTERNALS_H
/**
 * @file
 * @brief Declares general purpose functionality like error handling, init and runtime variables.
 */

#include <complex.h>
#include <stdarg.h>
#include <stdbool.h>
#ifndef LOCAL_BUILD
#include <tmsolve/m_errors.h>
#include <tmsolve/tms_math_strs.h>
#else

#include "m_errors.h"
#include "tms_math_strs.h"
#endif

#define array_length(z) (sizeof(z) / sizeof(*z))
/// Maximum number of errors in tms_error_handler
#define EH_MAX_ERRORS 5

#define EH_SAVE 1
#define EH_PRINT 2
#define EH_CLEAR 3
#define EH_SEARCH 4
#define EH_ERROR_COUNT 5
#define EH_BACKUP 6
#define EH_RESTORE 7

#define EH_MAIN_DB 8
#define EH_BACKUP_DB 9
#define EH_ALL_DB 10

#define EH_NONFATAL_ERROR 11
#define EH_FATAL_ERROR 12
#define EH_ALL_ERRORS 13

// Simple macro to ease dynamic resizing
#define DYNAMIC_RESIZE(ptr, current, max, type)         \
    if (current == max)                                 \
    {                                                   \
        max *= 2;                                       \
        ptr = (type *)realloc(ptr, max * sizeof(type)); \
    }

/// @brief Stores the answer of the last calculation to allow reuse.
extern double complex tms_g_ans;

/// @brief Use to store the current expression being processed, used by tms_error_handler() to generate the error prompt.
extern char *_tms_g_expr;

/// @brief Indicates if the library initialization function should be run.
extern bool _tms_do_init;

/// @brief Toggles additional debugging information.
extern bool _tms_debug;

/// @brief Contains all built in variables (like pi).
extern tms_var tms_g_builtin_vars[];

/// @brief Contains all variables, including built in ones, dynamically expanded on demand.
extern tms_var *tms_g_vars;

/// @brief Contains all runtime functions.
extern tms_ufunc *tms_g_ufunc;

/// @brief Total number of functions, those with complex variant are counted once.
extern int tms_g_func_count;

/// @brief Total number of runtime functions.
extern int tms_g_ufunc_count;

/// @brief Max number of runtime functions.
extern int tms_g_ufunc_max;

/// @brief Total number of variables, including built in ones.
extern int tms_g_var_count;

/// @brief Current maximum variable capacity, use to dynamically resize variable storage.
extern int tms_g_var_max;

/// @brief All function names, including built in and extended but not user defined functions.
extern char **tms_g_all_func_names;

/// @brief Names that should not be usable.
extern char *tms_g_illegal_names[];

/// @brief Number of illegal names (for use in loops);
extern const int tms_g_illegal_names_count;

/**
 * @brief Error metadata structure.
 */
typedef struct tms_error_data
{
    char *error_msg, bad_snippet[50];
    bool fatal;
    int relative_index;
    int real_index;
    int expr_len;
} tms_error_data;

/// @brief Initializes the variables required for the proper operation of the calculator.
/// @details The variables to initialize are: tms_g_all_func_names, tms_g_func_count, tms_g_var_count, tms_vars.
void tmsolve_init() __attribute__((constructor));

/**
 * @brief Creates a new variable if it doesn't exist
 * @param name Name of the variable
 * @param is_constant Sets the constant bit to protect the variable from being changed by the evaluator
 * @return The index of the variable in the all vars array (whether it already exists or is newly created)
 */
int tms_new_var(char *name, bool is_constant);

int tms_set_ufunction(char *name, char *function);

/**
 * @brief Duplicates an existing math expression.
 */
tms_math_expr *tms_dup_mexpr(tms_math_expr *M);

/**
 * @brief Error handling function, collects and manages errors.
 * @param _mode The mode of error handler.
 * @param arg The list of argumets to pass to the error handler, according to the mode.
 * @details
 * Possible arguments: \n
 * EH_SAVE, char *error, EH_FATAL_ERROR | EH_NONFATAL_ERROR, error_index \n
 * EH_PRINT (returns number of printed errors). \n
 * EH_CLEAR, EH_MAIN_DB | EH_BACKUP_DB | EH_ALL_DB \n
 * EH_SEARCH, char *error, EH_MAIN_DB | EH_BACKUP_DB | EH_ALL_DB (returns EH_MAIN_DB on match in main, EH_BACKUP_DB on
 match in backup). \n
 * EH_ERROR_COUNT, EH_FATAL_ERROR | EH_NONFATAL_ERROR | EH_ALL_ERRORS (returns number of errors specified). \n
 * EH_BACKUP \n
 * EH_RESTORE \n

 * @return Depends on the argument list.
 */
int tms_error_handler(int _mode, ...);

/**
 * @brief Prints the expression and points at the location of the error found.
 * @param E The error metadata
 */
void tms_print_error(tms_error_data E);

/**
 * @brief Simple function to find the minimum of 2 integers.
 * @return The minimum among the integers.
 */
int find_min(int a, int b);

/// @brief Comparator function for use with qsort(), use to sort in increasing order.
int compare_ints(const void *a, const void *b);

/// @brief Comparator function for use with qsort(), use to sort in decreasing order.
int compare_ints_reverse(const void *a, const void *b);
#endif