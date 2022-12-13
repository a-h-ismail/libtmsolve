/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef INTERNALS_H
#define INTERNALS_H
/**
 * @file
 * @brief Defines the functions necessary for the general fuctions of the calculator, some general error messages and the headers needed.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h>
#include <string.h>
// Errors
#define SYNTAX_ERROR "Syntax error."
#define NO_INPUT "Empty input."
/// @brief Stores metadata related to extended functions arguments.
typedef struct arg_list
{
    /// The number of arguments.
    int arg_count;
    // Array of C strings, stores the arguments.
    char **arguments;
} arg_list;

/**
 * @brief Error handling function, collects and manages errors.
 * @param error The error message to store.
 * @param arg The list of argumets to pass to the error handler.\n 
 * arg 1:\n 
 * 1: Save the *error to the errors database, arg 2: ( 0: not fatal, stack; 1: fatal, stack).\n 
 * For fatal errors, arg3 must have the index of the error (-1 means don't error_print).\n 
 * 2: Print the errors to stdout and clear the database, return number of errors printed.\n 
 * 3: Clear the error database. arg 2: clear (0: main; 1: backup; 2: all).\n 
 * 4: Search for *error in the errors database, return 1 on match in main, 2 in backup. arg 2: search (0: main; 1: backup; 2: all).\n 
 * 5: Return the amount of errors in the database. arg 2: errors in (0: main; 1: backup; 2: all).\n 
 * 6: Backup current errors, making room for new ones.\n 
 * 7: Restore the backed up errors, clearing the current errors in the process.\n 

 * @return Depends on the argument list.
 */
int error_handler(char *error, int arg, ...);

/**
 * @brief Prints the expression and points at the location of the error found.
 * @param expr The portion of the expression.
 * @param error_pos The index of the error in expr.
 */
void error_print(char *expr, int error_pos);

/**
 * @brief Simple function to find the minimum of 2 integers.
 * @return The minimum among the integers.
 */
int find_min(int a, int b);
#endif