/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "internals.h"
#include "error_handler.h"
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

void tms_set_int_mask(int size)
{
    tms_lock_parser(TMS_INT_PARSER);
    tms_lock_evaluator(TMS_INT_EVALUATOR);
    double power = log2(size);

    // The int mask size should be a power of 2
    if (!tms_is_integer(power))
    {
        power = ceil(power);
        size = pow(2, power);
    }

    if (size < 0)
        fputs("libtmsolve warning: int mask size can't be negative, current size unchanged.", stderr);
    else if (size > 64)
        fputs("libtmsolve warning: The maximum int mask size is 64 bits, current size unchanged.", stderr);
    else if (size == 64)
    {
        tms_int_mask = ~(int64_t)0;
        tms_int_mask_size = 64;
    }
    else
    {
        tms_int_mask = ((int64_t)1 << size) - 1;
        tms_int_mask_size = size;
    }
    tms_unlock_parser(TMS_INT_PARSER);
    tms_unlock_evaluator(TMS_INT_EVALUATOR);
}

bool _tms_validate_args_count(int expected, int actual, int facility_id)
{
    if (expected == actual)
        return true;
    else if (expected > actual)
        tms_save_error(facility_id, TOO_FEW_ARGS, EH_FATAL, NULL, 0);
    else if (expected < actual)
        tms_save_error(facility_id, TOO_MANY_ARGS, EH_FATAL, NULL, 0);
    return false;
}

