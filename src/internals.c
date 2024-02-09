/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "evaluator.h"
#include "int_parser.h"
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

double complex tms_g_ans = 0;
int64_t tms_g_int_ans = 0;

bool _tms_do_init = true;
bool _tms_debug = false;

// Function names used by the parser
char **tms_g_all_func_names;
int tms_g_all_func_count, tms_g_all_func_max = 64;

char **tms_g_all_int_func_names;
int tms_g_all_int_func_count, tms_g_all_int_func_max = 16;

char *tms_g_illegal_names[] = {"x", "i", "ans"};
const int tms_g_illegal_names_count = array_length(tms_g_illegal_names);

tms_var tms_g_builtin_vars[] = {{"i", I, true}, {"pi", M_PI, true}, {"e", M_E, true}, {"c", 299792458, true}};
tms_var *tms_g_vars = NULL;
int tms_g_var_count, tms_g_var_max = array_length(tms_g_builtin_vars);

tms_int_var *tms_g_int_vars = NULL;
int tms_g_int_var_count = 0, tms_g_int_var_max = 8;

tms_ufunc *tms_g_ufunc = NULL;
int tms_g_ufunc_count, tms_g_ufunc_max = 8;

uint64_t tms_int_mask = 0xFFFFFFFF;

int8_t tms_int_mask_size = 32;

void tmsolve_init()
{
    int i, j;
    if (_tms_do_init == true)
    {
        // Seed the random number generator
        srand(time(NULL));

        // Initialize variable names and values arrays
        tms_g_vars = malloc(tms_g_var_max * sizeof(tms_var));
        tms_g_var_count = array_length(tms_g_builtin_vars);

        for (i = 0; i < tms_g_var_count; ++i)
            tms_g_vars[i] = tms_g_builtin_vars[i];

        // Int64 variables
        tms_g_int_vars = malloc(tms_g_int_var_max * sizeof(tms_int_var));

        // Initialize runtime function array
        tms_g_ufunc = malloc(tms_g_ufunc_max * sizeof(tms_ufunc));

        // Generate the array of all floating point related functions
        tms_g_all_func_count = tms_g_rcfunct_count + tms_g_extf_count;

        while (tms_g_all_func_max < tms_g_all_func_count)
            tms_g_all_func_max *= 2;

        tms_g_all_func_names = malloc(tms_g_all_func_max * sizeof(char *));

        i = 0;
        for (j = 0; j < tms_g_rcfunct_count; ++j)
            tms_g_all_func_names[i++] = tms_g_rc_func[j].name;
        for (j = 0; j < tms_g_extf_count; ++j)
            tms_g_all_func_names[i++] = tms_g_extf[j].name;

        // Generate the array of all int functions
        tms_g_all_int_func_count = tms_g_int_func_count + tms_g_int_extf_count;

        while (tms_g_all_int_func_max < tms_g_all_int_func_count)
            tms_g_all_int_func_max *= 2;

        tms_g_all_int_func_names = malloc(tms_g_all_int_func_max * sizeof(char *));

        i = 0;
        for (j = 0; j < tms_g_int_func_count; ++j)
            tms_g_all_int_func_names[i++] = tms_g_int_func[j].name;
        for (j = 0; j < tms_g_extf_count; ++j)
            tms_g_all_int_func_names[i++] = tms_g_int_extf[j].name;

        _tms_do_init = false;
    }
}

bool _tms_validate_args_count(int expected, int actual)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_error_handler(EH_SAVE, TOO_FEW_ARGS, EH_FATAL_ERROR, NULL);
    else if (expected < actual)
        tms_error_handler(EH_SAVE, TOO_MANY_ARGS, EH_FATAL_ERROR, NULL);
    return false;
}

int tms_new_var(char *name, bool is_constant)
{
    int i;
    // Check if the name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_error_handler(EH_SAVE, ILLEGAL_NAME, EH_FATAL_ERROR, NULL);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, INVALID_NAME, EH_FATAL_ERROR, NULL);
        return -1;
    }

    // Check if the variable already exists
    i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        if (tms_g_vars[i].is_constant)
        {
            tms_error_handler(EH_SAVE, OVERWRITE_CONST_VARIABLE, EH_FATAL_ERROR, NULL);
            return -1;
        }
        else
            return i;
    }
    else
    {
        if (tms_find_str_in_array(name, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC) != -1)
        {
            tms_error_handler(EH_SAVE, VAR_NAME_MATCHES_FUNCTION, EH_FATAL_ERROR, NULL);
            return -1;
        }
        else
        {
            // Create a new variable
            DYNAMIC_RESIZE(tms_g_vars, tms_g_var_count, tms_g_var_max, tms_var);
            tms_g_vars[tms_g_var_count].name = strdup(name);
            // Initialize the new variable to zero
            tms_g_vars[tms_g_var_count].value = 0;
            tms_g_vars[tms_g_var_count].is_constant = is_constant;
            ++tms_g_var_count;
            return tms_g_var_count - 1;
        }
    }
}

