/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef INTERNALS_H
#define INTERNALS_H
/**
 * @file
 * @brief Declares general purpose functions and globals used in the calculator. Includes all required standard library headers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#ifndef LOCAL_BUILD
#include <tmsolve/m_errors.h>
#else
#include "m_errors.h"
#endif

#define array_length(z) (sizeof(z) / sizeof(*z))
/// Maximum number of errors in error_handler
#define MAX_ERRORS 5

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

/// @brief Stores the answer of the last calculation to allow reuse.
extern double complex ans;

/// @brief Use to store the current expression being processed, used by error_handler() to generate the error prompt.
extern char *_glob_expr;

/// @brief Indicates if the library intialization function should be run.
extern bool _init_needed;

/// @brief Total number of functions, those with complex variant are counted once.
extern int function_count;

/// @brief Total number of variables, including built in ones.
extern int variable_count;

/// @brief Current maximum variable capacity, use to dynamically resize variable storage.
extern int variable_max;

/// @brief All function names, including built in, extended, and user defined functions.
extern char **all_functions;

/// @brief Contains all variable values except ans which has its own variable.
extern double complex *variable_values;

/// @brief Contains all variable names including built in ones.
extern char **variable_names;

/// @brief Number of hardcoded variables.
extern const int hardcoded_variable_count;

/// @brief Names that should not be usable for variables.
extern char *illegal_names[];

/// @brief Number of illegal names (for use in loops);
extern const int illegal_names_count;

/// @brief Stores metadata related to extended functions arguments.
typedef struct arg_list
{
    /// The number of arguments.
    int count;
    // Array of C strings, stores the arguments.
    char **arguments;
} arg_list;

/**
 * @brief Error metadata structure.
 */
typedef struct error_data
{
    char *error_msg, bad_snippet[50];
    bool fatal;
    int index;
} error_data;

/// @brief Initializes the variables required for the proper operation of the calculator.
/// @details The variables to initialize are: all_functions, function_count, variable_names, variable_count.
void tmsolve_init();

/**
 * @brief Error handling function, collects and manages errors.
 * @param error The error message to store.
 * @param arg The list of argumets to pass to the error handler.
 * @details
 * Possible arguments: \n
 * EH_SAVE, EH_FATAL_ERROR | EH_NONFATAL_ERROR, error_index \n
 * EH_PRINT (returns number of printed errors). \n
 * EH_CLEAR, EH_MAIN_DB | EH_BACKUP_DB | EH_ALL_DB \n
 * EH_SEARCH, EH_MAIN_DB | EH_BACKUP_DB | EH_ALL_DB (returns 1 on match in main, 2 on match in backup). \n
 * EH_ERROR_COUNT, EH_FATAL_ERROR | EH_NONFATAL_ERROR | EH_ALL_ERRORS (returns number of errors specified). \n
 * EH_BACKUP \n
 * EH_RESTORE \n

 * @return Depends on the argument list.
 */
int error_handler(char *error, int arg, ...);

/**
 * @brief Prints the expression and points at the location of the error found.
 * @param expr The portion of the expression.
 * @param error_pos The index of the error in expr.
 */
void print_errors(char *expr, int error_pos);

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