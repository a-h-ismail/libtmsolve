/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "scientific.h"

char *tms_lib_version = {"0.1.3"};

char *_tms_g_expr = NULL;
double complex tms_g_ans = 0;

bool _tms_init = true;
char **tms_g_all_func_names;

tms_var tms_g_builtin_vars[] = {{"i", I, true}, {"pi", M_PI, true}, {"exp", M_E, true}, {"c", 299792458, true}};
tms_var *tms_g_vars = NULL;

char *tms_g_illegal_names[] = {"e", "E", "i"};
const int tms_g_illegal_names_count = array_length(tms_g_illegal_names);
int tms_g_func_count = 0, tms_g_var_count, tms_g_var_max = array_length(tms_g_builtin_vars);

void tmsolve_init()
{
    int i;
    if (_tms_init == true)
    {
        // Initialize variable names and values arrays
        tms_g_vars = malloc(tms_g_var_max * sizeof(tms_var));
        tms_g_var_count = array_length(tms_g_builtin_vars);

        for (i = 0; i < tms_g_var_count; ++i)
            tms_g_vars[i] = tms_g_builtin_vars[i];

        // Get the number of functions.
        for (i = 0; tms_r_func_name[i] != NULL; ++i)
            ++tms_g_func_count;
        for (i = 0; tms_cmplx_func_name[i] != NULL; ++i)
            ++tms_g_func_count;
        for (i = 0; tms_ext_func_name[i] != NULL; ++i)
            ++tms_g_func_count;

        // Generate the all functions array, avoiding repetition.
        char *tmp[tms_g_func_count];
        // Copy tms_r_func_name
        for (i = 0; tms_r_func_name[i] != NULL; ++i)
            tmp[i] = tms_r_func_name[i];
        bool found = false;
        // Copy what wasn't already copied
        for (int j = 0; tms_cmplx_func_name[j] != NULL; ++j)
        {
            found = false;
            for (int k = 0; tms_r_func_name[k] != NULL; ++k)
            {
                if (strcmp(tms_r_func_name[k], tms_cmplx_func_name[j]) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                tmp[i++] = tms_cmplx_func_name[j];
        }

        for (int j = 0; tms_ext_func_name[j] != NULL; ++j)
        {
            found = false;
            for (int k = 0; tms_r_func_name[k] != NULL; ++k)
            {
                if (strcmp(tms_r_func_name[k], tms_ext_func_name[j]) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                tmp[i++] = tms_ext_func_name[j];
        }
        tms_g_all_func_names = malloc(i * sizeof(char *));
        tms_g_func_count = i;
        for (i = 0; i < tms_g_func_count; ++i)
            tms_g_all_func_names[i] = tmp[i];
        _tms_init = false;
    }
}

int tms_error_handler(int _mode, ...)
{
    static int last_error = 0, fatal = 0, non_fatal = 0, backup_error_count = 0, backup_fatal, backup_non_fatal;
    va_list arguments;
    char *error;
    va_start(arguments, _mode);
    int i, arg2;

    static tms_error_data error_table[EH_MAX_ERRORS], backup[EH_MAX_ERRORS];
    switch (_mode)
    {
    case EH_SAVE:
        error = va_arg(arguments, char *);
        // Case of error table being full
        if (last_error == EH_MAX_ERRORS - 1)
        {
            // Before ovewriting oldest error, update error counters to reflect the new state
            if (error_table[0].fatal)
                --fatal;
            else
                --non_fatal;

            // Overwrite the oldest error
            for (i = 0; i < EH_MAX_ERRORS - 1; ++i)
                error_table[i] = error_table[i + 1];

            // The last position is free, update last_error
            --last_error;
        }
        error_table[last_error].error_msg = strdup(error);
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_NONFATAL_ERROR:
            error_table[last_error].fatal = false;
            ++non_fatal;
            break;
        case EH_FATAL_ERROR:
            error_table[last_error].fatal = true;
            ++fatal;
            break;
        default:
            return -1;
        }

        int position = va_arg(arguments, int);
        if (position != -1)
        {
            // Center the error in the string
            if (position > 49)
            {
                strncpy(error_table[last_error].bad_snippet, _tms_g_expr + position - 24, 49);
                error_table[last_error].bad_snippet[49] = '\0';
                error_table[last_error].index = 24;
            }
            else
            {
                strncpy(error_table[last_error].bad_snippet, _tms_g_expr, 49);
                error_table[last_error].bad_snippet[49] = '\0';
                error_table[last_error].index = position;
            }
        }
        else
            error_table[last_error].index = -1;
        last_error = fatal + non_fatal;
        return 0;

    case EH_PRINT:
        for (i = 0; i < last_error; ++i)
        {
            if (error_table[i].error_msg == NULL)
            {
                puts(INTERNAL_ERROR);
                return -1;
            }
            puts(error_table[i].error_msg);
            if (error_table[i].index != -1)
                tms_print_errors(error_table[i].bad_snippet, error_table[i].index);
            else
                printf("\n");
        }
        tms_error_handler(EH_CLEAR, EH_MAIN_DB);

    case EH_CLEAR:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            for (i = 0; i < last_error; ++i)
                free(error_table[i].error_msg);

            memset(error_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            // i carries the number of errors for the return value because the counter is reset here
            i = last_error;
            last_error = fatal = non_fatal = 0;
            break;
        case EH_BACKUP_DB:
            for (i = 0; i < backup_error_count; ++i)
                free(error_table[i].error_msg);

            memset(backup, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            i = backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            break;
        case EH_ALL_DB:
            for (i = 0; i < last_error; ++i)
                free(error_table[i].error_msg);
            for (i = 0; i < backup_error_count; ++i)
                free(error_table[i].error_msg);

            memset(error_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            memset(backup, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            i = last_error + backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            last_error = fatal = non_fatal = 0;
            break;
        default:
            i = -1;
        }
        return i;

    case EH_SEARCH:
        error = va_arg(arguments, char *);
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            for (i = 0; i < last_error; ++i)
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return EH_MAIN_DB;
            break;
        case EH_BACKUP_DB:
            for (i = 0; i < last_error; ++i)
                if (strcmp(error, backup[i].error_msg) == 0)
                    return EH_BACKUP_DB;
            break;
        case EH_ALL_DB:
            for (i = 0; i < last_error; ++i)
            {
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return EH_MAIN_DB;
                else if (strcmp(error, backup[i].error_msg) == 0)
                    return EH_BACKUP_DB;
            }
        }
        return -1;

    // Return the number of saved errors
    case EH_ERROR_COUNT:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_NONFATAL_ERROR:
            return non_fatal;
        case EH_FATAL_ERROR:
            return fatal;
        case EH_ALL_ERRORS:
            return non_fatal + fatal;
        }

    case EH_BACKUP:
        tms_error_handler(EH_CLEAR, EH_BACKUP);
        // Copy the current error database to the backup database
        for (i = 0; i < last_error; ++i)
            backup[i] = error_table[i];
        backup_error_count = last_error;
        backup_non_fatal = non_fatal;
        backup_fatal = fatal;
        // reset current database error count
        last_error = fatal = non_fatal = 0;
        return 0;

    // Overwrite main error database with the backup error database
    case EH_RESTORE:
        // Clear current database
        tms_error_handler(EH_CLEAR, EH_MAIN_DB);
        for (i = 0; i < backup_error_count; ++i)
            error_table[i] = backup[i];
        last_error = backup_error_count;
        fatal = backup_fatal;
        non_fatal = backup_non_fatal;
        // Remove all references of the errors from the backup
        for (i = 0; i < backup_error_count; ++i)
            backup[i].error_msg = NULL;
        break;
    }
    return -1;
}

void tms_print_errors(char *expr, int error_pos)
{
    int i;
    fprintf(stderr, "%s\n", expr);
    for (i = 0; i < error_pos; ++i)
        fprintf(stderr, "~");
    fprintf(stderr, "^\n\n");
}

int compare_ints(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return -1;
    else if (*(int *)a > *(int *)b)
        return 1;
    else
        return 0;
}

int compare_ints_reverse(const void *a, const void *b)
{
    if (*(int *)a < *(int *)b)
        return 1;
    else if (*(int *)a > *(int *)b)
        return -1;
    else
        return 0;
}