int tms_new_int_var(char *name)
{
    int i;
    // Check if the name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_error_handler(EH_SAVE, ILLEGAL_NAME, EH_FATAL_ERROR, NULL);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, INVALID_NAME, EH_FATAL_ERROR, NULL);
        return -1;
    }

    // Check if the variable already exists
    i = tms_find_str_in_array(name, tms_g_int_vars, tms_g_int_var_count, TMS_V_INT64);

    if (i != -1)
        return i;
    else
    {
        if (tms_find_str_in_array(name, tms_g_all_int_func_names, tms_g_all_int_func_count, TMS_NOFUNC) != -1)
        {
            tms_error_handler(EH_SAVE, VAR_NAME_MATCHES_FUNCTION, EH_FATAL_ERROR, NULL);
            return -1;
        }
        // Create a new variable
        DYNAMIC_RESIZE(tms_g_int_vars, tms_g_int_var_count, tms_g_int_var_max, tms_int_var);
        tms_g_int_vars[tms_g_int_var_count].name = strdup(name);
        // Initialize the new variable to zero
        tms_g_int_vars[tms_g_int_var_count].value = 0;
        ++tms_g_int_var_count;
        return tms_g_int_var_count - 1;
    }
}

bool tms_has_ufunc_self_ref(tms_math_expr *F)
{
    tms_math_subexpr *S = F->subexpr_ptr;
    for (int s_index = 0; s_index < F->subexpr_count; ++s_index)
    {
        // Detect self reference (new function has pointer to its previous version)
        if (S[s_index].func_type == TMS_F_RUNTIME && S[s_index].func.runtime->F == F)
        {
            tms_error_handler(EH_SAVE, NO_FSELF_REFERENCE, EH_FATAL_ERROR, NULL);
            return true;
        }
    }
    return false;
}

bool is_ufunc_referenced_by(tms_math_expr *referrer, tms_math_expr *F)
{
    tms_math_subexpr *S = referrer->subexpr_ptr;
    for (int s_index = 0; s_index < referrer->subexpr_count; ++s_index)
    {
        if (S[s_index].func_type == TMS_F_RUNTIME && S[s_index].func.runtime->F == F)
            return true;
    }
    return false;
}

bool tms_has_ufunc_circular_refs(tms_math_expr *F)
{
    int i;
    tms_math_subexpr *S = F->subexpr_ptr;
    for (i = 0; i < F->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_RUNTIME)
        {
            // A self reference, none of our business here
            if (S[i].func.runtime->F == F)
                continue;

            if (is_ufunc_referenced_by(S[i].func.runtime->F, F))
            {
                tms_error_handler(EH_SAVE, NO_FCIRCULAR_REFERENCE, EH_FATAL_ERROR, NULL);
                return true;
            }
        }
    }
    return false;
}

