/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
/*
Error handling function.
arg 1:
1: Save the *error to the errors database, arg 2: ( 0: not fatal-stack; 1: fatal-stack; 2: not fatal-heap; 3: fatal-heap).
For fatal errors, arg3 must have the index of the error (-1 means don't error_print)
2: Print the errors to stdout and clear the database, return number of errors printed.
3: Clear the error database. arg 2: clear (0: main; 1: backup; 2: all)
4: Search for *error in the errors database, return 1 on match in main, 2 in backup. arg 2: search (0: main; 1: backup; 2: all)
5: Return the amount of errors in the database. arg 2: errors in (0: main; 1: backup; 2: all)
6: Backup current errors, making room for new ones
7: Restore the backed up errors, clearing the current errors in the process.
*/
int error_handler(char *error, int arg1, ...)
{
    static int error_count = 0, fatal = 0, non_fatal = 0, backup_error_count = 0, backup_fatal, backup_non_fatal;
    va_list arguments;
    va_start(arguments, arg1);
    int i, arg2;
    struct error_structure
    {
        char *error_msg, err_exp[50];
        bool heap_allocated, fatal_error;
        int error_index;
    };
    static struct error_structure error_table[10], backup[10];
    switch (arg1)
    {
    case 1:
        error_table[error_count].error_msg = error;
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            error_table[error_count].heap_allocated = false;
            error_table[error_count].fatal_error = false;
            ++non_fatal;
            break;
        case 1:
            error_table[error_count].heap_allocated = false;
            error_table[error_count].fatal_error = true;
            error_table[error_count].error_index = va_arg(arguments, int);
            ++fatal;
            break;
        case 2:
            error_table[error_count].heap_allocated = true;
            error_table[error_count].fatal_error = false;
            ++non_fatal;
            break;
        case 3:
            error_table[error_count].heap_allocated = true;
            error_table[error_count].fatal_error = true;
            error_table[error_count].error_index = va_arg(arguments, int);
            ++fatal;
            break;
        default:
            return -1;
        }
        error_count = fatal + non_fatal;
        return 0;

    case 2:
        arg2 = va_arg(arguments, int);

        for (i = 0; i < error_count; ++i)
        {
            puts(error_table[i].error_msg);
            if (error_table[i].fatal_error == true && error_table[i].error_index != -1)
                error_print(error_table[i].err_exp, error_table[i].error_index);
        }
        error_handler(NULL, 3, 0);
        if (arg2 == 1)
            printf("\n");

    case 3:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            for (i = 0; i < error_count; ++i)
                if (error_table[i].heap_allocated == true)
                {
                    free(error_table[i].error_msg);
                    error_table[i].error_msg = NULL;
                }
            error_count = fatal = non_fatal = 0;
            break;
        case 1:
            for (i = 0; i < backup_error_count; ++i)
                if (backup[i].heap_allocated == true)
                {
                    free(backup[i].error_msg);
                    backup[i].error_msg = NULL;
                }
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            break;
        case 2:
            for (i = 0; i < error_count; ++i)
                if (error_table[i].heap_allocated == true)
                {
                    free(error_table[i].error_msg);
                    error_table[i].error_msg = NULL;
                }
            error_count = i;
            for (i = 0; i < backup_error_count; ++i)
                if (backup[i].heap_allocated == true)
                {
                    free(backup[i].error_msg);
                    backup[i].error_msg = NULL;
                }
            i += error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            error_count = fatal = non_fatal = 0;
            break;
        default:
            return false;
        }
        return i;

    case 4:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            for (i = 0; i < error_count; ++i)
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return 1;
            break;
        case 1:
            for (i = 0; i < error_count; ++i)
                if (strcmp(error, backup[i].error_msg) == 0)
                    return 2;
            break;
        case 2:
            for (i = 0; i < error_count; ++i)
            {
                if (strcmp(error, error_table[i].error_msg) == 0)
                    return 1;
                else if (strcmp(error, backup[i].error_msg) == 0)
                    return 2;
            }
        }
        return -1;

    case 5:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            return non_fatal;
        case 1:
            return fatal;
        case 2:
            return non_fatal + fatal;
        }

    case 6:
        // Clear the previous backup
        error_handler(NULL, 3, 1);
        // Copy the current error database to the backup database
        for (i = 0; i < error_count; ++i)
            backup[i] = error_table[i];
        backup_error_count = error_count;
        backup_non_fatal = non_fatal;
        backup_fatal = fatal;
        // reset current database error count
        error_count = fatal = non_fatal = 0;
        return 0;

    case 7:
        // Clear current database
        error_handler(NULL, 3);
        for (i = 0; i < backup_error_count; ++i)
            error_table[i] = backup[i];
        error_count = backup_error_count;
        fatal = backup_fatal;
        non_fatal = backup_non_fatal;
        // Remove all references of the errors from the backup
        for (i = 0; i < backup_error_count; ++i)
            backup[i].error_msg = NULL;
        break;
    }
    return -5;
}

// Function that points at the location of the error found
void error_print(char *exp, int p)
{
    int i;
    puts(exp);
    for (i = 0; i < p; ++i)
        printf("~");
    printf("^\n");
}
// Function to sort subexpressions by depth, also for use with qsort
int compare_subexps_depth(const void *a, const void *b)
{
    if ((*((s_expression **)a))->depth < (*((s_expression **)b))->depth)
        return 1;
    else if ((*((s_expression **)a))->depth > (*((s_expression **)b))->depth)
        return -1;
    else
        return 0;
}