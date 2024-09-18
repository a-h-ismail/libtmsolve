/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_ERROR_HANDLER_H
#define _TMS_ERROR_HANDLER_H

/**
 * @file
 * @brief Declares functions and structures related to libtmsolve's error handler.
 */

#include <stdbool.h>

/// @brief Error metadata structure.
typedef struct tms_error_data
{
    char *message, bad_snippet[50], *prefix;
    bool fatal;
    int relative_index;
    int real_index;
    int expr_len;
    int facilities;
} tms_error_data;

typedef struct tms_error_database
{
    tms_error_data *error_table;
    int fatal_count;
    int non_fatal_count;
} tms_error_database;

/// @brief Maximum number of errors in tms_error_handler.
#define EH_MAX_ERRORS 10

/// @brief All libtmsolve facilities, used in error handling and to lock parser/evaluator.
enum tms_facilities
{
    TMS_GENERAL = 1,
    TMS_PARSER = 2,
    TMS_EVALUATOR = 4,
    TMS_INT_PARSER = 8,
    TMS_INT_EVALUATOR = 16,
    TMS_MATRIX = 32,
    TMS_ALL_FACILITIES = -1
};

#define EH_FATAL 1
#define EH_NONFATAL 2
#define EH_ALL_ERRORS (EH_FATAL | EH_NONFATAL)

/**
 * @brief Saves an error in the global error database.
 * @param facilities Facilities where the error originated (ex: TMS_PARSER).
 * @param error_msg The error message.
 * @param severity Indicates the seriousness of the error (fatal or not fatal) (see EH_FATAL, EH_NONFATAL macros).
 * @param expr The expression string where the error occured
 * @param error_position Index where the error is in the expression string.
 * @return 0 on success, 1 on success with warnings, -1 on failure.
 */
int tms_save_error(int facilities, char *error_msg, int severity, char *expr, int error_position);

/**
 * @brief Print all errors for the specified facilities.
 * @note This function clears the printed errors from the database.
 * @return Number of printed errors.
 */
int tms_print_errors(int facilities);

/**
 * @brief Clear all errors for the specified facilities.
 * @return Number of cleared errors.
 */
int tms_clear_errors(int facilities);

/**
 * @brief Find the index of the first occurence of an error in the error database
 * @param facilities Facilities where the error originated.
 * @param error_msg The exact error message to find.
 * @return The index of the error in the database, or -1 if no match is found.
 */
int tms_find_error(int facilities, char *error_msg);

/**
 * @brief Get the number of errors per facilities and specified type.
 * @return Number of errors, or -1 on failure.
 */
int tms_get_error_count(int facilities, int error_type);

/**
 * @brief Replaces the expression and index in the last saved error.
 * @param facilities Facilities to match.
 * @param expr The new expr to replace the old one (if any).
 * @param error_position The new error position.
 * @param prefix A string to add to as prefix to the existing error message if necessary.
 * @return 0 on success, 1 on success with warnings, -1 on failure.
 */
int tms_modify_last_error(int facilities, char *expr, int error_position, char *prefix);

#endif