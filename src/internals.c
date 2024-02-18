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
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double complex tms_g_ans = 0;
int64_t tms_g_int_ans = 0;

bool _tms_do_init = true;
bool _tms_debug = false;

atomic_bool _parser_lock = false, _int_parser_lock = false;
atomic_bool _ufunc_lock = false;
atomic_bool _variables_lock = false, _int_variables_lock = false;
atomic_bool _evaluator_lock = false, _int_evaluator_lock = false;

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
        tms_g_all_func_count = tms_g_rc_func_count + tms_g_extf_count;

        while (tms_g_all_func_max < tms_g_all_func_count)
            tms_g_all_func_max *= 2;

        tms_g_all_func_names = malloc(tms_g_all_func_max * sizeof(char *));

        i = 0;
        for (j = 0; j < tms_g_rc_func_count; ++j)
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
        for (j = 0; j < tms_g_int_extf_count; ++j)
            tms_g_all_int_func_names[i++] = tms_g_int_extf[j].name;

        _tms_do_init = false;
    }
}

void tms_lock_parser(int variant)
{
    switch (variant)
    {
    case TMS_PARSER:
        while (atomic_flag_test_and_set(&_parser_lock))
            ;
        while (atomic_flag_test_and_set(&_variables_lock))
            ;
        while (atomic_flag_test_and_set(&_ufunc_lock))
            ;
        return;

    case TMS_INT_PARSER:
        while (atomic_flag_test_and_set(&_int_parser_lock))
            ;
        while (atomic_flag_test_and_set(&_int_variables_lock))
            ;
        return;

    default:
        fputs("libtmsolve: Error while locking parsers: invalid ID...", stderr);
        exit(1);
    }
}

void tms_unlock_parser(int variant)
{
    switch (variant)
    {
    case TMS_PARSER:
        atomic_flag_clear(&_parser_lock);
        atomic_flag_clear(&_variables_lock);
        atomic_flag_clear(&_ufunc_lock);
        return;

    case TMS_INT_PARSER:
        atomic_flag_clear(&_int_parser_lock);
        atomic_flag_clear(&_int_variables_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking parsers: invalid ID...", stderr);
        exit(1);
    }
}

void tms_lock_evaluator(int variant)
{
    switch (variant)
    {
    case TMS_EVALUATOR:
        while (atomic_flag_test_and_set(&_evaluator_lock))
            ;
        while (atomic_flag_test_and_set(&_ufunc_lock))
            ;
        return;

    case TMS_INT_EVALUATOR:
        while (atomic_flag_test_and_set(&_int_evaluator_lock))
            ;
        return;

    default:
        fputs("libtmsolve: Error while locking evaluator: invalid ID...", stderr);
        exit(1);
    }
}

void tms_unlock_evaluator(int variant)
{
    switch (variant)
    {
    case TMS_EVALUATOR:
        atomic_flag_clear(&_evaluator_lock);
        atomic_flag_clear(&_ufunc_lock);
        return;

    case TMS_INT_EVALUATOR:
        atomic_flag_clear(&_int_evaluator_lock);
        return;

    default:
        fputs("libtmsolve: Error while unlocking evaluator: invalid ID...", stderr);
        exit(1);
    }
}

bool _tms_validate_args_count(int expected, int actual, int facility_id)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_error_handler(EH_SAVE, facility_id, TOO_FEW_ARGS, EH_FATAL, NULL);
    else if (expected < actual)
        tms_error_handler(EH_SAVE, facility_id, TOO_MANY_ARGS, EH_FATAL, NULL);
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
            tms_error_handler(EH_SAVE, TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, TMS_PARSER, INVALID_NAME, EH_FATAL, NULL);
        return -1;
    }

    // Check if the variable already exists
    i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        if (tms_g_vars[i].is_constant)
        {
            tms_error_handler(EH_SAVE, TMS_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL);
            return -1;
        }
        else
            return i;
    }
    else
    {
        if (tms_find_str_in_array(name, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC) != -1)
        {
            tms_error_handler(EH_SAVE, TMS_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL);
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
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, ILLEGAL_NAME, EH_FATAL, NULL);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, TMS_INT_PARSER, INVALID_NAME, EH_FATAL, NULL);
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
            tms_error_handler(EH_SAVE, TMS_INT_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL);
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

bool tms_ufunc_has_self_ref(tms_math_expr *F)
{
    tms_math_subexpr *S = F->S;
    for (int s_index = 0; s_index < F->subexpr_count; ++s_index)
    {
        // Detect self reference (new function has pointer to its previous version)
        if (S[s_index].func_type == TMS_F_RUNTIME && S[s_index].func.runtime->F == F)
        {
            tms_error_handler(EH_SAVE, TMS_PARSER, NO_FSELF_REFERENCE, EH_FATAL, NULL);
            return true;
        }
    }
    return false;
}

