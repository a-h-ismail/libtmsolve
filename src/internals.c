/*
Copyright (C) 2022-2024 Ahmad Ismail
SPDX-License-Identifier: LGPL-2.1-only
*/
#include "hashmap.h"
#include "libtmsolve.h"
#include "m_errors.h"
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

double complex tms_g_ans = 0;
int64_t tms_g_int_ans = 0;

const tms_rc_func tms_g_rc_func[] = {
    {"fact", tms_fact, tms_cfact}, {"abs", fabs, cabs_z},        {"exp", exp, tms_cexp},
    {"ceil", ceil, tms_cceil},     {"floor", floor, tms_cfloor}, {"round", round, tms_cround},
    {"sign", tms_sign, tms_csign}, {"arg", tms_carg_d, carg_z},  {"sqrt", sqrt, csqrt},
    {"cbrt", cbrt, tms_ccbrt},     {"cos", tms_cos, tms_ccos},   {"sin", tms_sin, tms_csin},
    {"tan", tms_tan, tms_ctan},    {"acos", acos, cacos},        {"asin", asin, casin},
    {"atan", atan, catan},         {"cosh", cosh, ccosh},        {"sinh", sinh, csinh},
    {"tanh", tanh, ctanh},         {"acosh", acosh, cacosh},     {"asinh", asinh, casinh},
    {"atanh", atanh, catanh},      {"ln", log, tms_cln},         {"log2", log2, tms_clog2},
    {"log10", log10, tms_clog10}};

// Extended functions, may take more than one argument (stored in a comma separated string)
const tms_extf tms_g_extf[] = {{"avg", tms_avg},
                               {"min", tms_min},
                               {"max", tms_max},
                               {"integrate", tms_integrate},
                               {"derivative", tms_derivative},
                               {"logn", tms_logn},
                               {"hex", tms_hex},
                               {"oct", tms_oct},
                               {"bin", tms_bin},
                               {"rand", tms_rand},
                               {"int", tms_int}};

const tms_int_func tms_g_int_func[] = {{"not", tms_not},
                                       {"mask", tms_mask},
                                       {"mask_bit", tms_mask_bit},
                                       {"inv_mask", tms_inv_mask},
                                       {"ipv4_prefix", tms_ipv4_prefix},
                                       {"zeros", tms_zeros},
                                       {"ones", tms_ones}};

const tms_int_extf tms_g_int_extf[] = {{"rand", tms_int_rand}, {"rr", tms_rr},
                                       {"rl", tms_rl},         {"sr", tms_sr},
                                       {"sra", tms_sra},       {"sl", tms_sl},
                                       {"nand", tms_nand},     {"and", tms_and},
                                       {"xor", tms_xor},       {"nor", tms_nor},
                                       {"or", tms_or},         {"ipv4", tms_ipv4},
                                       {"dotted", tms_dotted}, {"mask_range", tms_mask_range}};

bool _tms_do_init = true;
bool _tms_debug = false;

atomic_bool _parser_lock = false, _int_parser_lock = false;
atomic_bool _ufunc_lock = false;
atomic_bool _variables_lock = false, _int_variables_lock = false;
atomic_bool _evaluator_lock = false, _int_evaluator_lock = false;

char **tms_g_all_int_func_names;
int tms_g_all_int_func_count, tms_g_all_int_func_max = 16;

char *tms_g_illegal_names[] = {"ans"};
const int tms_g_illegal_names_count = array_length(tms_g_illegal_names);

tms_var tms_g_builtin_vars[] = {{"i", I, true}, {"pi", M_PI, true}, {"e", M_E, true}, {"c", 299792458, true}};
hashmap *var_hmap, *int_var_hmap, *ufunc_hmap, *int_ufunc_hmap;
hashmap *rc_func_hmap, *extf_hmap, *int_func_hmap, *int_extf_hmap;

uint64_t tms_int_mask = 0xFFFFFFFF;

int8_t tms_int_mask_size = 32;

