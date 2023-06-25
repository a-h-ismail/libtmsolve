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

#define array_length(z) (sizeof(z)/sizeof(*z))
/// Maximum number of errors in error_handler
#define MAX_ERRORS 5

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
extern int hardcoded_variable_count;

/// @brief Stores metadata related to extended functions arguments.
typedef struct arg_list
{
    /// The number of arguments.
    int count;
    // Array of C strings, stores the arguments.
    char **arguments;
} arg_list;

/// @brief Initializes the variables required for the proper operation of the calculator.
/// @details The variables to initialize are: all_functions, function_count, variable_names, variable_count.
void tmsolve_init();

/**
 * @brief Error handling function, collects and manages errors.
 * @param error The error message to store.
 * @param arg The list of argumets to pass to the error handler. \n
 * arg 1: \n
 * 1: Save the *error to the errors database, arg 2: ( 0: not fatal, stack; 1: fatal, stack). \n
 * For fatal errors, arg3 must have the index of the error (-1 means don't print_errors). \n
 * 2: Print the errors to stdout and clear the database, return number of errors printed. \n
 * 3: Clear the error database. arg 2: clear (0: main; 1: backup; 2: all). \n
 * 4: Search for *error in the errors database, return 1 on match in main, 2 in backup. arg 2: search (0: main; 1: backup; 2: all). \n
 * 5: Return the amount of errors in the main database. arg 2: 0: non-fatal; 1: fatal; 2:all . \n
 * 6: Backup current errors, making room for new ones. \n
 * 7: Restore the backed up errors, clearing the current errors in the process. \n

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