bool is_ufunc_referenced_by(tms_math_expr *referrer, tms_math_expr *F)
{
    tms_math_subexpr *S = referrer->S;
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
    tms_math_subexpr *S = F->S;
    for (i = 0; i < F->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_RUNTIME)
        {
            // A self reference, none of our business here
            if (S[i].func.runtime->F == F)
                continue;

            if (is_ufunc_referenced_by(S[i].func.runtime->F, F))
            {
                tms_error_handler(EH_SAVE, TMS_PARSER, NO_FCIRCULAR_REFERENCE, EH_FATAL, NULL);
                return true;
            }
        }
    }
    return false;
}

int tms_set_ufunction(char *name, char *function)
{
    int i;
    int ufunc_index = tms_find_str_in_array(name, tms_g_ufunc, tms_g_ufunc_count, TMS_F_RUNTIME);
    ;

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_error_handler(EH_SAVE, TMS_PARSER, INVALID_NAME, EH_FATAL, NULL);
        return -1;
    }

    // Check if the function name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_error_handler(EH_SAVE, TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL);
            return -1;
        }
    }

    // Check if the name was already used by builtin functions
    i = tms_find_str_in_array(name, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC);
    if (i != -1 && ufunc_index == -1)
    {
        tms_error_handler(EH_SAVE, TMS_PARSER, NO_FUNCTION_SHADOWING, EH_FATAL, NULL);
        return -1;
    }

    // Check if the name is used by a variable
    i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        tms_error_handler(EH_SAVE, TMS_PARSER, FUNCTION_NAME_MATCHES_VAR, EH_FATAL, NULL);
        return -1;
    }

    // Function already exists
    if (ufunc_index != -1)
    {
        i = ufunc_index;
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
            if (tms_ufunc_has_self_ref(new) || tms_has_ufunc_circular_refs(new))
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
            DYNAMIC_RESIZE(tms_g_all_func_names, tms_g_all_func_count, tms_g_all_func_max, char *);
            tms_g_all_func_names[tms_g_all_func_count++] = tms_g_ufunc[i].name;
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
    NM->S = malloc(NM->subexpr_count * sizeof(tms_math_subexpr));
    // Copy subexpressions
    memcpy(NM->S, M->S, M->subexpr_count * sizeof(tms_math_subexpr));

    int op_count, node_count;
    tms_math_subexpr *S, *NS;

    // Allocate nodes and set the result pointers
    // Here NM is the new math expression and NS is the new subexpressions array
    for (int s = 0; s < NM->subexpr_count; ++s)
    {
        // New variables to keep lines from becoming too long
        NS = NM->S + s;
        S = M->S + s;
        if (M->S[s].nodes != NULL)
        {
            op_count = M->S[s].op_count;
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
        S = M->S + s;
        void *result = S->result, *start, *end, *tmp;

        // Find which operand in the original expression received the answer of the current subexpression
        tmp = *(double complex **)result;
        for (int i = 0; i < M->subexpr_count; ++i)
        {
            op_count = M->S[i].op_count;
            node_count = (op_count > 0 ? op_count : 1);
            start = M->S[i].nodes;
            end = M->S[i].nodes + node_count;

            // The nodes are contiguous, so we can pinpoint the correct node by pointer comparisons
            if (tmp >= start && tmp <= end)
            {
                int n = (tmp - start) / sizeof(tms_op_node);

                if (&(M->S[i].nodes[n].right_operand) == *(double complex **)result)
                    *(NM->S[s].result) = &(NM->S[i].nodes[n].right_operand);
                else if (&(M->S[i].nodes[n].left_operand) == *(double complex **)result)
                    *(NM->S[s].result) = &(NM->S[i].nodes[n].left_operand);

                break;
            }
        }
    }

    // If the nodes have unknowns, regenerate the unknowns pointers array
    if (NM->unknowns_count > 0)
        _tms_generate_unknowns_refs(NM);

    return NM;
}

int _tms_error_handler_unsafe(int _mode, va_list handler_args)
{
    static int last_error = 0, fatal = 0, non_fatal = 0;
    static tms_error_data error_table[EH_MAX_ERRORS];

    int i, error_type;
    int facility_id = va_arg(handler_args, int);

    switch (_mode)
    {
    case EH_SAVE: {
        char *error_msg = va_arg(handler_args, char *);
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
            for (i = 0; i < EH_MAX_ERRORS - 1; ++i)
                error_table[i] = error_table[i + 1];

            // The last position is free, update last_error
            --last_error;
        }
        error_table[last_error].message = strdup(error_msg);
        error_table[last_error].facility_id = facility_id;
        error_type = va_arg(handler_args, int);
        switch (error_type)
        {
        case EH_NONFATAL:
            error_table[last_error].fatal = false;
            ++non_fatal;
            break;
        case EH_FATAL:
            error_table[last_error].fatal = true;
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

            error_table[last_error].expr_len = strlen(expr);

            if (error_position < 0 || error_position > error_table[last_error].expr_len)
            {
                fputs("libtmsolve warning: Error index out of expression range, ignoring...\n\n", stderr);
                error_table[last_error].relative_index = -1;
                error_table[last_error].real_index = -1;
                error_table[last_error].bad_snippet[0] = '\0';
                last_error = fatal + non_fatal;
                return 1;
            }

            // Center the error in the string
            if (error_position > 49)
            {
                strncpy(error_table[last_error].bad_snippet, expr + error_position - 24, 49);
                error_table[last_error].bad_snippet[49] = '\0';
                error_table[last_error].relative_index = 24;
                error_table[last_error].real_index = error_position;
            }
            else
            {
                strncpy(error_table[last_error].bad_snippet, expr, 49);
                error_table[last_error].bad_snippet[49] = '\0';
                error_table[last_error].relative_index = error_position;
                error_table[last_error].real_index = error_position;
            }
        }
        else
            error_table[last_error].relative_index = error_table[last_error].real_index = -1;

        last_error = fatal + non_fatal;
        return 0;
    }
    case EH_PRINT:
        if (facility_id == TMS_ALL_FACILITIES)
        {
            for (i = 0; i < last_error; ++i)
                tms_print_error(error_table[i]);
        }
        else
        {
            for (i = 0; i < last_error; ++i)
                if (error_table[i].facility_id == facility_id)
                    tms_print_error(error_table[i]);
        }

        // Intentional fall through since printing errors should clear them too
    case EH_CLEAR: {
        int deleted_count = 0;
        if (facility_id == TMS_ALL_FACILITIES)
        {
            for (i = 0; i < last_error; ++i)
                free(error_table[i].message);

            deleted_count = last_error;
            last_error = fatal = non_fatal = 0;
        }
        else
        {
            for (i = 0; i < last_error; ++i)
            {
                if (error_table[i].facility_id == facility_id)
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
                    for (int j = i + i; j < last_error; ++j)
                    {
                        error_table[i] = error_table[j];
                        // Move the hole forward
                        error_table[j].message = NULL;
                    }
                }
            }

            last_error = fatal + non_fatal;
        }
        return deleted_count;
    }

    case EH_SEARCH: {
        char *error_msg = va_arg(handler_args, char *);
        if (facility_id == TMS_ALL_FACILITIES)
        {
            for (i = 0; i < last_error; ++i)
                if (strcmp(error_msg, error_table[i].message) == 0)
                    return i;
        }
        else
        {
            for (i = 0; i < last_error; ++i)
                if (facility_id == error_table[i].facility_id && strcmp(error_msg, error_table[i].message) == 0)
                    return i;
        }

        return -1;
    }

    // Return the number of saved errors
    case EH_ERROR_COUNT: {
        error_type = va_arg(handler_args, int);
        if (facility_id == TMS_ALL_FACILITIES)
        {
            switch (error_type)
            {
            case EH_NONFATAL:
                return non_fatal;
            case EH_FATAL:
                return fatal;
            case EH_ALL_ERRORS:
                return non_fatal + fatal;
            }
        }
        else
        {
            // These shadow the static variables
            int fatal = 0, non_fatal = 0;
            for (int i = 0; i < last_error; ++i)
                if (facility_id == error_table[i].facility_id)
                {
                    if (error_table[i].fatal)
                        ++fatal;
                    else
                        ++non_fatal;
                }

            switch (error_type)
            {
            case EH_NONFATAL:
                return non_fatal;
            case EH_FATAL:
                return fatal;
            case EH_ALL_ERRORS:
                return non_fatal + fatal;
            }
        }
        return -1;
    }

    case EH_MODIFY: {
        // Find the last error
        for (i = last_error - 1; i >= 0; --i)
        {
            if (error_table[i].facility_id == facility_id)
                break;
        }
        if (i == -1)
            return -1;

        char *expr = va_arg(handler_args, char *);
        error_table[i].expr_len = strlen(expr);
        int error_position = va_arg(handler_args, int);

        if (error_position < 0 || error_position > error_table[i].expr_len)
        {
            fputs("libtmsolve warning: Error index out of expression range, ignoring...\n\n", stderr);
            error_table[i].relative_index = -1;
            error_table[i].real_index = -1;
            error_table[i].bad_snippet[0] = '\0';
            return 1;
        }

        // Center the error in the string
        if (error_position > 49)
        {
            strncpy(error_table[i].bad_snippet, expr + error_position - 24, 49);
            error_table[i].bad_snippet[49] = '\0';
            error_table[i].relative_index = 24;
            error_table[i].real_index = error_position;
        }
        else
        {
            strncpy(error_table[i].bad_snippet, expr, 49);
            error_table[i].bad_snippet[49] = '\0';
            error_table[i].relative_index = error_position;
            error_table[i].real_index = error_position;
        }
        return 0;
    }
    }
    return -1;
}

int tms_error_handler(int _mode, ...)
{
    va_list handler_args;
    static atomic_bool lock;
    va_start(handler_args, _mode);

    // Wait if the error handler is used by someone else
    while (atomic_flag_test_and_set(&lock))
        ;

    int status = _tms_error_handler_unsafe(_mode, handler_args);
    atomic_flag_clear(&lock);
    return status;
}

void tms_print_error(tms_error_data E)
{
    int i;
    char *facility_name;

    switch (E.facility_id)
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
        fprintf(stderr, "%s\n\n", E.message);
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