// Too many boilerplates, incoming!
// What they do is hash, compare and retrieve for all hashmaps
int _tms_var_compare(const void *a, const void *b, void *udata)
{
    const tms_var *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_var_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_var *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_var_compare(const void *a, const void *b, void *udata)
{
    const tms_int_var *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_var_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_var *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_ufunc_compare(const void *a, const void *b, void *udata)
{
    const tms_ufunc *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_ufunc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_ufunc *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_ufunc_compare(const void *a, const void *b, void *udata)
{
    const tms_int_ufunc *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_ufunc_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_ufunc *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_rc_func_compare(const void *a, const void *b, void *udata)
{
    const tms_rc_func *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_rc_func_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_rc_func *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_extf_compare(const void *a, const void *b, void *udata)
{
    const tms_extf *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_extf_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_extf *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_func_compare(const void *a, const void *b, void *udata)
{
    const tms_int_func *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_func_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_func *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

int _tms_int_extf_compare(const void *a, const void *b, void *udata)
{
    const tms_int_extf *va = a, *vb = b;
    return strcmp(va->name, vb->name);
}

uint64_t _tms_int_extf_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tms_int_extf *v = item;
    return hashmap_xxhash3(v->name, strlen(v->name), seed0, seed1);
}

const tms_var *tms_get_var_by_name(char *name)
{
    tms_var tmp = {.name = name};
    return hashmap_get(var_hmap, &tmp);
}

const tms_int_var *tms_get_int_var_by_name(char *name)
{
    tms_int_var tmp = {.name = name};
    return hashmap_get(int_var_hmap, &tmp);
}

const tms_rc_func *tms_get_rc_func_by_name(char *name)
{
    tms_rc_func tmp = {.name = name};
    return hashmap_get(rc_func_hmap, &tmp);
}

const tms_extf *tms_get_extf_by_name(char *name)
{
    tms_extf tmp = {.name = name};
    return hashmap_get(extf_hmap, &tmp);
}

const tms_int_func *tms_get_int_func_by_name(char *name)
{
    tms_rc_func tmp = {.name = name};
    return hashmap_get(int_func_hmap, &tmp);
}

const tms_int_extf *tms_get_int_extf_by_name(char *name)
{
    tms_int_extf tmp = {.name = name};
    return hashmap_get(int_extf_hmap, &tmp);
}

const tms_ufunc *tms_get_ufunc_by_name(char *name)
{
    tms_ufunc tmp = {.name = name};
    return hashmap_get(ufunc_hmap, &tmp);
}

const tms_int_ufunc *tms_get_int_ufunc_by_name(char *name)
{
    tms_int_ufunc tmp = {.name = name};
    return hashmap_get(int_ufunc_hmap, &tmp);
}

tms_var *tms_get_all_vars(size_t *out)
{
    return hashmap_to_array(var_hmap, out);
}

tms_int_var *tms_get_all_int_vars(size_t *out)
{
    return hashmap_to_array(int_var_hmap, out);
}

bool tms_function_exists(char *name)
{
    if (tms_get_rc_func_by_name(name) != NULL || tms_get_extf_by_name(name) != NULL ||
        tms_get_ufunc_by_name(name) != NULL)
        return true;
    else
        return false;
}

bool tms_int_function_name_exists(char *name)
{
    if (tms_get_int_func_by_name(name) != NULL || tms_get_int_extf_by_name(name) != NULL ||
        tms_get_int_ufunc_by_name(name) != NULL)
        return true;
    else
        return false;
}

void tmsolve_init()
{
    if (_tms_do_init)
    {
        // Seed the random number generator
        srand(time(NULL));

        // Prepare hashmaps
        var_hmap = hashmap_new(sizeof(tms_var), 0, rand(), rand(), _tms_var_hash, _tms_var_compare, NULL, NULL);

        int_var_hmap =
            hashmap_new(sizeof(tms_int_var), 0, rand(), rand(), _tms_int_var_hash, _tms_int_var_compare, NULL, NULL);

        ufunc_hmap = hashmap_new(sizeof(tms_ufunc), 0, rand(), rand(), _tms_ufunc_hash, _tms_ufunc_compare, NULL, NULL);

        int_ufunc_hmap = hashmap_new(sizeof(tms_int_ufunc), 0, rand(), rand(), _tms_int_ufunc_hash,
                                     _tms_int_ufunc_compare, NULL, NULL);

        rc_func_hmap =
            hashmap_new(sizeof(tms_rc_func), 0, rand(), rand(), _tms_rc_func_hash, _tms_rc_func_compare, NULL, NULL);

        extf_hmap = hashmap_new(sizeof(tms_extf), 0, rand(), rand(), _tms_extf_hash, _tms_extf_compare, NULL, NULL);

        int_func_hmap =
            hashmap_new(sizeof(tms_int_func), 0, rand(), rand(), _tms_int_func_hash, _tms_int_func_compare, NULL, NULL);

        int_extf_hmap =
            hashmap_new(sizeof(tms_int_extf), 0, rand(), rand(), _tms_int_extf_hash, _tms_int_extf_compare, NULL, NULL);
        int i;
        for (i = 0; i < array_length(tms_g_builtin_vars); ++i)
            hashmap_set(var_hmap, tms_g_builtin_vars + i);

        for (i = 0; i < array_length(tms_g_rc_func); ++i)
            hashmap_set(rc_func_hmap, tms_g_rc_func + i);

        for (i = 0; i < array_length(tms_g_extf); ++i)
            hashmap_set(extf_hmap, tms_g_extf + i);

        for (i = 0; i < array_length(tms_g_int_func); ++i)
            hashmap_set(int_func_hmap, tms_g_int_func + i);

        for (i = 0; i < array_length(tms_g_int_func); ++i)
            hashmap_set(int_func_hmap, tms_g_int_func + i);

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

int _tms_set_var_unsafe(char *name, double complex value, bool is_constant)
{
    if (isnan(creal(value)) || isnan(cimag(value)))
        return -1;

    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is allowed
    if (!tms_legal_name(name))
    {
        tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    if (tms_function_exists(name))
    {
        tms_save_error(TMS_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
        return -1;
    }

    const tms_var *tmp = tms_get_var_by_name(name);
    if (tmp != NULL && tmp->is_constant)
    {
        tms_save_error(TMS_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL, 0);
        return -1;
    }

    tms_var v = {.name = strdup(name), .value = value, .is_constant = is_constant};
    hashmap_set(var_hmap, &v);
    return 0;
}

int tms_set_var(char *name, double complex value, bool is_constant)
{
    while (atomic_flag_test_and_set(&_variables_lock))
        ;
    int status = _tms_set_var_unsafe(name, value, is_constant);
    atomic_flag_clear(&_variables_lock);
    return status;
}

int _tms_set_int_var_unsafe(char *name, int64_t value, bool is_constant)
{
    // Check if the name has illegal characters
    if (tms_valid_name(name) == false)
    {
        tms_save_error(TMS_INT_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is allowed
    if (!tms_legal_name(name))
    {
        tms_save_error(TMS_INT_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    if (tms_find_str_in_array(name, tms_g_all_int_func_names, tms_g_all_int_func_count, TMS_NOFUNC) != -1)
    {
        tms_save_error(TMS_INT_PARSER, VAR_NAME_MATCHES_FUNCTION, EH_FATAL, NULL, 0);
        return -1;
    }

    const tms_int_var *tmp = tms_get_int_var_by_name(name);
    if (tmp != NULL && tmp->is_constant)
    {
        tms_save_error(TMS_INT_PARSER, OVERWRITE_CONST_VARIABLE, EH_FATAL, NULL, 0);
        return -1;
    }

    tms_int_var v = {.name = strdup(name), .value = value, .is_constant = is_constant};
    hashmap_set(int_var_hmap, &v);
    return 0;
}

int tms_set_int_var(char *name, int64_t value, bool is_constant)
{
    while (atomic_flag_test_and_set(&_int_variables_lock))
        ;
    int status = _tms_set_int_var_unsafe(name, value, is_constant);
    atomic_flag_clear(&_int_variables_lock);
    return status;
}

bool is_ufunc_referenced_by(tms_math_expr *M, char *fname)
{
    tms_math_subexpr *S = M->S;
    for (int i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_USER && strcmp(fname, S[i].func.user) == 0)
            return true;
    }
    return false;
}

bool _tms_ufunc_has_bad_refs(char *fname)
{
    int i;
    const tms_ufunc *F = tms_get_ufunc_by_name(fname);
    if (F == NULL)
        return false;
    tms_math_expr *M = F->F;
    tms_math_subexpr *S = M->S;

    for (i = 0; i < M->subexpr_count; ++i)
    {
        if (S[i].func_type == TMS_F_USER)
        {
            if (strcmp(S[i].func.user, fname) == 0)
            {
                tms_save_error(TMS_PARSER, NO_FSELF_REFERENCE, EH_FATAL, NULL, 0);
                return true;
            }
            else if (is_ufunc_referenced_by(M, fname))
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
    const tms_ufunc *old = tms_get_ufunc_by_name(fname);

    // Check if the name has illegal characters
    if (tms_valid_name(fname) == false)
    {
        tms_save_error(TMS_PARSER, INVALID_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the function name is allowed
    if (!tms_legal_name(fname))
    {
        tms_save_error(TMS_PARSER, ILLEGAL_NAME, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name was already used by builtin functions
    if (tms_function_exists(fname) && old == NULL)
    {
        tms_save_error(TMS_PARSER, NO_FUNCTION_SHADOWING, EH_FATAL, NULL, 0);
        return -1;
    }

    // Check if the name is used by a variable
    if (tms_get_var_by_name(fname) != NULL)
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
    tms_math_expr *new = tms_parse_expr(function, TMS_ENABLE_CMPLX | TMS_ENABLE_UNK, unknowns);
    // Function already exists
    if (old != NULL)
    {
        // The old ufunc members is kept to either free the old function or restore it on failure of the new function
        tms_ufunc old_F;
        old_F = *old;

        tms_ufunc tmp = {.F = new, .name = old->name};

        // We need to update the hashmap because the function checks will lookup the name in the hashmap
        // otherwise we will get the old function checked instead
        hashmap_set(ufunc_hmap, new);
        if (_tms_ufunc_has_bad_refs(function))
        {
            // Restore the original function since the new one is problematic
            hashmap_set(ufunc_hmap, &old_F);
            tms_delete_math_expr(tmp.F);
            return -1;
        }
        else
        {
            tms_delete_math_expr(old_F.F);
            return 0;
        }
    }
    else
    {
        tms_ufunc tmp;
        tmp.F = new;
        tmp.name = strdup(fname);
        hashmap_set(ufunc_hmap, &tmp);
        return 0;
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