int tms_set_ufunction(char *name, char *function)
{
    int i;

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, INVALID_NAME, EH_FATAL_ERROR, NULL);
        return -1;
    }

    // Check if the function name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_error_handler(EH_SAVE, ILLEGAL_NAME, EH_FATAL_ERROR, NULL);
            return -1;
        }
    }

    // Check if the name was already used by builtin functions
    i = tms_find_str_in_array(name, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC);
    if (i != -1)
    {
        tms_error_handler(EH_SAVE, NO_FUNCTION_SHADOWING, EH_FATAL_ERROR, NULL);
        return -1;
    }

    // Check if the name is used by a variable
    i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        tms_error_handler(EH_SAVE, FUNCTION_NAME_MATCHES_VAR, EH_FATAL_ERROR, NULL);
        return -1;
    }

    // Check if the function already exists as runtime function
    i = tms_find_str_in_array(name, tms_g_ufunc, tms_g_ufunc_count, TMS_F_RUNTIME);
    if (i != -1)
    {
        tms_math_expr *old = tms_g_ufunc[i].F, *new = tms_parse_expr(function, true, true);
        if (new == NULL)
        {
            return -1;
        }
        else
        {
            // Update the pointer before checks otherwise they won't work
            tms_g_ufunc[i].F = new;
            // Check for self reference, then fix old references in other functions
            if (tms_has_ufunc_self_ref(new) || tms_has_ufunc_circular_refs(new))
            {
                tms_delete_math_expr(new);
                tms_g_ufunc[i].F = old;
                return -1;
            }
            else
            {
                tms_delete_math_expr(old);
                return 0;
            }
        }
    }
    else
    {
        // Create a new function
        DYNAMIC_RESIZE(tms_g_ufunc, tms_g_ufunc_count, tms_g_ufunc_max, tms_ufunc);
        tms_math_expr *F = tms_parse_expr(function, true, true);
        if (F == NULL)
        {
            return -1;
        }
        else
        {
            i = tms_g_ufunc_count;
            // Set function
            tms_g_ufunc[i].F = F;
            tms_g_ufunc[i].name = strdup(name);
            ++tms_g_ufunc_count;
            return 0;
        }
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
    NM->expr = strdup(M->expr);
    NM->local_expr = NM->expr + (M->local_expr - M->expr);
    NM->subexpr_ptr = malloc(NM->subexpr_count * sizeof(tms_math_subexpr));
    // Copy subexpressions
    memcpy(NM->subexpr_ptr, M->subexpr_ptr, M->subexpr_count * sizeof(tms_math_subexpr));

    int op_count, node_count;
    tms_math_subexpr *S, *NS;

    // Allocate nodes and set the result pointers
    // Here NM is the new math expression and NS is the new subexpressions array
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
    for (int s = 0; s < NM->subexpr_count; ++s)
    {
        S = M->subexpr_ptr + s;
        void *result = S->result, *start, *end, *tmp;

        // Find which operand in the original expression received the answer of the current subexpression
        tmp = *(double complex **)result;
        for (int i = 0; i < M->subexpr_count; ++i)
        {
            op_count = M->subexpr_ptr[i].op_count;
            node_count = (op_count > 0 ? op_count : 1);
            start = M->subexpr_ptr[i].nodes;
            end = M->subexpr_ptr[i].nodes + node_count;

            // The nodes are contiguous, so we can pinpoint the correct node by pointer comparisons
            if (tmp >= start && tmp <= end)
            {
                int n = (tmp - start) / sizeof(tms_op_node);

                if (&(M->subexpr_ptr[i].nodes[n].right_operand) == *(double complex **)result)
                    *(NM->subexpr_ptr[s].result) = &(NM->subexpr_ptr[i].nodes[n].right_operand);
                else if (&(M->subexpr_ptr[i].nodes[n].left_operand) == *(double complex **)result)
                    *(NM->subexpr_ptr[s].result) = &(NM->subexpr_ptr[i].nodes[n].left_operand);

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

    va_list handler_args;
    va_start(handler_args, _mode);
    int i, error_type, db_select;

    switch (_mode)
    {
    case EH_SAVE:
        char *error = va_arg(handler_args, char *);
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
        error_type = va_arg(handler_args, int);
        switch (error_type)
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

        // Get the current expression
        char *expr = va_arg(handler_args, char *);
        if (expr != NULL)
        {
            int error_position = va_arg(handler_args, int);

            main_table[last_error].expr_len = strlen(expr);

            if (error_position < 0 || error_position > main_table[last_error].expr_len)
            {
                fputs("libtmsolve warning: Error index out of expression range, ignoring...\n\n", stderr);
                main_table[last_error].relative_index = -1;
                main_table[last_error].real_index = -1;
                main_table[last_error].bad_snippet[0] = '\0';
                last_error = fatal + non_fatal;
                return 1;
            }

            // Center the error in the string
            if (error_position > 49)
            {
                strncpy(main_table[last_error].bad_snippet, expr + error_position - 24, 49);
                main_table[last_error].bad_snippet[49] = '\0';
                main_table[last_error].relative_index = 24;
                main_table[last_error].real_index = error_position;
            }
            else
            {
                strncpy(main_table[last_error].bad_snippet, expr, 49);
                main_table[last_error].bad_snippet[49] = '\0';
                main_table[last_error].relative_index = error_position;
                main_table[last_error].real_index = error_position;
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
        db_select = va_arg(handler_args, int);
        switch (db_select)
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

            // "i" is used here to preserve total errors for the return value
            i = last_error + backup_error_count;
            backup_error_count = backup_fatal = backup_non_fatal = 0;
            last_error = fatal = non_fatal = 0;
            break;
        default:
            i = -1;
        }
        return i;

    case EH_SEARCH:
        error = va_arg(handler_args, char *);
        db_select = va_arg(handler_args, int);
        switch (db_select)
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
        error_type = va_arg(handler_args, int);
        switch (error_type)
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
    // Error index is provided
    // Print the snippet where the error occured
    if (E.real_index != -1)
    {
        fprintf(stderr, "At col %d: %s\n", E.real_index, E.error_msg);
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
        // No index is provided -> print the error only
        fprintf(stderr, "%s\n\n", E.error_msg);
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