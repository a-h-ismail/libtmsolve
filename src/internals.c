/*
Copyright (C) 2021-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "scientific.h"
char *_glob_expr = NULL;
double complex ans = 0;

bool _init_needed = true;
char **all_functions;
double complex *variable_values;
char **variable_names;

char *hardcoded_variable_names[] = {"pi", "exp", "c"};
double complex hardcoded_variable_values[] = {M_PI, M_E, 299792458};
const int hardcoded_variable_count = array_length(hardcoded_variable_names);
char *illegal_names[] = {"e", "E", "i"};
const int illegal_names_count = array_length(illegal_names);
int function_count = 0, variable_count, variable_max = 8;
void tmsolve_init()
{
    int i;
    if (_init_needed == true)
    {
        // Initialize variable names and values arrays
        variable_names = malloc(8 * sizeof(char *));
        variable_values = malloc(8 * sizeof(double complex));
        variable_count = hardcoded_variable_count;
        for (i = 0; i < variable_count; ++i)
        {
            variable_names[i] = hardcoded_variable_names[i];
            variable_values[i] = hardcoded_variable_values[i];
        }

        // Get the number of functions.
        for (i = 0; r_function_name[i] != NULL; ++i)
            ++function_count;
        for (i = 0; cmplx_function_name[i] != NULL; ++i)
            ++function_count;
        for (i = 0; ext_function_name[i] != NULL; ++i)
            ++function_count;

        // Generate the all functions array, avoiding repetition.
        char *tmp[function_count];
        // Copy r_function_name
        for (i = 0; r_function_name[i] != NULL; ++i)
            tmp[i] = r_function_name[i];
        bool found = false;
        // Copy what wasn't already copied
        for (int j = 0; cmplx_function_name[j] != NULL; ++j)
        {
            found = false;
            for (int k = 0; r_function_name[k] != NULL; ++k)
            {
                if (strcmp(r_function_name[k], cmplx_function_name[j]) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                tmp[i++] = cmplx_function_name[j];
        }

        for (int j = 0; ext_function_name[j] != NULL; ++j)
        {
            found = false;
            for (int k = 0; r_function_name[k] != NULL; ++k)
            {
                if (strcmp(r_function_name[k], ext_function_name[j]) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                tmp[i++] = ext_function_name[j];
        }
        all_functions = malloc(i * sizeof(char *));
        function_count = i;
        for (i = 0; i < function_count; ++i)
            all_functions[i] = tmp[i];
        _init_needed = false;
    }
}

int error_handler(char *error, int arg1, ...)
{
    static int last_error = 0, fatal = 0, non_fatal = 0, backup_error_count = 0, backup_fatal, backup_non_fatal;
    va_list arguments;
    va_start(arguments, arg1);
    int i, arg2;

    static error_data error_table[MAX_ERRORS], backup[MAX_ERRORS];
    switch (arg1)
    {
    case EH_SAVE:
        error_table[last_error].error_msg = error;
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
                strncpy(error_table[last_error].bad_snippet, _glob_expr + position - 24, 49);
                error_table[last_error].bad_snippet[49] = '\0';
                error_table[last_error].index = 24;
            }
            else
            {
                strcpy(error_table[last_error].bad_snippet, _glob_expr);
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
                print_errors(error_table[i].bad_snippet, error_table[i].index);
            else
                printf("\n");
        }
        error_handler(NULL, EH_CLEAR, EH_MAIN_DB);

    case EH_CLEAR:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            memset(error_table, 0, MAX_ERRORS * sizeof(struct error_data));
            i = last_error;
            last_error = fatal = non_fatal = 0;
            break;
        case EH_BACKUP_DB:
            memset(backup, 0, MAX_ERRORS * sizeof(struct error_data));
            i = backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            break;
        case EH_ALL_DB:
            memset(error_table, 0, MAX_ERRORS * sizeof(struct error_data));
            memset(backup, 0, MAX_ERRORS * sizeof(struct error_data));
            i = last_error + backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            last_error = fatal = non_fatal = 0;
            break;
        default:
            i = -1;
        }
        return i;

    case EH_SEARCH:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            for (i = 0; i < last_error; ++i)
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return 1;
            break;
        case EH_BACKUP_DB:
            for (i = 0; i < last_error; ++i)
                if (strcmp(error, backup[i].error_msg) == 0)
                    return 2;
            break;
        case EH_ALL_DB:
            for (i = 0; i < last_error; ++i)
            {
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return 1;
                else if (strcmp(error, backup[i].error_msg) == 0)
                    return 2;
            }
        }
        return -1;

    // Return the number of saved errors
    case EH_ERROR_COUNT:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            return non_fatal;
        case EH_BACKUP_DB:
            return fatal;
        case EH_ALL_DB:
            return non_fatal + fatal;
        }

    case EH_BACKUP:
        error_handler(NULL, EH_CLEAR, EH_BACKUP);
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
        error_handler(NULL, EH_CLEAR, EH_MAIN_DB);
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

void print_errors(char *expr, int error_pos)
{
    int i;
    fputs(expr, stderr);
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