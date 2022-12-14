/*
Copyright (C) 2021-2022 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
char *glob_expr = NULL;

int error_handler(char *error, int arg1, ...)
{
    static int error_count = 0, fatal = 0, non_fatal = 0, backup_error_count = 0, backup_fatal, backup_non_fatal;
    va_list arguments;
    va_start(arguments, arg1);
    int i, arg2;
    struct error_structure
    {
        char *error_msg, err_str[50];
        bool fatal_error;
        int error_index;
    };
    static struct error_structure error_table[MAX_ERRORS], backup[MAX_ERRORS];
    switch (arg1)
    {
    // Save mode
    case 1:
        error_table[error_count].error_msg = error;
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            error_table[error_count].fatal_error = false;
            ++non_fatal;
            break;
        case 1:
            error_table[error_count].fatal_error = true;
            int position = va_arg(arguments, int);
            if (position != -1)
            {
                // Center the error in the string
                if (position > 49)
                {
                    strncpy(error_table[error_count].err_str, glob_expr + position - 24, 49);
                    error_table[error_count].err_str[49] = '\0';
                    error_table[error_count].error_index = 24;
                }
                else
                {
                    strcpy(error_table[error_count].err_str, glob_expr);
                    error_table[error_count].error_index = position;
                }
            }
            ++fatal;
            break;
        default:
            return -1;
        }
        error_count = fatal + non_fatal;
        return 0;
    // Print errors
    case 2:
        for (i = 0; i < error_count; ++i)
        {
            puts(error_table[i].error_msg);
            if (error_table[i].fatal_error == true && error_table[i].error_index != -1)
                error_print(error_table[i].err_str, error_table[i].error_index);
        }
        error_handler(NULL, 3, 0);

    // Clear errors
    case 3:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case 0:
            memset(error_table, 0, MAX_ERRORS * sizeof(struct error_structure));
            i = error_count;
            error_count = fatal = non_fatal = 0;
            break;
        case 1:
            memset(backup, 0, MAX_ERRORS * sizeof(struct error_structure));
            i = backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            break;
        case 2:
            memset(error_table, 0, MAX_ERRORS * sizeof(struct error_structure));
            memset(backup, 0, MAX_ERRORS * sizeof(struct error_structure));
            i = error_count + backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            error_count = fatal = non_fatal = 0;
            break;
        default:
            i = -1;
        }
        return i;
    // Search for a specific error (search by pointer not text)
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

    // Return the number of saved errors
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

    // Backup errors, useful if data is processed in another way to prevent mixing of errors
    case 6:
        // Clear the previous backup
        error_handler(NULL,3, 1);
        // Copy the current error database to the backup database
        for (i = 0; i < error_count; ++i)
            backup[i] = error_table[i];
        backup_error_count = error_count;
        backup_non_fatal = non_fatal;
        backup_fatal = fatal;
        // reset current database error count
        error_count = fatal = non_fatal = 0;
        return 0;

    // Overwrite main error database with the backup error database
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
    return -1;
}

void error_print(char *expr, int error_pos)
{
    int i;
    puts(expr);
    for (i = 0; i < error_pos; ++i)
        printf("~");
    printf("^\n");
}
