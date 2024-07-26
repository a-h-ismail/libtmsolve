/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "error_handler.h"
#include "string_tools.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int last_error = 0, fatal = 0, non_fatal = 0;
tms_error_data error_table[EH_MAX_ERRORS];

atomic_bool db_lock;

void _lock_error_database()
{
    while (atomic_flag_test_and_set(&db_lock))
        ;
}

void _unlock_error_database()
{
    atomic_flag_clear(&db_lock);
}

void tms_print_error(tms_error_data E)
{
    int i;
    char *facility_name;

    switch (E.facilities)
    {
    case TMS_PARSER:
        facility_name = "parser";
        break;
    case TMS_INT_PARSER:
        facility_name = "int_parser";
        break;
    case TMS_EVALUATOR:
        facility_name = "evaluator";
        break;
    case TMS_INT_EVALUATOR:
        facility_name = "int_evaluator";
        break;
    case TMS_MATRIX:
        facility_name = "matrix";
        break;
    default:
        facility_name = NULL;
        break;
    }
    if (facility_name != NULL)
        fprintf(stderr, "%s: ", facility_name);

    // Error index is provided
    // Print the snippet where the error occured
    if (E.real_index != -1)
    {
        fprintf(stderr, "%s\nAt col %d: \n", E.message, E.real_index);
        if (E.real_index > 24)
        {
            E.relative_index += 3;
            fprintf(stderr, "...");
        }
        fprintf(stderr, "%s", E.bad_snippet);
        if (E.expr_len > E.real_index + 25)
            fprintf(stderr, "...");
        fprintf(stderr, "\n");

        for (i = 0; i < E.relative_index; ++i)
            fprintf(stderr, "~");
        fprintf(stderr, "^\n\n");
    }
    else
        // No index is provided -> print the error only
        fprintf(stderr, "%s\n\n", E.message);
}

int tms_save_expr_with_error(char *expr, int error_position, tms_error_data *E)
{
    if (expr != NULL)
    {
        E->expr_len = strlen(expr);

        if (error_position < 0 || error_position > E->expr_len)
        {
            fputs("libtmsolve warning: Error index out of expression range, ignoring the expr...\n\n", stderr);
            E->relative_index = -1;
            E->real_index = -1;
            E->bad_snippet[0] = '\0';
            last_error = fatal + non_fatal;
            return 1;
        }

        // Center the error in the string
        if (error_position > 24)
        {
            strncpy(E->bad_snippet, expr + error_position - 24, 49);
            E->bad_snippet[49] = '\0';
            E->relative_index = 24;
            E->real_index = error_position;
        }
        else
        {
            strncpy(E->bad_snippet, expr, 49);
            E->bad_snippet[49] = '\0';
            E->relative_index = error_position;
            E->real_index = error_position;
        }
    }
    else
        E->relative_index = E->real_index = -1;

    return 0;
}

int tms_save_error(int facilities, char *error_msg, int severity, char *expr, int error_position)
{
    _lock_error_database();

    // Case of error table being full
    if (last_error == EH_MAX_ERRORS - 1)
    {
        free(error_table[0].message);
        // Before ovewriting oldest error, update error counters to reflect the new state
        if (error_table[0].fatal)
            --fatal;
        else
            --non_fatal;

        // Overwrite the oldest error
        for (int i = 0; i < EH_MAX_ERRORS - 1; ++i)
            error_table[i] = error_table[i + 1];

        // The last position is free, update last_error
        --last_error;
    }
    error_table[last_error].message = strdup(error_msg);
    error_table[last_error].facilities = facilities;

    if (severity == EH_NONFATAL)
    {
        error_table[last_error].fatal = false;
        ++non_fatal;
    }
    else if (severity == EH_FATAL)
    {
        error_table[last_error].fatal = true;
        ++fatal;
    }

    int status = tms_save_expr_with_error(expr, error_position, error_table + last_error);

    last_error = fatal + non_fatal;

    _unlock_error_database();
    return status;
}

int tms_print_errors(int facilities)
{
    for (int i = 0; i < last_error; ++i)
        if ((error_table[i].facilities & facilities) != 0)
            tms_print_error(error_table[i]);

    return tms_clear_errors(facilities);
}

int tms_clear_errors(int facilities)
{
    _lock_error_database();

    int i, deleted_count = 0;
    for (i = 0; i < last_error; ++i)
    {
        if ((error_table[i].facilities & facilities) != 0)
        {
            free(error_table[i].message);
            error_table[i].message = NULL;
            if (error_table[i].fatal)
                --fatal;
            else
                --non_fatal;

            ++deleted_count;
        }
    }
    // Move remaining errors back to make the table contiguous again
    // No need to do anything if the last element is empty
    for (i = 0; i < last_error - 1; ++i)
    {
        if (error_table[i].message == NULL)
        {
            for (int j = i + 1; j < last_error; ++j)
            {
                error_table[i] = error_table[j];
                // Move the hole forward
                error_table[j].message = NULL;
            }
        }
    }

    last_error = fatal + non_fatal;

    _unlock_error_database();
    return deleted_count;
}

int tms_find_error(int facilities, char *error_msg)
{
    _lock_error_database();
    for (int i = 0; i < last_error; ++i)
        if ((facilities & error_table[i].facilities) != 0 && (strcmp(error_msg, error_table[i].message) == 0))
        {
            _unlock_error_database();
            return i;
        }
    _unlock_error_database();
    return -1;
}

int tms_get_error_count(int facilities, int error_type)
{
    _lock_error_database();
    int select_fatal, select_non_fatal;
    if (facilities == TMS_ALL_FACILITIES)
    {
        select_fatal = fatal;
        select_non_fatal = non_fatal;
    }
    else
    {
        select_fatal = select_non_fatal = 0;
        for (int i = 0; i < last_error; ++i)
            if ((facilities & error_table[i].facilities) != 0)
            {
                if (error_table[i].fatal)
                    ++select_fatal;
                else
                    ++select_non_fatal;
            }
    }
    _unlock_error_database();
    switch (error_type)
    {
    case EH_NONFATAL:
        return select_non_fatal;
    case EH_FATAL:
        return select_fatal;
    case EH_FATAL | EH_NONFATAL:
        return select_non_fatal + select_fatal;
    }
    return -1;
}

int tms_modify_last_error(int facilities, char *expr, int error_position, char *prefix)
{
    _lock_error_database();
    int i;
    // Find the last error
    for (i = last_error - 1; i >= 0; --i)
    {
        if ((error_table[i].facilities & facilities) != 0)
            break;
    }
    if (i == -1)
    {
        _unlock_error_database();
        return -1;
    }

    error_table[i].expr_len = strlen(expr);

    if (prefix != NULL)
    {
        char *tmp = error_table[i].message;
        error_table[i].message = tms_strcat_dup(prefix, error_table[i].message);
        free(tmp);
    }

    int status = tms_save_expr_with_error(expr, error_position, error_table + i);
    _unlock_error_database();
    return status;
}
