/*
Copyright (C) 2022-2023 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "evaluator.h"
#include "m_errors.h"
#include "parser.h"
#include "scientific.h"
#include "string_tools.h"
#include "tms_math_strs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *_tms_g_expr = NULL;
double complex tms_g_ans = 0;

bool _tms_do_init = true;
bool _tms_debug = false;

char **tms_g_all_func_names;

tms_var tms_g_builtin_vars[] = {{"i", I, true}, {"pi", M_PI, true}, {"exp", M_E, true}, {"c", 299792458, true}};
tms_var *tms_g_vars = NULL;

char *tms_g_illegal_names[] = {"x", "e", "E", "i"};
const int tms_g_illegal_names_count = array_length(tms_g_illegal_names);
int tms_g_func_count = 0, tms_g_var_count, tms_g_var_max = array_length(tms_g_builtin_vars);

void tmsolve_init()
{
    int i;
    if (_tms_do_init == true)
    {
        // Seed the random number generator
        srand(time(NULL));
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
        _tms_do_init = false;
    }
}

int tms_new_var(char *name, bool is_constant)
{
    int i;
    // Check if the name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_error_handler(EH_SAVE, ILLEGAL_VARIABLE_NAME, EH_FATAL_ERROR, -1);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, INVALID_VARIABLE_NAME, EH_FATAL_ERROR, -1);
        return -1;
    }

    // Check if the variable already exists
    for (i = 0; i < tms_g_var_count; ++i)
    {
        if (strcmp(tms_g_vars[i].name, name) == 0)
        {
            if (tms_g_vars[i].is_constant)
            {
                tms_error_handler(EH_SAVE, OVERWRITE_CONST_VARIABLE, EH_FATAL_ERROR, -1);
                return -1;
            }
            return i;
        }
    }

    // Create a new variable
    if (i == tms_g_var_count)
    {
        // Dynamically expand size if required
        if (tms_g_var_count == tms_g_var_max)
        {
            tms_g_var_max *= 2;
            tms_g_vars = realloc(tms_g_vars, tms_g_var_max * sizeof(tms_var));
        }
        tms_g_vars[tms_g_var_count].name = malloc((strlen(name) + 1) * sizeof(char));
        // Initialize the new variable to zero
        tms_g_vars[tms_g_var_count].value = 0;
        tms_g_vars[tms_g_var_count].is_constant = is_constant;
        strcpy(tms_g_vars[tms_g_var_count].name, name);
        ++tms_g_var_count;
        return i;
    }
    return -1;
}

tms_math_expr *tms_dup_mexpr(tms_math_expr *M)
{
    if (M == NULL)
        return NULL;

    tms_math_expr *NM = malloc(sizeof(tms_math_expr));
    // Copy the math expression
    *NM = *M;
    NM->subexpr_ptr = malloc(NM->subexpr_count * sizeof(tms_math_subexpr));
    // Copy subexpressions
    memcpy(NM->subexpr_ptr, M->subexpr_ptr, M->subexpr_count * sizeof(tms_math_subexpr));

    int op_count, node_count;
    tms_math_subexpr *S, *NS;

    // Allocate nodes and set the result pointers
    for (int s = 0; s < NM->subexpr_count; ++s)
    {
        // New variables to keep lines from becoming too long
        NS = NM->subexpr_ptr + s;
        S = M->subexpr_ptr + s;
        if (M->subexpr_ptr[s].nodes != NULL)
        {
            op_count = M->subexpr_ptr[s].op_count;
            node_count = (op_count > 0 ? op_count : 1);
            NS->nodes = malloc(node_count * sizeof(tms_op_node));
            // Copy nodes
            memcpy(NS->nodes, S->nodes, node_count * sizeof(tms_op_node));

            // Copying nodes will also copy result and next pointers which point to the original node members
            // Regenerate the pointers using parser functions
            _tms_set_evaluation_order(NS);
            _tms_set_result_pointers(NM, s);
        }
        else
        {
            // Likely this is an extended function subexpr
            NS->result = malloc(sizeof(double complex *));
        }
    }

    // Fix the subexpressions result pointers
    for (int s_index = 0; s_index < NM->subexpr_count; ++s_index)
    {
        NS = NM->subexpr_ptr + s_index;
        S = M->subexpr_ptr + s_index;
        void *result = S->result, *start, *end, *tmp;

        // Find which operand in the original expression received the answer of the current subexpression
        tmp = *(double complex **)result;
        for (int s = 0; s < M->subexpr_count; ++s)
        {
            op_count = M->subexpr_ptr[s].op_count;
            node_count = (op_count > 0 ? op_count : 1);
            start = M->subexpr_ptr[s].nodes;
            end = M->subexpr_ptr[s].nodes + node_count;

            // The nodes are contiguous, so we can pinpoint the correct node by pointer comparisons
            if (tmp >= start && tmp <= end)
            {
                int n = (tmp - start) / sizeof(tms_op_node);

                if (&(M->subexpr_ptr[s].nodes[n].right_operand) == *(double complex **)result)
                    *(NM->subexpr_ptr[s_index].result) = &(NM->subexpr_ptr[s].nodes[n].right_operand);
                else if (&(M->subexpr_ptr[s].nodes[n].left_operand) == *(double complex **)result)
                    *(NM->subexpr_ptr[s_index].result) = &(NM->subexpr_ptr[s].nodes[n].left_operand);

                break;
            }
        }
    }

    // If the nodes have unknowns, regenerate the unknowns pointers array
    if (NM->unknown_count > 0)
        _tms_set_unknowns_data(NM);

    return NM;
}

int tms_error_handler(int _mode, ...)
{
    static int last_error = 0, fatal = 0, non_fatal = 0, backup_error_count = 0, backup_fatal, backup_non_fatal;
    static tms_error_data main_table[EH_MAX_ERRORS], backup_table[EH_MAX_ERRORS];

    va_list arguments;
    char *error;
    va_start(arguments, _mode);
    int i, arg2;

    switch (_mode)
    {
    case EH_SAVE:
        error = va_arg(arguments, char *);
        // Case of error table being full
        if (last_error == EH_MAX_ERRORS - 1)
        {
            free(main_table[0].error_msg);
            // Before ovewriting oldest error, update error counters to reflect the new state
            if (main_table[0].fatal)
                --fatal;
            else
                --non_fatal;

            // Overwrite the oldest error
            for (i = 0; i < EH_MAX_ERRORS - 1; ++i)
                main_table[i] = main_table[i + 1];

            // The last position is free, update last_error
            --last_error;
        }
        main_table[last_error].error_msg = strdup(error);
        main_table[last_error].expr_len = strlen(_tms_g_expr);
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_NONFATAL_ERROR:
            main_table[last_error].fatal = false;
            ++non_fatal;
            break;
        case EH_FATAL_ERROR:
            main_table[last_error].fatal = true;
            ++fatal;
            break;
        default:
            return -1;
        }

        int err_position = va_arg(arguments, int);
        if (err_position != -1)
        {
            // Center the error in the string
            if (err_position > 49)
            {
                strncpy(main_table[last_error].bad_snippet, _tms_g_expr + err_position - 24, 49);
                main_table[last_error].bad_snippet[49] = '\0';
                main_table[last_error].relative_index = 24;
                main_table[last_error].real_index = err_position;
            }
            else
            {
                strncpy(main_table[last_error].bad_snippet, _tms_g_expr, 49);
                main_table[last_error].bad_snippet[49] = '\0';
                main_table[last_error].relative_index = err_position;
                main_table[last_error].real_index = err_position;
            }
        }
        else
            main_table[last_error].relative_index = main_table[last_error].real_index = -1;

        last_error = fatal + non_fatal;
        return 0;

    case EH_PRINT:
        for (i = 0; i < last_error; ++i)
        {
            tms_print_error(main_table[i]);
        }
        return tms_error_handler(EH_CLEAR, EH_MAIN_DB);

    case EH_CLEAR:
        arg2 = va_arg(arguments, int);
        switch (arg2)
        {
        case EH_MAIN_DB:
            for (i = 0; i < last_error; ++i)
                free(main_table[i].error_msg);

            memset(main_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            // i carries the number of errors for the return value because the counter is reset here
            i = last_error;
            last_error = fatal = non_fatal = 0;
            break;
        case EH_BACKUP_DB:
            for (i = 0; i < backup_error_count; ++i)
                free(main_table[i].error_msg);

            memset(backup_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            i = backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            break;
        case EH_ALL_DB:
            for (i = 0; i < last_error; ++i)
                free(main_table[i].error_msg);
            for (i = 0; i < backup_error_count; ++i)
                free(main_table[i].error_msg);

            memset(main_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
            memset(backup_table, 0, EH_MAX_ERRORS * sizeof(struct tms_error_data));
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
                if (strcmp(error, main_table[i].error_msg) == 0)
                    return EH_MAIN_DB;
            break;
        case EH_BACKUP_DB:
            for (i = 0; i < last_error; ++i)
                if (strcmp(error, backup_table[i].error_msg) == 0)
                    return EH_BACKUP_DB;
            break;
        case EH_ALL_DB:
            for (i = 0; i < last_error; ++i)
            {
                if (strcmp(error, main_table[i].error_msg) == 0)
                    return EH_MAIN_DB;
                else if (strcmp(error, backup_table[i].error_msg) == 0)
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
        return -1;

    case EH_BACKUP:
        tms_error_handler(EH_CLEAR, EH_BACKUP);
        // Copy the current error database to the backup database
        for (i = 0; i < last_error; ++i)
            backup_table[i] = main_table[i];
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
            main_table[i] = backup_table[i];
        last_error = backup_error_count;
        fatal = backup_fatal;
        non_fatal = backup_non_fatal;
        // Remove all references of the errors from the backup
        for (i = 0; i < backup_error_count; ++i)
            backup_table[i].error_msg = NULL;
        break;
    }
    return -1;
}

void tms_print_error(tms_error_data E)
{
    int i;
    puts(E.error_msg);
    if (E.real_index != -1)
    {
        if (E.real_index > 49)
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
        fprintf(stderr, "\n");
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