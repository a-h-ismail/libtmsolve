/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#ifndef _TMS_ERROR_HANDLER_H
#define _TMS_ERROR_HANDLER_H

#include <stdbool.h>

/**
 * @brief Error metadata structure.
 */
typedef struct tms_error_data
{
    char *message, bad_snippet[50];
    bool fatal;
    int relative_index;
    int real_index;
    int expr_len;
    int facility_id;
} tms_error_data;

typedef struct tms_error_database
{
    tms_error_data *error_table;
    int fatal_count;
    int non_fatal_count;
} tms_error_database;

/// @brief Maximum number of errors in tms_error_handler
#define EH_MAX_ERRORS 10

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

int tms_save_error(int facility_id, char *error_msg, int severity, char *expr, int error_position);

int tms_print_errors(int facility_id);

int tms_clear_errors(int facility_id);

int tms_find_error(int facility_id, char *error_msg);

int tms_get_error_count(int facility_id, int error_type);

int tms_modify_last_error(int facility_id, char *expr, int error_position, char *prefix);

/**
 * @brief Prints the expression and points at the location of the error found.
 * @param E The error metadata
 */
void tms_print_error(tms_error_data E);

#endif