bool _tms_validate_args_count_range(int actual, int min, int max, int facility_id)
{
    // If max is set to -1 this means no argument limit (for functions like min and max)
    if (max != -1 && actual > max)
        tms_save_error(facility_id, TOO_MANY_ARGS, EH_FATAL, NULL, 0);
    else if (actual < min)
        tms_save_error(facility_id, TOO_FEW_ARGS, EH_FATAL, NULL, 0);
    else
        return true;

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
            tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the variable already exists
    i = tms_find_str_in_array(name, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        if (tms_g_vars[i].is_constant)
        {
            tms_save_error(TMS_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL, 0);
            return -1;
        }
        else
            return i;
    }
    else
    {
        if (tms_find_str_in_array(name, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC) != -1)
        {
            tms_save_error(TMS_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
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

void tms_set_var(double complex value, int index)
{
    while (atomic_flag_test_and_set(&_variables_lock))
        ;
    tms_g_vars[index].value = value;
    atomic_flag_clear(&_variables_lock);
}

int tms_new_int_var(char *name)
{
    int i;
    // Check if the name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(name, tms_g_illegal_names[i]) == 0)
        {
            tms_save_error(TMS_INT_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
            return -1;
        }
    }

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_INT_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
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
            tms_save_error(TMS_INT_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
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

bool _tms_ufunc_has_self_ref(tms_math_expr *F)
{
    tms_math_subexpr *S = F->S;
    for (int s_index = 0; s_index < F->subexpr_count; ++s_index)
    {
        // Detect self reference (new function has pointer to its previous version)
        if (S[s_index].func_type == TMS_F_USER && S[s_index].func.runtime->F == F)
        {
            tms_save_error(TMS_PARSER, NO_FSELF_REFERENCE, EH_FATAL, NULL, 0);
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
        if (S[s_index].func_type == TMS_F_USER && S[s_index].func.runtime->F == F)
            return true;
    }
    return false;
}

bool _tms_ufunc_has_circular_refs(tms_math_expr *F)
{
    int i;
    tms_math_subexpr *S = F->S;
    for (i = 0; i < F->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_USER)
        {
            // A self reference, none of our business here
            if (S[i].func.runtime->F == F)
                continue;

            if (is_ufunc_referenced_by(S[i].func.runtime->F, F))
            {
                tms_save_error(TMS_PARSER, NO_FCIRCULAR_REFERENCE, EH_FATAL, NULL, 0);
                return true;
            }
        }
    }
    return false;
}

int tms_set_ufunction(char *fname, char *unknowns_list, char *function)
{
    int i;
    int ufunc_index = tms_find_str_in_array(fname, tms_g_ufunc, tms_g_ufunc_count, TMS_F_USER);

    // Check if the name has illegal characters
    if (tms_valid_name(fname) == false)
    {
        tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the function name is allowed
    for (i = 0; i < tms_g_illegal_names_count; ++i)
    {
        if (strcmp(fname, tms_g_illegal_names[i]) == 0)
        {
            tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
            return -1;
        }
    }

    // Check if the name was already used by builtin functions
    i = tms_find_str_in_array(fname, tms_g_all_func_names, tms_g_all_func_count, TMS_NOFUNC);
    if (i != -1 && ufunc_index == -1)
    {
        tms_save_error(TMS_PARSER, NO_FUNCTION_SHADOWING, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is used by a variable
    i = tms_find_str_in_array(fname, tms_g_vars, tms_g_var_count, TMS_V_DOUBLE);
    if (i != -1)
    {
        tms_save_error(TMS_PARSER, FUNCTION_NAME_MATCHES_VAR, EH_FATAL, NULL, 0);
        return -1;
    }

    tms_arg_list *unknowns = tms_get_args(unknowns_list);

    if (unknowns->count > 64)
    {
        tms_save_error(TMS_PARSER, TOO_MANY_LABELS, EH_FATAL, NULL, 0);
        tms_free_arg_list(unknowns);
        return -1;
    }

    // Check that names are unique
    if (!tms_is_unique_string_array(unknowns->arguments, unknowns->count))
    {
        tms_save_error(TMS_PARSER, LABELS_NOT_UNIQUE, EH_FATAL, NULL, 0);
        tms_free_arg_list(unknowns);
        return -1;
    }

    // Check that argument names are valid
    for (int j = 0; j < unknowns->count; ++j)
    {
        if (!tms_valid_name(unknowns->arguments[j]))
        {
            tms_save_error(TMS_PARSER, "In user function args: " INVALID_NAME, EH_FATAL, NULL, 0);
            tms_free_arg_list(unknowns);
            return -1;
        }
    }

    // Function already exists
    if (ufunc_index != -1)
    {
        tms_math_expr *new = tms_parse_expr(function, TMS_ENABLE_CMPLX | TMS_ENABLE_UNK, unknowns),
                      *old = tms_g_ufunc[ufunc_index].F;
        tms_math_subexpr *old_subexpr = tms_g_ufunc[ufunc_index].F->S;
        if (new == NULL)
            return -1;
        else
        {
            // Place the newly generated subexpr in the global array for checks to work
            old->S = new->S;
            // Check for self and circular function references
            if (_tms_ufunc_has_self_ref(old) || _tms_ufunc_has_circular_refs(old))
            {
                // Restore the original function since the new one is problematic
                old->S = old_subexpr;
                tms_delete_math_expr(new);
                return -1;
            }
            else
            {
                // Temporarily restore the old subexpr array for deletion
                old->S = old_subexpr;
                tms_delete_math_expr_members(old);
                // Copy the content of the new function math_expr
                *old = *new;
                free(new);
                return 0;
            }
        }
    }
    else
    {
        // Create a new function
        DYNAMIC_RESIZE(tms_g_ufunc, tms_g_ufunc_count, tms_g_ufunc_max, tms_ufunc);
        tms_math_expr *F = tms_parse_expr(function, TMS_ENABLE_CMPLX | TMS_ENABLE_UNK, unknowns);
        if (F == NULL)
        {
            return -1;
        }
        else
        {
            i = tms_g_ufunc_count;
            // Set function
            tms_g_ufunc[i].F = F;
            tms_g_ufunc[i].name = strdup(fname);
            ++tms_g_ufunc_count;
            DYNAMIC_RESIZE(tms_g_all_func_names, tms_g_all_func_count, tms_g_all_func_max, char *);
            tms_g_all_func_names[tms_g_all_func_count++] = tms_g_ufunc[i].name;
            return 0;
        }
    }
    return -1;
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

double tms_random_weight()
{
    // Averaging 2 rands should give higher quality weights, especially for systems where int is 16 bits
    return (((double)rand() / RAND_MAX) + ((double)rand() / RAND_MAX)) / 2;
}
